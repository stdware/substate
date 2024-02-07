#include "bytearray.h"

#include <cstring>

namespace Substate {

    ByteArray::ByteArray(const char *data, int size) {
        if (data == nullptr || size == 0) {
            m_data = nullptr;
            m_size = 0;
            return;
        }

        if (size < 0) {
            size = int(strlen(data));
        }

        auto buf = new char[size];
        memcpy(buf, data, size);
        m_data.reset(buf);
    }

    ByteArray::~ByteArray() = default;

    bool ByteArray::operator==(const ByteArray &other) const {
        if (this == &other) {
            return true;
        }
        return m_size == other.m_size && memcmp(m_data.get(), other.m_data.get(), m_size) == 0;
    }

    bool ByteArray::operator!=(const ByteArray &other) const {
        return !(*this == other);
    }

    IStream &operator>>(IStream &stream, ByteArray &a) {
        int size;
        stream >> size;
        if (stream.fail())
            return stream;

        if (size == 0) {
            a = {};
            return stream;
        }

        auto buf = new char[size];
        stream.readRawData(buf, size);
        if (stream.fail()) {
            delete[] buf;
            return stream;
        }

        a.m_data.reset(buf);
        a.m_size = size;
        return stream;
    }

    OStream &operator<<(OStream &stream, const ByteArray &a) {
        stream << a.m_size;
        if (a.m_size > 0) {
            stream.writeRawData(a.m_data.get(), a.m_size);
        }
        return stream;
    }

}