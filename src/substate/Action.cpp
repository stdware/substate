#include "Action.h"

#include "Model.h"
#include "Node_p.h"

namespace ss {

    Action::~Action() = default;

    void Action::forEachHeldNode(const std::function<void(Node *)> &func) const {
        (void) func;
    }

    RootChangeAction::RootChangeAction(Model *model, std::unique_ptr<Node> newRoot)
        : Action(RootChange), m_model(model), m_newRoot(newRoot.get()), m_oldRoot(model->root()),
          m_held(std::move(newRoot)) {
    }

    RootChangeAction::~RootChangeAction() = default;

    void RootChangeAction::forEachHeldNode(const std::function<void(Node *)> &func) const {
        if (m_held) {
            func(m_held.get());
        }
    }

    void RootChangeAction::execute(Operation operation) {
        // Every operation exchanges the held root with the root of the model.
        (void) operation;

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

}
