#ifndef SUBSTATE_STREAM_H
#define SUBSTATE_STREAM_H

#include <map>
#include <set>
#include <list>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <string>
#include <cstdint>

#include <substate/substate_global.h>

namespace ss {

    class SUBSTATE_EXPORT IStream {
    public:
        inline explicit IStream(std::istream *in);
        ~IStream() = default;

        IStream(const IStream &) = delete;
        IStream &operator=(const IStream &) = delete;

    public:
        inline std::istream *device() const;
        inline std::ios::iostate state() const;
        inline void setState(std::ios::iostate state);

        inline bool good() const;
        inline bool fail() const;

        int readRawData(char *data, int len);
        int skipRawData(int len);
        int align(int size);

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

    protected:
        std::istream *in;
    };

    inline IStream::IStream(std::istream *in) : in(in) {
    }

    inline std::istream *IStream::device() const {
        return in;
    }

    inline std::ios::iostate IStream::state() const {
        return in->rdstate();
    }

    inline void IStream::setState(std::ios::iostate state) {
        in->setstate(state);
    }

    inline bool IStream::good() const {
        return in->good();
    }

    inline bool IStream::fail() const {
        return in->fail();
    }

    class SUBSTATE_EXPORT OStream {
    public:
        inline explicit OStream(std::ostream *out);
        ~OStream() = default;

        OStream(const OStream &) = delete;
        OStream &operator=(const OStream &) = delete;

    public:
        inline std::ostream *device() const;
        inline std::ios::iostate state() const;
        inline void setState(std::ios::iostate state);

        inline bool good() const;
        inline bool fail() const;

        int writeRawData(const char *data, int len);
        int skipRawData(int len);
        int align(int size);

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
        OStream &operator<<(const std::string_view &s);
        OStream &operator<<(const std::string &s);
        OStream &operator<<(const char *s);

    protected:
        std::ostream *out;
    };

    inline OStream::OStream(std::ostream *out) : out(out) {
    }

    inline std::ostream *OStream::device() const {
        return out;
    }

    inline std::ios::iostate OStream::state() const {
        return out->rdstate();
    }

    inline void OStream::setState(std::ios::iostate state) {
        out->setstate(state);
    }

    inline bool OStream::good() const {
        return out->good();
    }

    inline bool OStream::fail() const {
        return out->fail();
    }

    template <class Container>
    IStream &readArrayBasedContainer(IStream &s, Container &c) {
        c.clear();
        int n;
        s >> n;
        c.reserve(n);
        for (int i = 0; i < n; ++i) {
            typename Container::value_type t;
            s >> t;
            if (s.fail()) {
                c.clear();
                break;
            }
            c.push_back(t);
        }
        return s;
    }

    template <class Container>
    IStream &readAssociativeContainer(IStream &s, Container &c) {
        c.clear();
        int n;
        s >> n;
        for (int i = 0; i < n; ++i) {
            typename Container::key_type k;
            typename Container::mapped_type t;
            s >> k >> t;
            if (s.fail()) {
                c.clear();
                break;
            }
            c.insert(std::make_pair(k, t));
        }
        return s;
    }

    template <class Container>
    OStream &writeSequentialContainer(OStream &s, const Container &c) {
        s << int(c.size());
        for (const auto &t : c)
            s << t;
        return s;
    }

    template <class Container>
    OStream &writeAssociativeContainer(OStream &s, const Container &c) {
        s << int(c.size());
        for (const auto &p : c) {
            s << p.first << p.second;
            if (s.fail())
                break;
        }
        return s;
    }

    template <class T>
    inline IStream &operator>>(IStream &s, std::list<T> &l) {
        return readArrayBasedContainer(s, l);
    }

    template <typename T>
    inline OStream &operator<<(OStream &s, const std::list<T> &l) {
        return writeSequentialContainer(s, l);
    }

    template <typename T>
    inline IStream &operator>>(IStream &s, std::vector<T> &v) {
        return readArrayBasedContainer(s, v);
    }

    template <typename T>
    inline OStream &operator<<(OStream &s, const std::vector<T> &v) {
        return writeSequentialContainer(s, v);
    }

    template <typename T>
    inline IStream &operator>>(IStream &s, std::set<T> &set) {
        set.clear();
        int n;
        s >> n;
        for (int i = 0; i < n; ++i) {
            T t;
            s >> t;
            if (s.fail()) {
                set.clear();
                break;
            }
            set.insert(t);
        }
        return s;
    }

    template <typename T>
    inline OStream &operator<<(OStream &s, const std::set<T> &set) {
        return writeSequentialContainer(s, set);
    }

    template <class Key, class T>
    inline IStream &operator>>(IStream &s, std::unordered_map<Key, T> &hash) {
        return readAssociativeContainer(s, hash);
    }

    template <class Key, class T>
    inline OStream &operator<<(OStream &s, const std::unordered_map<Key, T> &hash) {
        return writeAssociativeContainer(s, hash);
    }

    template <class Key, class T>
    inline IStream &operator>>(IStream &s, std::map<Key, T> &map) {
        return readAssociativeContainer(s, map);
    }

    template <class Key, class T>
    inline OStream &operator<<(OStream &s, const std::map<Key, T> &map) {
        return writeAssociativeContainer(s, map);
    }

    template <class T1, class T2>
    inline IStream &operator>>(IStream &s, std::pair<T1, T2> &p) {
        s >> p.first >> p.second;
        return s;
    }

    template <class T1, class T2>
    inline OStream &operator<<(OStream &s, const std::pair<T1, T2> &p) {
        s << p.first << p.second;
        return s;
    }

}

#endif // SUBSTATE_STREAM_H