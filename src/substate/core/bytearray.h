#ifndef BYTEARRAY_H
#define BYTEARRAY_H

#include <memory>

#include <substate/stream.h>

namespace Substate {

    class SUBSTATE_EXPORT ByteArray {
    public:
        inline ByteArray();
        ByteArray(const char *data, int size = -1);

        inline const char *data() const;
        inline int size() const;

        bool operator==(const ByteArray &other) const;
        bool operator!=(const ByteArray &other) const;

    protected:
        std::shared_ptr<char> m_data;
        int m_size;

        SUBSTATE_EXPORT friend IStream &operator>>(IStream &stream, ByteArray &a);
        SUBSTATE_EXPORT friend OStream &operator<<(OStream &stream, const ByteArray &a);
    };

    inline ByteArray::ByteArray() : m_data(nullptr), m_size(0) {
    }

    inline const char *ByteArray::data() const {
        return m_data.get();
    }

    inline int ByteArray::size() const {
        return m_size;
    }

}

#endif // BYTEARRAY_H
