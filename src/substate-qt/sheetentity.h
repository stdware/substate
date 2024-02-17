#ifndef SHEETENTITY_H
#define SHEETENTITY_H

#include <substate/sheetnode.h>
#include <qsubstate/entity.h>
#include <qsubstate/arrayentity.h>

namespace Substate {

    class SheetEntityBasePrivate;

    class QSUBSTATE_EXPORT SheetEntityBase : public Entity {
        Q_OBJECT
        Q_DECLARE_PRIVATE(SheetEntityBase)
    public:
        ~SheetEntityBase();

    protected:
        int insertImpl(Entity *item);
        bool removeImpl(int id);
        bool removeImpl(Entity *item);
        Entity *valueImpl(int id) const;
        int indexOfImpl(Entity *item) const;
        QList<int> idsImpl() const;
        int sizeImpl() const;

    protected:
        virtual void sendInserted(int index, Entity *item) = 0;
        virtual void sendAboutToRemove(int index, Entity *item) = 0;
        virtual void sendRemoved(int index, Entity *item) = 0;

    protected:
        SheetEntityBase(Node *node, QObject *parent = nullptr);
    };

    class SheetEntityBasePrivate;

    template <class Container, class T>
    class SheetEntityHelper {
    public:
        bool insert(T *item) {
            return static_cast<Container *>(this)->insertImpl(item);
        }

        bool remove(int id) {
            return static_cast<Container *>(this)->removeImpl(id);
        }

        bool remove(T *item) {
            return static_cast<Container *>(this)->removeImpl(item);
        }

        T *value(int id) const {
            return static_cast<T *>(static_cast<const Container *>(this)->valueImpl(id));
        }

        int indexOf(T *item) const {
            return static_cast<const Container *>(this)->indexOfImpl(item);
        }

        QList<int> ids() const {
            return static_cast<const Container *>(this)->idsImpl();
        }

        int size() const {
            return static_cast<const Container *>(this)->sizeImpl();
        }

    private:
        inline void sendInsertedHelper(int id, Entity *item) {
            Q_EMIT static_cast<Container *>(this)->inserted(id, static_cast<T *>(item));
        }

        inline void sendAboutToRemoveHelper(int id, Entity *item) {
            Q_EMIT static_cast<Container *>(this)->aboutToRemove(id, static_cast<T *>(item));
        }

        inline void sendRemovedHelper(int id, Entity *item) {
            Q_EMIT static_cast<Container *>(this)->removed(id, static_cast<T *>(item));
        }

        friend Container;
    };

#define Q_SUBSTATE_DECLARE_SHEET(Container, T)                                                     \
    friend class Substate::SheetEntityHelper<Container, T>;                                        \
                                                                                                   \
Q_SIGNALS:                                                                                         \
    void inserted(int id, T *item);                                                                \
    void aboutToRemove(int id, T *item);                                                           \
    void removed(int id, T *item);                                                                 \
                                                                                                   \
protected:                                                                                         \
    inline void sendInserted(int id, Substate::Entity *item) {                                     \
        sendInsertedHelper(id, item);                                                              \
    }                                                                                              \
                                                                                                   \
    inline void sendAboutToRemove(int id, Substate::Entity *item) {                                \
        sendAboutToRemoveHelper(id, item);                                                         \
    }                                                                                              \
                                                                                                   \
    inline void sendRemoved(int id, Substate::Entity *item) {                                      \
        sendRemovedHelper(id, item);                                                               \
    }

#define Q_SUBSTATE_DECLARE_SHEET_CLASS(Container, T)                                               \
    class Container : public Substate::SheetEntityBase,                                            \
                      public Substate::SheetEntityHelper<Container, T> {                           \
        Q_OBJECT                                                                                   \
        Q_SUBSTATE_DECLARE_SHEET(Container, T)                                                     \
    public:                                                                                        \
        explicit Container(QObject *parent = nullptr)                                              \
            : Container(new Substate::SheetNode(), parent) {                                       \
        }                                                                                          \
                                                                                                   \
    protected:                                                                                     \
        inline explicit Container(Substate::Node *node, QObject *parent = nullptr)                 \
            : Substate::SheetEntityBase(node, parent) {                                            \
        }                                                                                          \
    };

    Q_SUBSTATE_DECLARE_SHEET_CLASS(TestSheetEntity, Int8ArrayEntity)

}

#endif // SHEETENTITY_H
