#ifndef NODE_H
#define NODE_H

#include <string>

#include <substate/substate_global.h>

namespace Substate {

     class SUBSTATE_EXPORT Node {
     public:
         Node();
         ~Node();

     protected:
         virtual void addChild(Node *node);
         virtual void removeChild(Node *node);

     protected:
         SUBSTATE_DISABLE_COPY(Node)
     };

}

#endif // NODE_H
