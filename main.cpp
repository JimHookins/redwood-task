#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "boardmodel.h"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;

    BoardModel boardModel;

    qRegisterMetaType<BoardModel::BoardConstants>("BoardConstants");

    engine.rootContext()->setContextProperty("boardModel", &boardModel);
    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
    if (engine.rootObjects().isEmpty())
        return -1;
    return app.exec();
}
