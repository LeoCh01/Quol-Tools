#include "plugins/screen/lib/SnipOverlay.hpp"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QPropertyAnimation>

namespace {
const QColor kAccent(86, 156, 255, 235);
const QColor kDim(0, 0, 0, 90);
}  // namespace

SnipOverlay::SnipOverlay(const QPixmap &screenshot, QWidget *parent)
    : QWidget(parent), m_screenshot(screenshot) {
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setCursor(Qt::CrossCursor);
    setWindowOpacity(0.0);
    setAttribute(Qt::WA_DeleteOnClose);
}

QRect SnipOverlay::selectionToScreenshotRect() const {
    if (m_selectionRect.isNull() || width() <= 0 || height() <= 0 || m_screenshot.isNull())
        return {};

    const qreal sx = static_cast<qreal>(m_screenshot.width()) / static_cast<qreal>(width());
    const qreal sy = static_cast<qreal>(m_screenshot.height()) / static_cast<qreal>(height());

    const int x = qRound(m_selectionRect.x() * sx);
    const int y = qRound(m_selectionRect.y() * sy);
    const int w = qRound(m_selectionRect.width() * sx);
    const int h = qRound(m_selectionRect.height() * sy);

    return QRect(x, y, w, h).intersected(m_screenshot.rect());
}

void SnipOverlay::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);

    QPropertyAnimation *fade = new QPropertyAnimation(this, "windowOpacity", this);
    fade->setDuration(160);
    fade->setStartValue(0.0);
    fade->setEndValue(1.0);
    fade->start(QAbstractAnimation::DeleteWhenStopped);
}

void SnipOverlay::mousePressEvent(QMouseEvent *event) {
    if (event->button() != Qt::LeftButton)
        return;

    m_isSelecting = true;
    m_startPoint = event->position().toPoint();
    m_endPoint = m_startPoint;
    m_selectionRect = {};
    update();
}

void SnipOverlay::mouseMoveEvent(QMouseEvent *event) {
    if (!m_isSelecting)
        return;

    m_endPoint = event->position().toPoint();
    m_selectionRect = QRect(m_startPoint, m_endPoint).normalized().intersected(rect());
    update();
}

void SnipOverlay::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() != Qt::LeftButton)
        return;

    m_isSelecting = false;
    m_endPoint = event->position().toPoint();
    m_selectionRect = QRect(m_startPoint, m_endPoint).normalized().intersected(rect());

    if (m_selectionRect.width() < 8 || m_selectionRect.height() < 8) {
        m_selectionRect = {};
        update();
        return;
    }

    const QRect screenshotRect = selectionToScreenshotRect();
    if (screenshotRect.isNull()) {
        update();
        return;
    }

    const QPixmap cropped = m_screenshot.copy(screenshotRect);
    close();
    emit snipReady(cropped);
}

void SnipOverlay::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape) {
        close();
        return;
    }
    QWidget::keyPressEvent(event);
}

void SnipOverlay::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.drawPixmap(rect(), m_screenshot);

    const QRect s = m_selectionRect;
    if (s.isNull()) {
        painter.fillRect(rect(), kDim);
    } else {
        painter.fillRect(QRect(0, 0, width(), s.top()), kDim);
        painter.fillRect(QRect(0, s.bottom() + 1, width(), height() - s.bottom() - 1), kDim);
        painter.fillRect(QRect(0, s.top(), s.left(), s.height()), kDim);
        painter.fillRect(QRect(s.right() + 1, s.top(), width() - s.right() - 1, s.height()), kDim);

        painter.setPen(QPen(QColor(255, 255, 255, 40), 1));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(QRectF(s).adjusted(2, 2, -2, -2));

        painter.setPen(QPen(kAccent, 2));
        painter.drawRect(QRectF(s).adjusted(1, 1, -1, -1));
    }
}
