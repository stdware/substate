#ifndef BYTESNODE_P_H
#define BYTESNODE_P_H

#include <vector>

#include <substate/bytesnode.h>
#include <substate/private/node_p.h>

namespace Substate {

    class BytesNodePrivate : public NodePrivate {
        QMSETUP_DECL_PUBLIC(BytesNode)
    public:
        BytesNodePrivate(const std::string &name);
        ~BytesNodePrivate();

        static BytesNode *read(IStream &stream);

        void replaceBytes_helper(int index, const char *data, int size);
        void insertBytes_helper(int index, const char *data, int size);
        void removeBytes_helper(int index, int size);

        std::vector<char> byteArray;
    };

    BytesAction *readBytesAction(Action::Type type, IStream &stream);

}

#endif // BYTESNODE_P_H
