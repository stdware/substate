#ifndef VECTORENTITY_H
#define VECTORENTITY_H

#include <qsubstate/entity.h>

namespace Substate {

    class VectorEntityBasePrivate;

    class QSUBSTATE_EXPORT VectorEntityBase : public Entity {
        Q_OBJECT
        Q_DECLARE_PRIVATE(VectorEntityBase)
    public:
        ~VectorEntityBase();

    protected:
        bool insertImpl(int index, const QVector<Entity *> &items);
        bool moveImpl(int index, int count, int dest); // dest: destination index before move
        bool removeImpl(int index, int count);
        Entity *atImpl(int index) const;
        int sizeImpl() const;

    protected:
        virtual void sendInserted(int index, const QVector<Entity *> &items) = 0;
        virtual void sendAboutToMove(int index, int count, int dest) = 0;
        virtual void sendMoved(int index, int count, int dest) = 0;
        virtual void sendAboutToRemove(int index, const QVector<Entity *> &items) = 0;
        virtual void sendRemoved(int index, int count) = 0;

    protected:
        VectorEntityBase(Node *node, QObject *parent = nullptr);
    };

    template <class Container, class T>
    class VectorEntityHelper {
    public:
        bool insert(int index, const std::vector<T *> &items) {
            QVector<Entity *> newItems;
            newItems.reserve(items.size());
            for (const auto &item : items) {
                newItems.push_back(item);
            }
            return static_cast<Container *>(this)->insertImpl(index, newItems);
        }

        bool move(int index, int count, int dest) {
            return static_cast<Container *>(this)->moveImpl(index, count, dest);
        }

        bool remove(int index, int count) {
            return static_cast<Container *>(this)->removeImpl(index, count);
        }

        T *at(int index) const {
            return static_cast<T *>(static_cast<const Container *>(this)->atImpl(index));
        }

        int size() const {
            return static_cast<const Container *>(this)->sizeImpl();
        }

    private:
        void sendInsertedHelper(int index, const QVector<Entity *> &items) {
            QVector<T *> newItems;
            newItems.reserve(items.size());
            for (const auto &item : items) {
                newItems.push_back(static_cast<T *>(item));
            }
            Q_EMIT static_cast<Container *>(this)->inserted(index, newItems);
        }

        inline void sendAboutToMoveHelper(int index, int count, int dest) {
            Q_EMIT static_cast<Container *>(this)->aboutToMove(index, count, dest);
        }

        inline void sendMovedHelper(int index, int count, int dest) {
            Q_EMIT static_cast<Container *>(this)->moved(index, count, dest);
        }

        void sendAboutToRemoveHelper(int index, const QVector<Entity *> &items) {
            QVector<T *> newItems;
            newItems.reserve(items.size());
            for (const auto &item : items) {
                newItems.push_back(static_cast<T *>(item));
            }
            Q_EMIT static_cast<Container *>(this)->aboutToRemove(index, newItems);
        }

        inline void sendRemovedHelper(int index, int count) {
            Q_EMIT static_cast<Container *>(this)->removed(index, count);
        }

        friend Container;
    };

#define Q_SUBSTATE_DECLARE_VECTOR(Container, T)                                                    \
    friend class Substate::VectorEntityHelper<Container, T>;                                       \
                                                                                                   \
Q_SIGNALS:                                                                                         \
    void inserted(int index, const QVector<T *> &items);                                           \
    void aboutToMove(int index, int count, int dest);                                              \
    void moved(int index, int count, int dest);                                                    \
    void aboutToRemove(int index, const QVector<T *> &items);                                      \
    void removed(int index, int count);                                                            \
                                                                                                   \
protected:                                                                                         \
    void sendInserted(int index, const QVector<Substate::Entity *> &items) override {              \
        sendInsertedHelper(index, items);                                                          \
    }                                                                                              \
    void sendAboutToMove(int index, int count, int dest) override {                                \
        sendAboutToMoveHelper(index, count, dest);                                                 \
    }                                                                                              \
    void sendMoved(int index, int count, int dest) override {                                      \
        sendMovedHelper(index, count, dest);                                                       \
    }                                                                                              \
    void sendAboutToRemove(int index, const QVector<Substate::Entity *> &items) override {         \
        sendAboutToRemoveHelper(index, items);                                                     \
    }                                                                                              \
    void sendRemoved(int index, int count) override {                                              \
        sendRemovedHelper(index, count);                                                           \
    }

}

#endif // VECTORENTITY_H
