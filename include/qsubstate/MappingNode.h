// Copyright (C) 2022-present Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#ifndef QSUBSTATE_MAPPINGNODE_H
#define QSUBSTATE_MAPPINGNODE_H

#include <map>

#include <QtCore/QString>
#include <QtCore/QStringList>

#include <qsubstate/Property.h>

namespace ss {

    /// A node with entries addressed by string keys, each holding a non-empty Property.
    class QSUBSTATE_EXPORT MappingNode : public Node {
    public:
        inline MappingNode();
        ~MappingNode();

        inline int size() const;
        bool contains(const QString &key) const;

        /// The value of \a key, or an empty Property if the entry does not exist.
        const Property &at(const QString &key) const;

        inline QVariant variant(const QString &key) const;
        inline Node *child(const QString &key) const;

        /// The keys in ascending order.
        QStringList keys() const;

        /// Stores \a value under \a key. An empty \a value removes the entry. A child in \a value
        /// must be free and without a parent.
        ///
        /// In a model, the previous value is owned by the action and returns if the assignment is
        /// undone. In a free node, it is destroyed. Use take() to keep it.
        ///
        /// \return whether the entry changed. A value equal to the current one creates no action.
        bool setProperty(const QString &key, Property value);

        /// Removes the entry of \a key and returns its value, or returns an empty Property if the
        /// entry does not exist. The node must be free.
        Property take(const QString &key);

        /// Moves \a node from its parent in the same model to the entry of \a key, keeping its
        /// identity. See TransferAction.
        ///
        /// \return whether the transfer was performed. It is rejected, creating no action, if the
        ///         entry exists, if \a node is the root, if its parent is this node, or if this
        ///         node is \a node or one of its descendants.
        bool transferIn(const QString &key, Node *node);

        std::unique_ptr<Node> clone() const override;

    protected:
        inline explicit MappingNode(int type);

        void forEachChild(const std::function<void(Node *)> &func) const override;
        std::unique_ptr<TransferEndpoint> endpointOf(const std::vector<Node *> &children) override;
        std::unique_ptr<TransferEndpoint> readEndpoint(Decoder &decoder) override;
        void writeContent(Encoder &encoder) const override;
        bool readContent(Decoder &decoder) override;

        /// Stores copies of the entries of \a source, for the clone() of a subclass. This node
        /// must be free and empty.
        void cloneEntriesFrom(const MappingNode &source);

    private:
        std::map<QString, Property> m_entries;

        friend class MappingAssignAction;
        friend class MappingNodeEndpoint;
    };

    inline MappingNode::MappingNode() : MappingNode(Mapping) {
    }

    inline MappingNode::MappingNode(int type) : Node(type) {
    }

    inline int MappingNode::size() const {
        return int(m_entries.size());
    }

    inline QVariant MappingNode::variant(const QString &key) const {
        return at(key).variant();
    }

    inline Node *MappingNode::child(const QString &key) const {
        return at(key).child();
    }

    /// Assignment to an entry of a MappingNode, including its creation and removal. See
    /// PropertyAction for the ownership.
    class QSUBSTATE_EXPORT MappingAssignAction : public PropertyAction {
    public:
        ~MappingAssignAction();

        inline const QString &key() const;

    protected:
        void execute(Operation operation) override;
        void write(Encoder &encoder) const override;

    private:
        MappingAssignAction(MappingNode *parent, QString key, Property value);
        MappingAssignAction(MappingNode *parent, QString key, DecodedValues values);

        static std::unique_ptr<Action> read(Decoder &decoder, State state);

        QString m_key;

        friend class MappingNode;
        friend class QCodec;
    };

    inline const QString &MappingAssignAction::key() const {
        return m_key;
    }

}

#endif // QSUBSTATE_MAPPINGNODE_H
