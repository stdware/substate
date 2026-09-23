#include "Action.h"

#include <cassert>

#include "Codec.h"
#include "Model.h"
#include "Node_p.h"
#include "Transfer_p.h"

namespace ss {

    Action::~Action() = default;

    void Action::forEachHeldNode(const std::function<void(Node *)> &func) const {
        (void) func;
    }

    void Action::write(Encoder &encoder) const {
        encoder.setFailed();
    }

    RootChangeAction::RootChangeAction(Model *model, Node *newRoot, Node *oldRoot,
                                       std::unique_ptr<Node> held)
        : Action(RootChange), m_model(model), m_newRoot(newRoot), m_oldRoot(oldRoot),
          m_held(std::move(held)) {
        assert(!m_held || m_held.get() == m_newRoot || m_held.get() == m_oldRoot);
    }

    RootChangeAction::~RootChangeAction() = default;

    void RootChangeAction::forEachHeldNode(const std::function<void(Node *)> &func) const {
        if (m_held) {
            func(m_held.get());
        }
    }

    void RootChangeAction::execute(Operation operation) {
        // Every operation exchanges the held root with the root of the model.
        assert(m_model->root() == oldRoot(operation));

        std::unique_ptr<Node> leaving = std::move(m_model->m_root);
        if (leaving) {
            NodePrivate::detach(leaving.get());
        }
        m_model->m_root = std::move(m_held);
        if (m_model->m_root) {
            NodePrivate::attach(m_model->m_root.get(), nullptr, m_model);
        }
        m_held = std::move(leaving);
    }

    void RootChangeAction::write(Encoder &encoder) const {
        encoder.writeNode(m_newRoot);
        encoder.writeReference(m_oldRoot);
    }

    std::unique_ptr<Action> RootChangeAction::read(Decoder &decoder, State state) {
        // The root that is not in the tree is owned by the action: the new root while
        // unapplied, created from its content, and the old root while applied, taken from the
        // pool.
        Node *newRoot = nullptr;
        Node *oldRoot = nullptr;
        std::unique_ptr<Node> held;
        if (state == Unapplied) {
            held = decoder.readNode();
            newRoot = held.get();
            oldRoot = decoder.readReference();
        } else {
            newRoot = decoder.readExistingNode();
            held = decoder.takeReference();
            oldRoot = held.get();
        }
        if (decoder.fail()) {
            return nullptr;
        }
        return std::unique_ptr<Action>(
            new RootChangeAction(decoder.model(), newRoot, oldRoot, std::move(held)));
    }

    TransferEndpoint::~TransferEndpoint() = default;

    TransferAction::TransferAction(std::unique_ptr<TransferEndpoint> source,
                                   std::unique_ptr<TransferEndpoint> target,
                                   std::vector<Node *> nodes)
        : Action(Transfer), m_source(std::move(source)), m_target(std::move(target)),
          m_nodes(std::move(nodes)) {
    }

    TransferAction::~TransferAction() = default;

    Node *TransferAction::source(Operation operation) const {
        return (isForward(operation) ? m_source : m_target)->container();
    }

    Node *TransferAction::target(Operation operation) const {
        return (isForward(operation) ? m_target : m_source)->container();
    }

    void TransferAction::execute(Operation operation) {
        auto &from = isForward(operation) ? m_source : m_target;
        auto &to = isForward(operation) ? m_target : m_source;

        auto nodes = from->take(int(m_nodes.size()));
        for (const auto &node : nodes) {
            NodePrivate::reparent(node.get(), to->container());
        }
        to->put(std::move(nodes));
    }

    void TransferAction::write(Encoder &encoder) const {
        encoder.writeReference(m_source->container());
        m_source->write(encoder);
        encoder.writeReference(m_target->container());
        m_target->write(encoder);
        encoder.stream() << int32_t(m_nodes.size());
        for (auto node : m_nodes) {
            encoder.writeReference(node);
        }
    }

    std::unique_ptr<Action> TransferAction::read(Decoder &decoder, State state) {
        // A transfer owns no node in either state.
        (void) state;
        // Reads a container and a position within it.
        const auto readEnd = [&decoder]() -> std::unique_ptr<TransferEndpoint> {
            auto container = decoder.readReference();
            if (!container) {
                decoder.setFailed();
                return nullptr;
            }
            auto end = NodePrivate::readEndpoint(container, decoder);
            if (!end) {
                decoder.setFailed();
            }
            return end;
        };
        auto source = readEnd();
        auto target = readEnd();
        int32_t count = 0;
        decoder.stream() >> count;
        if (decoder.fail() || count < 1 || source->container() == target->container()) {
            decoder.setFailed();
            return nullptr;
        }
        std::vector<Node *> nodes;
        for (int32_t i = 0; i < count; ++i) {
            auto node = decoder.readReference();
            if (!node) {
                decoder.setFailed();
                return nullptr;
            }
            nodes.push_back(node);
        }
        return std::unique_ptr<Action>(
            new TransferAction(std::move(source), std::move(target), std::move(nodes)));
    }

}
