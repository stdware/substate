#ifndef ACTION_H
#define ACTION_H

#include <memory>
#include <string>
#include <vector>

#include <substate/stream.h>

namespace Substate {

    class ActionHelper;

    class SUBSTATE_EXPORT Action {
    public:
        inline Action(int type);
        virtual ~Action();

        enum Type {
            BytesReplace,
            BytesInsert,
            BytesRemove,

            VectorInsert,
            VectorMove,
            VectorRemove,

            RecordInsert,
            RecordRemove,

            MappingInsert,
            MappingRemove,

            RootChange,

            User = 1024,
        };

        inline Type type() const;
        inline int userType() const;

        typedef Action *(*Factory)(IStream &);

        static Action *read(IStream &stream);
        static bool registerFactory(int type, Factory fac);

    public:
        virtual Action *clone() const = 0;

    protected:
        virtual void execute(bool undo) = 0;
        virtual void clean();

    protected:
        int t;

        friend class ActionHelper;

        SUBSTATE_DISABLE_COPY_MOVE(Action)
    };

    inline Action::Action(int type) : t(type) {
    }

    inline Action::Type Action::type() const {
        return t >= User ? User : static_cast<Type>(t);
    }

    inline int Action::userType() const {
        return t;
    }

}

#endif // ACTION_H
