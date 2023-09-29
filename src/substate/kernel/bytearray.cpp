#include "bytearray.h"

namespace Substate {

    ByteArray::ByteArray() : m_data(nullptr), m_size(0) {
    }

    ByteArray::ByteArray(const char *data, int size) {
        if (data == nullptr || size == 0) {
            m_data = nullptr;
            m_size = 0;
            return;
        }

        auto buf = new char[size];
        memcpy(buf, data, size);
        m_data.reset(buf);
    }

    ByteArray::~ByteArray() {
    }

}