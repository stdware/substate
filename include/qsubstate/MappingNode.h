// Copyright (C) 2022-2025 Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#ifndef SUBSTATE_MAPPINGNODE_H
#define SUBSTATE_MAPPINGNODE_H

#include <map>

#include <qsubstate/Property.h>

namespace ss {

    class MappingAction;

    class MappingNodePrivate;

    /// MappingNode - Vector data structure node.
    class QSUBSTATE_EXPORT MappingNode : public Node {
    public:
        inline explicit MappingNode(int type = Mapping);
        ~MappingNode();

    public:
        inline Property property(const QString &key) const;
        bool setProperty(const QString &key, const Property &value);
        inline const std::map<QString, Property> &data() const;
        inline int count() const;
        inline int size() const;

    protected:
        std::shared_ptr<Node> clone(bool copyId) const override;
        void propagateChildren(const std::function<void(Node *)> &func) override;

        std::map<QString, Property> _map;

        friend class MappingNodePrivate;
        friend class MappingAction;
    };

    inline MappingNode::MappingNode(int type) : Node(type) {
    }

    inline Property MappingNode::property(const QString &key) const {
        auto it = _map.find(key);
        if (it == _map.end()) {
            return {};
        }
        return it->second;
    }

    inline const std::map<QString, Property> &MappingNode::data() const {
        return _map;
    }

    inline int MappingNode::count() const {
        return int(_map.size());
    }

    inline int MappingNode::size() const {
        return int(_map.size());
    }


    /// MappingAction - Action for \c MappingNode operations.
    class QSUBSTATE_EXPORT MappingAction : public PropertyAction {
    public:
        inline MappingAction(const std::shared_ptr<MappingNode> &parent, QString key,
                             Property oldValue, Property value);
        ~MappingAction();

    public:
        void execute(bool undo) override;

    public:
        inline QString key() const;

    public:
        QString _key;
    };

    inline MappingAction::MappingAction(const std::shared_ptr<MappingNode> &parent, QString key,
                                        Property oldValue, Property value)
        : PropertyAction(MappingAssign, parent, std::move(oldValue), std::move(value)),
          _key(std::move(key)) {
    }

    inline QString MappingAction::key() const {
        return _key;
    }

}

#endif // SUBSTATE_MAPPINGNODE_H