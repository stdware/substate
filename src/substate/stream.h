#ifndef STREAM_H
#define STREAM_H

#include <iostream>
#include <sstream>

#include <substate/substate_global.h>

namespace Substate {

    class SUBSTATE_EXPORT IStream {
    public:
        explicit IStream(std::istream *in);
        explicit IStream(std::string *s);
        ~IStream();

    public:
        inline std::istream *device() const;
        inline std::ios::iostate state() const;
        inline void setState(std::ios::iostate state);

        inline bool good() const;
        inline bool fail() const;

        int readRawData(char *data, int len);
        int skipRawData(int len);

        IStream &operator>>(bool &b);
        IStream &operator>>(int8_t &c);
        IStream &operator>>(uint8_t &uc);
        IStream &operator>>(int16_t &s);
        IStream &operator>>(uint16_t &us);
        IStream &operator>>(int32_t &i);
        IStream &operator>>(uint32_t &u);
        IStream &operator>>(int64_t &l);
        IStream &operator>>(uint64_t &ul);
        IStream &operator>>(float &f);
        IStream &operator>>(double &d);
        IStream &operator>>(std::string &s);

    private:
        std::istream *in;
        bool own_stream;
        std::streambuf *buf;

        SUBSTATE_DISABLE_COPY(IStream)
    };

    inline std::istream *IStream::device() const {
        return in;
    }

    inline std::ios::iostate IStream::state() const {
        return in->rdstate();
    }

    inline void IStream::setState(std::ios::iostate state) {
        in->setstate(state);
    }

    bool IStream::good() const {
        return in->good();
    }

    bool IStream::fail() const {
        return in->fail();
    }

    class SUBSTATE_EXPORT OStream {
    public:
        explicit OStream(std::ostream *out);
        explicit OStream(const std::string *s);
        ~OStream();

    public:
        inline std::ostream *device() const;
        inline std::ios::iostate state() const;
        inline void setState(std::ios::iostate state);

        inline bool good() const;
        inline bool fail() const;

        int writeRawData(const char *data, int len);
        int skipRawData(int len);

        OStream &operator<<(int8_t c);
        OStream &operator<<(uint8_t uc);
        OStream &operator<<(int16_t s);
        OStream &operator<<(uint16_t us);
        OStream &operator<<(int32_t i);
        OStream &operator<<(uint32_t u);
        OStream &operator<<(int64_t l);
        OStream &operator<<(uint64_t ul);
        OStream &operator<<(float f);
        OStream &operator<<(double d);
        OStream &operator<<(const std::string &s);

    private:
        std::ostream *out;
        bool own_stream;
        std::streambuf *buf;

        SUBSTATE_DISABLE_COPY(OStream)
    };

    inline std::ostream *OStream::device() const {
        return out;
    }

    inline std::ios::iostate OStream::state() const {
        return out->rdstate();
    }

    inline void OStream::setState(std::ios::iostate state) {
        out->setstate(state);
    }

    bool OStream::good() const {
        return out->good();
    }

    bool OStream::fail() const {
        return out->fail();
    }

}

#endif // STREAM_H
