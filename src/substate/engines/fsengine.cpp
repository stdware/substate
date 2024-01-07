#include "fsengine.h"
#include "fsengine_p.h"

#include <fstream>
#include <unordered_set>

#include <mutex>
#include <condition_variable>
#include <thread>

#include "substateglobal_p.h"
#include "model/nodehelper.h"

#define BINARY_FILE_DAT "dat"

namespace fs = std::filesystem;

namespace {

    // Scheduler

    class BaseProducer;

    class TaskManager;

    class BaseTask {
    public:
        explicit BaseTask(bool deleteOnFinish);
        virtual ~BaseTask();

        inline void abort();
        inline void del();          // Call delete only

        virtual void execute() = 0; // May need to check if obsolete and need to delete?

        std::mutex mtx;
        std::condition_variable cv;

        volatile bool obsolete;
        volatile bool finished;

        bool deleteOnFinish;
    };

    class BaseProducer {
    public:
        BaseProducer();
        ~BaseProducer();

        inline void pushTask(BaseTask *task, bool trivial = false, bool unshift = false);
        inline void delTrivialTask(BaseTask *task);

    protected:
        std::unordered_set<BaseTask *> m_trivialTasks;

        static inline TaskManager *taskManager = nullptr;
        static inline std::unordered_set<BaseProducer *> instances;
    };

    // Singleton
    class TaskManager {
    public:
        TaskManager();
        ~TaskManager();

        void pushTask(BaseTask *task, bool unshift); // Producer
        void workRoutine();                          // Consumer

    protected:
        volatile bool finished;

        std::mutex mtx;
        std::condition_variable cv;
        std::thread *workThread;
        static inline std::list<BaseTask *> task_queue;
    };

    BaseTask::BaseTask(bool deleteOnFinish)
        : obsolete(false), finished(false), deleteOnFinish(deleteOnFinish) {
    }

    BaseTask::~BaseTask() = default;

    inline void BaseTask::abort() {
        std::unique_lock<std::mutex> lock(mtx);
        if (finished) {
            del();
        } else {
            // Will be deleted by the consumer
            obsolete = true;
        }
    }

    inline void BaseTask::del() {
        delete this;
    }

    BaseProducer::BaseProducer() {
        instances.insert(this);
        if (!taskManager) {
            taskManager = new TaskManager();
        }
    }

    BaseProducer::~BaseProducer() {
        // Remove trivial tasks
        for (const auto &task : std::as_const(m_trivialTasks)) {
            task->abort();
        }

        if (instances.size() == 1 && taskManager) {
            delete taskManager;
            taskManager = nullptr;
        }
        instances.erase(this);
    }

    inline void BaseProducer::pushTask(BaseTask *task, bool trivial, bool unshift) {
        QM_UNUSED(this)
        if (trivial) {
            m_trivialTasks.insert(task);
        }
        taskManager->pushTask(task, unshift);
    }

    inline void BaseProducer::delTrivialTask(BaseTask *task) {
        m_trivialTasks.erase(task);
        task->del();
    }

    TaskManager::TaskManager()
        : finished(false), workThread(new std::thread(&TaskManager::workRoutine, this)) {
    }

    TaskManager::~TaskManager() {
        finished = true;
        cv.notify_all();
        workThread->join(); // Wait for finish
        delete workThread;
    }

    void TaskManager::pushTask(BaseTask *task, bool unshift) {
        std::unique_lock<std::mutex> lock(mtx);
        if (unshift) {
            task_queue.push_front(task);
        } else {
            task_queue.push_back(task);
        }
        lock.unlock();

        // Notify consumer
        cv.notify_all();
    }

    void TaskManager::workRoutine() {
        while (true) {
            std::unique_lock<std::mutex> lock(mtx);
            while (!finished && task_queue.empty()) {
                cv.wait(lock); // Wait for task or finish
            }

            if (!task_queue.empty()) {
                auto cur_task = task_queue.front();
                task_queue.pop_front();

                // Unlock and consume
                lock.unlock();
                cur_task->execute();

                if (cur_task->deleteOnFinish || cur_task->obsolete) {
                    cur_task->del();
                }
                continue;
            }

            if (finished) {
                break;
            }
        }
    }

}

/* Steps data (model_stepss.dat)
 *
 * 0x0          maxSteps in a checkpoint
 * 0x4          maxCheckpoints
 * 0x8          min step in log
 * 0xC          max step in log
 * 0x10         current step
 * 0x14         max index in model
 *
 */

/* Checkpoint data (ckpt_XXX.dat)
 *
 * 0x0          CKPT
 * 0x4          removed items pos
 * 0xC          root id (0 if null)
 * 0x14         root data
 * 0xN          removed items size
 * 0xN+4        removed items data
 *
 */

/* Transaction data (journal_XXX.dat)
 *
 * 0x8              max entries
 * 0x10             entry 1
 * 0x18             entry 2
 * ...
 * 8*maxStep        entry maxStep
 * 8*maxStep+8      0xFFFFFFFFFFFFFFFF
 * 8*maxStep+16     transaction 0 (attributes + operations)
 * ...
 *
 */

/* Action data
 *
 * 0x0              type (int32_t)
 * 0x4              inserted items size
 * 0xN+4            inserted items data
 * ...
 * 0xM              action data
 *
 */

namespace Substate {

    // Tasks

    using TXData = MemoryEnginePrivate::TransactionData;

    [[maybe_unused]] static constexpr const auto INT32_SIZE = int64_t(sizeof(int32_t));
    static constexpr const auto INT64_SIZE = int64_t(sizeof(int64_t));

    static inline fs::path journal_path(const fs::path &dir, int i) {
        return dir / ("journal_" + std::to_string(i) + "." BINARY_FILE_DAT);
    }

    static inline fs::path steps_path(const fs::path &dir) {
        return dir / "model_steps." BINARY_FILE_DAT;
    }

    static inline fs::path ckpt_path(const fs::path &dir, int i) {
        return dir / ("ckpt_" + std::to_string(i) + "." BINARY_FILE_DAT);
    }

    static inline bool truncateJournals(const fs::path &dir, int i, bool dryRun = false) {
        auto func = dryRun ? [](const fs::path &path) { return fs::exists(path); }
                           : [](const fs::path &path) { return fs::remove(path); };
        bool b1 = func(journal_path(dir, i)) || (i == 0);
        bool b2 = func(ckpt_path(dir, i));
        return b1 || b2;
    }

    static void writeAction(std::ostream &file, Action *a) {
        OStream out(&file);

        // Write type
        out << a->type();

        // Write inserted actions
        std::vector<Node *> insertedNotes;
        a->virtual_hook(Action::InsertedNodesHook, &insertedNotes);

        // Write inserted data size (make it convenient to skip if read in brief mode)
        out << int64_t(0);
        int64_t pos = file.tellp();
        out << int(insertedNotes.size());
        for (const auto &item : std::as_const(insertedNotes)) {
            item->write(out);
        }

        int64_t pos1 = file.tellp();
        file.seekp(pos - INT64_SIZE);
        out << (pos1 - pos);
        file.seekp(pos1);

        // Write action data
        a->write(out);
    }

    static void readAction(std::istream &file, Action **a, bool brief,
                           std::unordered_map<int, Node *> &existingNodes) {
        IStream in(&file);

        // read type
        int type;
        in >> type;

        if (brief) {
            // Skip
            int64_t insertedDataSize;
            in >> insertedDataSize;
            in.skipRawData(int(insertedDataSize));
        } else {
            // Read inserted actions
            int size;
            in >> size;
            for (int i = 0; i < size; ++i) {
                auto node = Node::read(in);
                if (!node) {
                    QMSETUP_FATAL("Failed to read inserted when reading action of type %d.", type);
                }

                // Add inserted nodes
                existingNodes.insert(std::make_pair(node->index(), node));
            }
        }

        // Read action data
        auto action = Action::read(in);
        if (!action) {
            QMSETUP_FATAL("Failed to read action of type %d.", type);
        }
        *a = action;
    }

    static void writeCheckPoint(std::ostream &file, Node *root,
                                const std::vector<Node *> &removedItems) {
        file.write("CKPT", 4);

        OStream out(&file);
        out << int64_t(0);
        if (root) {
            // Write index
            out << root->index();

            // Write root data
            root->write(out);
        } else {
            // Write 0
            out << size_t(0);
        }

        // Write removed items pos
        int64_t pos = file.tellp();
        file.seekp(4);
        out << pos;
        file.seekp(pos);

        // Write removed items size
        out << int(removedItems.size());

        // Write removed items data
        for (const auto &item : std::as_const(removedItems)) {
            item->write(out);
        }
    }

    static void readCheckPoint(std::istream &file, Node **rootRef,
                               std::vector<Node *> *removedItemsRef) {
        IStream in(&file);
        in.skipRawData(4);

        Node *root = nullptr;

        if (!rootRef) {
            // Read removed items pos
            int64_t pos;
            in >> pos;
            file.seekg(pos);
        } else {
            // Skip removed items pos
            in.skipRawData(INT64_SIZE);

            // Read root id
            size_t id;
            in >> id;
            if (id != 0) {
                // Read root
                root = Node::read(in);
                if (!root) {
                    QMSETUP_FATAL("Failed to read root item.");
                }
            }
        }

        if (root)
            *rootRef = root;

        if (removedItemsRef) {
            // Read removed items size
            int sz;
            in >> sz;

            // Read removed items data
            std::vector<Node *> removedItems;
            removedItems.reserve(sz);
            for (int i = 0; i < sz; ++i) {
                auto item = Node::read(in);
                if (!item) {
                    QMSETUP_FATAL("Failed to read one of the removed items.");
                }
                removedItems.push_back(item);
            }

            *removedItemsRef = std::move(removedItems);
        }
    }

    static void readJournal(std::ifstream &file, int maxSteps, std::vector<TXData> &res, bool brief,
                            std::unordered_map<int, Node *> &existingNodes) {
        IStream in(&file);
        std::vector<int64_t> positions{
            static_cast<int64_t>((maxSteps + 2) * INT64_SIZE),
        };

        int64_t positions_cnt;
        in >> positions_cnt;

        positions.reserve(positions_cnt);
        for (int i = 0; i < positions_cnt - 1; ++i) {
            int64_t pos;
            in >> pos;
            positions.push_back(pos);
        }

        if (!in.good()) {
            QMSETUP_FATAL("Read journal failed when reading positions.");
        }

        std::vector<TXData> data;
        data.reserve(positions_cnt);
        for (const auto &pos : std::as_const(positions)) {
            file.seekg(pos);

            // Read attributes
            Engine::StepMessage attrs;
            in >> attrs;

            // Read operations
            int actions_cnt;
            in >> actions_cnt;
            std::vector<Action *> actions;
            actions.reserve(actions_cnt);
            for (int i = 0; i < actions_cnt; ++i) {
                Action *a;
                readAction(file, &a, brief, existingNodes);
                actions.push_back(a);
            }
            data.push_back({actions, attrs});
        }
        res = std::move(data);
    }

    namespace {

        class CommitTask : public BaseTask {
        public:
            CommitTask()
                : BaseTask(true), fsStep(-1), fsMin(-1), oldFsMin(-1), oldFsMax(-1), maxSteps(0),
                  maxId(0) {
            }

            ~CommitTask() {
                deleteAll(data.actions);
            }

            void execute() override {
                int fsMax = fsStep;

                // Write transaction
                {
                    int num = (fsStep - 1) / maxSteps;
                    auto path = journal_path(dir, num);
                    auto exists = fs::exists(path);
                    std::fstream file(path, std::ios::binary);

                    // Write initial zeros
                    if (!exists) {
                        std::string zero((maxSteps + 2) * INT64_SIZE, '\0');
                        file.write(zero.data(), std::streamsize(zero.size()));
                        file.seekp(0);
                    }

                    // Get current transaction start pos
                    int cur = (fsStep - 1) % maxSteps + 1;
                    if (cur == 1) {
                        int64_t pos = (maxSteps + 2) * INT64_SIZE;
                        file.seekp(pos); // Data section start
                    } else {
                        IStream in(&file);
                        int64_t pos;

                        file.seekg((cur - 1) * INT64_SIZE); // Previous transaction end
                        in >> pos;
                        file.seekp(pos);
                    }

                    OStream out(&file);

                    // Write attributes
                    out << data.message;

                    // Write operation count
                    out << int(data.actions.size());

                    // Write operations
                    for (const auto &a : std::as_const(data.actions)) {
                        writeAction(file, a);
                    }

                    file.flush();

                    // Update count and pos
                    int64_t pos = file.tellp();
                    file.seekp(0);
                    out << int64_t(cur);
                    file.seekp(cur * INT64_SIZE);
                    out << pos;
                    out << int64_t(-1); // end of pos table

                    // Flush
                    file.flush();
                    file.close();

                    if (fs::file_size(path) > pos) {
                        fs::resize_file(path, pos);
                    }
                }

                // Write steps (Must do it after writing transaction)
                {
                    auto path = steps_path(dir);
                    std::fstream file(path, std::ios::binary);

                    std::string buf;
                    OStream out(&buf);
                    out << fsMin << fsMax << fsStep;
                    if (maxId > 0) {
                        out << maxId;
                    }

                    // Call write once to ensure atomicity
                    file.seekp(8);
                    file.write(buf.data(), std::streamsize(buf.size()));
                    file.flush();
                }

                // Truncate
                {
                    // Remove backward logs
                    int oldMinNum = oldFsMin / maxSteps;
                    int minNum = fsMin / maxSteps;
                    for (int i = minNum - 1; i >= oldMinNum; --i) {
                        truncateJournals(dir, i);
                    }

                    // Remove forward logs
                    int oldMaxNum = (oldFsMax - 1) / maxSteps;
                    int maxNum = (fsMax - 1) / maxSteps;
                    for (int i = maxNum + 1; i <= oldMaxNum; ++i) {
                        truncateJournals(dir, i);
                    }
                }
            }

            TXData data;
            int fsStep, fsMin;
            int oldFsMin, oldFsMax;
            int maxSteps;
            fs::path dir;
            size_t maxId;
        };

        class ChangeStepTask : public BaseTask {
        public:
            ChangeStepTask() : BaseTask(true), fsStep(0) {
            }

            ~ChangeStepTask() = default;

            void execute() override {
                // Write step
                auto path = steps_path(dir);
                std::fstream file(path, std::ios::binary);
                file.seekg(16);

                // Call write once
                OStream out(&file);
                out << fsStep;

                file.flush();
            }

            int fsStep;
            fs::path dir;
        };

        // Writing checkpoint with root item and all items removed during last period
        class WriteCkptTask : public BaseTask {
        public:
            WriteCkptTask() : BaseTask(true), num(0), root(nullptr) {
            }

            ~WriteCkptTask() {
                // Delete items after writing
                delete root;
                deleteAll(removedItems);
            }

            void execute() override {
                auto path = ckpt_path(dir, num);
                std::ofstream file(path, std::ios::binary);
                writeCheckPoint(file, root, removedItems);
            }

            int num;
            Node *root;
            std::vector<Node *> removedItems;
            fs::path dir;
        };

        class ReadCkptTask : public BaseTask {
        public:
            ReadCkptTask() : BaseTask(false), num(0), brief(false), maxSteps(0) {
            }

            ~ReadCkptTask() {
                deleteAll(removedItems);
                for (const auto &item : std::as_const(data)) {
                    deleteAll(item.actions);
                }
            }

            void execute() override {
                if (obsolete) {
                    return;
                }

                // Read checkpoint
                if (brief) {
                    auto path = ckpt_path(dir, num + 1);
                    std::ifstream file(path, std::ios::binary);
                    readCheckPoint(file, nullptr, &removedItems);
                }

                if (obsolete) {
                    return;
                }

                // Read transactions
                {
                    auto path = journal_path(dir, num);
                    std::ifstream file(path, std::ios::binary);
                    readJournal(file, maxSteps, data, brief, existingNodes);
                }

                std::unique_lock<std::mutex> lock(mtx);
                if (obsolete) {
                    return;
                }
                finished = true;
                lock.unlock();

                cv.notify_all();
            }

            int num;
            bool brief; // Read only id of insert operation
            std::vector<Node *> removedItems;
            std::vector<TXData> data;
            std::unordered_map<int, Node *> existingNodes;
            int maxSteps;
            fs::path dir;
        };

        class ReadStepMessageTask : public BaseTask {
        public:
            ReadStepMessageTask() : BaseTask(false), step(0), maxSteps(0) {
            }

            void execute() override {
                std::unique_lock<std::mutex> lock(mtx);
                {
                    int num = (step - 1) / maxSteps;

                    auto path = journal_path(dir, num);
                    std::ifstream file(path, std::ios::binary);
                    IStream in(&file);

                    int64_t pos0 = file.tellg();

                    int cur = (step - 1) % maxSteps + 1;
                    if (cur == 1) {
                        file.seekg((maxSteps + 2) * INT64_SIZE); // Data section start
                    } else {
                        file.seekg((cur - 1) * INT64_SIZE);      // Previous transaction end
                        int64_t pos;
                        in >> pos;
                        file.seekg(pos);
                    }

                    in >> res;

                    // Restore pos
                    file.seekg(pos0);
                }
                lock.unlock();
                cv.notify_all();
            }

            int step;
            int maxSteps;
            Engine::StepMessage res;
            fs::path dir;
        };

        class ResetTask : public BaseTask {
        public:
            ResetTask()
                : BaseTask(true), fsMin(0), fsMax(0), fsStep(0), maxSteps(0), maxCheckPoints(0) {
            }

            void execute() override {
                int oldMin = fsMin;
                int oldMax = fsMax;

                // Write steps
                {
                    auto path = steps_path(dir);
                    bool exists = fs::exists(path);
                    std::ofstream file(path, std::ios::binary);
                    OStream out(&file);
                    if (!exists) {
                        // Write initial values
                        out << maxSteps << maxCheckPoints;
                    } else {
                        file.seekp(8);
                    }
                    out << fsMin << fsMax << fsStep << size_t(0);
                    file.flush();
                }

                // Truncate
                {
                    int oldMinNum = oldMin / maxSteps;
                    int oldMaxNum = (oldMax - 1) / maxSteps;
                    for (int i = oldMaxNum; i >= oldMinNum; --i) {
                        truncateJournals(dir, i);
                    }
                }
            }

            int fsMin, fsMax, fsStep;
            int maxSteps, maxCheckPoints;
            fs::path dir;
        };

    }

}

namespace Substate {

    struct RecoverData {
        int fsMin;
        int fsMax;
        int fsStep;
        int currentNum;
        size_t maxId;
        Node *root;
        std::vector<Node *> removedItems;
        std::vector<TXData> backwardData;
        std::vector<TXData> forwardData;

        ~RecoverData() {
        }
    };

    class FileSystemEnginePrivate::JournalData : public BaseProducer {
    public:
        int fsMin;
        int fsMax;

        Engine::StepMessage getFsStepMessage(int step) {
            auto task = new ReadStepMessageTask();
            task->step = step;

            std::unique_lock<std::mutex> lock(task->mtx);
            pushTask(task, false, true);
            while (!task->finished) {
                task->cv.wait(lock);
            }
            lock.unlock();

            auto res = std::move(task->res);
            task->del();
            return res;
        }
    };

    FileSystemEnginePrivate::FileSystemEnginePrivate()
        : journalData(std::make_unique<JournalData>()) {
        finished = false;
        maxCheckPoints = 1;
    }

    FileSystemEnginePrivate::~FileSystemEnginePrivate() {
        if (finished) {
            // Remove journal
        }
    }

    void FileSystemEnginePrivate::init() {
    }

    bool FileSystemEnginePrivate::acceptChangeMaxSteps(int steps) const {
        return MemoryEnginePrivate::acceptChangeMaxSteps(steps);
    }

    void FileSystemEnginePrivate::afterCurrentChange() {
        MemoryEnginePrivate::afterCurrentChange();
    }

    void FileSystemEnginePrivate::afterCommit(const std::vector<Action *> &actions,
                                              const Engine::StepMessage &message) {
        MemoryEnginePrivate::afterCommit(actions, message);
    }

    void FileSystemEnginePrivate::afterReset() {
        MemoryEnginePrivate::afterReset();
    }

    FileSystemEngine::FileSystemEngine() {
    }

    FileSystemEngine::~FileSystemEngine() {
    }

    int FileSystemEngine::checkpoints() const {
        QM_D(const FileSystemEngine);
        return d->maxCheckPoints;
    }

    void FileSystemEngine::setCheckpoints(int n) {
        QM_D(FileSystemEngine);
        d->maxCheckPoints = n;
    }

    bool FileSystemEngine::finished() const {
        QM_D(const FileSystemEngine);
        return d->finished;
    }

    void FileSystemEngine::setFinished(bool finished) {
        QM_D(FileSystemEngine);
        d->finished = finished;
    }

    bool FileSystemEngine::start(const fs::path &dir) {
        return false;
    }

    bool FileSystemEngine::recover(const fs::path &dir) {
        return false;
    }

    void FileSystemEngine::setup(Model *model) {
        Engine::setup(model);
    }

    int FileSystemEngine::minimum() const {
        return MemoryEngine::minimum();
    }

    int FileSystemEngine::maximum() const {
        return MemoryEngine::maximum();
    }

    Engine::StepMessage FileSystemEngine::stepMessage(int step) const {
        QM_D(const FileSystemEngine);

        auto d2 = d->journalData.get();
        if (step <= d2->fsMin || step > d2->fsMax) {
            return {};
        }

        int step2 = step - (d->min + 1);
        if (step2 < 0 || step2 >= d->stack.size()) {
            return d2->getFsStepMessage(step);
        }
        return d->stack.at(step2).message;
    }

    bool FileSystemEngine::createWarningFile(const fs::path &dir) {
        std::ofstream file(dir / "WARNING.txt");
        if (!file.is_open()) {
            return false;
        }
        file << "This directory contains the binary log files of the current editing project, "
                "you can remove the whole directory, but do not delete or change any file in "
                "the directory, otherwise it may cause crash!"
             << std::endl;
        return true;
    }

    FileSystemEngine::FileSystemEngine(FileSystemEnginePrivate &d) : MemoryEngine(d) {
        d.init();
    }

}