#include "arrayentity.h"

#include <substate/bytesnode.h>

#include "entity_p.h"

namespace Substate {

    class ArrayEntityBasePrivate : public EntityPrivate {
        Q_DECLARE_PUBLIC(ArrayEntityBase)
    public:
        ArrayEntityBasePrivate(Node *node) : EntityPrivate(node) {
        }

        ~ArrayEntityBasePrivate() = default;

        void init() {
        }

        void notified(Notification *n) {
            switch (n->type()) {
                case Notification::ActionTriggered: {
                    auto n2 = static_cast<ActionNotification *>(n);
                    auto a = n2->action();
                    switch (a->type()) {
                        case Action::BytesReplace: {
                            auto a2 = static_cast<BytesAction *>(a);
                            sendReplace(a2->index(), a2->bytes(), a2->oldBytes());
                            break;
                        }
                        case Action::BytesInsert: {
                            auto a2 = static_cast<BytesAction *>(a);
                            sendInsert(a2->index(), a2->bytes());
                            break;
                        }
                        case Action::BytesRemove: {
                            auto a2 = static_cast<BytesAction *>(a);
                            sendRemove(a2->index(), a2->bytes());
                            break;
                        }
                        default:
                            break;
                    }
                    break;
                }
                default:
                    break;
            }
        }

        inline void sendReplace(int index, const ByteArray &bytes, const ByteArray &oldBytes) {
            Q_Q(ArrayEntityBase);
            q->sendReplace(index, bytes.data(), bytes.size());
        }

        inline void sendInsert(int index, const ByteArray &bytes) {
            Q_Q(ArrayEntityBase);
            q->sendInsert(index, bytes.data(), bytes.size());
        }

        inline void sendRemove(int index, const ByteArray &bytes) {
            Q_Q(ArrayEntityBase);
            q->sendInsert(index, bytes.data(), bytes.size());
        }
    };

    ArrayEntityBase::~ArrayEntityBase() {
    }

    int ArrayEntityBase::sizeImpl() const {
        Q_D(const ArrayEntityBase);
        return static_cast<BytesNode *>(d->internalData())->size();
    }

    const char *ArrayEntityBase::valuesImpl() const {
        Q_D(const ArrayEntityBase);
        return static_cast<BytesNode *>(d->internalData())->data();
    }

    void ArrayEntityBase::replaceImpl(int index, const char *data, int size) {
        Q_D(ArrayEntityBase);
        static_cast<BytesNode *>(d->internalData())->replace(index, data, size);
    }

    void ArrayEntityBase::insertImpl(int index, const char *data, int size) {
        Q_D(ArrayEntityBase);
        static_cast<BytesNode *>(d->internalData())->insert(index, data, size);
    }

    void ArrayEntityBase::removeImpl(int index, int size) {
        Q_D(ArrayEntityBase);
        static_cast<BytesNode *>(d->internalData())->remove(index, size);
    }

    ArrayEntityBase::ArrayEntityBase(Node *node, QObject *parent)
        : Entity(*new ArrayEntityBasePrivate(node), parent) {
    }

    ArrayEntityBase::ArrayEntityBase(ArrayEntityBasePrivate &d, QObject *parent)
        : Entity(d, parent) {
        d.init();
    }

}