#ifndef FSENGINE_P_H
#define FSENGINE_P_H

#include <substate/private/memengine_p.h>
#include <substate/fsengine.h>

namespace Substate {

    class SUBSTATE_EXPORT FileSystemEnginePrivate : public MemoryEnginePrivate {
        QMSETUP_DECL_PUBLIC(FileSystemEngine)
    public:
        FileSystemEnginePrivate();
        ~FileSystemEnginePrivate();
        void init();

        void setup_helper();

        // If true, the journal files will be removed when destructs
        bool finished;

        // The maximum number of checkpoints kept in filesystem
        int maxCheckPoints;

        // The directory containing the journal files
        std::filesystem::path dir;

        // Real bound in filesystem
        int fsMin;
        int fsMax;

        // Internal journal data
        class JournalData;
        std::unique_ptr<JournalData> journalData;

        void updateStackSize();
        void extractBackwardJournal(std::vector<TransactionData> &data,
                                    std::vector<Node *> &removedItems);
        void extractForwardJournal(std::vector<TransactionData> &data,
                                   std::unordered_map<int, Node *> &insertedItems);

        // Override functions
        bool acceptChangeMaxSteps(int steps) const override;
        void afterCurrentChange() override;
        void afterCommit(const std::vector<Action *> &actions,
                         const Engine::StepMessage &message) override;
        void afterReset() override;
    };

}

#endif // FSENGINE_P_H
