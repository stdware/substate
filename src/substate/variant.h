#ifndef VARIANT_H
#define VARIANT_H

#include <atomic>
#include <string>

#include <substate/substate_global.h>

namespace Substate {

    struct VariantHandler {
        void *(*read)(std::istream &);
        bool (*write)(const void *, std::ostream &);
        void *(*construct)(const void *);
        void (*destroy)(void *);
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
        inline Variant(char c);
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
        Variant(int type, const void *data); // copy
        ~Variant();

        Variant(const Variant &other);
        Variant &operator=(const Variant &other);
        inline Variant(Variant &&other) noexcept;
        inline Variant &operator=(Variant &&other) noexcept;
        inline void swap(Variant &other) noexcept;

        bool isValid() const;
        Type type() const;
        int userType() const;

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

        static Variant read(std::istream &in);
        void write(std::ostream &out) const;

        template <class T>
        inline T value() const;

        static bool addUserType(int id, const VariantHandler &handler);
        static bool removeUserType(int id);

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

    protected:
        Private d;
    };

    Variant::Variant() noexcept : d() {
    }

    Variant::Variant(char c) : Variant(int8_t(c)) {
    }

    inline Variant::Variant(Variant &&other) noexcept : d(other.d) {
        other.d = Private();
    }

    inline Variant &Variant::operator=(Variant &&other) noexcept {
        std::swap(d, other.d);
        return *this;
    }

    inline void Variant::swap(Variant &other) noexcept {
        std::swap(d, other.d);
    }

    template <class T>
    T Variant::value() const {
        return *reinterpret_cast<const T *>(constData());
    }

}

#endif // VARIANT_H
