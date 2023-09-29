#include "bytesnode.h"
#include "bytesnode_p.h"

#include "nodehelper.h"

#include <cassert>

namespace Substate {

    static bool validateArrayQueryArguments(int index, int size) {
        return index >= 0 && index <= size;
    }

    static bool validateArrayRemoveArguments(int index, int &count, int size) {
        return (index >= 0 && index <= size)                                     // index bound
               && ((count = std::min(count, size - index)) > 0 && count <= size) // count bound
            ;
    }

    BytesNodePrivate::BytesNodePrivate() : NodePrivate(Node::Bytes) {
    }

    BytesNodePrivate::~BytesNodePrivate() {
    }

    BytesNode *BytesNodePrivate::read(IStream &stream) {
        return nullptr;
    }

    void BytesNodePrivate::replaceBytes_helper(int index, const char *data, int size) {
        Q_Q(BytesNode);
        q->beginAction();

        ByteArray oldBytes(byteArray.data() + index, size);

        // Do change
        int newSize = index + size;
        if (newSize > byteArray.size())
            byteArray.resize(newSize);
        std::copy(data, data + size, byteArray.begin() + index);

        // Propagate signal
        {
            BytesAction a(Action::BytesReplace, q, index, ByteArray(data, size), oldBytes);
            ActionNotification n(Notification::ActionTriggered, &a);
            q->dispatch(&n);
        }

        q->endAction();
    }

    void BytesNodePrivate::insertBytes_helper(int index, const char *data, int size) {
        Q_Q(BytesNode);
        q->beginAction();

        // Do change
        byteArray.insert(byteArray.begin() + index, data, data + size);

        // Propagate signal
        {
            BytesAction a(Action::BytesInsert, q, index, ByteArray(data, size));
            ActionNotification n(Notification::ActionTriggered, &a);
            q->dispatch(&n);
        }

        q->endAction();
    }

    void BytesNodePrivate::removeBytes_helper(int index, int size) {
        Q_Q(BytesNode);
        q->beginAction();

        ByteArray bytes(byteArray.data() + index, size);

        // Do change
        byteArray.erase(byteArray.begin() + index, byteArray.begin() + index + size);

        // Propagate signal
        {
            BytesAction a(Action::BytesRemove, q, index, bytes);
            ActionNotification n(Notification::ActionTriggered, &a);
            q->dispatch(&n);
        }

        q->endAction();
    }

    BytesNode::BytesNode() : Node(*new BytesNodePrivate()) {
    }

    BytesNode::~BytesNode() {
    }

    bool BytesNode::insert(int index, const char *data, int size) {
        Q_D(BytesNode);
        assert(d->testModifiable());

        // Validate
        if (!validateArrayQueryArguments(index, d->byteArray.size()) || (!data || size == 0)) {
            SUBSTATE_WARNING("invalid parameters");
            return false;
        }

        d->insertBytes_helper(index, data, size);
        return true;
    }

    bool BytesNode::remove(int index, int size) {
        Q_D(BytesNode);
        assert(d->testModifiable());

        // Validate
        if (!validateArrayRemoveArguments(index, size, d->byteArray.size()) || size == 0) {
            SUBSTATE_WARNING("invalid parameters");
            return false;
        }

        d->removeBytes_helper(index, size);
        return true;
    }

    bool BytesNode::replace(int index, const char *data, int size) {
        Q_D(BytesNode);
        assert(d->testModifiable());

        // Validate
        if (!validateArrayQueryArguments(index, d->byteArray.size()) || (!data || size == 0)) {
            SUBSTATE_WARNING("invalid parameters");
            return false;
        }

        d->replaceBytes_helper(index, data, size);
        return true;
    }

    const char *BytesNode::data() const {
        Q_D(const BytesNode);
        return d->byteArray.data();
    }

    int BytesNode::size() const {
        Q_D(const BytesNode);
        return int(d->byteArray.size());
    }

    void BytesNode::write(OStream &stream) const {
    }

    Node *BytesNode::clone(bool user) const {
        Q_D(const BytesNode);

        auto node = new BytesNode();
        auto d2 = node->d_func();
        if (user)
            d2->index = d->index;
        d2->byteArray = d->byteArray;
        return node;
    }

    BytesAction::BytesAction(Action::Type type, Node *parent, int index, const ByteArray &bytes,
                             const ByteArray &oldBytes)
        : NodeAction(type, parent), m_index(index), b(bytes), oldb(oldBytes) {
    }

    BytesAction::~BytesAction() {
    }

    Action *BytesAction::clone() const {
        return new BytesAction(static_cast<Type>(t), m_parent, m_index, b, oldb);
    }

    void BytesAction::execute(bool undo) {
        switch (t) {
            case BytesReplace: {
                auto d = static_cast<BytesNodePrivate *>(NodeHelper::get(m_parent));
                if (undo) {
                    d->replaceBytes_helper(m_index, oldb.data(), oldb.size());

                    // Need truncate
                    int delta = b.size() - oldb.size();
                    if (delta > 0) {
                        d->removeBytes_helper(d->byteArray.size() - delta, delta);
                    }
                } else {
                    d->replaceBytes_helper(m_index, b.data(), b.size());
                }
                break;
            }
            case BytesInsert:
            case BytesRemove: {
                auto d = static_cast<BytesNodePrivate *>(NodeHelper::get(m_parent));
                ((t == BytesRemove) ^ undo) ? d->removeBytes_helper(m_index, b.size())
                                            : d->insertBytes_helper(m_index, b.data(), b.size());
                break;
            }
            default:
                // Unreachable
                break;
        }
    }

}