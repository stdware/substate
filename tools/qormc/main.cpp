#include <set>

#include <QtCore/QCommandLineOption>
#include <QtCore/QCommandLineParser>
#include <QtCore/QCoreApplication>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QFile>
#include <QtCore/QRegularExpression>
#include <QtCore/QDir>

static void error(const char *msg = "Invalid argument") {
    if (msg)
        fprintf(stderr, "qormc: %s\n", msg);
}

namespace {

    class Property {
    public:
        enum ContainerType {
            NoContainer = 0,
            Vector = 1,
            Sheet = 2,
            Bytes = 3,
        };
        Q_DECLARE_FLAGS(ContainerTypes, ContainerType)

        QString type;
        QString defaultValue;
        ContainerType containerType = NoContainer;

        QString translatedTypeName() const {
            switch (containerType) {
                case Vector:
                    return type + "Vector";
                case Sheet:
                    return type + "Sheet";
                case Bytes:
                    return "Substate::" + type + "Array";
                default:
                    break;
            }
            return {};
        }

        static void parseType(const QString &type, Property &res, QString *errorMessage) {
            QRegularExpression re(R"(Substate::(.+?)<(.+)>)");
            QRegularExpressionMatch match = re.match(type);
            if (match.hasMatch()) {
                QString containerType = match.captured(1);
                if (containerType == "VectorNode") {
                    res.containerType = Vector;
                } else if (containerType == "SheetNode") {
                    res.containerType = Sheet;
                } else if (containerType == "BytesNode") {
                    res.containerType = Bytes;
                } else {
                    *errorMessage = QStringLiteral("unknown container \"%1\"").arg(containerType);
                    return;
                }
                res.type = match.captured(2);
            } else {
                res.type = type;
            }
        }

        static Property parse(const QJsonValue &val, QString *errorMessage) {
            Property res;
            if (val.isString()) {
                parseType(val.toString(), res, errorMessage);
                if (!errorMessage->isEmpty()) {
                    return {};
                }
                return res;
            } else if (!val.isObject()) {
                *errorMessage = QStringLiteral("not an object");
                return res;
            }

            QJsonObject obj = val.toObject();
            QJsonValue value;
            value = obj.value(QStringLiteral("type"));
            if (value.isString()) {
                parseType(value.toString(), res, errorMessage);
                if (!errorMessage->isEmpty()) {
                    return {};
                }
            } else {
                *errorMessage = QStringLiteral("missing \"type\" attribute");
                return {};
            }

            value = obj.value(QStringLiteral("default"));
            if (value.isString()) {
                res.defaultValue = value.toString();
            } else if (value.isDouble()) {
                res.defaultValue = QString::number(value.toDouble());
            }

            return res;
        }
    };

    class Enumeration {
    public:
        QString name;
        QMap<int, QString> values;

        static Enumeration parse(const QJsonValue &val, QString *errorMessage) {
            if (!val.isObject()) {
                *errorMessage = QStringLiteral("not an object");
                return {};
            }

            Enumeration res;
            QJsonObject obj = val.toObject();
            QJsonValue value;

            value = obj.value(QStringLiteral("name"));
            if (value.isString()) {
                res.name = value.toString();
            } else {
                *errorMessage = QStringLiteral("missing \"name\" attribute");
                return {};
            }

            value = obj.value(QStringLiteral("values"));
            if (!value.isArray()) {
                *errorMessage = QStringLiteral("missing \"values\" attribute");
                return {};
            }

            int current = 0;
            const auto &arr = value.toArray();
            for (const auto &item : arr) {
                if (item.isString()) {
                    res.values.insert(current, item.toString());
                } else if (item.isObject()) {
                    QString name;

                    auto itemObj = item.toObject();
                    value = itemObj.value(QStringLiteral("name"));
                    if (value.isString()) {
                        name = value.toString();
                    } else {
                        *errorMessage = QStringLiteral("missing \"name\" attribute");
                        return {};
                    }

                    value = itemObj.value(QStringLiteral("index"));
                    if (value.isDouble()) {
                        int index = value.toInt();
                        if (index < current && current != 0) {
                            *errorMessage = QStringLiteral("invalid index of token \"%1\"")
                                                .arg(name, QString::number(index));
                            return {};
                        }
                        current = index;
                    }

                    res.values.insert(current, name);
                }
                current++;
            }

            return res;
        }
    };

    class Constructor {
    public:
        struct Argument {
            QString type;
            QString name;
            QString member;
            QString defaultValue;
        };

        QStringList super;
        QList<Argument> args;

        static Constructor parse(const QJsonValue &val, QString *errorMessage) {
            if (!val.isObject()) {
                *errorMessage = QStringLiteral("not an object");
                return {};
            }

            Constructor res;
            QJsonObject obj = val.toObject();
            QJsonValue value;

            value = obj.value(QStringLiteral("super"));
            if (value.isString()) {
                res.super.append(value.toString());
            } else if (value.isArray()) {
                for (const auto &item : value.toArray()) {
                    if (item.isString()) {
                        res.super.append(item.toString());
                    }
                }
            }

            value = obj.value(QStringLiteral("args"));
            if (value.isArray()) {
                auto argsArr = value.toArray();
                for (auto it = argsArr.begin(); it != argsArr.end(); ++it) {
                    if (!it->isObject()) {
                        *errorMessage = QStringLiteral("arg at index %1 is not an object")
                                            .arg(QString::number(it - argsArr.begin()));
                        return {};
                    }

                    Argument arg;

                    auto argObj = it->toObject();
                    value = argObj.value(QStringLiteral("type"));
                    if (value.isString()) {
                        arg.type = value.toString();
                    } else {
                        *errorMessage = QStringLiteral("arg at index %1 is missing the type")
                                            .arg(QString::number(it - argsArr.begin()));
                        return {};
                    }

                    value = argObj.value(QStringLiteral("name"));
                    if (value.isString()) {
                        arg.name = value.toString();
                    } else {
                        *errorMessage = QStringLiteral("arg at index %1 is missing the name")
                                            .arg(QString::number(it - argsArr.begin()));
                        return {};
                    }

                    value = argObj.value(QStringLiteral("member"));
                    if (value.isString()) {
                        arg.member = value.toString();
                    } else {
                        *errorMessage = QStringLiteral("arg at index %1 is missing the member")
                                            .arg(QString::number(it - argsArr.begin()));
                        return {};
                    }

                    value = argObj.value(QStringLiteral("default"));
                    if (value.isString()) {
                        arg.defaultValue = value.toString();
                    } else if (value.isDouble()) {
                        arg.defaultValue = QString::number(value.toDouble());
                    }

                    res.args.append(arg);
                }
            }

            return res;
        }
    };

    class Class {
    public:
        QString name;
        QString super;
        QMap<QString, Property> properties;
        QList<Enumeration> enums;

        bool reserved = false;
        Constructor ctor;

        QString fileName;

        static Class parse(const QJsonValue &val, QString *errorMessage) {
            if (!val.isObject()) {
                *errorMessage = QStringLiteral("not an object");
                return {};
            }

            Class res;
            QJsonObject obj = val.toObject();
            QJsonValue value;

            value = obj.value(QStringLiteral("name"));
            if (value.isString()) {
                res.name = value.toString();
            } else {
                *errorMessage = QStringLiteral("missing \"name\" attribute");
                return {};
            }

            value = obj.value(QStringLiteral("fileName"));
            if (value.isString()) {
                res.fileName = value.toString();
            } else {
                res.fileName = res.name + ".h";
            }

            value = obj.value(QStringLiteral("super"));
            if (value.isString()) {
                res.super = value.toString();
            }

            value = obj.value(QStringLiteral("enums"));
            if (value.isArray()) {
                auto enumsArr = value.toArray();
                for (int i = 0; i < enumsArr.size(); ++i) {
                    auto e = Enumeration::parse(enumsArr.at(i), errorMessage);
                    if (!errorMessage->isEmpty()) {
                        *errorMessage = QStringLiteral("parse enumeration at index %1 failed: %2")
                                            .arg(QString::number(i), *errorMessage);
                        return {};
                    }
                    res.enums.append(e);
                }
            }

            value = obj.value(QStringLiteral("reserved"));
            if (value.toBool()) {
                res.reserved = true;
            }

            value = obj.value("ctor");
            if (value.isObject()) {
                auto ctor = Constructor::parse(value, errorMessage);
                if (!errorMessage->isEmpty()) {
                    *errorMessage =
                        QStringLiteral("parse constructor failed: %1").arg(*errorMessage);
                    return {};
                }
                res.ctor = ctor;
            }

            value = obj.value("properties");
            if (value.isObject()) {
                auto propObj = value.toObject();
                for (auto it = propObj.begin(); it != propObj.end(); ++it) {
                    auto prop = Property::parse(it.value(), errorMessage);
                    if (!errorMessage->isEmpty()) {
                        *errorMessage = QStringLiteral("parse property \"%1\" failed: %2")
                                            .arg(it.key(), *errorMessage);
                        return {};
                    }
                    res.properties.insert(it.key(), prop);
                }
            }

            return res;
        }
    };

    class ClassCollection {
    public:
        ClassCollection() = default;

    public:
        bool addClass(const Class &cls) {
            if (nameIndexes.contains(cls.name))
                return false;

            classes.push_back(cls);
            nameIndexes[cls.name] = classes.size() - 1;
            fileIndexes[cls.fileName].append(classes.size() - 1);

            for (const auto &prop : cls.properties) {
                if (prop.containerType != Property::NoContainer) {
                    classContainers[prop.type] |= prop.containerType;
                }
            }

            return true;
        }

        QVector<Class> classList() const {
            return classes;
        }

        bool contains(const QString &name) const {
            return nameIndexes.contains(name);
        }

        QHash<QString, QVector<Class>> files() const {
            QHash<QString, QVector<Class>> res;
            res.reserve(fileIndexes.size());
            for (auto it = fileIndexes.begin(); it != fileIndexes.end(); ++it) {
                QVector<Class> cur;
                for (const auto &idx : qAsConst(it.value())) {
                    cur.append(classes.at(idx));
                }
                res.insert(it.key(), cur);
            }
            return res;
        }

        Property::ContainerTypes containerTypes(const QString &className) const {
            return classContainers.value(className);
        }

    protected:
        QVector<Class> classes;
        QHash<QString, int> nameIndexes;
        QHash<QString, QVector<int>> fileIndexes;
        QHash<QString, Property::ContainerTypes> classContainers;
    };

}

int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);
    QCoreApplication::setApplicationVersion(APP_VERSION);

    QString fileName;
    QString output;

    QCommandLineParser parser;
    parser.setApplicationDescription(
        QStringLiteral("Qt Substate Classes Generator %1").arg(APP_VERSION));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption outputOption(QStringLiteral("o"));
    outputOption.setDescription(
        QStringLiteral("Write output to the given directory rather than current directory."));
    outputOption.setValueName(QStringLiteral("dir"));
    outputOption.setFlags(QCommandLineOption::ShortOptionStyle);
    parser.addOption(outputOption);

    parser.addPositionalArgument(QStringLiteral("<file>"),
                                 QStringLiteral("Class schema in json format."));

    parser.process(a.arguments());

    const QStringList files = parser.positionalArguments();
    output = parser.value(outputOption);

    if (output.isEmpty())
        output = QDir::currentPath();

    if (files.size() > 1) {
        error(qPrintable(QLatin1String("Too many input files specified: '") +
                         files.join(QLatin1String("' '")) + QLatin1Char('\'')));
        parser.showHelp(1);
    } else if (files.isEmpty()) {
        error(qPrintable(QLatin1String("No input file specified")));
        parser.showHelp(1);
    } else {
        fileName = files.first();
    }

    QJsonObject docObj;
    {
        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly)) {
            error(QStringLiteral("Cannot open the input file \"%1\"")
                      .arg(fileName)
                      .toLocal8Bit()
                      .constData());
            return -1;
        }

        QJsonParseError err;
        auto doc = QJsonDocument::fromJson(file.readAll(), &err);
        file.close();
        if (err.error != QJsonParseError::NoError || !doc.isObject()) {
            error(qPrintable(QLatin1String("Invalid input file format")));
            return -1;
        }
        docObj = doc.object();
    }

    QHash<QString, QString> includeMap;
    ClassCollection classes;
    QString ns;
    QString dllexport;

    QJsonValue value;
    value = docObj.value(QStringLiteral("include"));
    if (value.isObject()) {
        const auto &obj = value.toObject();
        for (auto it = obj.begin(); it != obj.end(); ++it) {
            if (!it->isString())
                continue;
            includeMap.insert(it.key(), it->toString());
        }
    }

    value = docObj.value(QStringLiteral("namespace"));
    if (value.isString()) {
        ns = value.toString();
    }

    value = docObj.value(QStringLiteral("dllexport"));
    if (value.isString()) {
        dllexport = value.toString();
    }

    value = docObj.value(QStringLiteral("classes"));
    if (!value.isArray()) {
        error(qPrintable("\"classes\" is not an array"));
        return -1;
    } else {
        auto arr = value.toArray();
        for (int i = 0; i < arr.size(); ++i) {
            QString errorMessage;
            auto cls = Class::parse(arr.at(i), &errorMessage);
            if (!errorMessage.isEmpty()) {
                error(QStringLiteral("parse class at index %1 failed: %2")
                          .arg(QString::number(i), errorMessage)
                          .toLocal8Bit()
                          .constData());
                return -1;
            }

            if (!classes.addClass(cls)) {
                error(
                    QStringLiteral("duplicated class %1").arg(cls.name).toLocal8Bit().constData());
                return -1;
            }
        }
    }

    // Check super classes
    for (const auto &cls : classes.classList()) {
        if (!cls.super.isEmpty() && !classes.contains(cls.super)) {
            error(QStringLiteral("unknown super class %1 of %2")
                      .arg(cls.super, cls.name)
                      .toLocal8Bit()
                      .constData());
            return -1;
        }
    }

    // Generate
    const auto &classFiles = classes.files();
    for (auto it = classFiles.begin(); it != classFiles.end(); ++it) {
        const auto &queue = it.value();

        // Write header
        QFile file(output + "/" + it.key());
        if (!file.open(QIODevice::WriteOnly)) {
            error(QStringLiteral("Cannot open the output file \"%1\"")
                      .arg(file.fileName())
                      .toLocal8Bit()
                      .constData());
            return -1;
        }

        QTextStream out(&file);

        // Get include
        std::set<QString> inc;
        for (const auto &cls : qAsConst(queue)) {
            for (const auto &prop : qAsConst(cls.properties)) {
                auto it2 = includeMap.find(prop.type);
                if (it2 != includeMap.end()) {
                    inc.insert(it2.value());
                }
            }
        }

        // Write header guard
        QString headerGuard = it.key().toUpper();
        headerGuard.replace(QStringLiteral("."), QStringLiteral("_"));
        out << QStringLiteral("#ifndef %1").arg(headerGuard) << Qt::endl;
        out << QStringLiteral("#define %1").arg(headerGuard) << Qt::endl;
        out << Qt::endl;

        // Write include
        for (const auto &item : qAsConst(inc)) {
            out << QStringLiteral("#include <%1>").arg(item) << Qt::endl;
        }
        out << Qt::endl;

        // Write Entity include
        out << QStringLiteral("#include <qsubstate/entity.h>") << Qt::endl;
        out << Qt::endl;

        // Write namespace begin
        if (!ns.isEmpty()) {
            out << QStringLiteral("namespace %1 {").arg(ns) << Qt::endl;
            out << Qt::endl;
        }

        // Write forward declarations
        for (const auto &cls : qAsConst(queue)) {
            out << QStringLiteral("class %1;").arg(cls.name) << Qt::endl;
        }
        out << Qt::endl;

        // Write classes
        for (const auto &cls : qAsConst(queue)) {
            QString super = cls.super.isEmpty() ? "Substate::Entity" : cls.super;

            // Write private class forward declaration
            out << QStringLiteral("class %1Private;").arg(cls.name) << Qt::endl;
            out << Qt::endl;

            // Write class begin
            out << QStringLiteral("class %1%2 : public %3 {")
                       .arg(dllexport.isEmpty() ? QString() : (dllexport + " "), cls.name, super)
                << Qt::endl;
            out << QStringLiteral("    Q_OBJECT") << Qt::endl;
            out << QStringLiteral("public:") << Qt::endl;

            // Write enums
            if (!cls.enums.isEmpty()) {
                for (const auto &item : cls.enums) {
                    out << QStringLiteral("    enum %1 {").arg(item.name) << Qt::endl;
                    int current = -1;
                    for (auto it3 = item.values.begin(); it3 != item.values.end(); ++it3) {
                        const auto &key = it3.key();
                        const auto &val = it3.value();
                        if (key - current == 1) {
                            out << QStringLiteral("        %1,").arg(val) << Qt::endl;
                        } else {
                            out << QStringLiteral("        %1 = %2,").arg(val, QString::number(key))
                                << Qt::endl;
                        }
                        current = key;
                    }
                    out << QStringLiteral("    };") << Qt::endl;
                    out << QStringLiteral("    Q_ENUM(%1)").arg(item.name) << Qt::endl;
                    out << Qt::endl;
                }
            }

            // Write constructor
            if (!cls.reserved) {
                out << QStringLiteral("    %1(").arg(cls.name);
                for (const auto &arg : cls.ctor.args) {
                    if (arg.type.endsWith('*')) {
                        out << QStringLiteral("%1%2").arg(arg.type, arg.name);
                    } else {
                        out << QStringLiteral("%1 %2").arg(arg.type, arg.name);
                    }
                    if (!arg.defaultValue.isEmpty()) {
                        out << QStringLiteral(" = ") << arg.defaultValue;
                    }
                    out << QStringLiteral(", ");
                }
                out << QStringLiteral("QObject *parent = nullptr);") << Qt::endl;
            }

            // Write destructor
            out << QStringLiteral("    ~%1();").arg(cls.name) << Qt::endl;
            out << Qt::endl;

            // Write properties
            if (!cls.properties.isEmpty()) {
                out << QStringLiteral("public:") << Qt::endl;
                for (auto it2 = cls.properties.begin(); it2 != cls.properties.end(); ++it2) {
                    const auto &prop = it2.value();
                    if (prop.containerType != Property::NoContainer) {
                        out << QStringLiteral("    %1 *%2() const;")
                                   .arg(prop.translatedTypeName(), it2.key());
                    } else if (classes.contains(prop.type)) {
                        out << QStringLiteral("    %1 *%2() const;").arg(prop.type, it2.key());
                    } else {
                        out << QStringLiteral("    %1 %2() const;").arg(prop.type, it2.key());
                    }
                    out << Qt::endl;
                }
                out << Qt::endl;
            }

            // Write protect constructor
            out << QStringLiteral("protected:") << Qt::endl;
            out << QStringLiteral("    %1(%2Private &d, ").arg(cls.name, cls.name);
            for (const auto &arg : cls.ctor.args) {
                if (arg.type.endsWith('*')) {
                    out << QStringLiteral("%1%2").arg(arg.type, arg.name);
                } else {
                    out << QStringLiteral("%1 %2").arg(arg.type, arg.name);
                }
                if (!arg.defaultValue.isEmpty()) {
                    out << QStringLiteral(" = ") << arg.defaultValue;
                }
                out << QStringLiteral(", ");
            }
            out << QStringLiteral("QObject *parent = nullptr);") << Qt::endl;

            // Write class end
            out << "};" << Qt::endl;
            out << Qt::endl;

            auto containerTypes = classes.containerTypes(cls.name);
            if (containerTypes & Property::Vector) {
                out << QStringLiteral("class %1%2Vector : public Entity {")
                           .arg(dllexport + " ", cls.name)
                    << Qt::endl;
                out << QStringLiteral("    Q_OBJECT") << Qt::endl;
                out << QStringLiteral("public:") << Qt::endl;
                out << QStringLiteral("    %1Vector(QObject *parent = nullptr);").arg(cls.name)
                    << Qt::endl;
                out << QStringLiteral("    ~%1Vector();").arg(cls.name) << Qt::endl;
                out << Qt::endl;
                out << QStringLiteral("public:") << Qt::endl;
                out << QStringLiteral("    bool prepend(T *item);\n"
                                      "    bool prepend(const QVector<T *> &items);\n"
                                      "    bool append(T *item);\n"
                                      "    bool append(const QVector<T *> &items);\n"
                                      "    bool insert(int index, T *item);\n"
                                      "    bool remove(int index);\n"
                                      "    bool insert(int index, const QVector<T *> &items);\n"
                                      "    bool move(int index, int count, int dest);\n"
                                      "    bool remove(int index, int count);\n"
                                      "    T *at(int index) const;\n"
                                      "    QVector<T *> values() const;\n"
                                      "    int indexOf(T *item) const;\n"
                                      "    int size() const;\n")
                           .replace(QStringLiteral("T *"), QStringLiteral("%1 *").arg(cls.name));
                out << Qt::endl;
                out << QStringLiteral("Q_SIGNALS:") << Qt::endl;
                out << QStringLiteral(
                           "    void inserted(int index, const QVector<T *> &items);\n"
                           "    void aboutToMove(int index, int count, int dest);\n"
                           "    void moved(int index, int count, int dest);\n"
                           "    void aboutToRemove(int index, const QVector<T *> &items);\n"
                           "    void removed(int index, int count);\n")
                           .replace(QStringLiteral("T *"), QStringLiteral("%1 *").arg(cls.name));
                out << "};" << Qt::endl;
                out << Qt::endl;
            }

            if (containerTypes & Property::Sheet) {
                out << QStringLiteral("class %1%2Sheet : public Entity {")
                           .arg(dllexport + " ", cls.name)
                    << Qt::endl;
                out << QStringLiteral("    Q_OBJECT") << Qt::endl;
                out << QStringLiteral("public:") << Qt::endl;
                out << QStringLiteral("    %1Sheet(QObject *parent = nullptr);").arg(cls.name)
                    << Qt::endl;
                out << QStringLiteral("    ~%1Sheet();").arg(cls.name) << Qt::endl;
                out << Qt::endl;
                out << QStringLiteral("public:") << Qt::endl;
                out << QStringLiteral("    int insert(T *item);\n"
                                      "    bool remove(int index);\n"
                                      "    bool remove(T *item);\n"
                                      "    T *at(int index);\n"
                                      "    int indexOf(T *item) const;\n"
                                      "    QList<int> indexes() const;\n"
                                      "    int size() const;\n")
                           .replace(QStringLiteral("T *"), QStringLiteral("%1 *").arg(cls.name));
                out << Qt::endl;
                out << QStringLiteral("Q_SIGNALS:") << Qt::endl;
                out << QStringLiteral("    void inserted(int seq, T *item);\n"
                                      "    void aboutToRemove(int seq, T *item);\n"
                                      "    void removed(int seq);\n")
                           .replace(QStringLiteral("T *"), QStringLiteral("%1 *").arg(cls.name));
                out << "};" << Qt::endl;
                out << Qt::endl;
            }
        }

        // Write namespace end
        if (!ns.isEmpty()) {
            out << QStringLiteral("}") << Qt::endl;
            out << Qt::endl;
        }

        // Write header guard end
        out << QStringLiteral("#endif // %1").arg(headerGuard);
    }

    return 0;
}