#ifndef SUBSTATE_BINARYSTREAM_H
#define SUBSTATE_BINARYSTREAM_H

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

    class SUBSTATE_EXPORT IBinaryStream {
    public:
        inline explicit IBinaryStream(std::istream &in);
        ~IBinaryStream() = default;

        IBinaryStream(const IBinaryStream &) = delete;
        IBinaryStream &operator=(const IBinaryStream &) = delete;

    public:
        inline std::istream &in() const;
        inline std::ios::iostate state() const;
        inline void setState(std::ios::iostate state);

        inline bool good() const;
        inline bool fail() const;

        int readRawData(char *data, int len);
        int skipRawData(int len);
        int align(int size);

        IBinaryStream &operator>>(bool &b);
        IBinaryStream &operator>>(int8_t &c);
        IBinaryStream &operator>>(uint8_t &uc);
        IBinaryStream &operator>>(int16_t &s);
        IBinaryStream &operator>>(uint16_t &us);
        IBinaryStream &operator>>(int32_t &i);
        IBinaryStream &operator>>(uint32_t &u);
        IBinaryStream &operator>>(int64_t &l);
        IBinaryStream &operator>>(uint64_t &ul);
        IBinaryStream &operator>>(float &f);
        IBinaryStream &operator>>(double &d);
        IBinaryStream &operator>>(std::string &s);

    protected:
        std::istream &_in;
    };

    inline IBinaryStream::IBinaryStream(std::istream &in) : _in(in) {
    }

    inline std::istream &IBinaryStream::in() const {
        return _in;
    }

    inline std::ios::iostate IBinaryStream::state() const {
        return _in.rdstate();
    }

    inline void IBinaryStream::setState(std::ios::iostate state) {
        _in.setstate(state);
    }

    inline bool IBinaryStream::good() const {
        return _in.good();
    }

    inline bool IBinaryStream::fail() const {
        return _in.fail();
    }

    class SUBSTATE_EXPORT OBinaryStream {
    public:
        inline explicit OBinaryStream(std::ostream &out);
        ~OBinaryStream() = default;

        OBinaryStream(const OBinaryStream &) = delete;
        OBinaryStream &operator=(const OBinaryStream &) = delete;

    public:
        inline std::ostream &out() const;
        inline std::ios::iostate state() const;
        inline void setState(std::ios::iostate state);

        inline bool good() const;
        inline bool fail() const;

        int writeRawData(const char *data, int len);
        int skipRawData(int len);
        int align(int size);

        OBinaryStream &operator<<(int8_t c);
        OBinaryStream &operator<<(uint8_t uc);
        OBinaryStream &operator<<(int16_t s);
        OBinaryStream &operator<<(uint16_t us);
        OBinaryStream &operator<<(int32_t i);
        OBinaryStream &operator<<(uint32_t u);
        OBinaryStream &operator<<(int64_t l);
        OBinaryStream &operator<<(uint64_t ul);
        OBinaryStream &operator<<(float f);
        OBinaryStream &operator<<(double d);
        OBinaryStream &operator<<(const std::string_view &s);
        OBinaryStream &operator<<(const std::string &s);
        OBinaryStream &operator<<(const char *s);

    protected:
        std::ostream &_out;
    };

    inline OBinaryStream::OBinaryStream(std::ostream &out) : _out(out) {
    }

    inline std::ostream &OBinaryStream::out() const {
        return _out;
    }

    inline std::ios::iostate OBinaryStream::state() const {
        return _out.rdstate();
    }

    inline void OBinaryStream::setState(std::ios::iostate state) {
        _out.setstate(state);
    }

    inline bool OBinaryStream::good() const {
        return _out.good();
    }

    inline bool OBinaryStream::fail() const {
        return _out.fail();
    }

    template <class Container>
    IBinaryStream &readArrayBasedContainer(IBinaryStream &s, Container &c) {
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
    IBinaryStream &readAssociativeContainer(IBinaryStream &s, Container &c) {
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
    OBinaryStream &writeSequentialContainer(OBinaryStream &s, const Container &c) {
        s << int(c.size());
        for (const auto &t : c)
            s << t;
        return s;
    }

    template <class Container>
    OBinaryStream &writeAssociativeContainer(OBinaryStream &s, const Container &c) {
        s << int(c.size());
        for (const auto &p : c) {
            s << p.first << p.second;
            if (s.fail())
                break;
        }
        return s;
    }

    template <class T>
    inline IBinaryStream &operator>>(IBinaryStream &s, std::list<T> &l) {
        return readArrayBasedContainer(s, l);
    }

    template <typename T>
    inline OBinaryStream &operator<<(OBinaryStream &s, const std::list<T> &l) {
        return writeSequentialContainer(s, l);
    }

    template <typename T>
    inline IBinaryStream &operator>>(IBinaryStream &s, std::vector<T> &v) {
        return readArrayBasedContainer(s, v);
    }

    template <typename T>
    inline OBinaryStream &operator<<(OBinaryStream &s, const std::vector<T> &v) {
        return writeSequentialContainer(s, v);
    }

    template <typename T>
    inline IBinaryStream &operator>>(IBinaryStream &s, std::set<T> &set) {
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
    inline OBinaryStream &operator<<(OBinaryStream &s, const std::set<T> &set) {
        return writeSequentialContainer(s, set);
    }

    template <class Key, class T>
    inline IBinaryStream &operator>>(IBinaryStream &s, std::unordered_map<Key, T> &hash) {
        return readAssociativeContainer(s, hash);
    }

    template <class Key, class T>
    inline OBinaryStream &operator<<(OBinaryStream &s, const std::unordered_map<Key, T> &hash) {
        return writeAssociativeContainer(s, hash);
    }

    template <class Key, class T>
    inline IBinaryStream &operator>>(IBinaryStream &s, std::map<Key, T> &map) {
        return readAssociativeContainer(s, map);
    }

    template <class Key, class T>
    inline OBinaryStream &operator<<(OBinaryStream &s, const std::map<Key, T> &map) {
        return writeAssociativeContainer(s, map);
    }

    template <class T1, class T2>
    inline IBinaryStream &operator>>(IBinaryStream &s, std::pair<T1, T2> &p) {
        s >> p.first >> p.second;
        return s;
    }

    template <class T1, class T2>
    inline OBinaryStream &operator<<(OBinaryStream &s, const std::pair<T1, T2> &p) {
        s << p.first << p.second;
        return s;
    }

}

#endif // SUBSTATE_BINARYSTREAM_H