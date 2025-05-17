#include "sheetnode.h"
#include "sheetnode_p.h"

#include <algorithm>
#include <cassert>
#include <utility>

#include "substateglobal_p.h"
#include "nodehelper.h"

namespace Substate {

    SheetNodePrivate::SheetNodePrivate(const std::string &name) : NodePrivate(Node::Sheet, name) {
    }

    SheetNodePrivate::~SheetNodePrivate() {
        for (const auto &pair : std::as_const(records)) {
            delete pair.second;
        }
    }

    void SheetNodePrivate::init() {
    }

    SheetNode *SheetNodePrivate::read(IStream &stream) {
        std::string name;
        stream >> name;
        if (stream.fail())
            return nullptr;

        auto node = new SheetNode(name);
        auto d = node->d_func();

        int size;
        stream >> d->index >> size;
        if (stream.fail()) {
            goto abort;
        }

        d->records.reserve(size);
        d->recordIndexes.reserve(size);
        for (int i = 0; i < size; ++i) {
            int id;
            stream >> id;
            auto child = Node::read(stream);
            if (!child) {
                goto abort;
            }

            node->addChild(child);
            d->records.insert(std::make_pair(id, child));
            d->recordIds.insert(id);
            d->recordIndexes.insert(std::make_pair(child, id));
        }
        return node;

    abort:
        delete node;
        return nullptr;
    }

    void SheetNodePrivate::addRecord_helper(int id, Node *node) {
        QM_Q(SheetNode);
        q->beginAction();

        // Do change
        q->addChild(node);

        records.insert(std::make_pair(id, node));
        recordIds.insert(id);
        recordIndexes.insert(std::make_pair(node, id));

        // Propagate signal
        {
            SheetAction a(Action::SheetInsert, q, id, node);
            ActionNotification n(Notification::ActionTriggered, &a);
            q->dispatch(&n);
        }

        q->endAction();
    }

    void SheetNodePrivate::removeRecord_helper(int id) {
        QM_Q(SheetNode);
        q->beginAction();

        auto it = records.find(id);
        auto node = it->second;

        SheetAction a(Action::SheetRemove, q, id, node);

        // Pre-Propagate signal
        {
            ActionNotification n(Notification::ActionAboutToTrigger, &a);
            q->dispatch(&n);
        }

        // Do change
        q->removeChild(node);
        records.erase(it);
        recordIds.erase(id);
        recordIndexes.erase(node);

        // Propagate signal
        {
            ActionNotification n(Notification::ActionTriggered, &a);
            q->dispatch(&n);
        }

        q->endAction();
    }

    SheetAction *readSheetAction(Action::Type type, IStream &stream) {
        int parentIndex, id, index;
        stream >> parentIndex >> id >> index;
        if (stream.fail())
            return nullptr;

        auto parent = reinterpret_cast<Node *>(uintptr_t(parentIndex));
        auto child = reinterpret_cast<Node *>(uintptr_t(index));

        auto a = new SheetAction(type, parent, id, child);
        a->setState(Action::Unreferenced);
        return a;
    }

    SheetNode::SheetNode(const std::string &name) : SheetNode(*new SheetNodePrivate(name)) {
    }

    SheetNode::~SheetNode() {
    }

    int SheetNode::insert(Node *node) {
        QM_D(SheetNode);
        assert(d->testModifiable());

        // Validate
        if (!d->testInsertable(node)) {
            SUBSTATE_WARNING("node %p is not able to be inserted", node);
            return false;
        }

        auto id = d->recordIds.empty() ? 1 : (*d->recordIds.rbegin() + 1);
        d->addRecord_helper(id, node);
        return id;
    }

    bool SheetNode::remove(int id) {
        QM_D(SheetNode);
        assert(d->testModifiable());

        // Validate
        if (d->records.find(id) == d->records.end()) {
            SUBSTATE_WARNING("sequence id %d doesn't exist in %p", id, this);
            return false;
        }

        d->removeRecord_helper(id);
        return true;
    }

    bool SheetNode::remove(Node *node) {
        QM_D(SheetNode);
        assert(d->testModifiable());

        // Validate
        if (!node) {
            SUBSTATE_WARNING("trying to remove a null node from %p", this);
            return false;
        }

        auto it = d->recordIndexes.find(node);
        if (it == d->recordIndexes.end()) {
            SUBSTATE_WARNING("node %p is not the child of %p", node, this);
            return false;
        }

        d->removeRecord_helper(it->second);
        return true;
    }

    Node *SheetNode::record(int id) const {
        QM_D(const SheetNode);
        auto it = d->records.find(id);
        if (it == d->records.end())
            return nullptr;
        return it->second;
    }

    int SheetNode::indexOf(Node *node) const {
        QM_D(const SheetNode);
        auto it = d->recordIndexes.find(node);
        if (it == d->recordIndexes.end())
            return -1;
        return it->second;
    }

    std::vector<int> SheetNode::ids() const {
        QM_D(const SheetNode);
        std::vector<int> keys(d->records.size());
        std::transform(d->records.begin(), d->records.end(), keys.begin(),
                       [](const std::pair<int, Node *> &pair) {
                           return pair.first; //
                       });
        return keys;
    }

    const std::unordered_map<int, Node *> &SheetNode::data() const {
        QM_D(const SheetNode);
        return d->records;
    }

    int SheetNode::size() const {
        QM_D(const SheetNode);
        return int(d->records.size());
    }

    void SheetNode::write(OStream &stream) const {
        QM_D(const SheetNode);
        // Write name and index
        stream << d->name << d->index;

        // Write children
        stream << int(d->records.size());
        for (const auto &pair : d->records) {
            stream << pair.first;
            pair.second->writeWithType(stream);
        }
    }

    Node *SheetNode::clone(bool user) const {
        QM_D(const SheetNode);

        auto node = new SheetNode(d->name);
        auto d2 = node->d_func();
        if (!user)
            d2->index = d->index;

        // Clone children
        d2->records.reserve(d->records.size());
        d2->recordIndexes.reserve(d->recordIndexes.size());
        for (auto it = d->records.begin(); it != d->records.end(); ++it) {
            auto newChild = NodeHelper::clone(it->second, user);
            node->addChild(newChild);

            d2->records.insert(std::make_pair(it->first, newChild));
            d2->recordIndexes.insert(std::make_pair(newChild, it->first));
        }
        d2->recordIds = d->recordIds;
        return node;
    }

    void SheetNode::childDestroyed(Node *node) {
        remove(node);
    }

    void SheetNode::propagateChildren(const std::function<void(Node *)> &func) {
        QM_D(SheetNode);
        for (const auto &pair : std::as_const(d->records)) {
            NodeHelper::propagateNode(pair.second, func);
        }
    }

    SheetNode::SheetNode(SheetNodePrivate &d) : Node(d) {
        d.init();
    }

    SheetAction::SheetAction(Type type, Node *parent, int id, Node *child)
        : NodeAction(type, parent), m_id(id), m_child(child) {
    }

    SheetAction::~SheetAction() {
        if (s == Detached) {
            if (t == SheetInsert) {
                delete m_child;
            }
        } else if (s == Deleted) {
            if (t == SheetInsert && m_child->isManaged()) {
                NodeHelper::forceDelete(m_child);
            }
        }
    }

    void SheetAction::write(OStream &stream) const {
        NodeAction::write(stream);
        stream << m_id << m_child->index();
    }

    Action *SheetAction::clone() const {
        return new SheetAction(static_cast<Type>(t), m_parent, m_id, m_child);
    }

    void SheetAction::execute(bool undo) {
        auto d = static_cast<SheetNode *>(m_parent)->d_func();
        ((t == SheetRemove) ^ undo) ? d->removeRecord_helper(m_id)
                                    : d->addRecord_helper(m_id, m_child);
    }

    void SheetAction::virtual_hook(int id, void *data) {
        switch (id) {
            case DetachHook: {
                if (t == SheetInsert) {
                    m_child = NodeHelper::clone(m_child, false);
                }
                NodeAction::virtual_hook(id, data);
                return;
            }
            case InsertedNodesHook: {
                if (t == SheetInsert) {
                    auto &res = *reinterpret_cast<std::vector<Node *> *>(data);
                    res.push_back(m_child);
                }
                return;
            }
            case RemovedNodesHook: {
                if (t == SheetRemove) {
                    auto &res = *reinterpret_cast<std::vector<Node *> *>(data);
                    res.push_back(m_child);
                }
                return;
            }
            case DeferredReferenceHook: {
                SUBSTATE_FIND_DEFERRED_REFERENCE_NODE(data, m_parent, m_parent);
                SUBSTATE_FIND_DEFERRED_REFERENCE_NODE(data, m_child, m_child);
                return;
            }
            default:
                break;
        }
    }

}