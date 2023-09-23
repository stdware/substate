#ifndef DOCUMENT_P_H
#define DOCUMENT_P_H

#include <substate/document.h>

namespace Substate {

    class DocumentPrivate {
        SUBSTATE_DECL_PUBLIC(Document)
    public:
        DocumentPrivate();
        virtual ~DocumentPrivate();
        void init();
        Document *q_ptr;
    };

}

#endif // DOCUMENT_P_H
