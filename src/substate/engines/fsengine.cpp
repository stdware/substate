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

    FileSystemEngine::FileSystemEngine(FileSystemEnginePrivate &d) : MemoryEngine(d) {
        d.init();
    }

}