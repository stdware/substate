#ifndef FSENGINE_H
#define FSENGINE_H

#include <filesystem>

#include <substate/memengine.h>

namespace Substate {

    class FileSystemEnginePrivate;

    class SUBSTATE_EXPORT FileSystemEngine : public MemoryEngine {
        SUBSTATE_DECL_PRIVATE(FileSystemEngine)
    public:
        FileSystemEngine();
        ~FileSystemEngine();

    public:
        int checkpoints() const;
        void setCheckpoints(int n);

        bool finished() const;
        void setFinished(bool finished);

        bool start(const std::filesystem::path &dir);
        bool recover(const std::filesystem::path &dir);

    public:
        void setup(Model *model) override;
        void prepare() override;
        void abort() override;

        int minimum() const override;
        int maximum() const override;
        StepMessage stepMessage(int step) const override;

    protected:
        virtual bool createWarningFile(const std::filesystem::path &dir);

    protected:
        FileSystemEngine(FileSystemEnginePrivate &d);
    };

}

#endif // FSENGINE_H
