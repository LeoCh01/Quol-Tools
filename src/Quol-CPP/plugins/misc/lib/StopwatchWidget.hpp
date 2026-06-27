#pragma once

#include "plugins/misc/lib/ToolBase.hpp"

#include <QElapsedTimer>
#include <QLabel>
#include <QPushButton>
#include <QString>
#include <QTimer>
#include <QWidget>

class QListWidget;

class StopwatchWidget final : public QWidget, public ToolBase {
    Q_OBJECT

public:
    explicit StopwatchWidget(const QString &pluginRootPath, QWidget *parent = nullptr);

    QString label() const override { return QStringLiteral("Stopwatch"); }
    void start(QuolServices *services) override;
    void stop() override;
    QWidget *widget() override;

    void resetStopwatch();
    void toggleRunning();

signals:
    void closed();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    void doReset();
    void toggleMode();
    void updateDisplay();
    QString formatTime(qint64 ms) const;

    QString m_rootPath;
    QuolServices *m_services = nullptr;
    QString m_hotkeyId;

    bool m_running = false;
    bool m_lapMode = false;
    qint64 m_baseTime = 0;
    qint64 m_lastLapTime = 0;
    QElapsedTimer m_elapsed;

    QTimer *m_timer = nullptr;
    QLabel *m_timeLabel = nullptr;
    QPushButton *m_playBtn = nullptr;
    QPushButton *m_resetBtn = nullptr;
    QPushButton *m_modeBtn = nullptr;
    QListWidget *mLapList = nullptr;

    QPoint m_dragOffset;
    bool m_dragging = false;
};
