#include "fsengine.h"
#include "fsengine_p.h"

namespace Substate {
    
    FileSystemEnginePrivate::FileSystemEnginePrivate() {
    }

    FileSystemEnginePrivate::~FileSystemEnginePrivate() {
    }

    void FileSystemEnginePrivate::init() {
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