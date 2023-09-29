#include "sheetnode.h"
#include "sheetnode_p.h"

#include <algorithm>
#include <cassert>

#include "nodehelper.h"

namespace Substate {

    SheetNodePrivate::SheetNodePrivate(int type) : NodePrivate(type) {
    }

    SheetNodePrivate::~SheetNodePrivate() {
    }

    void SheetNodePrivate::init() {
    }

    SheetNode *SheetNodePrivate::read(IStream &stream) {
        return nullptr;
    }

    void SheetNodePrivate::addRecord_helper(int id, Node *node) {
        Q_Q(SheetNode);
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
        Q_Q(SheetNode);
        q->beginAction();

        auto it = records.find(id);
        auto node = it->second;

        // Pre-Propagate signal
        {
            SheetAction a(Action::SheetRemove, q, id, node);
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
            SheetAction a(Action::SheetRemove, q, id, node);
            ActionNotification n(Notification::ActionTriggered, &a);
            q->dispatch(&n);
        }

        q->endAction();
    }

    SheetNode::SheetNode() : SheetNode(*new SheetNodePrivate(Sheet)) {
    }

    SheetNode::~SheetNode() {
    }

    int SheetNode::insert(Node *node) {
        Q_D(SheetNode);
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
        Q_D(SheetNode);
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
        Q_D(SheetNode);
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

    Node *SheetNode::record(int id) {
        Q_D(const SheetNode);
        auto it = d->records.find(id);
        if (it == d->records.end())
            return nullptr;
        return it->second;
    }

    int SheetNode::indexOf(Node *node) const {
        Q_D(const SheetNode);
        auto it = d->recordIndexes.find(node);
        if (it == d->recordIndexes.end())
            return -1;
        return it->second;
    }

    std::vector<int> SheetNode::ids() const {
        Q_D(const SheetNode);
        std::vector<int> keys(d->records.size());
        std::transform(d->records.begin(), d->records.end(), keys.begin(),
                       [](const std::pair<int, Node *> &pair) {
                           return pair.first; //
                       });
        return keys;
    }

    const std::unordered_map<int, Node *> SheetNode::data() const {
        Q_D(const SheetNode);
        return d->records;
    }

    int SheetNode::size() const {
        Q_D(const SheetNode);
        return int(d->records.size());
    }

    void SheetNode::write(OStream &stream) const {
    }

    Node *SheetNode::clone(bool user) const {
        Q_D(const SheetNode);

        auto node = new SheetNode();
        auto d2 = node->d_func();
        if (user)
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

    SheetNode::SheetNode(SheetNodePrivate &d) : Node(d) {
        d.init();
    }

    SheetAction::SheetAction(Type type, Node *parent, int id, Node *child)
        : NodeAction(type, parent), m_id(id), m_child(child) {
    }

    SheetAction::~SheetAction() {
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
            case CleanNodesHook: {
                if (t == SheetInsert) {
                    NodeHelper::forceDelete(m_child);
                }
                break;
            }
            case InsertedNodesHook: {
                if (t == SheetInsert) {
                    auto &res = *reinterpret_cast<std::vector<Node *> *>(data);
                    res.push_back(m_child);
                }
                break;
            }
            case RemovedNodesHook: {
                if (t == SheetRemove) {
                    auto &res = *reinterpret_cast<std::vector<Node *> *>(data);
                    res.push_back(m_child);
                }
                break;
            }
            case AcquireInsertedNodesHook: {
                if (t == SheetInsert) {
                }
                break;
            }
            default:
                break;
        }
    }

}