#include "fsengine.h"
#include "fsengine_p.h"

namespace Substate {

    FileSystemEnginePrivate::FileSystemEnginePrivate() {
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
        return 0;
    }

    void FileSystemEngine::setCheckpoints(int n) {
    }

    bool FileSystemEngine::finished() const {
        QM_D(const FileSystemEngine);
        return d->finished;
    }

    void FileSystemEngine::setFinished(bool finished) {
        QM_D(FileSystemEngine);
        d->finished = finished;
    }

    bool FileSystemEngine::start(const std::filesystem::path &dir) {
        return false;
    }

    bool FileSystemEngine::recover(const std::filesystem::path &dir) {
        return false;
    }

    bool FileSystemEngine::switchDir(const std::filesystem::path &dir) {
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
        return MemoryEngine::stepMessage(step);
    }

    FileSystemEngine::FileSystemEngine(FileSystemEnginePrivate &d) : MemoryEngine(d) {
        d.init();
    }

}