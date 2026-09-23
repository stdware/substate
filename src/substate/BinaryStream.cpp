#include "BinaryStream.h"

#include <cstddef>
#include <cstdint>

// Numbers are written in the byte order of the platform, which the persistent format requires to
// be little-endian. MSVC targets only little-endian platforms.
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#  error "substate requires a little-endian platform"
#endif

namespace ss {

    namespace {

        // Strings are padded to a multiple of this size.
        constexpr int stringAlignment = 4;

        template <class T>
        void readNumber(std::istream &in, T &value) {
            value = 0;
            if (in.read(reinterpret_cast<char *>(&value), sizeof(T)).fail()) {
                value = 0;
            }
        }

        template <class T>
        void writeNumber(std::ostream &out, T value) {
            out.write(reinterpret_cast<const char *>(&value), sizeof(T));
        }

    }

    int IBinaryStream::readRawData(char *data, int length) {
        m_in.read(data, length);
        return int(m_in.gcount());
    }

    int IBinaryStream::skipRawData(int length) {
        m_in.ignore(length);
        return int(m_in.gcount());
    }

    int IBinaryStream::align(int size) {
        const auto remainder = int(m_in.tellg() % size);
        if (remainder == 0) {
            return 0;
        }
        return skipRawData(size - remainder);
    }

    IBinaryStream &IBinaryStream::operator>>(bool &value) {
        int8_t byte = 0;
        (*this) >> byte;
        value = !m_in.fail() && byte != 0;
        return *this;
    }

    IBinaryStream &IBinaryStream::operator>>(int8_t &value) {
        readNumber(m_in, value);
        return *this;
    }

    IBinaryStream &IBinaryStream::operator>>(uint8_t &value) {
        readNumber(m_in, value);
        return *this;
    }

    IBinaryStream &IBinaryStream::operator>>(int16_t &value) {
        readNumber(m_in, value);
        return *this;
    }

    IBinaryStream &IBinaryStream::operator>>(uint16_t &value) {
        readNumber(m_in, value);
        return *this;
    }

    IBinaryStream &IBinaryStream::operator>>(int32_t &value) {
        readNumber(m_in, value);
        return *this;
    }

    IBinaryStream &IBinaryStream::operator>>(uint32_t &value) {
        readNumber(m_in, value);
        return *this;
    }

    IBinaryStream &IBinaryStream::operator>>(int64_t &value) {
        readNumber(m_in, value);
        return *this;
    }

    IBinaryStream &IBinaryStream::operator>>(uint64_t &value) {
        readNumber(m_in, value);
        return *this;
    }

    IBinaryStream &IBinaryStream::operator>>(float &value) {
        readNumber(m_in, value);
        return *this;
    }

    IBinaryStream &IBinaryStream::operator>>(double &value) {
        readNumber(m_in, value);
        return *this;
    }

    IBinaryStream &IBinaryStream::operator>>(std::string &value) {
        int32_t size = 0;
        (*this) >> size;
        if (m_in.fail()) {
            return *this;
        }
        if (size < 0) {
            m_in.setstate(std::ios::failbit);
            return *this;
        }
        if (size == 0) {
            value.clear();
            return *this;
        }

        std::string text(std::size_t(size), '\0');
        m_in.read(&text[0], size);
        if (const int remainder = size % stringAlignment; remainder > 0) {
            skipRawData(stringAlignment - remainder);
        }
        if (!m_in.fail()) {
            value = std::move(text);
        }
        return *this;
    }

    int OBinaryStream::writeRawData(const char *data, int length) {
        m_out.write(data, length);
        return m_out.fail() ? 0 : length;
    }

    int OBinaryStream::skipRawData(int length) {
        if (length <= 0) {
            return 0;
        }
        // Zero bytes, so that the output depends only on the written values.
        constexpr int blockSize = 1024;
        const char zeros[blockSize] = {};
        for (int remaining = length; remaining > 0; remaining -= blockSize) {
            m_out.write(zeros, remaining < blockSize ? remaining : blockSize);
        }
        return m_out.fail() ? 0 : length;
    }

    int OBinaryStream::align(int size) {
        const auto remainder = int(m_out.tellp() % size);
        if (remainder == 0) {
            return 0;
        }
        return skipRawData(size - remainder);
    }

    OBinaryStream &OBinaryStream::operator<<(bool value) {
        return (*this) << int8_t(value ? 1 : 0);
    }

    OBinaryStream &OBinaryStream::operator<<(int8_t value) {
        writeNumber(m_out, value);
        return *this;
    }

    OBinaryStream &OBinaryStream::operator<<(uint8_t value) {
        writeNumber(m_out, value);
        return *this;
    }

    OBinaryStream &OBinaryStream::operator<<(int16_t value) {
        writeNumber(m_out, value);
        return *this;
    }

    OBinaryStream &OBinaryStream::operator<<(uint16_t value) {
        writeNumber(m_out, value);
        return *this;
    }

    OBinaryStream &OBinaryStream::operator<<(int32_t value) {
        writeNumber(m_out, value);
        return *this;
    }

    OBinaryStream &OBinaryStream::operator<<(uint32_t value) {
        writeNumber(m_out, value);
        return *this;
    }

    OBinaryStream &OBinaryStream::operator<<(int64_t value) {
        writeNumber(m_out, value);
        return *this;
    }

    OBinaryStream &OBinaryStream::operator<<(uint64_t value) {
        writeNumber(m_out, value);
        return *this;
    }

    OBinaryStream &OBinaryStream::operator<<(float value) {
        writeNumber(m_out, value);
        return *this;
    }

    OBinaryStream &OBinaryStream::operator<<(double value) {
        writeNumber(m_out, value);
        return *this;
    }

    OBinaryStream &OBinaryStream::operator<<(const std::string_view &value) {
        (*this) << int32_t(value.size());
        m_out.write(value.data(), std::streamsize(value.size()));
        if (const int remainder = int(value.size() % stringAlignment); remainder > 0) {
            skipRawData(stringAlignment - remainder);
        }
        return *this;
    }

    OBinaryStream &OBinaryStream::operator<<(const std::string &value) {
        return (*this) << std::string_view(value);
    }

    OBinaryStream &OBinaryStream::operator<<(const char *value) {
        return (*this) << std::string_view(value);
    }

}
