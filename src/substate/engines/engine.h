#ifndef ENGINE_H
#define ENGINE_H

#include <memory>
#include <vector>

#include <substate/action.h>
#include <substate/node.h>
#include <substate/variant.h>

namespace Substate {

    class Model;

    class EnginePrivate;

    class SUBSTATE_EXPORT Engine {
        QMSETUP_DECL_PRIVATE(Engine)
    public:
        virtual ~Engine();

        using StepMessage = std::unordered_map<std::string, Variant>;

    public:
        Model *model() const;
        Node *indexOf(int index) const;

        virtual void setup(Model *model);
        virtual void prepare();
        virtual void abort();
        virtual void commit(const std::vector<Action *> &actions, const StepMessage &message);
        virtual void execute(bool undo) = 0;
        virtual void reset();

        virtual int minimum() const = 0;
        virtual int maximum() const = 0;
        virtual int current() const = 0;
        virtual StepMessage stepMessage(int step) const = 0;

    protected:
        std::unique_ptr<EnginePrivate> d_ptr;
        Engine(EnginePrivate &d);

        friend class Node;
        friend class NodePrivate;

        QMSETUP_DISABLE_COPY_MOVE(Engine)
    };

}

#endif // ENGINE_H
