// Copyright (C) 2022-2025 Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#ifndef QSUBSTATE_STRUCTNODE_H
#define QSUBSTATE_STRUCTNODE_H

#include <array>
#include <cstddef>

#include <qsubstate/Property.h>

namespace ss {

    /// A node with a fixed number of slots addressed by index, each holding a Property. The
    /// storage of the slots is provided by StructNode.
    class QSUBSTATE_EXPORT StructNodeBase : public Node {
    public:
        ~StructNodeBase();

        inline int size() const;
        inline const Property &at(int index) const;
        inline QVariant variant(int index) const;
        inline Node *child(int index) const;

        /// Stores \a value in the slot \a index. A child in \a value must be free and without a
        /// parent. Assigning a value equal to the current one creates no action.
        ///
        /// In a model, the previous value is owned by the action and returns if the assignment is
        /// undone. In a free node, it is destroyed. Use take() to keep it.
        void setAt(int index, Property value);

        /// Moves the value out of the slot \a index and returns it, leaving the slot empty. The
        /// node must be free.
        Property take(int index);

        /// Moves \a node from its parent in the same model to the slot \a index, keeping its
        /// identity. See TransferAction.
        ///
        /// \return whether the transfer was performed. It is rejected, creating no action, if the
        ///         slot is not empty, if \a node is the root, if its parent is this node, or if
        ///         this node is \a node or one of its descendants.
        bool transferIn(int index, Node *node);

    protected:
        inline StructNodeBase(int type, int size);

        /// Sets the storage of the slots, which must hold size() values. Called by the constructor
        /// of StructNode after its storage is constructed.
        inline void setStorage(Property *storage);

        void forEachChild(const std::function<void(Node *)> &func) const override;
        std::unique_ptr<TransferEndpoint> endpointOf(const std::vector<Node *> &children) override;
        std::unique_ptr<TransferEndpoint> readEndpoint(Decoder &decoder) override;

        /// Writes the number of slots and their values. Reading fails if the number differs from
        /// size().
        void writeContent(Encoder &encoder) const override;

        bool readContent(Decoder &decoder) override;

        /// Stores copies of the values of \a source, for the clone() of a subclass. This node must
        /// be free, empty and of the same size.
        void cloneSlotsFrom(const StructNodeBase &source);

    private:
        Property *m_slots = nullptr;
        int m_size;

        friend class StructAssignAction;
        friend class StructNodeEndpoint;
    };

    inline StructNodeBase::StructNodeBase(int type, int size) : Node(type), m_size(size) {
    }

    inline void StructNodeBase::setStorage(Property *storage) {
        m_slots = storage;
    }

    inline int StructNodeBase::size() const {
        return m_size;
    }

    inline const Property &StructNodeBase::at(int index) const {
        return m_slots[index];
    }

    inline QVariant StructNodeBase::variant(int index) const {
        return m_slots[index].variant();
    }

    inline Node *StructNodeBase::child(int index) const {
        return m_slots[index].child();
    }

    /// A StructNodeBase with \a N slots stored within the node, without a separate allocation.
    template <std::size_t N>
    class StructNode : public StructNodeBase {
    public:
        inline explicit StructNode(int type = Struct);

        inline std::unique_ptr<Node> clone() const override;

    private:
        std::array<Property, N> m_storage;
    };

    template <std::size_t N>
    inline StructNode<N>::StructNode(int type) : StructNodeBase(type, int(N)) {
        setStorage(m_storage.data());
    }

    template <std::size_t N>
    inline std::unique_ptr<Node> StructNode<N>::clone() const {
        auto node = std::make_unique<StructNode<N>>(type());
        node->cloneSlotsFrom(*this);
        return node;
    }

    /// Assignment to a slot of a StructNodeBase. See PropertyAction for the ownership.
    class QSUBSTATE_EXPORT StructAssignAction : public PropertyAction {
    public:
        ~StructAssignAction();

        inline int index() const;

    protected:
        void execute(Operation operation) override;
        void write(Encoder &encoder) const override;

    private:
        StructAssignAction(StructNodeBase *parent, int index, Property value);
        StructAssignAction(StructNodeBase *parent, int index, QVariant oldVariant, Node *oldChild,
                           Property value);

        static std::unique_ptr<Action> read(Decoder &decoder);

        int m_index;

        friend class QCodec;
        friend class StructNodeBase;
    };

    inline int StructAssignAction::index() const {
        return m_index;
    }

}

#endif // QSUBSTATE_STRUCTNODE_H
