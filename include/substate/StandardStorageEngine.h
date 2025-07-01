#ifndef SUBSTATE_STANDARDSTORAGEENGINE_H
#define SUBSTATE_STANDARDSTORAGEENGINE_H

#include <substate/StorageEngine.h>

namespace ss {

    class SUBSTATE_EXPORT StandardStorageEngine : public StorageEngine {
    public:
        StandardStorageEngine();
        ~StandardStorageEngine();

        inline int maxSteps() const;
        void setMaxSteps(int steps);

    public:
        void commit(std::vector<std::unique_ptr<Action>> actions,
                    std::map<std::string, std::string> message) override;
        void execute(bool undo) override;
        void reset() override;

        int minimum() const override;
        int maximum() const override;
        int current() const override;
        std::map<std::string, std::string> stepMessage(int step) const override;

    protected:
        int _maxSteps = 100; // Maximum number of actions kept in memory
        int _min = 0;        // Minimum step the engine can reach
        int _current = 0;    // Current index of undo stack

        struct TransactionData {
            std::vector<std::unique_ptr<Action>> actions;
            std::map<std::string, std::string> message;

            inline TransactionData(std::vector<std::unique_ptr<Action>> actions,
                                   std::map<std::string, std::string> message)
                : actions(std::move(actions)), message(std::move(message)) {
            }
            TransactionData(TransactionData &&other) = default;
            TransactionData &operator=(TransactionData &&other) = default;
        };
        std::vector<TransactionData> _stack; // Undo stack
    };

    inline int StandardStorageEngine::maxSteps() const {
        return _maxSteps;
    }

}

#endif // SUBSTATE_STANDARDSTORAGEENGINE_H