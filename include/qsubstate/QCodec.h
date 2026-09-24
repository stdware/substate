// Copyright (C) 2022-present Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#ifndef QSUBSTATE_QCODEC_H
#define QSUBSTATE_QCODEC_H

#include <QtCore/QDataStream>
#include <QtCore/QString>
#include <QtCore/QVariant>

#include <substate/Codec.h>

#include <qsubstate/qsubstate_global.h>

namespace ss {

    /// A Codec that also covers MappingNode with its default type and the actions of qsubstate,
    /// and provides the encoding of QVariant.
    ///
    /// A StructNode has no registration by default, because its type does not determine its
    /// number of slots. A StructNode type is registered by the caller with a factory.
    class QSUBSTATE_EXPORT QCodec : public Codec {
    public:
        /// The version of QDataStream for the values without a fixed encoding. It is fixed,
        /// because the encoding of a value can differ between versions.
        static constexpr int dataStreamVersion = QDataStream::Qt_5_15;

        QCodec();
        ~QCodec();

        /// Writes \a variant, which must be valid.
        ///
        /// Values of \c bool, the integer types, \c double, QString and QByteArray are written in
        /// a fixed encoding, and an integer keeps its type. Other values are written with their
        /// type name through QDataStream of version dataStreamVersion, and the write fails if the
        /// type has no stream operators.
        static void writeVariant(Encoder &encoder, const QVariant &variant);

        /// Reads a value written by writeVariant(). Returns an invalid QVariant on failure. The
        /// read fails if the type name of a value written through QDataStream is not registered
        /// with QMetaType.
        static QVariant readVariant(Decoder &decoder);

        /// Writes \a text as UTF-16 code units, which preserves unpaired surrogates.
        static void writeString(Encoder &encoder, const QString &text);

        static QString readString(Decoder &decoder);
    };

}

#endif // QSUBSTATE_QCODEC_H
