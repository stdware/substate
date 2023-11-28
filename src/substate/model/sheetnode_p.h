#ifndef SHEETNODE_P_H
#define SHEETNODE_P_H

#include <set>

#include <substate/sheetnode.h>
#include <substate/private/node_p.h>

namespace Substate {

    class SUBSTATE_EXPORT SheetNodePrivate : public NodePrivate {
        QMSETUP_DECL_PUBLIC(SheetNode)
    public:
        SheetNodePrivate(int type);
        ~SheetNodePrivate();
        void init();

        static SheetNode *read(IStream &stream);

        void addRecord_helper(int id, Node *node);
        void removeRecord_helper(int id);

        std::unordered_map<int, Node *> records;
        std::unordered_map<Node *, int> recordIndexes;

        std::set<int> recordIds; // To preserve max seq
    };

    SheetAction *readSheetAction(Action::Type type, IStream &stream,
                                 const std::unordered_map<int, Node *> &existingNodes);

}

#endif // SHEETNODE_P_H
