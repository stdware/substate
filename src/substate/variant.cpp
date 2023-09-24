#include "variant.h"

#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>

#include "stream.h"

/*!
    \namespace Substate

    Substate library general namespace.
*/

namespace Substate {

    static std::shared_mutex handlerLock;

    static std::unordered_map<int, Variant::Handler> handlerManager = {
        {
         Variant::String,
         {
                TypeFunctionHelper<std::string>::read,
                TypeFunctionHelper<std::string>::write,
                TypeFunctionHelper<std::string>::construct,
                TypeFunctionHelper<std::string>::destroy,
            }, }
    };

    static inline Variant::Handler getHandler(int type) {
        std::shared_lock<std::shared_mutex> lock(handlerLock);
        auto it = handlerManager.find(type);
        if (it == handlerManager.end()) {
            throw std::exception("Substate::Variant: Unknown type");
        }
        return it->second;
    }

    template <class T>
    static T substate_toNum(const Variant::Private &d) {
        switch (d.type) {
            case Variant::Boolean:
                return d.data.b;
            case Variant::Byte:
                return d.data.c;
            case Variant::Int16:
                return d.data.s;
            case Variant::Int32:
                return d.data.i;
            case Variant::Int64:
                return d.data.l;
            case Variant::UByte:
                return d.data.uc;
            case Variant::UInt16:
                return d.data.us;
            case Variant::UInt32:
                return d.data.u;
            case Variant::UInt64:
                return d.data.ul;
            case Variant::Single:
                return d.data.f;
            case Variant::Double:
                return d.data.d;
            default:
                break;
        }
        return T{};
    }

    static inline void substate_v_construct(int type, Variant::Private *d, const void *data) {
        d->data.shared = new Variant::PrivateShared(getHandler(type).construct(data));
    }

    static inline void substate_v_destroy(int type, Variant::Private *d) {
        getHandler(type).destroy(d->data.shared->ptr);
        delete d->data.shared;
    }

    /*!
        \struct VariantHelper

        This struct encompasses the basic handlers for the storage and serialization of a class.

        \property VariantHelper::read
        \brief Reads the data from the input stream and deserializes it into an instance of the
        class, allocates a new block of space to store it, and returns the pointer of the
        block.

        \property VariantHelper::write
        \brief Serializes the instance of the class and writes the data into the output
        stream.

        \property VariantHelper::construct
        \brief Constructs a new instance of the class, allocates a new block of space to store
        it, and returns the pointer of the block. If the given instance is null, constructs a
        default one, otherwise uses the copy constructor.

        \property VariantHelper::destroy
        \brief Deletes the instance of the class.
    */

    /*!
        \class Variant

        Variant is a storage class for C++ classes, providing a general serialization feature
        for each type.
    */

    /*!
        \fn Variant::Variant()

        Constructs an invalid variant.
    */

    /*!
        Constructs an uninitialized variant of type \a type.
    */

    Variant::Variant(Variant::Type type) : d(type) {
        if (type >= User) {
            d.is_shared = true;
            substate_v_construct(type, &d, nullptr);
            return;
        }
    }

    /*!
        Constructs a new variant with a boolean value.
    */
    Variant::Variant(bool b) : d(Boolean) {
        d.data.b = b;
    }

    /*!
        Constructs a new variant with a char value.
    */
    Variant::Variant(int8_t c) : d(Byte) {
        d.data.c = c;
    }

    /*!
        Constructs a new variant with a short value.
    */
    Variant::Variant(int16_t s) : d(Int16) {
        d.data.s = s;
    }

    /*!
        Constructs a new variant with a int value.
    */
    Variant::Variant(int32_t i) : d(Int32) {
        d.data.i = i;
    }

    /*!
        Constructs a new variant with a long value.
    */
    Variant::Variant(int64_t l) : d(Int64) {
        d.data.l = l;
    }

    /*!
        Constructs a new variant with an unsigned char value.
    */
    Variant::Variant(uint8_t uc) : d(UByte) {
        d.data.uc = uc;
    }

    /*!
        Constructs a new variant with an unsigned short value.
    */
    Variant::Variant(uint16_t us) : d(UInt16) {
        d.data.us = us;
    }

    /*!
        Constructs a new variant with an unsigned int value.
    */
    Variant::Variant(uint32_t u) : d(UInt32) {
        d.data.u = u;
    }

    /*!
        Constructs a new variant with an unsigned long value.
    */
    Variant::Variant(uint64_t ul) : d(UInt64) {
        d.data.ul = ul;
    }

    /*!
        Constructs a new variant with a float value.
    */
    Variant::Variant(float f) : d(Single) {
        d.data.f = f;
    }

    /*!
        Constructs a new variant with a double value.
    */
    Variant::Variant(double d) : d(Double) {
        this->d.data.d = d;
    }

    /*!
        Constructs a new variant with a string value.
    */
    Variant::Variant(const std::string &s) : d(String) {
        d.is_shared = true;
        substate_v_construct(String, &d, &s);
    }

    /*!
        Constructs variant of type \a type, and initializes with \a data if \a data is not null.
    */
    Variant::Variant(int type, const void *data) : d(type) {
        switch (type) {
            case Variant::Boolean:
                d.data.b = *reinterpret_cast<const decltype(d.data.b) *>(data);
                break;
            case Variant::Byte:
                d.data.c = *reinterpret_cast<const decltype(d.data.c) *>(data);
                break;
            case Variant::Int16:
                d.data.s = *reinterpret_cast<const decltype(d.data.s) *>(data);
                break;
            case Variant::Int32:
                d.data.i = *reinterpret_cast<const decltype(d.data.i) *>(data);
                break;
            case Variant::Int64:
                d.data.l = *reinterpret_cast<const decltype(d.data.l) *>(data);
                break;
            case Variant::UByte:
                d.data.uc = *reinterpret_cast<const decltype(d.data.uc) *>(data);
                break;
            case Variant::UInt16:
                d.data.us = *reinterpret_cast<const decltype(d.data.us) *>(data);
                break;
            case Variant::UInt32:
                d.data.u = *reinterpret_cast<const decltype(d.data.u) *>(data);
                break;
            case Variant::UInt64:
                d.data.ul = *reinterpret_cast<const decltype(d.data.ul) *>(data);
                break;
            case Variant::Single:
                d.data.f = *reinterpret_cast<const decltype(d.data.f) *>(data);
                break;
            case Variant::Double:
                d.data.d = *reinterpret_cast<const decltype(d.data.d) *>(data);
                break;
            default:
                d.is_shared = true;
                substate_v_construct(type, &d, data);
                break;
        }
    }

    /*!
        Destructor.
    */
    Variant::~Variant() {
        if (d.is_shared && d.data.shared->ref.fetch_sub(1) == 1) {
            substate_v_destroy(d.type, &d);
        }
    }

    Variant::Variant(const Variant &other) : d(other.d) {
        if (d.is_shared) {
            d.data.shared->ref.fetch_add(1);
        }
    }

    Variant &Variant::operator=(const Variant &other) {
        if (this == &other)
            return *this;
        clear();
        if (other.d.is_shared) {
            other.d.data.shared->ref.fetch_add(1);
            d = other.d;
        } else {
            d = other.d;
        }
        return *this;
    }

    /*!
        \fn void Variant::swap(QVariant &other)

        Swaps variant \a other with this variant.
    */

    /*!
        Returns \c true if the storage type of this variant is valid, otherwise returns \c false.
    */
    bool Variant::isValid() const {
        return d.type != Invalid;
    }

    /*!
        Returns the storage type of the value stored in the variant.
    */
    Variant::Type Variant::type() const {
        return d.type >= User ? User : static_cast<Type>(d.type);
    }

    /*!
        Returns the storage type of the value stored in the variant. For non-user types, this is
        the same as type().

        \sa type()
    */
    int Variant::userType() const {
        return d.type;
    }

    /*!
        Returns the variant as a boolean if the variant has userType() Bool.
    */
    bool Variant::toBool() const {
        if (d.type == Boolean)
            return d.data.b;
        return false;
    }

    /*!
        Returns the variant as a signed char if the variant is numerical.
    */
    int8_t Variant::toByte() const {
        return substate_toNum<int8_t>(d);
    }

    /*!
        Returns the variant as a short if the variant is numerical.
    */
    int16_t Variant::toInt16() const {
        return substate_toNum<int16_t>(d);
    }

    /*!
        Returns the variant as an int if the variant is numerical.
    */
    int32_t Variant::toInt32() const {
        return substate_toNum<int32_t>(d);
    }

    /*!
        Returns the variant as a long if the variant is numerical.
    */
    int64_t Variant::toInt64() const {
        return substate_toNum<int64_t>(d);
    }

    /*!
        Returns the variant as an unsigned char if the variant is numerical.
    */
    uint8_t Variant::toUByte() const {
        return substate_toNum<uint8_t>(d);
    }

    /*!
        Returns the variant as an unsigned short if the variant is numerical.
    */
    uint16_t Variant::toUInt16() const {
        return substate_toNum<uint16_t>(d);
    }

    /*!
        Returns the variant as an unsigned int if the variant is numerical.
    */
    uint32_t Variant::toUInt32() const {
        return substate_toNum<uint32_t>(d);
    }

    /*!
        Returns the variant as an unsigned long if the variant is numerical.
    */
    uint64_t Variant::toUInt64() const {
        return substate_toNum<uint64_t>(d);
    }

    /*!
        Returns the variant as a float if the variant is numerical.
    */
    float Variant::toSingle() const {
        return substate_toNum<float>(d);
    }

    /*!
        Returns the variant as a double if the variant is numerical.
    */
    double Variant::toDouble() const {
        return substate_toNum<double>(d);
    }

    /*!
        Returns the variant as a string if the variant is numerical.
    */
    std::string Variant::toString() const {
        if (d.type != String) {
            return {};
        }
        return *reinterpret_cast<std::string *>(d.data.shared->ptr);
    }

    /*!
        Returns the variant's internal data pointer, use this function to cast to the user value.
    */
    void *Variant::data() {
        if (d.type < String)
            return &d.data;
        if (!d.is_shared)
            return nullptr;
        return d.data.shared->ptr;
    }

    /*!
        Returns the variant's internal const data pointer.
    */
    const void *Variant::constData() const {
        if (d.type < String)
            return &d.data;
        if (!d.is_shared)
            return nullptr;
        return d.data.shared->ptr;
    }

    /*!
        Convert this variant to type QMetaType::UnknownType and free up any resources
        used.
    */
    void Variant::clear() {
        if (d.is_shared && d.data.shared->ref.fetch_sub(1) == 1) {
            substate_v_destroy(d.type, &d);
        }
        d.type = Invalid;
        d.is_shared = false;
    }

    /*!
        \internal
    */
    void Variant::detach() {
        if (!d.is_shared || d.data.shared->ref.load() == 1)
            return;
        Private dd;
        dd.type = d.type;
        dd.data.shared->ptr = new PrivateShared(getHandler(d.type).construct(constData()));
        if (d.data.shared->ref.fetch_sub(1) == 1) {
            substate_v_destroy(d.type, &d);
        }
        d.data.shared = dd.data.shared;
    }

    /*!
        \internal
    */
    bool Variant::isDetached() const {
        return !d.is_shared || d.data.shared->ref.load() == 1;
    }

    /*!
        \fn Variant fromValue(const T &val)

        Returns a variant containing a copy of value.
    */

    /*!
        \fn void setValue(const T &val)

        Stores a copy of value.
    */

    /*!
        \fn T value() const

        Returns the stored value if the type matches.
    */

    /*!
        \fn int typeId(int hint)

        Returns the id of the type. Registers the type if the type wasn't registered before, the
        hint provided will be used if it is available.
    */

    /*!
        \internal
    */
    int Variant::registerUserTypeImpl(const type_info &info, const Handler &handler, int hint) {
        static std::unordered_map<std::string, int> names;
        static int max = User;

        std::unique_lock<std::shared_mutex> lock(handlerLock);
        if (auto it = names.find(info.name()); it != names.end()) {
            return it->second;
        }

        int id;
        if (hint < User) {
            id = max++;
        } else {
            id = hint;
            max = std::max(id, max);
        }
        handlerManager.insert(std::make_pair(id, handler));
        names.insert(std::make_pair(info.name(), id));

        return id;
    }

    IStream &operator>>(IStream &stream, Variant &var) {
        int type;
        stream >> type;
        if (stream.fail()) {
            return stream;
        }

        switch (type) {
            case Variant::Invalid:
                break;
            case Variant::Boolean: {
                // Use 1 byte
                int8_t tmp;
                stream >> tmp;
                if (stream.good())
                    var = Variant(bool(tmp));
                return stream;
            }
            case Variant::Byte: {
                int8_t tmp;
                stream >> tmp;
                if (stream.good())
                    var = Variant(tmp);
                return stream;
            }
            case Variant::Int16: {
                int16_t tmp;
                stream >> tmp;
                if (stream.good())
                    var = Variant(tmp);
                return stream;
            }
            case Variant::Int32: {
                int32_t tmp;
                stream >> tmp;
                if (stream.good())
                    var = Variant(tmp);
                return stream;
            }
            case Variant::Int64: {
                int64_t tmp;
                stream >> tmp;
                if (stream.good())
                    var = Variant(tmp);
                return stream;
            }
            case Variant::UByte: {
                uint8_t tmp;
                stream >> tmp;
                if (stream.good())
                    var = Variant(tmp);
                return stream;
            }
            case Variant::UInt16: {
                uint16_t tmp;
                stream >> tmp;
                if (stream.good())
                    var = Variant(tmp);
                return stream;
            }
            case Variant::UInt32: {
                uint32_t tmp;
                stream >> tmp;
                if (stream.good())
                    var = Variant(tmp);
                return stream;
            }
            case Variant::UInt64: {
                uint64_t tmp;
                stream >> tmp;
                if (stream.good())
                    var = Variant(tmp);
                return stream;
            }
            case Variant::Single: {
                float tmp;
                stream >> tmp;
                if (stream.good())
                    var = Variant(tmp);
                return stream;
            }
            case Variant::Double: {
                double tmp;
                stream >> tmp;
                if (stream.good())
                    var = Variant(tmp);
                return stream;
            }
            default:
                break;
        }

        void *buf = getHandler(type).read(stream);
        if (!buf) {
            return stream;
        }

        // Construct
        {
            Variant res;
            res.d.type = type;
            res.d.is_shared = true;
            res.d.data.shared = new Variant::PrivateShared(buf);
            var = res;
        }
        return stream;
    }

    OStream &operator<<(OStream &stream, const Variant &var) {
        const auto &d = var.d;

        stream << d.type;

        if (stream.fail())
            return stream;

        switch (d.type) {
            case Variant::Invalid:
                return stream;
            case Variant::Boolean: {
                // Use 1 byte
                stream << d.data.b;
                return stream;
            }
            case Variant::Byte: {
                stream << d.data.c;
                return stream;
            }
            case Variant::Int16: {
                stream << d.data.s;
                return stream;
            }
            case Variant::Int32: {
                stream << d.data.i;
                return stream;
            }
            case Variant::Int64: {
                stream << d.data.l;
                return stream;
            }
            case Variant::UByte: {
                stream << d.data.uc;
                return stream;
            }
            case Variant::UInt16: {
                stream << d.data.us;
                return stream;
            }
            case Variant::UInt32: {
                stream << d.data.u;
                return stream;
            }
            case Variant::UInt64: {
                stream << d.data.ul;
                return stream;
            }
            case Variant::Single: {
                stream << d.data.f;
                return stream;
            }
            case Variant::Double: {
                stream << d.data.d;
                return stream;
            }
            default:
                if (!d.is_shared) {
                    stream.setState(std::ios::badbit);
                    return stream;
                }
                break;
        }
        getHandler(d.type).write(d.data.shared->ptr, stream);
        return stream;
    }

}