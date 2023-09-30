#include "stream.h"

#include <fstream>
#include <sstream>
#include <istream>

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

    IStream::IStream(std::istream *in) : in(in), own_stream(false), buf(nullptr) {
    }
    IStream::IStream(std::string *s) {
        buf = new std::stringbuf(*s);
        in = new std::istream(buf);
        own_stream = true;
    }
    IStream::~IStream() {
        if (own_stream) {
            delete in;
        }
        delete buf;
    }
    int IStream::readRawData(char *data, int len) {
        in->read(data, len);
        return 0;
    }
    int IStream::skipRawData(int len) {
        auto org = in->tellg();
        in->ignore(len);
        return int(in->tellg() - org);
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

    OStream::OStream(std::ostream *out) : out(out), own_stream(false), buf(nullptr) {
    }
    OStream::OStream(const std::string *s) {
        buf = new std::stringbuf(*s);
        out = new std::ostream(buf);
        own_stream = true;
    }
    OStream::~OStream() {
        if (own_stream) {
            delete out;
        }
        delete buf;
    }
    int OStream::writeRawData(const char *data, int len) {
        out->write(data, len);
        return 0;
    }
    int OStream::skipRawData(int len) {
        for (int i = 0; i < len; ++i) {
            out->put('\0');
            if (out->fail())
                return i;
        }
        return len;
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
    OStream &OStream::operator<<(const std::string &s) {
        // Write size
        (*this) << int(s.size());

        // Write string
        out->write(s.data(), std::streamsize(s.size()));
        return *this;
    }

}