#ifndef ENGINE_H
#define ENGINE_H

#include <memory>
#include <vector>

#include <substate/action.h>
#include <substate/node.h>
#include <substate/variant.h>

namespace Substate {

    class EngineHelper;

    class EnginePrivate;

    class SUBSTATE_EXPORT Engine {
        SUBSTATE_DECL_PRIVATE(Engine)
    public:
        virtual ~Engine();

    public:
        int minimum() const;
        int maximum() const;
        int current() const;

        virtual void commit(const std::vector<Action *> &actions, const Variant &message) = 0;
        virtual void execute(bool undo) = 0;

    protected:
        void setMinimum(int value);
        void setMaximum(int value);
        void setCurrent(int value);

    protected:
        std::unique_ptr<EnginePrivate> d_ptr;
        Engine(EnginePrivate &d);

        friend class EngineHelper;

        SUBSTATE_DISABLE_COPY_MOVE(Engine)
    };

}

#endif // ENGINE_H
