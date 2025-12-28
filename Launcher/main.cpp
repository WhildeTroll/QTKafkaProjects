#include <QCoreApplication>
#include <QLibrary>
#include <QDebug>
#include <QFileInfo>
#include <QCommandLineParser>
#include <iostream>

typedef void (*ConsumerFunction)();

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription("Consumer Library Launcher");
    parser.addHelpOption();

    parser.addPositionalArgument("library",
        "Path to shared library (.so file)");

    QCommandLineOption functionOption(QStringList() << "f" << "function",
        "Function name to call (default: 'consumer')",
        "function",
        "consumer");

    parser.addOption(functionOption);
    parser.process(app);

    const QStringList args = parser.positionalArguments();

    if (args.isEmpty()) {
        std::cerr << "Error: Library path not specified" << std::endl;
        parser.showHelp(1);
    }

    QString libraryPath = args.first();
    QString functionName = parser.value(functionOption);

    QFileInfo fileInfo(libraryPath);
    if (!fileInfo.exists()) {
        qCritical() << "Error: File" << libraryPath << "does not exist";
        return 1;
    }

    qInfo() << "Loading library:" << libraryPath;
    qInfo() << "Calling function:" << functionName;

    QLibrary library(libraryPath);

    if (!library.load()) {
        qCritical() << "Failed to load library:";
        qCritical() << library.errorString();
        return 1;
    }

    ConsumerFunction consumerFunc =
        reinterpret_cast<ConsumerFunction>(library.resolve(functionName.toUtf8().constData()));

    if (!consumerFunc) {
        qCritical() << "Error: Function" << functionName << "not found";
        library.unload();
        return 1;
    }

    qInfo() << "Function found, executing...";

    consumerFunc();

    qInfo() << "Function executed successfully";

    library.unload();

    return 0;
}
