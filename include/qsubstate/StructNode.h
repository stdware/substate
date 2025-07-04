// Copyright (C) 2022-2025 Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#ifndef SUBSTATE_STRUCTNODE_H
#define SUBSTATE_STRUCTNODE_H

#include <substate/ArrayView.h>

#include <qsubstate/Property.h>

namespace ss {

    class StructAction;

    class StructNodeBasePrivate;

    /// StructNode - MappingNode - Base struct data structure node base.
    class QSUBSTATE_EXPORT StructNodeBase : public Node {
    public:
        inline StructNodeBase(int type, Property *storage, size_t size);
        ~StructNodeBase();

    public:
        inline const Property &at(int index) const;
        void setAt(int index, Property value);

        inline ArrayView<Property> data() const;
        inline int count() const;
        inline int size() const;

    protected:
        void propagateChildren(const std::function<void(Node *)> &func) override;

    protected:
        Property *_storage;
        size_t _size;

        static void copy(StructNodeBase *dest, const StructNodeBase *src, bool copyId);

        friend class StructNodeBasePrivate;
        friend class StructAction;
    };

    inline StructNodeBase::StructNodeBase(int type, Property *storage, size_t size)
        : Node(type), _storage(storage), _size(size) {
    }

    inline const Property &StructNodeBase::at(int index) const {
        return _storage[index];
    }

    inline ArrayView<Property> StructNodeBase::data() const {
        return {_storage, _size};
    }

    inline int StructNodeBase::count() const {
        return size();
    }

    inline int StructNodeBase::size() const {
        return int(_size);
    }


    /// StructNode - Struct data structure node.
    template <size_t N>
    class StructNode : public StructNodeBase {
    public:
        inline StructNode(int type);
        ~StructNode() = default;

    protected:
        std::shared_ptr<Node> clone(bool copyId) const override;

    protected:
        Property _buf[N];
    };

    template <size_t N>
    inline StructNode<N>::StructNode(int type) : StructNodeBase(type, _buf, N) {
    }

    template <size_t N>
    inline std::shared_ptr<Node> StructNode<N>::clone(bool copyId) const {
        auto node = std::make_shared<StructNode<N>>(_type);
        StructNodeBase::copy(node.get(), this, copyId);
        return node;
    }


    /// StructAction - Action for \c StructNode operations.
    class QSUBSTATE_EXPORT StructAction : public PropertyAction {
    public:
        inline StructAction(const std::shared_ptr<StructNodeBase> &parent, int index,
                            Property oldValue, Property value);
        ~StructAction();

    public:
        void execute(bool undo) override;

    public:
        inline int index() const;

    public:
        int _index;
    };

    inline StructAction::StructAction(const std::shared_ptr<StructNodeBase> &parent, int index,
                                      Property oldValue, Property value)
        : PropertyAction(StructAssign, parent, std::move(oldValue), std::move(value)),
          _index(index) {
    }

    inline int StructAction::index() const {
        return _index;
    }

}

#endif // SUBSTATE_STRUCTNODE_H