#ifndef VARIANT_H
#define VARIANT_H

#include <atomic>
#include <string>

#include <substate/stream.h>

namespace Substate {

    template <class T>
    class TypeFunctionHelper {
    public:
        static void *read(IStream &stream) {
            auto p = new T();
            stream >> *p;
            if (stream.fail()) {
                delete p;
                return nullptr;
            }
            return p;
        }

        static bool write(const void *buf, OStream &stream) {
            stream << *reinterpret_cast<const T *>(buf);
            return stream.good();
        }

        static bool null(const void *buf) {
            return *reinterpret_cast<const T *>(buf) == T();
        }

        static bool equal(const void *buf, const void *buf2) {
            return *reinterpret_cast<const T *>(buf) == *reinterpret_cast<const T *>(buf2);
        }

        static void *construct(const void *buf) {
            return buf ? new T(*reinterpret_cast<const T *>(buf)) : new T();
        }

        static void destroy(void *buf) {
            delete reinterpret_cast<T *>(buf);
        }
    };

    class SUBSTATE_EXPORT Variant {
    public:
        enum Type {
            Invalid = 0,
            Boolean,
            Byte,
            Int16,
            Int32,
            Int64,
            UByte,
            UInt16,
            UInt32,
            UInt64,
            Single,
            Double,
            Null,
            String,
            User = 1024,
        };

        inline Variant() noexcept;
        Variant(Type type);
        Variant(bool b);
        Variant(int8_t c);
        Variant(int16_t s);
        Variant(int32_t i);
        Variant(int64_t l);
        Variant(uint8_t uc);
        Variant(uint16_t us);
        Variant(uint32_t u);
        Variant(uint64_t ul);
        Variant(float f);
        Variant(double d);
        Variant(const std::string &s);
        Variant(const char *s, int size = -1);
        Variant(int type, const void *data); // copy
        ~Variant();

        Variant(const Variant &other);
        Variant &operator=(const Variant &other);
        inline Variant(Variant &&other) noexcept;
        inline Variant &operator=(Variant &&other) noexcept;
        inline void swap(Variant &other) noexcept;

        bool operator==(const Variant &other) const;
        inline bool operator!=(const Variant &other) const;

        Type type() const;
        int userType() const;

        bool isValid() const;
        bool isNull() const;

        bool toBool() const;
        int8_t toByte() const;
        int16_t toInt16() const;
        int32_t toInt32() const;
        int64_t toInt64() const;
        uint8_t toUByte() const;
        uint16_t toUInt16() const;
        uint32_t toUInt32() const;
        uint64_t toUInt64() const;
        float toSingle() const;
        double toDouble() const;
        std::string toString() const;

        void *data();
        const void *constData() const;

        void clear();

        void detach();
        bool isDetached() const;

        template <class T>
        inline static Variant fromValue(const T &val);

        template <class T>
        inline void setValue(const T &val);

        template <class T>
        inline T value() const;

        template <class T>
        static inline int typeId(int hint = -1);

        SUBSTATE_EXPORT friend IStream &operator>>(IStream &stream, Variant &var);
        SUBSTATE_EXPORT friend OStream &operator<<(OStream &stream, const Variant &var);

    public:
        struct PrivateShared {
            inline PrivateShared(void *v) : ptr(v), ref(1) {
            }
            void *ptr;
            std::atomic_int ref;
        };

        struct Private {
            inline Private() noexcept : type(Invalid), is_shared(false) {
                data.ptr = nullptr;
            }
            // Internal constructor for initialized variants.
            explicit inline Private(int type) noexcept : type(type), is_shared(false) {
            }
            union Data {
                bool b;
                int8_t c;
                int16_t s;
                int32_t i;
                int64_t l;
                uint8_t uc;
                uint16_t us;
                uint32_t u;
                uint64_t ul;
                double d;
                float f;
                void *ptr;
                PrivateShared *shared;
            } data;
            int type;
            bool is_shared;
        };

        struct Handler {
            void *(*read)(IStream &);
            bool (*write)(const void *, OStream &);
            bool (*null)(const void *);
            bool (*equal)(const void *, const void *);
            void *(*construct)(const void *);
            void (*destroy)(void *);
        };

        static const Variant &sharedNull();

    protected:
        Private d;

        static int registerUserTypeImpl(const std::type_info &info, const Handler &handler,
                                        int hint = -1);
    };

    Variant::Variant() noexcept : d() {
    }

    inline Variant::Variant(Variant &&other) noexcept : d(other.d) {
        other.d = {};
    }

    inline Variant &Variant::operator=(Variant &&other) noexcept {
        std::swap(d, other.d);
        return *this;
    }

    inline void Variant::swap(Variant &other) noexcept {
        std::swap(d, other.d);
    }

    bool Variant::operator!=(const Variant &other) const {
        return !(*this == other);
    }

    template <class T>
    inline Variant Variant::fromValue(const T &val) {
        return {typeId<T>(), &val};
    }

    template <>
    inline Variant Variant::fromValue<Variant>(const Variant &val) {
        return val;
    }

    template <class T>
    inline void Variant::setValue(const T &val) {
        *this = fromValue(val);
    }

    template <class T>
    inline T Variant::value() const {
        // Storing a pointer is not supported
        static_assert(!std::is_pointer<T>::value, "Container is a pointer");

        return d.type == typeId<T>() ? *reinterpret_cast<const T *>(constData()) : T();
    }

    template <class T>
    inline int Variant::typeId(int hint) {
        // Storing a pointer is not supported
        static_assert(!std::is_pointer<T>::value, "Container is a pointer");

        static std::atomic_int type_id(0);
        int id = type_id.load();
        if (id > 0) {
            return id;
        }
        id = Variant::registerUserTypeImpl(typeid(T),
                                           {
                                               TypeFunctionHelper<T>::read,
                                               TypeFunctionHelper<T>::write,
                                               TypeFunctionHelper<T>::null,
                                               TypeFunctionHelper<T>::equal,
                                               TypeFunctionHelper<T>::construct,
                                               TypeFunctionHelper<T>::destroy,
                                           },
                                           hint);
        type_id.store(id);
        return id;
    }

#define SUBSTATE_STATIC_PRIMITIVE_TYPE(TYPE, ID)                                                   \
    template <>                                                                                    \
    inline int Variant::typeId<TYPE>(int hint) {                                                   \
        (void) hint;                                                                               \
        return ID;                                                                                 \
    }

    SUBSTATE_STATIC_PRIMITIVE_TYPE(bool, Variant::Boolean)
    SUBSTATE_STATIC_PRIMITIVE_TYPE(int8_t, Variant::Byte)
    SUBSTATE_STATIC_PRIMITIVE_TYPE(int16_t, Variant::Int16)
    SUBSTATE_STATIC_PRIMITIVE_TYPE(int32_t, Variant::Int32)
    SUBSTATE_STATIC_PRIMITIVE_TYPE(int64_t, Variant::Int64)
    SUBSTATE_STATIC_PRIMITIVE_TYPE(uint8_t, Variant::UByte)
    SUBSTATE_STATIC_PRIMITIVE_TYPE(uint16_t, Variant::UInt16)
    SUBSTATE_STATIC_PRIMITIVE_TYPE(uint32_t, Variant::UInt32)
    SUBSTATE_STATIC_PRIMITIVE_TYPE(uint64_t, Variant::UInt64)
    SUBSTATE_STATIC_PRIMITIVE_TYPE(float, Variant::Single)
    SUBSTATE_STATIC_PRIMITIVE_TYPE(double, Variant::Double)
    SUBSTATE_STATIC_PRIMITIVE_TYPE(std::string, Variant::String)

}

#endif // VARIANT_H
