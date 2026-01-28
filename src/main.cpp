#include <QtWidgets/QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QThread>
#include "signalgenerator.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    SignalGenerator generator; // This now stays in the UI thread

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("signalGenerator", &generator);

    const QUrl url(u"qrc:/crs_dso/qml/Main.qml"_qs);
    engine.load(url);

    return app.exec();
}
