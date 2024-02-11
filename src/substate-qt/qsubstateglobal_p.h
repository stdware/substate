#ifndef QSUBSTATEGLOBAL_P_H
#define QSUBSTATEGLOBAL_P_H

#include <QtCore/QVariant>

#include <substate/stream.h>

#include <qsubstate/qsubstateglobal.h>

namespace Substate {

    QSUBSTATE_EXPORT OStream &operator<<(OStream &s, const QVariant &var);

    QSUBSTATE_EXPORT IStream &operator>>(IStream &s, QVariant &var);

}

#endif // QSUBSTATEGLOBAL_P_H
