#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "LogModel.hpp"
#include "TelemetryBackend.hpp"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    
    // Create models
    LogModel logModel;
    TelemetryBackend backend(&logModel);
    
    // Setup QML engine
    QQmlApplicationEngine engine;
    
    // Expose to QML
    engine.rootContext()->setContextProperty("logModel", &logModel);
    engine.rootContext()->setContextProperty("backend", &backend);
    
    // Load main QML
    const QUrl url(QStringLiteral("qrc:/qml/main.qml"));
    engine.load(url);
    
    if (engine.rootObjects().isEmpty())
        return -1;
    
    return app.exec();
}