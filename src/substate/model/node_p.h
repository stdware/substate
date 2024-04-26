#ifndef NODE_P_H
#define NODE_P_H

#include <unordered_map>

#include <substate/node.h>
#include <substate/engine.h>
#include <substate/private/sender_p.h>

namespace Substate {

    class SUBSTATE_EXPORT NodePrivate : public SenderPrivate {
        QMSETUP_DECL_PUBLIC(Node)
    public:
        NodePrivate(int type, const std::string &name);
        ~NodePrivate();
        void init();

        int type;
        std::string name;
        Node *parent = nullptr;

        Engine *engine = nullptr;
        Model *model = nullptr;
        int index = 0;

        bool managed = false;
        bool allowDelete = false;

        NodeExtra *extra = nullptr;

        std::unordered_map<std::string, Variant> dynDataMap;

        void setManaged(bool _managed);
        void propagateEngine(Engine *_engine);

        bool testModifiable() const;
        inline bool testInsertable(const Node *item) const {
            return item && item->isFree();
        }
    };

    inline bool validateArrayQueryArguments(int index, int size) {
        return index >= 0 && index <= size;
    }

    inline bool validateArrayRemoveArguments(int index, int &count, int size) {
        return (index >= 0 && index <= size)                                     // index bound
               && ((count = std::min(count, size - index)) > 0 && count <= size) // count bound
            ;
    }

    RootChangeAction *readRootChangeAction(IStream &stream);

}

#endif // NODE_P_H
