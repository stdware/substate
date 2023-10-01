#include <QtCore/QCommandLineOption>
#include <QtCore/QCommandLineParser>
#include <QtCore/QCoreApplication>

static void error(const char *msg = "Invalid argument") {
    if (msg)
        fprintf(stderr, "qormc:%s\n", msg);
}

int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);
    QCoreApplication::setApplicationVersion(APP_VERSION);

    QString filename;
    QString output;

    QCommandLineParser parser;
    parser.setApplicationDescription(
        QStringLiteral("Substate Qt Classes Generator %1").arg(APP_VERSION));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption outputOption(QStringLiteral("o"));
    outputOption.setDescription(QStringLiteral("Write output to file rather than stdout."));
    outputOption.setValueName(QStringLiteral("file"));
    outputOption.setFlags(QCommandLineOption::ShortOptionStyle);
    parser.addOption(outputOption);

    parser.addPositionalArgument(QStringLiteral("[file]"),
                                 QStringLiteral("Class schema in json format."));

    parser.process(a.arguments());

    const QStringList files = parser.positionalArguments();
    output = parser.value(outputOption);

    if (files.size() > 1) {
        error(qPrintable(QLatin1String("Too many input files specified: '") +
                         files.join(QLatin1String("' '")) + QLatin1Char('\'')));
        parser.showHelp(1);
    } else if (!files.isEmpty()) {
        filename = files.first();
    }

    return 0;
}