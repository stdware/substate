#include "stream.h"

#include <cstdint>
#include <sstream>

namespace Substate {

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

    IStream::IStream(std::istream *in) : in(in) {
    }
    IStream::~IStream() {
    }
    int IStream::readRawData(char *data, int len) {
        auto org = in->tellg();
        in->read(data, len);
        return int(in->tellg() - org);
    }
    int IStream::skipRawData(int len) {
        auto org = in->tellg();
        in->ignore(len);
        return int(in->tellg() - org);
    }
    int IStream::align(int size) {
        auto rem = int(in->tellg() % size);
        if (rem == 0)
            return 0;
        return skipRawData(size - rem);
    }
    IStream &IStream::operator>>(bool &b) {
        int8_t c;
        (*this) >> c;
        b = in->good() && c;
        return *this;
    }
    IStream &IStream::operator>>(int8_t &c) {
        substate_readNum(*in, c);
        return *this;
    }
    IStream &IStream::operator>>(uint8_t &uc) {
        substate_readNum(*in, uc);
        return *this;
    }
    IStream &IStream::operator>>(int16_t &s) {
        substate_readNum(*in, s);
        return *this;
    }
    IStream &IStream::operator>>(uint16_t &us) {
        substate_readNum(*in, us);
        return *this;
    }
    IStream &IStream::operator>>(int32_t &i) {
        substate_readNum(*in, i);
        return *this;
    }
    IStream &IStream::operator>>(uint32_t &u) {
        substate_readNum(*in, u);
        return *this;
    }
    IStream &IStream::operator>>(int64_t &l) {
        substate_readNum(*in, l);
        return *this;
    }
    IStream &IStream::operator>>(uint64_t &ul) {
        substate_readNum(*in, ul);
        return *this;
    }
    IStream &IStream::operator>>(float &f) {
        substate_readNum(*in, f);
        return *this;
    }
    IStream &IStream::operator>>(double &d) {
        substate_readNum(*in, d);
        return *this;
    }
    IStream &IStream::operator>>(std::string &s) {
        int size;

        // Read size
        (*this) >> size;
        if (in->fail() || size == 0)
            return *this;

        // Read string
        auto buf = new char[size];
        in->read(buf, size);
        if (in->good()) {
            s = std::string(buf, size);
        }
        delete[] buf;

        return *this;
    }

    OStream::OStream(std::ostream *out) : out(out) {
    }
    OStream::~OStream() {
    }
    int OStream::writeRawData(const char *data, int len) {
        auto org = out->tellp();
        out->write(data, len);
        return int(out->tellp() - org);
    }
    int OStream::skipRawData(int len) {
        // for (int i = 0; i < len; ++i) {
        //     out->put('\0');
        //     if (out->fail())
        //         return i;
        // }
        // return len;

        if (len <= 0) {
            return 0;
        }

        static const constexpr std::size_t blockSize = 8;
        static const constexpr char buffer[blockSize] = {};

        std::size_t fullBlocks = len / blockSize;
        std::size_t lastBlockSize = len % blockSize;

        auto org = out->tellp();

        for (std::size_t i = 0; i < fullBlocks; ++i) {
            out->write(buffer, blockSize);
        }

        if (lastBlockSize > 0) {
            out->write(buffer, std::streamsize(lastBlockSize));
        }
        return int(out->tellp() - org);
    }
    int OStream::align(int size) {
        auto rem = int(out->tellp() % size);
        if (rem == 0)
            return 0;
        return skipRawData(size - rem);
    }
    OStream &OStream::operator<<(int8_t c) {
        substate_writeNum(*out, c);
        return *this;
    }
    OStream &OStream::operator<<(uint8_t uc) {
        substate_writeNum(*out, uc);
        return *this;
    }
    OStream &OStream::operator<<(int16_t s) {
        substate_writeNum(*out, s);
        return *this;
    }
    OStream &OStream::operator<<(uint16_t us) {
        substate_writeNum(*out, us);
        return *this;
    }
    OStream &OStream::operator<<(int32_t i) {
        substate_writeNum(*out, i);
        return *this;
    }
    OStream &OStream::operator<<(uint32_t u) {
        substate_writeNum(*out, u);
        return *this;
    }
    OStream &OStream::operator<<(int64_t l) {
        substate_writeNum(*out, l);
        return *this;
    }
    OStream &OStream::operator<<(uint64_t ul) {
        substate_writeNum(*out, ul);
        return *this;
    }
    OStream &OStream::operator<<(float f) {
        substate_writeNum(*out, f);
        return *this;
    }
    OStream &OStream::operator<<(double d) {
        substate_writeNum(*out, d);
        return *this;
    }
    OStream &OStream::operator<<(const std::string_view &s) {
        // Write size
        (*this) << int(s.size());

        // Write string
        out->write(s.data(), std::streamsize(s.size()));
        return *this;
    }
    OStream &OStream::operator<<(const std::string &s) {
        return (*this) << std::string_view(s);
    }

    OStream &OStream::operator<<(const char *s) {
        return (*this) << std::string_view(s);
    }

}