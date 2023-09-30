#ifndef SQLITEENGINE_P_H
#define SQLITEENGINE_P_H

#include <substate/private/memengine_p.h>
#include <sqliteengine/sqliteengine.h>

namespace Substate {

    class SQLITEENGINE_EXPORT SqliteEnginePrivate : public MemEnginePrivate {
    public:
        SqliteEnginePrivate();
        ~SqliteEnginePrivate();
        void init();
    };

}

#endif // SQLITEENGINE_P_H