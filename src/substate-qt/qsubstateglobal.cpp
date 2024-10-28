#include "qsubstateglobal_p.h"

#include <QtCore/QDataStream>
#include <QtCore/QIODevice>

#include <substate/variant.h>

namespace Substate {

    static int q_variant_type = Variant::typeId<QVariant>();

    OStream &operator<<(OStream &s, const QVariant &var) {
        QByteArray buf;
        {
            QDataStream out(&buf, QIODevice::WriteOnly);
            out << var;
        }

        s << buf.size();
        s.writeRawData(buf.data(), buf.size());
        return s;
    }

    IStream &operator>>(IStream &s, QVariant &var) {
        int size;
        s >> size;

        QByteArray buf;
        buf.reserve(size);
        s.readRawData(buf.data(), buf.size());
        if (!s.good())
            return s;

        {
            QDataStream in(buf);
            in >> var;
            if (in.status() != QDataStream::Ok) {
                s.setState(std::ios::badbit);
            }
        }

        return s;
    }

}