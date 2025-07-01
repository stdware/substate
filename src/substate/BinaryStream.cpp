#include "BinaryStream.h"

#include <cstdint>

namespace ss {

    static constexpr const int DATA_ALIGN = 4;

    template <class T>
    static bool substate_readNum(std::istream &in, T &i) {
        i = 0;
        if (in.read(reinterpret_cast<char *>(&i), sizeof(T)).fail()) {
            i = 0;
            return false;
        }
        return true;
    }

    template <class T>
    static bool substate_writeNum(std::ostream &out, T i) {
        if (out.write(reinterpret_cast<const char *>(&i), sizeof(T)).fail()) {
            return false;
        }
        return true;
    }

    int IBinaryStream::readRawData(char *data, int len) {
        auto org = _in.tellg();
        _in.read(data, len);
        return int(_in.tellg() - org);
    }

    int IBinaryStream::skipRawData(int len) {
        auto org = _in.tellg();
        _in.ignore(len);
        return int(_in.tellg() - org);
    }

    int IBinaryStream::align(int size) {
        auto rem = int(_in.tellg() % size);
        if (rem == 0)
            return 0;
        return skipRawData(size - rem);
    }

    IBinaryStream &IBinaryStream::operator>>(bool &b) {
        int8_t c;
        (*this) >> c;
        b = _in.good() && c;
        return *this;
    }

    IBinaryStream &IBinaryStream::operator>>(int8_t &c) {
        substate_readNum(_in, c);
        return *this;
    }

    IBinaryStream &IBinaryStream::operator>>(uint8_t &uc) {
        substate_readNum(_in, uc);
        return *this;
    }

    IBinaryStream &IBinaryStream::operator>>(int16_t &s) {
        substate_readNum(_in, s);
        return *this;
    }

    IBinaryStream &IBinaryStream::operator>>(uint16_t &us) {
        substate_readNum(_in, us);
        return *this;
    }

    IBinaryStream &IBinaryStream::operator>>(int32_t &i) {
        substate_readNum(_in, i);
        return *this;
    }

    IBinaryStream &IBinaryStream::operator>>(uint32_t &u) {
        substate_readNum(_in, u);
        return *this;
    }

    IBinaryStream &IBinaryStream::operator>>(int64_t &l) {
        substate_readNum(_in, l);
        return *this;
    }

    IBinaryStream &IBinaryStream::operator>>(uint64_t &ul) {
        substate_readNum(_in, ul);
        return *this;
    }

    IBinaryStream &IBinaryStream::operator>>(float &f) {
        substate_readNum(_in, f);
        return *this;
    }

    IBinaryStream &IBinaryStream::operator>>(double &d) {
        substate_readNum(_in, d);
        return *this;
    }

    IBinaryStream &IBinaryStream::operator>>(std::string &s) {
        int size;

        // Read size
        (*this) >> size;
        if (_in.fail() || size == 0)
            return *this;

        // Read string
        std::string str;
        str.resize(size);
        _in.read(&str[0], size);

        // align data size to DATA_ALIGN
        if (int align = size % DATA_ALIGN; align > 0) {
            skipRawData(DATA_ALIGN - align);
        }
        if (_in.good()) {
            s = std::move(str);
        }
        return *this;
    }

    int OBinaryStream::writeRawData(const char *data, int len) {
        auto org = _out.tellp();
        _out.write(data, len);
        return int(_out.tellp() - org);
    }

    int OBinaryStream::skipRawData(int len) {
        if (len <= 0) {
            return 0;
        }

        static const constexpr std::size_t blockSize = 8;
        char buffer[blockSize];

        std::size_t fullBlocks = len / blockSize;
        std::size_t lastBlockSize = len % blockSize;

        auto org = _out.tellp();

        for (std::size_t i = 0; i < fullBlocks; ++i) {
            _out.write(buffer, blockSize);
        }
        if (lastBlockSize > 0) {
            _out.write(buffer, lastBlockSize);
        }
        return int(_out.tellp() - org);
    }

    int OBinaryStream::align(int size) {
        auto rem = int(_out.tellp() % size);
        if (rem == 0)
            return 0;
        return skipRawData(size - rem);
    }

    OBinaryStream &OBinaryStream::operator<<(int8_t c) {
        substate_writeNum(_out, c);
        return *this;
    }

    OBinaryStream &OBinaryStream::operator<<(uint8_t uc) {
        substate_writeNum(_out, uc);
        return *this;
    }

    OBinaryStream &OBinaryStream::operator<<(int16_t s) {
        substate_writeNum(_out, s);
        return *this;
    }

    OBinaryStream &OBinaryStream::operator<<(uint16_t us) {
        substate_writeNum(_out, us);
        return *this;
    }

    OBinaryStream &OBinaryStream::operator<<(int32_t i) {
        substate_writeNum(_out, i);
        return *this;
    }

    OBinaryStream &OBinaryStream::operator<<(uint32_t u) {
        substate_writeNum(_out, u);
        return *this;
    }

    OBinaryStream &OBinaryStream::operator<<(int64_t l) {
        substate_writeNum(_out, l);
        return *this;
    }

    OBinaryStream &OBinaryStream::operator<<(uint64_t ul) {
        substate_writeNum(_out, ul);
        return *this;
    }

    OBinaryStream &OBinaryStream::operator<<(float f) {
        substate_writeNum(_out, f);
        return *this;
    }

    OBinaryStream &OBinaryStream::operator<<(double d) {
        substate_writeNum(_out, d);
        return *this;
    }

    OBinaryStream &OBinaryStream::operator<<(const std::string_view &s) {
        // Write size
        (*this) << int(s.size());

        // Write string
        _out.write(s.data(), std::streamsize(s.size()));

        // align data size to DATA_ALIGN
        if (int align = s.size() % DATA_ALIGN; align > 0) {
            skipRawData(DATA_ALIGN - align);
        }
        return *this;
    }

    OBinaryStream &OBinaryStream::operator<<(const std::string &s) {
        return (*this) << std::string_view(s);
    }

    OBinaryStream &OBinaryStream::operator<<(const char *s) {
        return (*this) << std::string_view(s);
    }

}