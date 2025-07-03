// Copyright (C) 2022-2025 Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#ifndef SUBSTATE_BYTESNODE_H
#define SUBSTATE_BYTESNODE_H

#include <vector>

#include <substate/Node.h>
#include <substate/Action.h>
#include <substate/ArrayView.h>

#include <qsubstate/qsubstate_global.h>

namespace ss {

    class BytesAction;

    class BytesNodePrivate;

    class SUBSTATE_EXPORT BytesNode : public Node {
    public:
        inline explicit BytesNode(int type);
        ~BytesNode();

    public:
        inline void prepend(std::vector<char> data);
        inline void append(std::vector<char> data);
        void insert(int index, std::vector<char> data);
        void remove(int index, int size);
        void replace(int index, std::vector<char> data);
        inline void truncate(int size);
        inline void clear();
        inline ArrayView<char> data() const;
        inline int count() const;
        inline int size() const;

    protected:
        std::vector<char> _data;

        friend class BytesNodePrivate;
        friend class BytesAction;
    };

    inline void BytesNode::prepend(std::vector<char> data) {
        insert(0, data);
    }

    inline void BytesNode::append(std::vector<char> data) {
        insert(size(), data);
    }

    inline void BytesNode::truncate(int size) {
        remove(size, this->size() - size);
    }

    inline void BytesNode::clear() {
        remove(0, size());
    }

    inline ArrayView<char> BytesNode::data() const {
        return _data;
    }

    inline int BytesNode::size() const {
        return int(_data.size());
    }

    int BytesNode::count() const {
        return size();
    }

    class SUBSTATE_EXPORT BytesAction : public NodeAction {
    public:
        inline BytesAction(Type type, const std::shared_ptr<Node> &parent, int index,
                           std::vector<char> oldBytes, std::vector<char> bytes);
        ~BytesAction();

    public:
        inline int index() const;
        inline ArrayView<char> oldBytes() const;
        inline ArrayView<char> bytes() const;

    public:
        void queryNodes(bool inserted,
                        const std::function<void(const std::shared_ptr<Node> &)> &add) override;
        void execute(bool undo) override;

    protected:
        int _index;
        std::vector<char> _oldBytes;
        std::vector<char> _bytes;
    };

    inline BytesAction::BytesAction(Type type, const std::shared_ptr<Node> &parent, int index,
                                    std::vector<char> oldBytes, std::vector<char> bytes)
        : NodeAction(type, parent), _index(index), _oldBytes(std::move(oldBytes)),
          _bytes(std::move(bytes)) {
    }

    inline int BytesAction::index() const {
        return _index;
    }

    inline ArrayView<char> BytesAction::oldBytes() const {
        return _oldBytes;
    }

    inline ArrayView<char> BytesAction::bytes() const {
        return _bytes;
    }

}

#endif // SUBSTATE_BYTESNODE_H