#include "fsengine.h"
#include "fsengine_p.h"

#include <fstream>
#include <algorithm>
#include <unordered_set>

#include <mutex>
#include <condition_variable>
#include <thread>
#include <utility>

#include "substateglobal_p.h"
#include "model/nodehelper.h"
#include "model/model_p.h"

#define BINARY_FILE_SUFFIX ".dat"

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

        inline void pushTask(BaseTask *task, bool unshift = false);

    protected:
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
        do {
            std::unique_lock<std::mutex> lock(mtx);
            if (finished) {
                break;
            }

            // Will be deleted by the consumer
            obsolete = true;
            return;
        } while (false);

        // Delete itself
        del();
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
        if (instances.size() == 1 && taskManager) {
            delete taskManager;
            taskManager = nullptr;
        }
        instances.erase(this);
    }

    inline void BaseProducer::pushTask(BaseTask *task, bool unshift) {
        QM_UNUSED(this)

        QMSETUP_DEBUG("Push task %s", typeid(*task).name());

        taskManager->pushTask(task, unshift);
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

/* Steps data (model_steps.dat)
 *
 * 0x0          maxSteps in a checkpoint
 * 0x4          maxCheckpoints
 * 0x8          min step in log
 * 0xC          max step in log
 * 0x10         current step
 * 0x14         max index in model (int32_t)
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
        return dir / ("journal_" + std::to_string(i) + BINARY_FILE_SUFFIX);
    }

    static inline fs::path steps_path(const fs::path &dir) {
        return dir / "model_steps" BINARY_FILE_SUFFIX;
    }

    static inline fs::path ckpt_path(const fs::path &dir, int i) {
        return dir / ("ckpt_" + std::to_string(i) + BINARY_FILE_SUFFIX);
    }

    static inline bool fs_exists(const fs::path &path) {
        return fs::exists(path);
    }

    static inline bool fs_remove(const fs::path &path) {
        return fs::remove(path);
    }

    static inline bool truncateJournals(const fs::path &dir, int i, bool dryRun = false) {
        auto func = dryRun ? fs_exists : fs_remove;
        bool b1 = func(journal_path(dir, i)) || (i == 0);
        bool b2 = func(ckpt_path(dir, i));
        return b1 || b2;
    }

    static void writeAction(std::ostream &file, Action *a) {
        OStream out(&file);

        // Write sign
        out << "ACT ";

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

    static bool readAction(std::istream &file, Action *&a, bool brief,
                           std::unordered_map<int, Node *> &insertedItems) {
        IStream in(&file);

        // Skip sign
        in.skipRawData(4);

        // Read type
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
                    QMSETUP_WARNING("Failed to read inserted when reading action of type %d.",
                                    type);
                    return false;
                }

                // Add inserted nodes
                insertedItems.insert(std::make_pair(node->index(), node));
            }
        }

        // Read action data
        auto action = Action::read(in);
        if (!action) {
            QMSETUP_WARNING("Failed to read action of type %d.", type);
            return false;
        }
        a = action;
        return true;
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
            out << int(0);
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

    static bool readCheckPoint(std::istream &file, Node **rootRef,
                               std::vector<Node *> *removedItemsRef) {
        IStream in(&file);
        in.skipRawData(4);

        Node *root = nullptr;
        std::vector<Node *> removedItems;

        if (!rootRef) {
            // Read removed items pos
            int64_t pos;
            in >> pos;
            file.seekg(pos);
        } else {
            // Skip removed items pos
            in.skipRawData(INT64_SIZE);

            // Read root id
            int id;
            in >> id;
            if (id != 0) {
                // Read root
                root = Node::read(in);
                if (!root) {
                    QMSETUP_WARNING("Failed to read root item.");
                    goto abort;
                }
            }
        }

        if (removedItemsRef) {
            // Read removed items size
            int sz;
            in >> sz;

            // Read removed items data
            removedItems.reserve(sz);
            for (int i = 0; i < sz; ++i) {
                auto item = Node::read(in);
                if (!item) {
                    QMSETUP_WARNING("Failed to read one of the removed items.");
                    goto abort;
                }
                removedItems.push_back(item);
            }

            *removedItemsRef = std::move(removedItems);
        }

        if (root)
            *rootRef = root;

        return true;

    abort:
        delete root;
        deleteAll(removedItems);
        return false;
    }

    // The caller should remove the inserted items if failed.
    static bool readJournal(std::ifstream &file, int maxSteps, std::vector<TXData> &res, bool brief,
                            std::unordered_map<int, Node *> &insertedItems) {
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
            QMSETUP_WARNING("Read journal failed when reading positions.");
            return false;
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
                if (!readAction(file, a, brief, insertedItems)) {
                    goto abort;
                }
                actions.push_back(a);
            }
            data.push_back({actions, attrs});
        }
        res = std::move(data);
        return true;

    abort:
        for (const auto &item : std::as_const(data)) {
            deleteAll(item.actions);
        }
        return false;
    }

    namespace {

        class CommitTask : public BaseTask {
        public:
            CommitTask(fs::path dir, int fsStep, int fsMin, int oldFsMin, int oldFsMax,
                       int maxSteps, int maxId, TXData data)
                : BaseTask(true), fsStep(fsStep), fsMin(fsMin), oldFsMin(oldFsMin),
                  oldFsMax(oldFsMax), maxSteps(maxSteps), maxId(maxId), data(std::move(data)),
                  dir(std::move(dir)) {
            }

            ~CommitTask() {
                deleteAll(data.actions); // Delete all cloned actions
            }

            void execute() override {
                int fsMax = fsStep;

                // Write transaction
                {
                    int num = (fsStep - 1) / maxSteps;
                    auto path = journal_path(dir, num);
                    auto exists = fs::exists(path);
                    std::fstream file(path, std::ios::binary | std::ios::in | std::ios::out |
                                                std::ios::app);

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
                    std::fstream file(path, std::ios::binary | std::ios::in | std::ios::out |
                                                std::ios::app);

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
            int maxId;
            fs::path dir;
        };

        class ChangeStepTask : public BaseTask {
        public:
            ChangeStepTask(fs::path dir, int fsStep)
                : BaseTask(true), fsStep(fsStep), dir(std::move(dir)) {
            }

            ~ChangeStepTask() = default;

            void execute() override {
                // Write step
                auto path = steps_path(dir);
                std::fstream file(path,
                                  std::ios::binary | std::ios::in | std::ios::out | std::ios::app);
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
            WriteCkptTask(fs::path dir, int num, Node *root, std::vector<Node *> removedItems)
                : BaseTask(true), num(num), dir(std::move(dir)), root(nullptr),
                  removedItems(std::move(removedItems)) {
            }

            ~WriteCkptTask() {
                // Delete items after writing
                delete root;
                deleteAll(removedItems);
            }

            void execute() override {
                auto path = ckpt_path(dir, num);
                std::ofstream file(path, std::ios::binary | std::ios::app);
                writeCheckPoint(file, root, removedItems);
            }

            int num;
            Node *root;
            std::vector<Node *> removedItems;
            fs::path dir;
        };

        class ReadCkptTask : public BaseTask {
        public:
            ReadCkptTask(fs::path dir, int num, bool brief, int maxSteps)
                : BaseTask(false), num(num), brief(brief), maxSteps(maxSteps), dir(std::move(dir)) {
            }

            ~ReadCkptTask() {
                // Delete items if not moved out by post actions
                deleteAll(removedItems);
                for (const auto &item : std::as_const(data)) {
                    deleteAll(item.actions);
                }
                for (const auto &it : std::as_const(insertedItems)) {
                    delete it.second;
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
                    if (!readCheckPoint(file, nullptr, &removedItems)) {
                        QMSETUP_FATAL("Read checkpoint task failed when reading checkpoint %d.",
                                      num + 1);
                    }
                }

                if (obsolete) {
                    return;
                }

                // Read transactions
                {
                    auto path = journal_path(dir, num);
                    std::ifstream file(path, std::ios::binary);
                    if (!readJournal(file, maxSteps, data, brief, insertedItems)) {
                        QMSETUP_FATAL("Read checkpoint task failed when reading journal %d.", num);
                    }
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
            int maxSteps;
            fs::path dir;

            std::vector<Node *> removedItems;
            std::vector<TXData> data;
            std::unordered_map<int, Node *> insertedItems;
        };

        class ReadStepMessageTask : public BaseTask {
        public:
            ReadStepMessageTask(fs::path dir, int step, int maxSteps)
                : BaseTask(false), step(step), maxSteps(maxSteps), dir(std::move(dir)) {
            }

            void execute() override {
                std::unique_lock<std::mutex> lock(mtx);
                {
                    int num = (step - 1) / maxSteps;

                    auto path = journal_path(dir, num);
                    std::ifstream file(path, std::ios::binary);
                    IStream in(&file);

                    QMSETUP_DEBUG("Read attributes at %d in journal %d.", step, num);

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
            fs::path dir;
            Engine::StepMessage res;
        };

        class ResetTask : public BaseTask {
        public:
            ResetTask(fs::path dir, int fsMin, int fsMax, int maxSteps, int maxCheckPoints)
                : BaseTask(true), fsMin(fsMin), fsMax(fsMax), maxSteps(maxSteps),
                  maxCheckPoints(maxCheckPoints), dir(std::move(dir)) {
            }

            void execute() override {
                int oldMin = fsMin;
                int oldMax = fsMax;

                // Write steps
                {
                    auto path = steps_path(dir);
                    bool exists = fs::exists(path);
                    std::ofstream file(path, std::ios::binary | std::ios::app);
                    OStream out(&file);
                    if (!exists) {
                        // Write initial values
                        out << maxSteps << maxCheckPoints;
                    } else {
                        file.seekp(8);
                    }
                    out << int(0) << int(0) << int(0) << int(0);
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

            int fsMin, fsMax;
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
        int maxId;
        Node *root;
        std::unordered_map<int, Node *> insertedItems;
        std::vector<Node *> removedItems;
        std::vector<TXData> backwardData;
        std::vector<TXData> forwardData;

        ~RecoverData() {
            delete root;
            deleteAll(removedItems);
            for (const auto &it : std::as_const(insertedItems)) {
                delete it.second;
            }
            for (const auto &item : std::as_const(backwardData)) {
                deleteAll(item.actions);
            }
            for (const auto &item : std::as_const(forwardData)) {
                deleteAll(item.actions);
            }
        }
    };

    static bool checkDir_helper(const fs::path &dir) {
        if (dir.empty() || !fs::is_directory(dir)) {
            QMSETUP_WARNING("%s: not a directory.", dir.string().data());
            return false;
        }
        return true;
    }

    static WriteCkptTask *generateWriteCkptTask(FileSystemEnginePrivate *d) {
        // Collect all removed items
        std::vector<Node *> removedItems;
        for (auto i = d->stack.size() - d->maxSteps; i != d->stack.size(); ++i) {
            const auto &tx = d->stack.at(i);
            for (const auto &a : std::as_const(tx.actions)) {
                a->virtual_hook(Action::RemovedNodesHook, &removedItems);
            }
        }
        int num = int(d->min + d->stack.size()) / d->maxSteps;
        auto root = NodeHelper::clone(d->model->root(), false);
        return new WriteCkptTask(d->dir, num, root, std::move(removedItems));
    }

    class FileSystemEnginePrivate::JournalData : public BaseProducer {
    public:
        std::unique_ptr<RecoverData> recoverData;

        ReadCkptTask *backward_task = nullptr;
        ReadCkptTask *forward_task = nullptr;
        WriteCkptTask *write_task = nullptr;

        JournalData() = default;

        ~JournalData() {
            abortBackwardReadTask();
            abortForwardReadTask();
            write_task->del();
        }

        Engine::StepMessage getFsStepMessage(const fs::path &_dir, int step, int maxSteps) {
            auto task = new ReadStepMessageTask(_dir, step, maxSteps);

            std::unique_lock<std::mutex> lock(task->mtx);
            pushTask(task, true);

            // Wait for finished
            while (!task->finished) {
                task->cv.wait(lock);
            }
            lock.unlock();

            auto res = std::move(task->res);
            task->del();
            return res;
        }

        inline void abortBackwardReadTask() {
            if (backward_task) {
                backward_task->abort();
                backward_task = nullptr;
            }
        }

        inline void abortForwardReadTask() {
            if (forward_task) {
                forward_task->abort();
                forward_task = nullptr;
            }
        }
    };

    FileSystemEnginePrivate::FileSystemEnginePrivate()
        : journalData(std::make_unique<JournalData>()) {
        fsMin = 0;
        fsMax = 0;
        finished = false;
        maxCheckPoints = 1;
    }

    FileSystemEnginePrivate::~FileSystemEnginePrivate() {
        // Terminate or wait all tasks
        journalData.reset();

        if (finished) {
            // Remove journal
        }
    }

    void FileSystemEnginePrivate::init() {
    }

    void FileSystemEnginePrivate::setup_helper() {
        QM_Q(FileSystemEngine);

        auto recoverData = journalData->recoverData.get();
        if (!recoverData) {
            // Write steps
            {
                auto path = steps_path(dir);
                std::ofstream file(path, std::ios::binary | std::ios::app);
                OStream out(&file);
                out << maxSteps << maxCheckPoints << int(0) << int(0) << int(0) << int(0);
            }

            q->createWarningFile(dir);
            return;
        }

        fsMin = recoverData->fsMin;
        fsMax = recoverData->fsMax;
        maxIndex = recoverData->maxId;

        if (recoverData->root) {
            NodeHelper::setModelRoot(recoverData->root, model);
            recoverData->root = nullptr; // Get ownership
        }

        current = 0;
        min = recoverData->currentNum * maxSteps;

        // Get backward transactions
        if (!recoverData->backwardData.empty()) {
            extractBackwardJournal(recoverData->backwardData, recoverData->removedItems);
        }

        // Get forward transactions
        if (!recoverData->forwardData.empty()) {
            extractForwardJournal(recoverData->forwardData, recoverData->insertedItems);
        }

        int expected = recoverData->fsStep - min;

        // Undo or redo
        while (current > expected) {
            const auto &tx = stack.at(current - 1);
            for (auto it = tx.actions.rbegin(); it != tx.actions.rend(); ++it) {
                (*it)->execute(true);
            }
            current--;
        }

        while (current < expected) {
            const auto &tx = stack.at(current);
            for (auto it = tx.actions.begin(); it != tx.actions.end(); ++it) {
                (*it)->execute(false);
            }
            current++;
        }

        delete recoverData;
        recoverData = nullptr;

        // Need to prepare a checkpoint to write as if a transaction has been committed
        if (stack.size() % maxSteps == 0) {
            journalData->write_task = generateWriteCkptTask(this);
        }
    }

    void FileSystemEnginePrivate::updateStackSize() {
        if (current <= maxSteps / 2) {
            int size = int(stack.size()) - 2 * maxSteps;
            if (size > 0) {
                // Abort forward transactions reading task
                journalData->abortForwardReadTask();

                // Remove tail
                removeActions(2 * maxSteps, int(stack.size()));

                QMSETUP_DEBUG(
                    "Remove forward transactions, size=%d, min=%d, current=%d, stack_size=%d", size,
                    min, current, int(stack.size()));
            }

        } else if (current > maxSteps * 2.5) {
            // Abort backward transactions reading task
            journalData->abortBackwardReadTask();

            // Remove head
            removeActions(0, maxSteps);
            min += maxSteps;
            current -= maxSteps;

            QMSETUP_DEBUG(
                "Remove backward transactions, size=%d, min=%d, current=%d, stack_size=%d",
                maxSteps, min, current, int(stack.size()));
        }
    }

    void FileSystemEnginePrivate::extractBackwardJournal(std::vector<TransactionData> &data,
                                                         std::vector<Node *> &removedItems) {
        QM_Q(FileSystemEngine);

        // Add the removed nodes
        // These nodes are now not on the root item
        for (const auto &node : std::as_const(removedItems)) {
            NodeHelper::propagateEngine(node, q);
            NodeHelper::setManaged(node, true);
        }

        // Now the pointers in the actions are the index value
        // We need to accomplish the deferred reference work here
        for (const auto &item : std::as_const(data)) {
            for (const auto &action : item.actions) {
                action->deferredReference(indexes);
            }
        }

        auto size = int(data.size());

        stack.insert(stack.begin(), data.begin(), data.end());
        min -= size;
        current += size;

        // Get ownership
        data.clear();
        removedItems.clear();
    }

    void FileSystemEnginePrivate::extractForwardJournal(
        std::vector<TransactionData> &data, std::unordered_map<int, Node *> &insertedItems) {
        QM_Q(FileSystemEngine);

        // Add the inserted nodes
        // These nodes are now not on the root item
        for (const auto &it : std::as_const(insertedItems)) {
            auto node = it.second;
            NodeHelper::propagateEngine(node, q);
            NodeHelper::setManaged(node, true);
        }

        // Now the pointers in the actions are the index value
        // We need to accomplish the deferred reference work here
        for (const auto &item : std::as_const(data)) {
            for (const auto &action : item.actions) {
                action->deferredReference(indexes);
            }
        }
        stack.insert(stack.end(), data.begin(), data.end());

        // Get ownership
        data.clear();
        insertedItems.clear();
    }

    bool FileSystemEnginePrivate::acceptChangeMaxSteps(int steps) const {
        return !journalData->recoverData && MemoryEnginePrivate::acceptChangeMaxSteps(steps);
    }

    void FileSystemEnginePrivate::afterCurrentChange() {
        QM_Q(FileSystemEngine);

        auto d2 = journalData.get();

        // Push step updating task
        {
            auto task = new ChangeStepTask(dir, q->current());
            task->fsStep = q->current();
            d2->pushTask(task);
        }

        auto &backward = d2->backward_task;
        auto &forward = d2->forward_task;

        // Check if backward transactions is enough to undo
        if (current <= maxSteps / 2) {

            if (fsMin < min) {
                // Abort forward transactions reading task
                d2->abortForwardReadTask();

                // Need to read backward transactions from file system
                if (!backward) {
                    int num = min / maxSteps - 1;

                    backward = new ReadCkptTask(dir, num, true, maxSteps);
                    d2->pushTask(backward);
                }

                // Need to wait until the reading task finished
                if (current == 0) {
                    std::unique_lock<std::mutex> lock(backward->mtx);
                    while (!backward->finished) {
                        backward->cv.wait(lock);
                    }
                }

                // Insert backward transactions
                if (backward && backward->finished) {
                    QMSETUP_DEBUG("Prepend backward transactions, size=%d",
                                  int(backward->data.size()));
                    extractBackwardJournal(backward->data, backward->removedItems);
                    backward->del();
                    backward = nullptr;
                }
            }

        }

        // Check if forward transactions is enough to redo
        else if (current > stack.size() - maxSteps / 2) {

            if (fsMax > min + stack.size()) {
                // Abort backward transactions reading task
                d2->abortBackwardReadTask();

                // Need to read forward transactions from file system
                if (!forward) {
                    int num = int(min + stack.size()) / maxSteps;
                    forward = new ReadCkptTask(dir, num, false, maxSteps);
                    d2->pushTask(forward);
                }

                // Need to wait until the reading task finished
                if (current == stack.size() - 1) {
                    std::unique_lock<std::mutex> lock(forward->mtx);
                    while (!forward->finished) {
                        forward->cv.wait(lock);
                    }
                }

                // Insert forward transactions
                if (forward && forward->finished) {
                    QMSETUP_DEBUG("Append backward transactions, size=%d",
                                  int(forward->data.size()));
                    extractForwardJournal(forward->data, forward->insertedItems);
                    forward->del();
                    forward = nullptr;
                }
            }
        }

        // Over
        updateStackSize();
    }

    void FileSystemEnginePrivate::afterCommit(const std::vector<Action *> &actions,
                                              const Engine::StepMessage &message) {
        auto d2 = journalData.get();
        auto &writeCkptTask = d2->write_task;

        auto oldFsMin = fsMin;
        auto oldFsMax = fsMax;

        // Update fsMax
        fsMax = min + current;

        // Update fsMin
        int expectMin;
        if (maxCheckPoints >= 0 &&
            (expectMin = ((fsMax - 1) / maxSteps - (maxCheckPoints + 3)) * maxSteps) > fsMin) {
            fsMin = expectMin;
        }

        // Abort forward transactions reading task
        d2->abortForwardReadTask();

        // Abort backward transactions reading task
        if (current > maxSteps * 1.5)
            d2->abortBackwardReadTask();

        // Delete formal checkpoint task
        {
            auto rem = stack.size() % maxSteps;
            if (rem == 0) {
                delete writeCkptTask;

                // Save checkpoint task
                writeCkptTask = generateWriteCkptTask(this);
            } else if (rem == 1 && stack.size() > 1) {
                // Deferred push checkpoint task
                if (writeCkptTask) {
                    d2->pushTask(writeCkptTask);
                    writeCkptTask = nullptr;
                }
            } else if (writeCkptTask) {
                delete writeCkptTask;
                writeCkptTask = nullptr;
            }
        }

        // Add commit task (Must do it after writing checkpoint)
        {
            auto newActions = actions;
            for (auto &action : newActions) {
                action = action->clone();
                action->detach();
            }
            auto task = new CommitTask(dir, fsMax, fsMin, oldFsMin, oldFsMax, maxSteps, maxIndex,
                                       {newActions, message});
            d2->pushTask(task);
        }
        updateStackSize();
    }

    void FileSystemEnginePrivate::afterReset() {
        auto oldFsMin = fsMin;
        auto oldFsMax = fsMax;

        fsMin = 0;
        fsMax = 0;

        auto d2 = journalData.get();
        auto &write_task = d2->write_task;
        if (write_task) {
            write_task->del();
            write_task = nullptr;
        }

        d2->abortForwardReadTask();
        d2->abortBackwardReadTask();

        auto task = new ResetTask(dir, oldFsMin, oldFsMax, maxSteps, maxCheckPoints);
        d2->pushTask(task);
    }

    FileSystemEngine::FileSystemEngine() : FileSystemEngine(*new FileSystemEnginePrivate()) {
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
        QM_D(FileSystemEngine);
        if (!checkDir_helper(dir)) {
            return false;
        }

        auto d2 = d->journalData.get();
        if (d2->recoverData) {
            d2->recoverData.reset();
        }
        d->dir = dir;
        return true;
    }

    bool FileSystemEngine::recover(const fs::path &dir) {
        QM_D(FileSystemEngine);

        if (!checkDir_helper(dir)) {
            return false;
        }

        // Read steps
        int maxSteps, maxCheckPoints, fsMin, fsMax, fsStep, maxId;
        auto readSteps = [&](std::istream &file) {
            IStream in(&file);
            in >> maxSteps >> maxCheckPoints >> fsMin >> fsMax >> fsStep >> maxId;
            return in.good();
        };

        {
            auto path = steps_path(dir);
            std::ifstream file(path, std::ios::binary);
            if (!file.is_open() || !readSteps(file)) {
                QMSETUP_WARNING("Read steps file failed.");
                return false;
            }
        }

        // Unhandled inconsistency (Merely impossible)
        // 1. Crash during updating steps

        // Fix possible inconsistency:
        // 1. Crash during writing committed transaction or writing checkpoint,
        //    before updating steps
        {
            int64_t expected = fsMax % maxSteps;
            if (expected > 0) {
                int num = (fsMax - 1) / maxSteps;
                auto path = journal_path(dir, num);
                std::fstream file(path,
                                  std::ios::binary | std::ios::in | std::ios::out | std::ios::app);
                if (!file.is_open()) {
                    QMSETUP_WARNING("Read journal %d failed.", num);
                    return false;
                }

                IStream in(&file);
                OStream out(&file);
                int64_t cur;
                in >> cur;

                if (cur != expected) {
                    QMSETUP_WARNING("Journal step inconsistent, expected %lld, actual %lld.",
                                    expected, cur);

                    file.seekp(0);
                    out << expected;

                    // Truncate
                    file.seekg(cur * INT64_SIZE); // Transaction end
                    int64_t pos;
                    in >> pos;

                    file.seekp(file.tellg());
                    out << int64_t(-1);

                    file.close();
                    fs::resize_file(path, pos);
                }
            } else {
                // The first transaction may fail to be flushed
                truncateJournals(dir, fsMax / maxSteps);
            }
        }

        // 2. Crash during truncating logs
        {
            // Remove backward logs
            int minNum = fsMin / maxSteps;
            for (int i = minNum - 1; i >= 0; --i) {
                if (!truncateJournals(dir, i))
                    break;
            }

            // Remove forward logs
            int maxNum = (fsMax - 1) / maxSteps;
            for (int i = maxNum + 1;; ++i) {
                if (!truncateJournals(dir, i))
                    break;
            }

            // Check existence
            for (int i = minNum; i <= maxNum; ++i) {
                if (!truncateJournals(dir, i, true)) {
                    QMSETUP_WARNING("Check checkpoint or journal failed at %d.", i);
                    return false;
                }
            }
        }

        if (fsMax == 0) {
            d->dir = dir;
            return true;
        }

        // Get nearest checkpoint
        // Suppose maxSteps = 100
        // 1. 51 <= fsStep <= 150                         -> num = 1
        // 2. 151 <= fsStep <= 250                        -> num = 2
        // 3. fsStep >= 151, fsMin = 100, fsMax <= 200    -> num = 1

        int minNum = fsMin / maxSteps;
        int maxNum = std::max((fsMax - 1) / maxSteps, 0);
        int num = std::min((fsStep + maxSteps / 2 - 1) / maxSteps, maxNum);

        QMSETUP_DEBUG("Restore, min=%d, max=%d, cur=%d", fsMin, fsMax, fsStep);
        QMSETUP_DEBUG("Read checkpoint %d", num);

        Node *root = nullptr;
        std::vector<Node *> removedItems;
        std::unordered_map<int, Node *> insertedItems;
        std::vector<TXData> backwardData;
        std::vector<TXData> forwardData;

        if (num > 0) {
            bool needBackward = num > minNum;

            // Read checkpoint
            {
                auto path = ckpt_path(dir, num);
                std::ifstream file(path, std::ios::binary);
                if (!file.is_open() ||
                    !readCheckPoint(file, &root, needBackward ? &removedItems : nullptr)) {
                    QMSETUP_WARNING("Read checkpoint %d failed.", num);
                    return false;
                }
            }

            // Read backward transactions
            if (needBackward) {
                QMSETUP_DEBUG("Restore backward transactions %d", num - 1);

                auto path = journal_path(dir, num - 1);
                std::ifstream file(path, std::ios::binary);
                if (!file.is_open() ||
                    !readJournal(file, maxSteps, backwardData, true, insertedItems)) {
                    QMSETUP_WARNING("Read journal %d failed.", num - 1);
                    goto failed;
                }
            }
        }

        {
            // Read forward transactions
            QMSETUP_DEBUG("Restore forward transactions %d", num);

            auto path = journal_path(dir, num);
            std::ifstream file(path, std::ios::binary);
            if (!file.is_open() ||
                !readJournal(file, maxSteps, forwardData, false, insertedItems)) {
                QMSETUP_WARNING("Read journal %d failed.", num);
                goto failed;
            }
        }

        goto success;

    failed:
        delete root;
        deleteAll(removedItems);
        for (const auto &it : std::as_const(insertedItems)) {
            delete it.second;
        }
        return false;

    success:
        auto rdata = new RecoverData();
        rdata->fsMin = fsMin;
        rdata->fsMax = fsMax;
        rdata->fsStep = fsStep;
        rdata->currentNum = num;
        rdata->maxId = maxId;
        rdata->root = root;
        rdata->insertedItems = std::move(insertedItems);
        rdata->removedItems = std::move(removedItems);
        rdata->backwardData = std::move(backwardData);
        rdata->forwardData = std::move(forwardData);
        d->maxSteps = maxSteps;
        d->maxCheckPoints = maxCheckPoints;
        d->journalData->recoverData.reset(rdata);
        d->dir = dir;
        return true;
    }

    void FileSystemEngine::setup(Model *model) {
        QM_D(FileSystemEngine);
        d->model = model;
        d->setup_helper();
    }

    int FileSystemEngine::minimum() const {
        QM_D(const FileSystemEngine);
        return d->fsMin;
    }

    int FileSystemEngine::maximum() const {
        QM_D(const FileSystemEngine);
        return d->fsMax;
    }

    Engine::StepMessage FileSystemEngine::stepMessage(int step) const {
        QM_D(const FileSystemEngine);

        if (step <= d->fsMin || step > d->fsMax) {
            return {};
        }

        int step2 = step - (d->min + 1);
        if (step2 < 0 || step2 >= d->stack.size()) {
            return d->journalData->getFsStepMessage(d->dir, step, d->maxSteps);
        }
        return d->stack.at(step2).message;
    }

    bool FileSystemEngine::createWarningFile(const fs::path &dir) {
        std::ofstream file(dir / "WARNING.txt", std::ios::trunc);
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