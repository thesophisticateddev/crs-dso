#include <QtWidgets/QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "signalgenerator.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    SignalGenerator generator;
    QQmlApplicationEngine engine;

    // Inject the generator into the QML context
    engine.rootContext()->setContextProperty("signalGenerator", &generator);

    const QUrl url(u"qrc:/crs_dso/qml/Main.qml"_qs);
    engine.load(url);

    return app.exec();
}
