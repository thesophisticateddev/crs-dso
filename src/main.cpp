#include <QtWidgets/QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QThread>
#include <QIcon>
#include <QtQml>
#include "signalgenerator.h"
#include "core/DeviceController.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // Set application icon (works on all platforms)
    app.setWindowIcon(QIcon(":/icons/crs-dso.png"));

    // Register DeviceController for QML
    qmlRegisterType<DeviceController>("CRS.DSO", 1, 0, "DeviceController");

    SignalGenerator generator; // This now stays in the UI thread

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("signalGenerator", &generator);

    const QUrl url(u"qrc:/crs_dso/qml/Main.qml"_qs);
    engine.load(url);

    return app.exec();
}
