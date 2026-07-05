#include "plugins/misc/lib/StopwatchWidget.hpp"
#include "core/InputManager.hpp"
#include "plugin_api/QuolServices.hpp"

#include <QHBoxLayout>
#include <QListWidget>
#include <QMouseEvent>
#include <QVBoxLayout>

StopwatchWidget::StopwatchWidget(const QString &pluginRootPath, QWidget *parent)
    : QWidget(parent), m_rootPath(pluginRootPath) {
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);

    auto *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(4);

    QIcon playIcon(m_rootPath + QStringLiteral("/res/stopwatch/img/play.svg"));
    QIcon pauseIcon(m_rootPath + QStringLiteral("/res/stopwatch/img/pause.svg"));
    QIcon resetIcon(m_rootPath + QStringLiteral("/res/stopwatch/img/reset.svg"));
    QIcon clockIcon(m_rootPath + QStringLiteral("/res/stopwatch/img/clock.svg"));
    QIcon flagIcon(m_rootPath + QStringLiteral("/res/stopwatch/img/flag.svg"));

    auto *container = new QWidget(this);
    container->setObjectName(QStringLiteral("swContainer"));
    container->setStyleSheet(
        QStringLiteral("#swContainer { background: rgba(0, 0, 0, 60); border-radius: 15px; padding: 10px; }")
    );
    auto *topRow = new QHBoxLayout(container);
    topRow->setContentsMargins(14, 10, 14, 10);
    topRow->setSpacing(8);

    m_timeLabel = new QLabel(QStringLiteral("00:00:00.00"), container);
    m_timeLabel->setAlignment(Qt::AlignCenter);
    m_timeLabel->setStyleSheet(QStringLiteral(
        "font-size: 28px; font-weight: bold; font-family: 'Consolas', monospace; "
        "color: white; background: transparent; letter-spacing: 1px;"
    ));
    topRow->addWidget(m_timeLabel, 1);

    m_playBtn = new QPushButton(container);
    m_playBtn->setIcon(playIcon);
    m_playBtn->setIconSize(QSize(18, 18));
    m_playBtn->setFixedSize(38, 30);
    m_playBtn->setCursor(Qt::PointingHandCursor);
    m_playBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: rgba(76, 175, 80, 120); border: none; border-radius: 7px; }"
        "QPushButton:hover { background: rgba(76, 175, 80, 180); }"
    ));
    topRow->addWidget(m_playBtn);

    m_resetBtn = new QPushButton(container);
    m_resetBtn->setIcon(resetIcon);
    m_resetBtn->setIconSize(QSize(18, 18));
    m_resetBtn->setFixedSize(38, 30);
    m_resetBtn->setCursor(Qt::PointingHandCursor);
    m_resetBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: rgba(244, 67, 54, 120); border: none; border-radius: 7px; }"
        "QPushButton:hover { background: rgba(244, 67, 54, 180); }"
    ));
    topRow->addWidget(m_resetBtn);

    m_modeBtn = new QPushButton(container);
    m_modeBtn->setIcon(clockIcon);
    m_modeBtn->setIconSize(QSize(18, 18));
    m_modeBtn->setFixedSize(38, 30);
    m_modeBtn->setCheckable(true);
    m_modeBtn->setCursor(Qt::PointingHandCursor);
    m_modeBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: rgba(255, 255, 255, 30); border: none; border-radius: 7px; }"
        "QPushButton:hover { background: rgba(255, 255, 255, 50); }"
        "QPushButton:checked { background: rgba(255, 193, 7, 120); }"
    ));
    topRow->addWidget(m_modeBtn);

    outerLayout->addWidget(container);

    mLapList = new QListWidget(this);
    mLapList->setObjectName(QStringLiteral("lapList"));
    mLapList->setMaximumHeight(120);
    mLapList->setMinimumHeight(0);
    mLapList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    mLapList->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    mLapList->setStyleSheet(QStringLiteral(
        "#lapList { background: rgba(0, 0, 0, 50); border: 1px solid rgba(255, 255, 255, 15); "
        "  border-radius: 10px; color: white; font-size: 11px; padding: 4px; }"
        "#lapList::item { padding: 2px 8px; }"
    ));
    mLapList->setVisible(false);
    outerLayout->addWidget(mLapList);

    QObject::connect(m_playBtn, &QPushButton::clicked, this, &StopwatchWidget::toggleRunning);
    QObject::connect(m_resetBtn, &QPushButton::clicked, this, &StopwatchWidget::doReset);
    QObject::connect(m_modeBtn, &QPushButton::clicked, this, &StopwatchWidget::toggleMode);

    m_timer = new QTimer(this);
    m_timer->setInterval(20);
    QObject::connect(m_timer, &QTimer::timeout, this, &StopwatchWidget::updateDisplay);

    adjustSize();
}

void StopwatchWidget::start(QuolServices *services) {
    m_services = services;
    if (m_services && m_hotkeyId.isEmpty()) {
        m_hotkeyId =
            m_services->inputManager()->addHotkey(QStringLiteral("space"), [this]() { toggleRunning(); }, true);
    }
    show();
}

void StopwatchWidget::stop() {
    resetStopwatch();
    hide();
    if (!m_hotkeyId.isEmpty() && m_services) {
        m_services->inputManager()->removeHotkey(m_hotkeyId);
        m_hotkeyId.clear();
    }
}

QWidget *StopwatchWidget::widget() {
    return this;
}

void StopwatchWidget::resetStopwatch() {
    m_running = false;
    m_timer->stop();
    m_baseTime = 0;
    m_lastLapTime = 0;
    m_timeLabel->setText(QStringLiteral("00:00:00.00"));
    m_playBtn->setIcon(QIcon(m_rootPath + QStringLiteral("/res/stopwatch/img/play.svg")));
    mLapList->clear();
    mLapList->setVisible(false);
    adjustSize();
}

void StopwatchWidget::toggleRunning() {
    if (!m_running) {
        m_elapsed.start();
        m_timer->start();
        m_running = true;
        m_playBtn->setIcon(QIcon(m_rootPath + QStringLiteral("/res/stopwatch/img/pause.svg")));
    } else if (m_lapMode) {
        qint64 now = m_baseTime + m_elapsed.elapsed();
        qint64 lapMs = now - m_lastLapTime;
        m_lastLapTime = now;
        mLapList->insertItem(0, QStringLiteral("Lap %1  %2").arg(mLapList->count() + 1).arg(formatTime(lapMs)));
        mLapList->setVisible(true);
        adjustSize();
    } else {
        m_baseTime += m_elapsed.elapsed();
        m_timer->stop();
        m_running = false;
        m_playBtn->setIcon(QIcon(m_rootPath + QStringLiteral("/res/stopwatch/img/play.svg")));
    }
}

void StopwatchWidget::doReset() {
    resetStopwatch();
}

void StopwatchWidget::toggleMode() {
    m_lapMode = m_modeBtn->isChecked();
    m_modeBtn->setIcon(QIcon(
        m_rootPath + QStringLiteral("/res/stopwatch/img/")
        + (m_lapMode ? QStringLiteral("flag.svg") : QStringLiteral("clock.svg"))
    ));
    resetStopwatch();
}

void StopwatchWidget::updateDisplay() {
    if (!m_running)
        return;
    m_timeLabel->setText(formatTime(m_baseTime + m_elapsed.elapsed()));
}

QString StopwatchWidget::formatTime(qint64 ms) const {
    int totalSec = static_cast<int>(ms / 1000);
    int h = totalSec / 3600;
    int m = (totalSec % 3600) / 60;
    int s = totalSec % 60;
    int cs = static_cast<int>((ms % 1000) / 10);
    return QStringLiteral("%1:%2:%3.%4")
        .arg(h, 2, 10, QLatin1Char('0'))
        .arg(m, 2, 10, QLatin1Char('0'))
        .arg(s, 2, 10, QLatin1Char('0'))
        .arg(cs, 2, 10, QLatin1Char('0'));
}

void StopwatchWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragOffset = event->globalPosition().toPoint() - frameGeometry().topLeft();
        event->accept();
    }
}

void StopwatchWidget::mouseMoveEvent(QMouseEvent *event) {
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPosition().toPoint() - m_dragOffset);
        event->accept();
    }
}

void StopwatchWidget::mouseDoubleClickEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        toggleRunning();
        event->accept();
    }
}
