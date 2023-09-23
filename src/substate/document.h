#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <memory>

#include <substate/substate_global.h>

namespace Substate {

    class DocumentPrivate;

    class SUBSTATE_EXPORT Document {
        SUBSTATE_DECL_PRIVATE(Document)
    public:
        Document();
        virtual ~Document();

    public:
    

    protected:
        Document(DocumentPrivate &d);
        std::unique_ptr<DocumentPrivate> d_ptr;
    };

}

#endif // DOCUMENT_H
