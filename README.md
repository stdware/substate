# Substate

Substate is a document model with transactions and undo history for editor applications.

## Overview

An editor built on the command pattern implements every user operation as a command class with its own undo and redo functions. The number of command classes grows with the features of the application, and each class must restore its state exactly.

Substate records changes at the level of a node tree instead. A document is a tree of nodes, every modification of the tree occurs in a transaction and is recorded as an action, and a committed transaction is one undo step. An application composes its operations from the modifications of the nodes and obtains undo and redo without writing them.

## Features

- **Ownership determined by type.** Every node has exactly one owner at any time: the caller, its parent, the model, or an action in the history. A node removed from the tree is owned by the action that removed it. The rules and their proof are documented in [`docs/Design.md`](docs/Design.md).
- **Exact undo and redo.** Undo restores the same node objects with their addresses and identifiers, and an aborted transaction restores the tree exactly.
- **Transfer.** A node moves to another parent within the same model without losing its identity.
- **Notifications.** Observers receive every applied action with the change it makes, including undo and redo, and every node about to be destroyed.
- **Serialization.** Nodes and actions are encoded in a binary format, and a history can be restored from a checkpoint and the encoded actions.

A storage engine with a write-ahead log, which provides crash consistency, is planned for the second phase. The history is kept in memory until then.

## Libraries

| Library | Dependencies | Content |
|---|---|---|
| `substate` | C++ standard library | Nodes of ordered children, keyed children and bytes, actions, transactions, the model, storage engines, codecs |
| `qsubstate` | `substate`, Qt 6 Core | Nodes with `QVariant` properties, the `QVariant` codec, a Qt signal adapter for the notifications |

## Requirements

| Component | Requirement |
|---|---|
| Compiler | C++17, little-endian target |
| CMake | 3.16 or later |
| Qt | 6, for `qsubstate` |
| Boost.Test | For the tests of `substate` |

## Build and Installation

```sh
cmake -B build -G Ninja -DSUBSTATE_BUILD_SHARED=ON -DSUBSTATE_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build
cmake --install build --prefix <prefix>
```

| Option | Default | Effect |
|---|---|---|
| `SUBSTATE_BUILD_SHARED` | `OFF` | Builds shared libraries. Without this option and `BUILD_SHARED_LIBS`, the libraries are static. |
| `SUBSTATE_BUILD_STATIC` | `OFF` | Builds static libraries regardless of the other options. |
| `SUBSTATE_BUILD_TESTS` | `OFF` | Builds the tests, which require Boost.Test and Qt Test. |
| `SUBSTATE_INSTALL` | `ON` | Installs the libraries, the headers and the CMake package. |

A consumer finds the installed package with `find_package(substate)` and links `substate::substate` or `substate::qsubstate`.

## Documentation

- [`docs/Design.md`](docs/Design.md): the design, including the ownership model and its proof, the actions, transactions, notifications and the persistence interface.
- [`docs/References.md`](docs/References.md): the analysis of the earlier implementation and of AceTreeModel, on which the design is based.

## License

Apache License 2.0. See [LICENSE](LICENSE).
