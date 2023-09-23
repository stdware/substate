#ifndef SUBSTATE_UTILS_P_H
#define SUBSTATE_UTILS_P_H

#include <iostream>

#include <substate/substate_global.h>

namespace Substate {

    SUBSTATE_EXPORT bool readInt8(std::istream &in, int8_t &c);
    SUBSTATE_EXPORT bool readUInt8(std::istream &in, uint8_t &uc);
    SUBSTATE_EXPORT bool readInt16(std::istream &in, int16_t &s);
    SUBSTATE_EXPORT bool readUInt16(std::istream &in, uint16_t &us);
    SUBSTATE_EXPORT bool readInt32(std::istream &in, int32_t &i);
    SUBSTATE_EXPORT bool readUInt32(std::istream &in, uint32_t &u);
    SUBSTATE_EXPORT bool readInt64(std::istream &in, int64_t &l);
    SUBSTATE_EXPORT bool readUInt64(std::istream &in, uint64_t &ul);
    SUBSTATE_EXPORT bool readFloat(std::istream &in, float &f);
    SUBSTATE_EXPORT bool readDouble(std::istream &in, double &d);
    SUBSTATE_EXPORT bool readString(std::istream &in, std::string &s);

    SUBSTATE_EXPORT bool writeInt8(std::ostream &out, int8_t c);
    SUBSTATE_EXPORT bool writeUInt8(std::ostream &out, uint8_t uc);
    SUBSTATE_EXPORT bool writeInt16(std::ostream &out, int16_t s);
    SUBSTATE_EXPORT bool writeUInt16(std::ostream &out, uint16_t us);
    SUBSTATE_EXPORT bool writeInt32(std::ostream &out, int32_t i);
    SUBSTATE_EXPORT bool writeUInt32(std::ostream &out, uint32_t u);
    SUBSTATE_EXPORT bool writeInt64(std::ostream &out, int64_t l);
    SUBSTATE_EXPORT bool writeUInt64(std::ostream &out, uint64_t ul);
    SUBSTATE_EXPORT bool writeFloat(std::ostream &out, float f);
    SUBSTATE_EXPORT bool writeDouble(std::ostream &out, double d);
    SUBSTATE_EXPORT bool writeString(std::ostream &out, const std::string &s);

}

#endif // SUBSTATE_UTILS_P_H
