#include "BytesNode.h"
#include "BytesNode_p.h"

#include "Model_p.h"
#include "Node_p.h"

namespace ss {

    void BytesNodePrivate::copy(BytesNode *dest, const BytesNode *src, bool copyId) {
        if (!copyId) {
            dest->_id = src->_id;
        }
        // Copy data
        dest->_data = src->_data;
    }

    BytesNode::~BytesNode() = default;

    void BytesNode::insert(int index, std::vector<char> data) {
        assert(isWritable());
        assert(NodePrivate::validateArrayQueryArguments(index, _data.size()) && !data.empty());

        auto action = std::make_unique<BytesAction>(Action::BytesInsert, shared_from_this(), index,
                                                    std::move(data));
        action->execute(false);
        ModelPrivate::pushAction(_model, std::move(action));
    }

    void BytesNode::remove(int index, int size) {
        assert(isWritable());
        assert(NodePrivate::validateArrayRemoveArguments(index, size, _data.size()));

        auto begin = _data.begin() + index;
        auto action = std::make_unique<BytesAction>(Action::BytesRemove, shared_from_this(), index,
                                                    std::vector<char>(begin, begin + size));
        action->execute(false);
        ModelPrivate::pushAction(_model, std::move(action));
    }

    void BytesNode::replace(int index, std::vector<char> data) {
        assert(isWritable());
        auto begin = _data.begin() + index;
        auto end = begin + data.size();
        if (auto off = end - _data.end(); off > 0) {
            auto action = std::make_unique<BytesAction>(Action::BytesInsert, shared_from_this(),
                                                        int(data.size()), std::vector<char>(off, 0));
            action->execute(false);
            ModelPrivate::pushAction(_model, std::move(action));
        }

        auto action = std::make_unique<BytesReplaceAction>(
            shared_from_this(), index, std::move(data), std::vector<char>(begin, end));
        action->execute(false);
        ModelPrivate::pushAction(_model, std::move(action));
    }

    std::shared_ptr<Node> BytesNode::clone(bool copyId) const {
        auto node = std::make_shared<BytesNode>(Bytes);
        BytesNodePrivate::copy(node.get(), this, copyId);
        return node;
    }

    BytesAction::~BytesAction() = default;

    void BytesAction::queryNodes(bool inserted,
                                 const std::function<void(const std::shared_ptr<Node> &)> &add) {
        (void) inserted;
        (void) add;
    }

    void BytesAction::execute(bool undo) {
        auto parent = static_cast<BytesNode *>(_parent.get());
        parent->beginAction();

        auto &data = parent->_data;
        // Pre-Propagate signal
        {
            ActionNotification n(Notification::ActionAboutToTrigger, this);
            parent->notify(&n);
        }

        // Do change
        if (((_type == BytesRemove) ^ undo)) {
            auto begin = data.begin() + _index;
            data.erase(begin, begin + _bytes.size());
        } else {
            data.insert(data.begin() + _index, _bytes.begin(), _bytes.end());
        }

        // Post-propagate signal
        {
            ActionNotification n(Notification::ActionTriggered, this);
            parent->notify(&n);
        }
        parent->endAction();
    }

    void BytesReplaceAction::queryNodes(
        bool inserted, const std::function<void(const std::shared_ptr<Node> &)> &add) {
        (void) inserted;
        (void) add;
    }

    void BytesReplaceAction::execute(bool undo) {
        auto parent = static_cast<BytesNode *>(_parent.get());
        parent->beginAction();

        auto &data = parent->_data;
        // Pre-Propagate signal
        {
            ActionNotification n(Notification::ActionAboutToTrigger, this);
            parent->notify(&n);
        }

        // Do change
        const auto &bytes = undo ? _oldBytes : _bytes;
        std::copy(bytes.begin(), bytes.end(), data.begin() + _index);

        // Post-propagate signal
        {
            ActionNotification n(Notification::ActionTriggered, this);
            parent->notify(&n);
        }
        parent->endAction();
    }

}