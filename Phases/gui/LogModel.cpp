#include "LogModel.hpp"

LogModel::LogModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int LogModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return static_cast<int>(m_logs.size());
}

QVariant LogModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= static_cast<int>(m_logs.size()))
        return QVariant();

    const LogEntry &log = m_logs[index.row()];

    switch (role) {
    case TimestampRole:
        return log.timestamp;
    case ContextRole:
        return log.context;
    case LevelRole:
        return log.level;
    case MessageRole:
        return log.message;
    case FormattedTextRole:
        return log.formattedText();
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> LogModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[TimestampRole] = "timestamp";
    roles[ContextRole] = "context";
    roles[LevelRole] = "level";
    roles[MessageRole] = "message";
    roles[FormattedTextRole] = "formattedText";
    return roles;
}

void LogModel::addLog(const QString &timestamp,
                      const QString &context,
                      const QString &level,
                      const QString &message)
{
    if (m_logs.size() >= MAX_LOGS) {
        beginRemoveRows(QModelIndex(), 0, 0);
        m_logs.erase(m_logs.begin());
        endRemoveRows();
    }

    beginInsertRows(QModelIndex(),
                    static_cast<int>(m_logs.size()),
                    static_cast<int>(m_logs.size()));

    m_logs.push_back({timestamp, context, level, message});

    endInsertRows();

    emit countChanged();
}

void LogModel::clear()
{
    if (m_logs.empty())
        return;

    beginResetModel();
    m_logs.clear();
    endResetModel();

    emit countChanged();
}