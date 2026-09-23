# 参考实现分析

本文档记录 [`Design.md`](Design.md) 所依据的两份参考实现的细节：本仓库 `e403859` 时的代码，以及 substate 的前身 AceTreeModel（`D:\GitHub\AceTreeModel`，HEAD `55834ed`）。设计文档只引用本文档的结论。

AceTreeModel 一节的两个问题已以探针程序复现，探针位于 HelloUTAU 的 `.cache/claude/tools/acetree-probe/`。

## substate `e403859`

该提交是一次未完成的重构，无法构建。

### 所有权表示

| 位置 | 表示 |
|---|---|
| `BytesNode`、`SheetNode`、`StructNode`、`MappingNode`、`Property`、`NodeAction` | `std::shared_ptr<Node>`，依赖 `shared_from_this()`，但 `Node` 已不再继承 `enable_shared_from_this` |
| `VectorNode`、`Model_p.h`、`StorageEngine_p.h` | `NodePtr`，即 `SmartPtr<Node>`，可在持有与借用之间切换。其定义 `SmartPtr.h` 只存在于被忽略的 `.cache/` 目录中 |
| `Model::_root`、`Node::_parent`、`NodeAction` 的构造参数 | 裸指针 `Node *`，但 `Model::_root` 的使用处按智能指针处理 |

`shared_ptr` 使所有权无法从类型上确定：一个被删除的节点同时被动作、调用方和 `Property` 的副本持有，析构时机取决于最后一个副本何时释放。`SmartPtr` 以运行时标志区分持有与借用，同样无法从类型上确定所有权，并且 `transferred()` 使同一对象在一次调用前后改变角色。

### 构建错误

- `include/substate/ArrayView.h` 已删除，`BytesNode.h` 与 `qsubstate/StructNode.h` 仍引用它。
- `SmartPtr.h` 不在 `include/` 中。
- `StorageEngine.cpp` 使用已从头文件中删除的 `_idMap` 与 `addId()`。
- `RootChangeAction::execute()` 使用未定义的 `root`、`node`、`a`。
- `VectorNode::propagateChildren()` 的参数类型与基类声明不一致。

### 行为缺陷

重构后每一项均须有对应的测试。

- `PropertyAction` 的构造函数将新旧值交换存储，两个访问函数再交换回来，`queryNodes()` 按存储字段判断，因此插入与删除的节点方向相反。
- `StructNodeBasePrivate::copy()` 与 `MappingNodePrivate::copy()` 在 `copyId` 为真时复制 ID，`VectorNodePrivate::copy()` 与 `BytesNodePrivate::copy()` 在其为假时复制 ID，含义相反。
- `SheetNode::remove()` 以 `_id`（节点自身的 ID）而非参数 `id` 查找，并且不经过 `SheetAction::execute()`，与撤销路径不对称。
- `BytesNode::replace()` 在替换范围超出末尾时，于下标 `data.size()` 处插入补齐字节，而非于原数组末尾插入。
- 撤销时的通知均为 `// TODO`，`Model::notify()` 为空，模型没有订阅接口。
- `Node::_parent` 未初始化。
- `ActionNotification` 的构造函数在头文件中定义而未声明为 `inline`。
- `StandardStorageEngine::setMaxSteps()` 对小于 4 的参数静默忽略。

## AceTreeModel

### 节点状态

`AceTreeItem` 有两个与所有权相关的状态：

| 状态 | 含义 |
|---|---|
| 自由（`isFree()`） | 既无父节点也无模型，由用户持有 |
| 已废弃（`isManaged()`，别名 `isObsolete()`） | 曾在模型中、当前已从树上移除，仍由模型的 ID 表索引，不可写 |

从树上移除时（`removeRows_helper()` 等），节点的父指针置空，并沿子树设置已废弃标志。撤销移除时（`insertRows_helper()` 等）清除该标志。substate 的 `Detached` 状态即源于此。

### 节点的释放

节点只在两处被释放：

- `AceTreeMemBackend::removeEvents()` 丢弃事件时调用 `AceTreeEvent::clean()`。**只有插入类事件（`RowsInsert`、`RecordAdd`、`ElementAdd`、`RootChange`）的 `clean()` 释放节点**，并且仅当节点此时处于已废弃状态。删除类事件的 `clean()` 为空。
- 模型重置时释放整棵树。

因此，被删除节点的实际所有者是**当初插入它的事件**，而非删除它的事件。

### 由此产生的两个问题

两者均已用探针程序在 AceTreeModel `55834ed` 上复现，构建为 MSVC 14.44 加 Qt 6.11.1，开启 AddressSanitizer。探针为 `.cache/claude/tools/acetree-probe/` 中的 `probe.cpp`、`probe_journal.cpp` 与 `probe_leak.cpp`，前两者分别使用内存后端与日志后端。

**淘汰历史时释放仍被引用的节点。** `AceTreeMemBackendPrivate::afterCommit()` 在 `current > 2 * maxSteps` 时丢弃最早的 `maxSteps` 个步骤。`probe.cpp` 以 `maxSteps` 为 100 执行：

1. 第 2 步在根节点下插入节点 X。
2. 第 150 步删除 X，X 进入已废弃状态。
3. 提交到第 205 步，最早的 100 步被丢弃。第 2 步的插入事件执行 `clean()`，X 处于已废弃状态，因此被释放。
4. 此时第 150 步的删除事件仍在历史中，仍持有 X 的指针。

AddressSanitizer 在两处报告 heap-use-after-free，释放点均为 `AceTreeRowsInsDelEvent::clean()` 经 `forceDeleteItem()`：

- 读取 X 的任何成员，例如 `x->index()`（`AceTreeItem.cpp:698`）。
- **撤销到第 149 步**，`AceTreeMemBackend::undo()` 调用 `AceTreeRowsInsDelEvent::execute(true)`，其 `insertRows_helper()`（`AceTreeItem.cpp:222`）将已释放的 X 写入并插回树中。第二处不需要调用方持有任何指针，只需撤销。

日志后端存在同样的问题。其 `updateStackSize()` 在 `current > 2.5 * maxSteps` 时调用同一个 `removeEvents(0, maxSteps)`。`probe_journal.cpp` 以日志后端执行同一场景，提交到第 260 步后撤销到第 149 步，AddressSanitizer 报告 heap-use-after-free：释放点为 `updateStackSize()`（`AceTreeJournalBackend.cpp:526`）经 `AceTreeRowsInsDelEvent::clean()`，使用点为第 150 步删除事件的 `execute(true)` 经 `insertRows_helper()`（`AceTreeItem.cpp:222`）。

日志后端从检查点读回被删除节点的机制（见下文「历史记录的换出与读入」）不能避免这一问题。该机制只为从磁盘读回的一段历史构造事件，所需节点取自检查点并重新构造。仍在内存中的事件不经过检查点，继续使用其持有的指针。读回机制本身也依赖这些指针有效：读回第 1 至 100 步后，第 2 步的插入操作只以 ID 引用 X，X 之所以能在模型中找到，是因为此前撤销第 150 步时，删除事件已用其持有的指针将 X 插回树中。X 于第 150 步删除，不在第 1 至 100 步的检查点 `ckpt_1.dat` 中。

**不经插入事件进入模型的节点被删除后不会释放。** 通过 `setRootItem()` 一次性设置的初始树中的节点，没有对应的插入事件。`probe_leak.cpp` 中，节点 Y 属于初始树，于第 2 步被删除，该步骤被淘汰后：

- `AceTreeModel::itemFromIndex()` 仍返回 Y，即 Y 仍在模型的 ID 表中。
- 销毁模型后 Y 仍可读取（AddressSanitizer 未报告），即 Y 从未被释放。模型的析构函数只释放树与插入事件持有的节点。

`Design.md` 的所有权模型以「删除它的动作持有它」取代「插入它的事件负责释放它」，两个问题均不再存在。

### 日志后端

`AceTreeJournalBackend` 继承内存后端，在其每个回调（提交、步数变化、重置、模型信息变化）中生成任务，交给一个后台线程写入。

**文件：**

| 文件 | 内容 |
|---|---|
| `model_steps.dat` | 每个检查点的步数、保留的检查点数、日志中的最小与最大步数、当前步数、最大 ID |
| `model_info.dat` | 模型信息（字符串到 `QVariant` 的映射） |
| `journal_<n>.dat` | 第 n 段的事务，每段 `maxSteps` 个。文件头是各事务结束位置的表，其后依次为各事务的消息与操作 |
| `ckpt_<n>.dat` | 第 n 段结束时的整棵树，以及该段内被删除的全部节点 |

**操作的序列化（`journal/Operations.h`）：** 每种事件对应一种操作，节点以 ID 引用。插入类操作写出被插入子树的完整内容，删除类操作只写出被删除节点的 ID。被删除节点的内容由检查点提供，因此检查点须包含该段内删除的节点。

**历史记录的换出与读入：** 内存中只保留约 2 至 3 段事务。撤销接近内存中最早的步骤时，后台线程读取前一段的检查点与日志并在前方补入，其中插入类操作只读取 ID（`readBrief()`），节点取自检查点中的被删除节点。重做接近最新的步骤时，在后方补入。读取未完成而需要执行时，主线程等待条件变量。

**恢复：** `recover()` 读取 `model_steps.dat`，从距当前步数最近的检查点重建树，读入前后的事务，再撤销或重做到记录的当前步数。

### 日志后端的持久性

- **写入只调用 `QFile::flush()`，全仓库没有 `fsync` 或 `FlushFileBuffers` 调用。** `flush()` 只将 Qt 的缓冲区交给操作系统，数据可在进程崩溃后保留，但不保证在断电后保留。
- `model_steps.dat` 的更新以「单次 `write()` 调用」保证原子性。单次 `write()` 在断电时不保证原子。
- 提交任务先写事务、再写步数文件，恢复时以步数文件为准，因此写到一半的事务会被忽略。这一顺序是正确的，但依赖上述两点才成立。
- 检查点任务在主线程上克隆整棵树（`genWriteCkptTask()`），每 `maxSteps` 次提交执行一次，代价与树的大小成正比。

### 测试

`src/tests/tst_Basic.cpp` 共 61 行，未覆盖撤销、历史淘汰与恢复。上述两个问题因此未被发现。两者都只需一个提交 200 余次事务的测试即可暴露。

## 对 substate 的结论

| 事项 | AceTreeModel | substate 的做法 |
|---|---|---|
| 被删除节点的所有者 | 插入它的事件，负责释放 | 删除它的动作，以 `std::unique_ptr` 持有 |
| 已废弃状态 | 需要，沿子树维护 | 不需要，以 `isAttached()` 表示是否在树上 |
| 删除操作的日志 | 只写 ID，内容在检查点中 | 相同。引擎须能枚举一个事务持有的节点，以写入检查点 |
| 执行与通知 | 后端执行事件 | 模型执行，引擎只保存 |
| 持久性 | `flush()` | 第二阶段设计须规定 `fsync` 的时机与原子更新的方式 |
| 后台写入 | 任务队列与一个线程 | 第二阶段再定，接口不假定同步或异步 |
