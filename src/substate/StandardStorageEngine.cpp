#include "StandardStorageEngine.h"

#include "Model.h"

namespace ss {

    StandardStorageEngine::StandardStorageEngine() = default;

    StandardStorageEngine::~StandardStorageEngine() = default;

    void StandardStorageEngine::setMaxSteps(int steps) {
        if (_model) {
            return;
        }

        if (steps < 4) {
            return;
        }
        _maxSteps = steps;
    }

    void StandardStorageEngine::commit(std::vector<std::unique_ptr<Action>> actions,
                                       const std::map<std::string, std::string> message) {
        // Truncate stack tail
        if (_current < _stack.size()) {
            _stack.erase(_stack.begin() + _current, _stack.end());
        }

        // Commit
        _stack.emplace_back(std::move(actions), message);
        _current++;

        // Post actions
        if (_current > 2 * _maxSteps) {
            // Remove head
            _stack.erase(_stack.begin(), _stack.begin() + _maxSteps);
            _min += _maxSteps;
            _current -= _maxSteps;
        }
    }

    void StandardStorageEngine::execute(bool undo) {
        if (undo) {
            if (_current == 0)
                return;

            // Step backward
            const auto &tx = _stack.at(_current - 1);
            for (auto it = tx.actions.rbegin(); it != tx.actions.rend(); ++it) {
                (*it)->execute(true);
            }
            _current--;
        } else {
            if (_current == _stack.size())
                return;

            // Step forward
            const auto &tx = _stack.at(_current);
            for (auto it = tx.actions.begin(); it != tx.actions.end(); ++it) {
                (*it)->execute(false);
            }
            _current++;
        }
    }

    void StandardStorageEngine::reset() {
        _model->_clearing = true;

        // Delete all nodes
        _model->_root.reset();
        _stack.clear();

        _min = 0;
        _current = 0;

        _idMap.clear();
        _maxId = 0;

        _model->_clearing = false;
    }

    int StandardStorageEngine::minimum() const {
        return _min;
    }

    int StandardStorageEngine::maximum() const {
        return _min + int(_stack.size());
    }

    int StandardStorageEngine::current() const {
        return _min + _current;
    }

    std::map<std::string, std::string> StandardStorageEngine::stepMessage(int step) const {
        step -= _min + 1;
        if (step < 0 || step >= _stack.size())
            return {};
        return _stack.at(step).message;
    }

}