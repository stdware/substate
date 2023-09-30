#include "memengine.h"
#include "memengine_p.h"

namespace Substate {

    MemoryEnginePrivate::MemoryEnginePrivate() {
        maxSteps = 4; // For test only
        min = 0;
        current = 0;
    }

    MemoryEnginePrivate::~MemoryEnginePrivate() {
        removeActions(0, stack.size());
    }

    void MemoryEnginePrivate::init() {
    }

    void MemoryEnginePrivate::removeActions(int b, int e) {
        if (b >= e) {
            return;
        }

        auto begin = stack.begin() + b;
        auto end = stack.begin() + e;
        for (auto it = begin; it != end; ++it) {
            auto &tx = *it;
            for (const auto &a : std::as_const(tx.actions)) {
                a->virtual_hook(Action::CleanNodesHook, nullptr);
                delete a;
            }
        }
        stack.erase(begin, end);
    }

    bool MemoryEnginePrivate::acceptChangeMaxSteps(int steps) const {
        return steps >= 100;
    }

    void MemoryEnginePrivate::afterCurrentChange() {
    }

    void MemoryEnginePrivate::afterCommit(const std::vector<Action *> &actions,
                                          const Engine::StepMessage &message) {
        SUBSTATE_UNUSED(actions)
        SUBSTATE_UNUSED(message)

        if (current > 2 * maxSteps) {
            // Remove head
            removeActions(0, maxSteps);
            min += maxSteps;
            current -= maxSteps;
        }
    }

    void MemoryEnginePrivate::afterReset() {
    }

    MemoryEngine::MemoryEngine() : MemoryEngine(*new MemoryEnginePrivate()) {
    }

    MemoryEngine::~MemoryEngine() {
    }

    int MemoryEngine::preservedSteps() const {
        Q_D(const MemoryEngine);
        return d->maxSteps;
    }

    void MemoryEngine::setPreservedSteps(int steps) {
        Q_D(MemoryEngine);
        if (d->model) {
            SUBSTATE_WARNING("changing engine parameters after setup is prohibited");
            return;
        }

        if (!d->acceptChangeMaxSteps(steps)) {
            SUBSTATE_WARNING("specified steps %d is too small", steps);
            return;
        }
        d->maxSteps = steps;
    }

    void MemoryEngine::commit(const std::vector<Action *> &actions, const StepMessage &message) {
        Q_D(MemoryEngine);

        // Truncate stack tail
        if (d->current < d->stack.size()) {
            d->removeActions(d->current, d->stack.size());
        }

        // Commit
        d->stack.push_back({actions, message});
        d->current++;

        // Post actions
        d->afterCommit(actions, message);
    }

    void MemoryEngine::execute(bool undo) {
        Q_D(MemoryEngine);
        if (undo) {
            if (d->current == 0)
                return;

            // Step backward
            const auto &tx = d->stack.at(d->current - 1);
            for (const auto &a : tx.actions) {
                a->execute(true);
            }
            d->current--;
        } else {
            if (d->current == d->stack.size())
                return;

            // Step forward
            const auto &tx = d->stack.at(d->current);
            for (const auto &a : tx.actions) {
                a->execute(false);
            }
            d->current++;
        }
        d->afterCurrentChange();
    }

    void MemoryEngine::reset() {
        Engine::reset();

        Q_D(MemoryEngine);
        d->stack.clear(); // All nodes have been deleted in Engine::reset()
        d->min = 0;
        d->current = 0;

        d->afterReset();
    }

    int MemoryEngine::minimum() const {
        Q_D(const MemoryEngine);
        return d->min;
    }

    int MemoryEngine::maximum() const {
        Q_D(const MemoryEngine);
        return d->min + int(d->stack.size());
    }

    int MemoryEngine::current() const {
        Q_D(const MemoryEngine);
        return d->min + d->current;
    }

    Engine::StepMessage MemoryEngine::stepMessage(int step) const {
        Q_D(const MemoryEngine);
        step -= d->min + 1;
        if (step < 0 || step >= d->stack.size())
            return {};
        return d->stack.at(step).message;
    }

    /*!
        \internal
    */
    MemoryEngine::MemoryEngine(MemoryEnginePrivate &d) : Engine(d) {
        d.init();
    }
}