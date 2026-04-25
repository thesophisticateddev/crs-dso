#include <QtWidgets/QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QThread>
#include <QIcon>
#include <QQuickWindow>
#include <QtQml>
#include "core/ScopeController.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // Set application icon (works on all platforms)
    app.setWindowIcon(QIcon(":/icons/crs-dso.png"));

    ScopeController scope;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("scope", &scope);

    const QUrl url(u"qrc:/crs_dso/qml/Main.qml"_qs);
    engine.load(url);

    // Set icon on QML window (needed for Linux window managers)
    if (QQuickWindow *window = qobject_cast<QQuickWindow *>(engine.rootObjects().value(0))) {
        window->setIcon(QIcon(":/icons/crs-dso.png"));
    }

    return app.exec();
}
