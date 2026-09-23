// Copyright (C) 2022-2025 Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#ifndef SUBSTATE_BINARYSTREAM_H
#define SUBSTATE_BINARYSTREAM_H

#include <cstdint>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include <substate/substate_global.h>

namespace ss {

    /// Reads numbers and strings written by OBinaryStream from a standard input stream.
    ///
    /// Numbers are read in the byte order of the platform, which the persistent format requires
    /// to be little-endian. A failed read leaves the value 0 or empty and sets the failure state
    /// of the underlying stream.
    class SUBSTATE_EXPORT IBinaryStream {
    public:
        inline explicit IBinaryStream(std::istream &in);
        ~IBinaryStream() = default;

        IBinaryStream(const IBinaryStream &) = delete;
        IBinaryStream &operator=(const IBinaryStream &) = delete;

        inline std::istream &in() const;
        inline std::ios::iostate state() const;
        inline void setState(std::ios::iostate state);

        inline bool good() const;
        inline bool fail() const;

        /// Reads \a length bytes into \a data and returns the number of bytes read.
        int readRawData(char *data, int length);

        /// Skips \a length bytes and returns the number of bytes skipped.
        int skipRawData(int length);

        /// Skips bytes up to the next position that is a multiple of \a size, and returns the
        /// number of bytes skipped.
        int align(int size);

        IBinaryStream &operator>>(bool &value);
        IBinaryStream &operator>>(int8_t &value);
        IBinaryStream &operator>>(uint8_t &value);
        IBinaryStream &operator>>(int16_t &value);
        IBinaryStream &operator>>(uint16_t &value);
        IBinaryStream &operator>>(int32_t &value);
        IBinaryStream &operator>>(uint32_t &value);
        IBinaryStream &operator>>(int64_t &value);
        IBinaryStream &operator>>(uint64_t &value);
        IBinaryStream &operator>>(float &value);
        IBinaryStream &operator>>(double &value);

        /// Reads a string written by OBinaryStream, which is its size, its bytes, and padding to a
        /// multiple of 4 bytes.
        IBinaryStream &operator>>(std::string &value);

    private:
        std::istream &m_in;
    };

    inline IBinaryStream::IBinaryStream(std::istream &in) : m_in(in) {
    }

    inline std::istream &IBinaryStream::in() const {
        return m_in;
    }

    inline std::ios::iostate IBinaryStream::state() const {
        return m_in.rdstate();
    }

    inline void IBinaryStream::setState(std::ios::iostate state) {
        m_in.setstate(state);
    }

    inline bool IBinaryStream::good() const {
        return m_in.good();
    }

    inline bool IBinaryStream::fail() const {
        return m_in.fail();
    }

    /// Writes numbers and strings to a standard output stream, for IBinaryStream.
    ///
    /// Numbers are written in the byte order of the platform, which the persistent format
    /// requires to be little-endian. A failed write sets the failure state of the underlying
    /// stream.
    class SUBSTATE_EXPORT OBinaryStream {
    public:
        inline explicit OBinaryStream(std::ostream &out);
        ~OBinaryStream() = default;

        OBinaryStream(const OBinaryStream &) = delete;
        OBinaryStream &operator=(const OBinaryStream &) = delete;

        inline std::ostream &out() const;
        inline std::ios::iostate state() const;
        inline void setState(std::ios::iostate state);

        inline bool good() const;
        inline bool fail() const;

        /// Writes \a length bytes of \a data and returns the number of bytes written.
        int writeRawData(const char *data, int length);

        /// Writes \a length zero bytes and returns the number of bytes written.
        int skipRawData(int length);

        /// Writes zero bytes up to the next position that is a multiple of \a size, and returns
        /// the number of bytes written.
        int align(int size);

        OBinaryStream &operator<<(bool value);
        OBinaryStream &operator<<(int8_t value);
        OBinaryStream &operator<<(uint8_t value);
        OBinaryStream &operator<<(int16_t value);
        OBinaryStream &operator<<(uint16_t value);
        OBinaryStream &operator<<(int32_t value);
        OBinaryStream &operator<<(uint32_t value);
        OBinaryStream &operator<<(int64_t value);
        OBinaryStream &operator<<(uint64_t value);
        OBinaryStream &operator<<(float value);
        OBinaryStream &operator<<(double value);

        /// Writes the size of \a value, its bytes, and zero bytes up to a multiple of 4 bytes.
        OBinaryStream &operator<<(const std::string_view &value);
        OBinaryStream &operator<<(const std::string &value);
        OBinaryStream &operator<<(const char *value);

    private:
        std::ostream &m_out;
    };

    inline OBinaryStream::OBinaryStream(std::ostream &out) : m_out(out) {
    }

    inline std::ostream &OBinaryStream::out() const {
        return m_out;
    }

    inline std::ios::iostate OBinaryStream::state() const {
        return m_out.rdstate();
    }

    inline void OBinaryStream::setState(std::ios::iostate state) {
        m_out.setstate(state);
    }

    inline bool OBinaryStream::good() const {
        return m_out.good();
    }

    inline bool OBinaryStream::fail() const {
        return m_out.fail();
    }

    /// Reads a container written by writeSequentialContainer() into \a container, which supports
    /// \c push_back(). The container is empty on failure.
    template <class Container>
    inline IBinaryStream &readSequentialContainer(IBinaryStream &stream, Container &container) {
        container.clear();
        int32_t count = 0;
        stream >> count;
        if (count < 0) {
            stream.setState(std::ios::failbit);
        }
        // The count is not reserved in advance, because it has not been validated.
        for (int32_t i = 0; i < count && !stream.fail(); ++i) {
            typename Container::value_type value;
            stream >> value;
            container.push_back(std::move(value));
        }
        if (stream.fail()) {
            container.clear();
        }
        return stream;
    }

    /// Reads a container written by writeAssociativeContainer() into \a container, which
    /// supports \c insert() of a key and value pair. The container is empty on failure.
    template <class Container>
    inline IBinaryStream &readAssociativeContainer(IBinaryStream &stream, Container &container) {
        container.clear();
        int32_t count = 0;
        stream >> count;
        if (count < 0) {
            stream.setState(std::ios::failbit);
        }
        for (int32_t i = 0; i < count && !stream.fail(); ++i) {
            typename Container::key_type key;
            typename Container::mapped_type value;
            stream >> key >> value;
            container.insert(std::make_pair(std::move(key), std::move(value)));
        }
        if (stream.fail()) {
            container.clear();
        }
        return stream;
    }

    /// Writes the number of elements of \a container and the elements in order.
    template <class Container>
    inline OBinaryStream &writeSequentialContainer(OBinaryStream &stream,
                                                   const Container &container) {
        stream << int32_t(container.size());
        for (const auto &value : container) {
            stream << value;
        }
        return stream;
    }

    /// Writes the number of entries of \a container and each key followed by its value.
    template <class Container>
    inline OBinaryStream &writeAssociativeContainer(OBinaryStream &stream,
                                                    const Container &container) {
        stream << int32_t(container.size());
        for (const auto &entry : container) {
            stream << entry.first << entry.second;
        }
        return stream;
    }

    template <class T>
    inline IBinaryStream &operator>>(IBinaryStream &stream, std::list<T> &list) {
        return readSequentialContainer(stream, list);
    }

    template <class T>
    inline OBinaryStream &operator<<(OBinaryStream &stream, const std::list<T> &list) {
        return writeSequentialContainer(stream, list);
    }

    template <class T>
    inline IBinaryStream &operator>>(IBinaryStream &stream, std::vector<T> &vector) {
        return readSequentialContainer(stream, vector);
    }

    template <class T>
    inline OBinaryStream &operator<<(OBinaryStream &stream, const std::vector<T> &vector) {
        return writeSequentialContainer(stream, vector);
    }

    template <class T>
    inline IBinaryStream &operator>>(IBinaryStream &stream, std::set<T> &set) {
        set.clear();
        int32_t count = 0;
        stream >> count;
        if (count < 0) {
            stream.setState(std::ios::failbit);
        }
        for (int32_t i = 0; i < count && !stream.fail(); ++i) {
            T value;
            stream >> value;
            set.insert(std::move(value));
        }
        if (stream.fail()) {
            set.clear();
        }
        return stream;
    }

    template <class T>
    inline OBinaryStream &operator<<(OBinaryStream &stream, const std::set<T> &set) {
        return writeSequentialContainer(stream, set);
    }

    template <class Key, class T>
    inline IBinaryStream &operator>>(IBinaryStream &stream, std::unordered_map<Key, T> &map) {
        return readAssociativeContainer(stream, map);
    }

    template <class Key, class T>
    inline OBinaryStream &operator<<(OBinaryStream &stream, const std::unordered_map<Key, T> &map) {
        return writeAssociativeContainer(stream, map);
    }

    template <class Key, class T>
    inline IBinaryStream &operator>>(IBinaryStream &stream, std::map<Key, T> &map) {
        return readAssociativeContainer(stream, map);
    }

    template <class Key, class T>
    inline OBinaryStream &operator<<(OBinaryStream &stream, const std::map<Key, T> &map) {
        return writeAssociativeContainer(stream, map);
    }

    template <class T1, class T2>
    inline IBinaryStream &operator>>(IBinaryStream &stream, std::pair<T1, T2> &pair) {
        stream >> pair.first >> pair.second;
        return stream;
    }

    template <class T1, class T2>
    inline OBinaryStream &operator<<(OBinaryStream &stream, const std::pair<T1, T2> &pair) {
        stream << pair.first << pair.second;
        return stream;
    }

}

#endif // SUBSTATE_BINARYSTREAM_H
