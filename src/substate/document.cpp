#include "document.h"
#include "document_p.h"

namespace Substate {

    DocumentPrivate::DocumentPrivate() {
    }

    DocumentPrivate::~DocumentPrivate() {
    }

    void DocumentPrivate::init() {
    }

    Document::Document() {
    }

    Document::~Document() {
    }

    Document::Document(DocumentPrivate &d) : d_ptr(&d) {
        d.q_ptr = this;
        d.init();
    }

}