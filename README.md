# Substate

Substate is a crash consistent undo framework for editor applications.

## Introduction

Undo/Redo command is an inevitable feature of an editing application.

Application developers tend to implement this functionality using the **Command** design pattern, typically creating a basic command class and then deriving a subclass for each type of user operation, and implementing undo and redo virtual functions for those subclasses.

Such workflow is not only time-consuming, but also tightly tied to the specific application features, which is too inflexiable to handle the ever-changing developing requirements.

We propose a flexible and efficient undo framework which not only allows developers to implement undo/redo features in less time, but also strictly ensures the **crash consistency** of the application.

## Requirements

| Component | Requirement |               Detailed               |
|:---------:|:-----------:|:------------------------------------:|
| Compiler  |  \>=C++17   |                  /                   |
|   CMake   |   \>=3.17   |        >=3.20 is recommended         |

## Dependencies

+ [qtmediate-cmake-modules](https://github.com/SineStriker/qtmediate-cmake-modules)

## Build From Source

```sh
cmake -B build -G Ninja \
    -DQTMEDIATE_CMAKE_MODULES_DIR:STRING=<dir>

cmake --build build --target all
```