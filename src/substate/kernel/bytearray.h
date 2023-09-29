#ifndef BYTEARRAY_H
#define BYTEARRAY_H

#include <memory>

#include <substate/substate_global.h>

namespace Substate {

    class SUBSTATE_EXPORT ByteArray {
    public:
        ByteArray();
        ByteArray(const char *data, int size);
        ~ByteArray();

        inline const char *data() const;
        inline int size() const;

    protected:
        std::shared_ptr<char> m_data;
        int m_size;
    };

    inline const char *ByteArray::data() const {
        return m_data.get();
    }

    inline int ByteArray::size() const {
        return m_size;
    }

}

#endif // BYTEARRAY_H
