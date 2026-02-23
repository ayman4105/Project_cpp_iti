#ifndef TELEMETRYBACKEND_HPP
#define TELEMETRYBACKEND_HPP

#include <QObject>
#include <QTimer>
#include <atomic>
#include <memory>
#include "LoggingApp.hpp"
#include "LogModel.hpp"

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// GUI Sink - Captures logs and sends to GUI
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
class GUISink : public ILogSink
{
public:
    explicit GUISink(LogModel *model) : m_model(model) {}
    
    void write(const LogMessage &message) override {
        if (!m_model)
            return;
        
        QString timestamp = QString::fromStdString(message.time);
        QString context = QString::fromStdString(message.context);
        QString level = QString::fromStdString(
            std::string(magic_enum::enum_name(message.level)));
        QString msg = QString::fromStdString(message.message);
        
        // Thread-safe call to model
        QMetaObject::invokeMethod(m_model, "addLog",
                                  Qt::QueuedConnection,
                                  Q_ARG(QString, timestamp),
                                  Q_ARG(QString, context),
                                  Q_ARG(QString, level),
                                  Q_ARG(QString, msg));
    }
    
private:
    LogModel *m_model;
};

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// TelemetryBackend - Main bridge between GUI and logging system
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
class TelemetryBackend : public QObject
{
    Q_OBJECT
    Q_PROPERTY(double cpuUsage READ cpuUsage NOTIFY cpuUsageChanged)
    Q_PROPERTY(double ramUsage READ ramUsage NOTIFY ramUsageChanged)
    Q_PROPERTY(double gpuUsage READ gpuUsage NOTIFY gpuUsageChanged)
    Q_PROPERTY(bool isRunning READ isRunning NOTIFY isRunningChanged)
    Q_PROPERTY(int messageCount READ messageCount NOTIFY messageCountChanged)
    Q_PROPERTY(int bufferUsage READ bufferUsage NOTIFY bufferUsageChanged)
    
public:
    explicit TelemetryBackend(LogModel *logModel, QObject *parent = nullptr);
    ~TelemetryBackend();
    
    // Property getters
    double cpuUsage() const { return m_cpuUsage; }
    double ramUsage() const { return m_ramUsage; }
    double gpuUsage() const { return m_gpuUsage; }
    bool isRunning() const { return m_isRunning; }
    int messageCount() const { return m_messageCount; }
    int bufferUsage() const { return m_bufferUsage; }
    
public slots:
    void start();
    void stop();
    void clearLogs();
    void setFileSourceEnabled(bool enabled);
    void setSocketSourceEnabled(bool enabled);
    void setSomeIPSourceEnabled(bool enabled);
    
signals:
    void cpuUsageChanged();
    void ramUsageChanged();
    void gpuUsageChanged();
    void isRunningChanged();
    void messageCountChanged();
    void bufferUsageChanged();
    
private slots:
    void updateStats();
    void parseLatestLog();
    
private:
    LogModel *m_logModel;
    std::unique_ptr<TelemetryLoggingApp> m_app;
    QTimer *m_statsTimer;
    
    double m_cpuUsage = 0.0;
    double m_ramUsage = 0.0;
    double m_gpuUsage = 0.0;
    bool m_isRunning = false;
    int m_messageCount = 0;
    int m_bufferUsage = 0;
    
    std::thread m_appThread;
};

#endif // TELEMETRYBACKEND_HPP