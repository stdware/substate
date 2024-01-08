#include "structnode.h"
#include "structnode_p.h"

#include <cassert>

#include "nodehelper.h"

namespace Substate {

    StructNodePrivate::StructNodePrivate(int type, int size) : NodePrivate(type) {
        array.resize(size);
    }

    StructNodePrivate::~StructNodePrivate() {
        for (const auto &pair : std::as_const(arrayIndexes)) {
            delete pair.first;
        }
    }

    void StructNodePrivate::init() {
    }

    StructNode *StructNodePrivate::read(IStream &stream) {
        int size, index;
        stream >> index >> size;
        if (stream.fail()) {
            return nullptr;
        }

        auto node = new StructNode(size);
        auto d = node->d_func();

        d->index = index;
        for (int i = 0; i < size; ++i) {
            Property prop;
            stream >> prop;
            if (stream.fail()) {
                goto abort;
            }
            if (prop.isNode())
                d->arrayIndexes.insert(std::make_pair(prop.node(), i));
            d->array[i] = prop;
        }
        return node;

    abort:
        delete node;
        return nullptr;
    }

    void StructNodePrivate::assign_helper(int i, const Property &prop) {
        QM_Q(StructNode);
        q->beginAction();

        Property oldProp = array.at(i);
        if (prop == oldProp) {
            return;
        }

        StructAction a(q, i, prop, oldProp);

        // Pre-Propagate signal
        {
            ActionNotification n(Notification::ActionAboutToTrigger, &a);
            q->dispatch(&n);
        }

        // Do change
        array[i] = prop;

        if (oldProp.isNode()) {
            auto oldNode = oldProp.node();
            arrayIndexes.erase(oldNode);
            q->removeChild(oldNode);
        }
        if (prop.node()) {
            auto node = prop.node();
            arrayIndexes.insert(std::make_pair(node, i));
            q->addChild(node);
        }

        // Propagate signal
        {
            ActionNotification n(Notification::ActionTriggered, &a);
            q->dispatch(&n);
        }

        q->endAction();
    }

    StructAction *readStructAction(IStream &stream) {
        int parentIndex;
        int index;

        stream >> parentIndex >> index;
        if (stream.fail())
            return nullptr;

        auto parent = reinterpret_cast<Node *>(uintptr_t(parentIndex));

        auto v = Property::read(stream);
        if (stream.fail())
            return nullptr;

        auto oldv = Property::read(stream);
        if (stream.fail())
            return nullptr;

        auto a = new StructAction(parent, index, v, oldv);
        a->setState(Action::Unreferenced);
        return a;
    }

    StructNode::StructNode(int size) : StructNode(*new StructNodePrivate(Struct, size)) {
    }

    StructNode::~StructNode() {
    }

    bool StructNode::assign(int i, const Property &value) {
        QM_D(StructNode);
        assert(d->testModifiable());

        if (i < 0 || i >= d->array.size()) {
            QMSETUP_WARNING("index %d out of range", i);
            return false;
        }

        d->assign_helper(i, value);
        return true;
    }

    Property StructNode::at(int i) const {
        QM_D(const StructNode);
        return d->array.at(i);
    }

    bool StructNode::remove(Node *node) {
        QM_D(StructNode);
        assert(d->testModifiable());

        // Validate
        if (!node) {
            QMSETUP_WARNING("trying to remove a null node from %p", this);
            return false;
        }

        auto it = d->arrayIndexes.find(node);
        if (it == d->arrayIndexes.end()) {
            QMSETUP_WARNING("node %p is not the child of %p", node, this);
            return false;
        }

        d->assign_helper(it->second, {});
        return true;
    }

    int StructNode::indexOf(Node *node) const {
        QM_D(const StructNode);
        auto it = d->arrayIndexes.find(node);
        if (it == d->arrayIndexes.end()) {
            return {};
        }
        return it->second;
    }

    const std::vector<Property> &StructNode::data() const {
        QM_D(const StructNode);
        return d->array;
    }

    int StructNode::size() const {
        QM_D(const StructNode);
        return d->array.size();
    }

    void StructNode::write(OStream &stream) const {
        QM_D(const StructNode);
        stream << d->index << d->array;
    }

    Node *StructNode::clone(bool user) const {
        QM_D(const StructNode);

        auto node = new StructNode(d->array.size());
        auto d2 = node->d_func();
        if (user)
            d2->index = d->index;

        // Clone children
        for (int i = 0; i < d->array.size(); ++i) {
            const auto &data = d->array.at(i);

            if (data.isVariant()) {
                d2->array[i] = data.variant();
                continue;
            }

            auto newChild = NodeHelper::clone(data.node(), user);
            node->addChild(newChild);

            d2->array[i] = newChild;
            d2->arrayIndexes.insert(std::make_pair(newChild, i));
        }
        return node;
    }

    void StructNode::childDestroyed(Node *node) {
        remove(node);
    }

    void StructNode::propagateChildren(const std::function<void(Node *)> &func) {
        QM_D(StructNode);
        for (const auto &pair : std::as_const(d->arrayIndexes)) {
            NodeHelper::propagateNode(pair.first, func);
        }
    }

    StructNode::StructNode(StructNodePrivate &d) : Node(d) {
        d.init();
    }

    StructAction::StructAction(Node *parent, int index, const Property &value,
                               const Property &oldValue)
        : PropertyAction(StructAssign, parent, value, oldValue), m_index(index) {
    }

    StructAction::~StructAction() {
    }

    void StructAction::write(OStream &stream) const {
        stream << m_parent->index() << m_index;
        v.write(stream);
        oldv.write(stream);
    }

    Action *StructAction::clone() const {
        return new StructAction(m_parent, m_index, v, oldv);
    }

    void StructAction::execute(bool undo) {
        auto d = static_cast<StructNode *>(m_parent)->d_func();
        d->assign_helper(m_index, undo ? oldv : v);
    }

}