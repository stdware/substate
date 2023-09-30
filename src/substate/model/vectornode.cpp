#include "vectornode.h"
#include "vectornode_p.h"

#include <cassert>

#include "nodehelper.h"

namespace Substate {

    // Move item inside the array
    template <class T>
    static void arrayMove(std::vector<T> &arr, int index, int count, int dest) {
        std::vector<T> tmp;
        tmp.resize(count);
        std::copy(arr.begin() + index, arr.begin() + index + count, tmp.begin());

        // Do change
        int correctDest;
        if (dest > index) {
            correctDest = dest - count;
            auto sz = correctDest - index;
            for (int i = 0; i < sz; ++i) {
                arr[index + i] = arr[index + count + i];
            }
        } else {
            correctDest = dest;
            auto sz = index - dest;
            for (int i = sz - 1; i >= 0; --i) {
                arr[dest + count + i] = arr[dest + i];
            }
        }
        std::copy(tmp.begin(), tmp.end(), arr.begin() + correctDest);
    }

    VectorNodePrivate::VectorNodePrivate(int type) : NodePrivate(type) {
    }

    VectorNodePrivate::~VectorNodePrivate() {
        for (const auto &node : std::as_const(vector)) {
            delete node;
        }
    }

    void VectorNodePrivate::init() {
    }

    VectorNode *VectorNodePrivate::read(IStream &stream) {
        auto node = new VectorNode();
        auto d = node->d_func();

        int size;
        stream >> d->index >> size;
        if (stream.fail()) {
            goto abort;
        }

        d->vector.reserve(size);
        for (int i = 0; i < size; ++i) {
            auto child = Node::read(stream);
            if (!child) {
                goto abort;
            }
            node->addChild(child);
            d->vector.push_back(node);
        }
        return node;

    abort:
        delete node;
        return nullptr;
    }

    void VectorNodePrivate::insertRows_helper(int index, const std::vector<Node *> &nodes) {
        Q_Q(VectorNode);
        q->beginAction();

        // Do change
        vector.insert(vector.begin() + index, nodes.size(), nullptr);
        for (int i = 0; i < nodes.size(); ++i) {
            auto item = nodes[i];
            vector[index + i] = item;
            q->addChild(item);
        }

        // Propagate signal
        {
            VectorInsDelAction a(Action::VectorInsert, q, index, nodes);
            ActionNotification n(Notification::ActionTriggered, &a);
            q->dispatch(&n);
        }

        q->endAction();
    }

    void VectorNodePrivate::moveRows_helper(int index, int count, int dest) {
        Q_Q(VectorNode);
        q->beginAction();

        VectorMoveAction a(q, index, count, dest);

        // Pre-Propagate signal
        {
            ActionNotification n(Notification::ActionAboutToTrigger, &a);
            q->dispatch(&n);
        }

        // Do change
        arrayMove(vector, index, count, dest);

        // Propagate signal
        {
            ActionNotification n(Notification::ActionTriggered, &a);
            q->dispatch(&n);
        }

        q->endAction();
    }

    void VectorNodePrivate::removeRows_helper(int index, int count) {
        Q_Q(VectorNode);
        q->beginAction();

        std::vector<Node *> tmp;
        tmp.resize(count);
        std::copy(vector.begin() + index, vector.begin() + index + count, tmp.begin());

        VectorInsDelAction a(Action::VectorRemove, q, index, tmp);

        // Pre-Propagate signal
        {
            ActionNotification n(Notification::ActionAboutToTrigger, &a);
            q->dispatch(&n);
        }

        // Do change
        for (const auto &item : std::as_const(tmp)) {
            q->removeChild(item);
        }
        vector.erase(vector.begin() + index, vector.begin() + index + count);

        // Propagate signal
        {
            ActionNotification n(Notification::ActionTriggered, &a);
            q->dispatch(&n);
        }
    }

    VectorNode::VectorNode() : VectorNode(*new VectorNodePrivate(Vector)) {
    }

    VectorNode::~VectorNode() {
    }

    bool VectorNode::insert(int index, const std::vector<Node *> &nodes) {
        Q_D(VectorNode);
        assert(d->testModifiable());

        // Validate
        if (!validateArrayQueryArguments(index, d->vector.size()) || nodes.empty()) {
            SUBSTATE_WARNING("invalid parameters");
            return false;
        }
        for (const auto &node : nodes) {
            if (!d->testInsertable(node)) {
                SUBSTATE_WARNING("node %p is not able to be inserted", node);
                return false;
            }
        }

        d->insertRows_helper(index, nodes);
        return true;
    }

    bool VectorNode::move(int index, int count, int dest) {
        Q_D(VectorNode);
        assert(d->testModifiable());

        // Validate
        if (!validateArrayRemoveArguments(index, count, d->vector.size()) ||
            (dest >= index && dest <= index + count) // dest bound
        ) {
            SUBSTATE_WARNING("invalid parameters");
            return false;
        }

        d->moveRows_helper(index, count, dest);
        return true;
    }

    bool VectorNode::remove(int index, int count) {
        Q_D(VectorNode);
        assert(d->testModifiable());

        // Validate
        if (!validateArrayRemoveArguments(index, count, d->vector.size())) {
            SUBSTATE_WARNING("invalid parameters");
            return false;
        }

        d->removeRows_helper(index, count);
        return true;
    }

    bool VectorNode::remove(Node *node) {
        Q_D(VectorNode);
        assert(d->testModifiable());

        // Validate
        if (!node) {
            SUBSTATE_WARNING("trying to remove a null node from %p", this);
            return false;
        }

        decltype(d->vector)::const_iterator it;
        if (node->parent() != this ||
            (it = std::find(d->vector.begin(), d->vector.end(), node)) == d->vector.end()) {
            SUBSTATE_WARNING("node %p is not the child of %p", node, this);
            return false;
        }

        d->removeRows_helper(int(it - d->vector.begin()), 1);
        return true;
    }

    Node *VectorNode::at(int index) const {
        Q_D(const VectorNode);
        return d->vector.at(index);
    }

    const std::vector<Node *> &VectorNode::data() const {
        Q_D(const VectorNode);
        return d->vector;
    }

    int VectorNode::indexOf(Node *node) const {
        Q_D(const VectorNode);
        auto it = std::find(d->vector.begin(), d->vector.end(), node);
        if (it == d->vector.end())
            return -1;
        return int(it - d->vector.begin());
    }

    int VectorNode::size() const {
        Q_D(const VectorNode);
        return int(d->vector.size());
    }

    void VectorNode::write(OStream &stream) const {
        Q_D(const VectorNode);
        // Write index
        stream << d->index;

        // Write children
        stream << int(d->vector.size());
        for (const auto &node : d->vector) {
            node->write(stream);
        }
    }

    Node *VectorNode::clone(bool user) const {
        Q_D(const VectorNode);

        auto node = new VectorNode();
        auto d2 = node->d_func();
        if (user)
            d2->index = d->index;

        // Clone children
        d2->vector.reserve(d->vector.size());
        for (auto &child : d->vector) {
            auto newChild = NodeHelper::clone(child, user);
            node->addChild(newChild);

            d2->vector.push_back(newChild);
        }
        return node;
    }

    void VectorNode::childDestroyed(Node *node) {
        remove(node);
    }

    void VectorNode::propagateChildren(const std::function<void(Node *)> &func) {
        Q_D(VectorNode);
        for (const auto &node : std::as_const(d->vector)) {
            func(node);
        }
    }

    VectorNode::VectorNode(VectorNodePrivate &d) : Node(d) {
        d.init();
    }

    VectorAction::VectorAction(Type type, Node *parent, int index)
        : NodeAction(type, parent), m_index(index) {
    }

    VectorAction::~VectorAction() {
    }

    VectorMoveAction::VectorMoveAction(Node *parent, int index, int count, int dest)
        : VectorAction(VectorMove, parent, index), cnt(count), dest(dest) {
    }

    VectorMoveAction::~VectorMoveAction() {
    }

    void VectorMoveAction::write(OStream &stream) const {
    }

    Action *VectorMoveAction::clone() const {
        return new VectorMoveAction(m_parent, m_index, cnt, dest);
    }

    void VectorMoveAction::execute(bool undo) {
        auto d = static_cast<VectorNode *>(m_parent)->d_func();
        if (undo) {
            int r_index;
            int r_dest;
            if (dest > m_index) {
                r_index = dest - cnt;
                r_dest = m_index;
            } else {
                r_index = dest;
                r_dest = m_index + cnt;
            }
            d->moveRows_helper(r_index, cnt, r_dest);
        } else {
            d->moveRows_helper(m_index, cnt, dest);
        }
    }

    VectorInsDelAction::VectorInsDelAction(Type type, Node *parent, int index,
                                           const std::vector<Node *> &children)
        : VectorAction(type, parent, index), m_children(children), m_tempIds(nullptr) {
    }

    VectorInsDelAction::~VectorInsDelAction() {
        delete m_tempIds;
    }

    void VectorInsDelAction::write(OStream &stream) const {
    }

    Action *VectorInsDelAction::clone() const {
        return new VectorInsDelAction(static_cast<Type>(t), m_parent, m_index, m_children);
    }

    void VectorInsDelAction::execute(bool undo) {
        auto d = static_cast<VectorNode *>(m_parent)->d_func();
        ((t == VectorRemove) ^ undo) ? d->removeRows_helper(m_index, m_children.size())
                                     : d->insertRows_helper(m_index, m_children);
    }

    void VectorInsDelAction::virtual_hook(int id, void *data) {
        switch (id) {
            case CleanNodesHook: {
                if (t == VectorInsert) {
                    for (const auto &child : std::as_const(m_children)) {
                        if (child->isManaged())
                            NodeHelper::forceDelete(child);
                    }
                }
                break;
            }
            case InsertedNodesHook: {
                if (t == VectorInsert) {
                    auto &res = *reinterpret_cast<std::vector<Node *> *>(data);
                    res.reserve(m_children.size());
                    for (const auto &child : std::as_const(m_children)) {
                        res.push_back(child);
                    }
                }
                break;
            }
            case RemovedNodesHook: {
                if (t == VectorRemove) {
                    auto &res = *reinterpret_cast<std::vector<Node *> *>(data);
                    res.reserve(m_children.size());
                    for (const auto &child : std::as_const(m_children)) {
                        res.push_back(child);
                    }
                }
                break;
            }
            default:
                break;
        }
    }

}