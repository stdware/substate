// Copyright (C) 2022-2025 Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#ifndef SUBSTATE_TRANSFER_P_H
#define SUBSTATE_TRANSFER_P_H

#include <memory>
#include <vector>

#include <substate/Node.h>

namespace ss {

    /// A position within a container node from which nodes are transferred or to which they are
    /// transferred. Each container type that supports transfer provides an implementation.
    class SUBSTATE_EXPORT TransferEndpoint {
    public:
        virtual ~TransferEndpoint();

        /// The container at this end.
        virtual Node *container() const = 0;

        /// Removes \a count nodes at the position from the container and returns them in order.
        /// Their parent is updated by the caller.
        virtual std::vector<std::unique_ptr<Node>> take(int count) = 0;

        /// Places \a nodes at the position in the container. Their parent is already updated.
        virtual void put(std::vector<std::unique_ptr<Node>> nodes) = 0;
    };

}

#endif // SUBSTATE_TRANSFER_P_H
