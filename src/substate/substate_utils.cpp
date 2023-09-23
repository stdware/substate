#include "substate_utils_p.h"

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

    bool readInt8(std::istream &in, int8_t &c) {
        return substate_readNum(in, c);
    }

    bool readUInt8(std::istream &in, uint8_t &uc) {
        return substate_readNum(in, uc);
    }

    bool readInt16(std::istream &in, int16_t &s) {
        return substate_readNum(in, s);
    }

    bool readUInt16(std::istream &in, uint16_t &us) {
        return substate_readNum(in, us);
    }

    bool readInt32(std::istream &in, int32_t &i) {
        return substate_readNum(in, i);
    }

    bool readUInt32(std::istream &in, uint32_t &u) {
        return substate_readNum(in, u);
    }

    bool readInt64(std::istream &in, int64_t &l) {
        return substate_readNum(in, l);
    }

    bool readUInt64(std::istream &in, uint64_t &ul) {
        return substate_readNum(in, ul);
    }

    bool readFloat(std::istream &in, float &f) {
        return substate_readNum(in, f);
    }

    bool readDouble(std::istream &in, double &d) {
        return substate_readNum(in, d);
    }

    bool readString(std::istream &in, std::string &s) {
        int size;
        // Read size
        readInt32(in, size);

        // Read string
        auto buf = new char[size];
        if (in.read(buf, size).fail()) {
            delete[] buf;
            return false;
        }
        s = std::string(buf, size);
        delete[] buf;

        return true;
    }

    template <class T>
    static bool substate_writeNum(std::ostream &out, T i) {
        if (out.write(reinterpret_cast<const char *>(&i), sizeof(T)).fail()) {
            return false;
        }
        return true;
    }

    bool writeInt8(std::ostream &out, int8_t c) {
        return substate_writeNum(out, c);
    }

    bool writeUInt8(std::ostream &out, uint8_t uc) {
        return substate_writeNum(out, uc);
    }

    bool writeInt16(std::ostream &out, int16_t s) {
        return substate_writeNum(out, s);
    }

    bool writeUInt16(std::ostream &out, uint16_t us) {
        return substate_writeNum(out, us);
    }

    bool writeInt32(std::ostream &out, int32_t i) {
        return substate_writeNum(out, i);
    }

    bool writeUInt32(std::ostream &out, uint32_t u) {
        return substate_writeNum(out, u);
    }

    bool writeInt64(std::ostream &out, int64_t l) {
        return substate_writeNum(out, l);
    }

    bool writeUInt64(std::ostream &out, uint64_t ul) {
        return substate_writeNum(out, ul);
    }

    bool writeFloat(std::ostream &out, float f) {
        return substate_writeNum(out, f);
    }

    bool writeDouble(std::ostream &out, double d) {
        return substate_writeNum(out, d);
    }

    bool writeString(std::ostream &out, const std::string &s) {
        // Write size
        writeInt32(out, int(s.size()));

        // Write string
        return out.write(s.data(), int(s.size())).good();
    }

}