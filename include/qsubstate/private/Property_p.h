// Copyright (C) 2022-2025 Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#ifndef QSUBSTATE_PROPERTY_P_H
#define QSUBSTATE_PROPERTY_P_H

#include <qsubstate/Property.h>

namespace ss {

    /// Operations on a Property stored in a node, shared by StructNode and MappingNode.
    class QSUBSTATE_EXPORT PropertyPrivate {
    public:
        /// Returns whether \a value can be stored in \a parent: it holds no child, or a free child
        /// without a parent that is not \a parent or one of its ancestors.
        static bool isAssignable(const Property &value, const Node *parent);

        /// Stores \a value in \a slot of the free node \a parent without an action. The previous
        /// value is destroyed.
        static void assignFree(Node *parent, Property &slot, Property value);

        /// Moves the value out of \a slot of a free node and returns it, with its child detached
        /// from the parent.
        static Property take(Property &slot);

        /// Exchanges \a slot of \a parent with \a other, and updates the parent and the
        /// attachment of the children involved. \a parent must be in the tree of a model.
        static void exchange(Node *parent, Property &slot, Property &other);

        /// Moves the child out of \a slot, which must hold one, and leaves \a slot empty. The
        /// parent of the child is not changed. Used by transfer.
        static std::unique_ptr<Node> releaseChild(Property &slot);
    };

}

#endif // QSUBSTATE_PROPERTY_P_H
