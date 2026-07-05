#include "plugins/misc/lib/DiceWidget.hpp"

#include <QGuiApplication>
#include <QMouseEvent>
#include <QPixmap>
#include <QRandomGenerator>
#include <QScreen>

DiceWidget::DiceWidget(const QString &pluginRootPath, QWidget *parent) : QWidget(parent), m_rootPath(pluginRootPath) {
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setFixedSize(80, 80);

    m_diceLabel = new QLabel(this);
    m_diceLabel->setGeometry(0, 0, 80, 80);
    m_diceLabel->setAlignment(Qt::AlignCenter);

    m_currentFace = 0;
    QPixmap pix(diceImagePath(0));
    m_diceLabel->setPixmap(pix.scaled(72, 72, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    m_physicsTimer = new QTimer(this);
    m_physicsTimer->setInterval(16);
    connect(m_physicsTimer, &QTimer::timeout, this, &DiceWidget::updatePhysics);

    adjustSize();
}

QString DiceWidget::diceImagePath(int face) const {
    if (face == 0)
        return m_rootPath + QStringLiteral("/res/dice/img/dice-x.png");
    return m_rootPath + QStringLiteral("/res/dice/img/dice-%1.png").arg(face);
}

void DiceWidget::setRandomFace() {
    int newFace = QRandomGenerator::global()->bounded(1, 7);
    m_currentFace = newFace;
    QPixmap pix(diceImagePath(newFace));
    m_diceLabel->setPixmap(pix.scaled(72, 72, Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void DiceWidget::start(QuolServices *services) {
    Q_UNUSED(services);
    m_physicsTimer->stop();
    m_vx = 0.0;
    m_vy = 0.0;
    m_dragging = false;
    m_currentFace = 0;

    QPixmap pix(diceImagePath(0));
    m_diceLabel->setPixmap(pix.scaled(72, 72, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    if (auto *screen = QGuiApplication::primaryScreen()) {
        QRect geo = screen->geometry();
        m_px = (geo.width() - 80) / 2.0;
        m_py = (geo.height() - 80) / 2.0;
        move(static_cast<int>(m_px), static_cast<int>(m_py));
    }

    show();
    raise();
}

void DiceWidget::stop() {
    m_physicsTimer->stop();
    hide();
}

QWidget *DiceWidget::widget() {
    return this;
}

void DiceWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_physicsTimer->stop();
        m_vx = 0.0;
        m_vy = 0.0;
        m_dragging = true;
        m_dragOffset = event->globalPosition().toPoint() - frameGeometry().topLeft();
        m_lastMousePos = event->globalPosition().toPoint();
        m_lastVelX = 0.0;
        m_lastVelY = 0.0;
        raise();
        event->accept();
    }
}

void DiceWidget::mouseMoveEvent(QMouseEvent *event) {
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        QPointF globalPos = event->globalPosition().toPoint();
        double dx = globalPos.x() - m_lastMousePos.x();
        double dy = globalPos.y() - m_lastMousePos.y();
        m_lastVelX = dx * 3;
        m_lastVelY = dy * 3;
        m_lastMousePos = globalPos.toPoint();

        m_distSinceLastFace += std::sqrt(dx * dx + dy * dy);
        if (m_distSinceLastFace >= 20.0) {
            setRandomFace();
            m_distSinceLastFace = 0.0;
        }

        QPoint newPos = globalPos.toPoint() - m_dragOffset;
        move(newPos);
        m_px = newPos.x();
        m_py = newPos.y();
        event->accept();
    }
}

void DiceWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton && m_dragging) {
        m_dragging = false;

        double vel = std::sqrt(m_lastVelX * m_lastVelX + m_lastVelY * m_lastVelY);
        if (vel > 1) {
            m_vx = m_lastVelX;
            m_vy = m_lastVelY;
            m_distSinceLastFace = 20.0;
            m_physicsTimer->start();
        }

        event->accept();
    }
}

void DiceWidget::updatePhysics() {
    m_vx *= 0.95;
    m_vy *= 0.95;

    m_px += m_vx;
    m_py += m_vy;

    if (auto *screen = QGuiApplication::primaryScreen()) {
        QRect geo = screen->geometry();
        int w = width();
        int h = height();

        if (m_px < 0) {
            m_px = 0;
            m_vx = -m_vx;
        } else if (m_px + w > geo.width()) {
            m_px = geo.width() - w;
            m_vx = -m_vx;
        }

        if (m_py < 0) {
            m_py = 0;
            m_vy = -m_vy;
        } else if (m_py + h > geo.height()) {
            m_py = geo.height() - h;
            m_vy = -m_vy;
        }
    }

    move(static_cast<int>(m_px), static_cast<int>(m_py));

    double speed = std::sqrt(m_vx * m_vx + m_vy * m_vy);
    m_distSinceLastFace += speed;
    if (m_distSinceLastFace >= 20.0) {
        setRandomFace();
        m_distSinceLastFace = 0.0;
    }

    if (speed < 0.5) {
        m_physicsTimer->stop();
        m_vx = 0.0;
        m_vy = 0.0;
    }
}
