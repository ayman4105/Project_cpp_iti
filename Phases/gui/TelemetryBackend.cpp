#include "TelemetryBackend.hpp"
#include <QDebug>
#include <QRegularExpression>

TelemetryBackend::TelemetryBackend(LogModel *logModel, QObject *parent)
    : QObject(parent)
    , m_logModel(logModel)
    , m_statsTimer(new QTimer(this))
{
    connect(m_statsTimer, &QTimer::timeout,
            this, &TelemetryBackend::updateStats);

    m_statsTimer->setInterval(500);
}

TelemetryBackend::~TelemetryBackend()
{
    stop();
}

void TelemetryBackend::start()
{
    if (m_isRunning)
        return;

    try {
        m_app = std::make_unique<TelemetryLoggingApp>(
            "/home/ayman/ITI/Project_cpp_iti/Phases/config.json");

        if (m_app->logger) {
            qDebug() << "Logger exists. Adding GUI sink...";
            m_app->logger->add_sink(
                std::make_unique<GUISink>(m_logModel));
        } else {
            qWarning() << "Logger is NULL!";
        }

        m_appThread = std::thread([this]() {
            m_app->start();
        });

        m_isRunning = true;
        m_statsTimer->start();
        emit isRunningChanged();

        qDebug() << "Telemetry system started";

    } catch (const std::exception &e) {
        qWarning() << "Failed to start:" << e.what();
    }
}

void TelemetryBackend::stop()
{
    if (!m_isRunning)
        return;

    m_isRunning = false;
    m_statsTimer->stop();

    if (m_app) {
        m_app->isRunning = false;
    }

    if (m_appThread.joinable()) {
        m_appThread.join();
    }

    m_app.reset();
    emit isRunningChanged();

    qDebug() << "Telemetry system stopped";
}

void TelemetryBackend::clearLogs()
{
    if (!m_logModel)
        return;

    m_logModel->clear();
    m_messageCount = 0;

    emit messageCountChanged();
}

void TelemetryBackend::updateStats()
{
    if (!m_logModel)
        return;

    m_messageCount = m_logModel->count();
    emit messageCountChanged();

    parseLatestLog();

    m_bufferUsage = m_messageCount % 200;
    emit bufferUsageChanged();
}

void TelemetryBackend::parseLatestLog()
{
    if (!m_logModel)
        return;

    int rows = m_logModel->rowCount();
    if (rows == 0)
        return;

    QModelIndex index = m_logModel->index(rows - 1, 0);

    QString context = m_logModel->data(index, LogModel::ContextRole).toString();
    QString message = m_logModel->data(index, LogModel::MessageRole).toString();

    QString fullText = context + " " + message;

    qDebug() << "Full log line:" << fullText;

    QRegularExpression rx("(\\d+\\.?\\d*)%");
    QRegularExpressionMatchIterator it = rx.globalMatch(fullText);

    if (!it.hasNext()) {
        qDebug() << "No percentage found";
        return;
    }

    QRegularExpressionMatch match = it.next();
    double value = match.captured(1).toDouble();

    qDebug() << "Parsed value:" << value;

    if (fullText.contains("CPU", Qt::CaseInsensitive)) {
        m_cpuUsage = value;
        emit cpuUsageChanged();
        qDebug() << "CPU updated:" << m_cpuUsage;
    }
    else if (fullText.contains("RAM", Qt::CaseInsensitive)) {
        m_ramUsage = value;
        emit ramUsageChanged();
        qDebug() << "RAM updated:" << m_ramUsage;
    }
    else if (fullText.contains("GPU", Qt::CaseInsensitive)) {
        m_gpuUsage = value;
        emit gpuUsageChanged();
        qDebug() << "GPU updated:" << m_gpuUsage;
    }
}
void TelemetryBackend::setFileSourceEnabled(bool enabled)
{
    qDebug() << "File source enabled:" << enabled;
}

void TelemetryBackend::setSocketSourceEnabled(bool enabled)
{
    qDebug() << "Socket source enabled:" << enabled;
}

void TelemetryBackend::setSomeIPSourceEnabled(bool enabled)
{
    qDebug() << "SOME/IP source enabled:" << enabled;
}
