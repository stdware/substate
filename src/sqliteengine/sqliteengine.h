#ifndef SQLITEENGINE_H
#define SQLITEENGINE_H

#include <substate/memengine.h>
#include <sqliteengine/sqliteengine_global.h>

namespace Substate {

    class SqliteEnginePrivate;

    class SQLITEENGINE_EXPORT SqliteEngine : public MemoryEngine {
        SUBSTATE_DECL_PRIVATE(SqliteEngine)
    public:
        SqliteEngine();
        ~SqliteEngine();

    protected:
        SqliteEngine(SqliteEnginePrivate &d);
    };

}

#endif // SQLITEENGINE_H
