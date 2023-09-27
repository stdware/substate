#ifndef MODEL_H
#define MODEL_H

#include <substate/engine.h>
#include <substate/node.h>

namespace Substate {

    class Subscriber;

    class ModelPrivate;

    class SUBSTATE_EXPORT Model {
        SUBSTATE_DECL_PRIVATE(Model)
    public:
        Model();
        virtual ~Model();

    public:
        Engine::State state() const;
        inline bool inTransaction() const;
        inline bool stepChanging() const;

        void beginTransaction();
        void abortTransaction();
        void commitTransaction(const Variant &message);

    public:
        void addSubscriber(Subscriber *sub);
        void removeSubscriber(Subscriber *sub);

    protected:
        void dispatch(Operation *op, bool done);

    protected:
        std::unique_ptr<ModelPrivate> d_ptr;
        Model(ModelPrivate &d);

        friend class Node;
    };

    inline bool Model::inTransaction() const {
        return state() == Engine::Transaction;
    }

    inline bool Model::stepChanging() const {
        return state() & Engine::UndoRedoFlag;
    }

    class SUBSTATE_EXPORT Subscriber {
    public:
        Subscriber();
        virtual ~Subscriber();

    protected:
        virtual void operation(Operation *op, bool done) = 0;

    private:
        Model *m_model;

        friend class Model;
        friend class ModelPrivate;
    };

}

#endif // MODEL_H
