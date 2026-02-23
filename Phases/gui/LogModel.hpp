#ifndef LOGMODEL_HPP
#define LOGMODEL_HPP

#include <QAbstractListModel>
#include <QString>
#include <vector>

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// LogEntry Structure
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
struct LogEntry {
    QString timestamp;
    QString context;    // CPU, RAM, GPU
    QString level;      // INFO, WARNING, ERROR
    QString message;

    QString formattedText() const {
        return QString("[%1] [%2] [%3] %4")
            .arg(timestamp)
            .arg(context)
            .arg(level)
            .arg(message);
    }
};

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// LogModel
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
class LogModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum LogRoles {
        TimestampRole = Qt::UserRole + 1,
        ContextRole,
        LevelRole,
        MessageRole,
        FormattedTextRole
    };

    explicit LogModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const { return static_cast<int>(m_logs.size()); }

public slots:
    void addLog(const QString &timestamp,
                const QString &context,
                const QString &level,
                const QString &message);

    void clear();

signals:
    void countChanged();

private:
    std::vector<LogEntry> m_logs;
    const int MAX_LOGS = 1000;
};

#endif