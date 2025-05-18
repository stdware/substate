#include "bytearray.h"

#include <cstring>

#include "substateglobal_p.h"

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
        m_data.reset(buf, std::default_delete<char[]>());
        m_size = size;
    }

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

        // align data size to DATA_ALIGN
        if (int align = size % DATA_ALIGN; align > 0) {
            stream.skipRawData(DATA_ALIGN - align);
        }

        if (stream.fail()) {
            delete[] buf;
            return stream;
        }

        a.m_data.reset(buf, std::default_delete<char[]>());
        a.m_size = size;
        return stream;
    }

    OStream &operator<<(OStream &stream, const ByteArray &a) {
        stream << a.m_size;
        if (a.m_size > 0) {
            stream.writeRawData(a.m_data.get(), a.m_size);
        }

        // align data size to DATA_ALIGN
        if (int align = a.m_size % DATA_ALIGN; align > 0) {
            stream.skipRawData(DATA_ALIGN - align);
        }
        return stream;
    }

}