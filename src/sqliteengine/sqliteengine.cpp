#include "sqliteengine.h"
#include "sqliteengine_p.h"

namespace Substate {

    SqliteEnginePrivate::SqliteEnginePrivate() {
    }

    SqliteEnginePrivate::~SqliteEnginePrivate() {
    }

    void SqliteEnginePrivate::init() {
    }

    SqliteEngine::SqliteEngine() {
    }

    SqliteEngine::~SqliteEngine() {
    }

    SqliteEngine::SqliteEngine(SqliteEnginePrivate &d) : MemoryEngine(d) {
        d.init();
    }

}