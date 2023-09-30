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
        SUBSTATE_DECL_PRIVATE(Engine)
    public:
        virtual ~Engine();

    public:
        Model *model() const;

        int minimum() const;
        int maximum() const;
        int current() const;

        virtual void setup(Model *model);
        virtual void commit(const std::vector<Action *> &actions, const Variant &message);
        virtual void execute(bool undo) = 0;

    protected:
        void setMinimum(int value);
        void setMaximum(int value);
        void setCurrent(int value);

    protected:
        std::unique_ptr<EnginePrivate> d_ptr;
        Engine(EnginePrivate &d);

        SUBSTATE_DISABLE_COPY_MOVE(Engine)
    };

}

#endif // ENGINE_H
