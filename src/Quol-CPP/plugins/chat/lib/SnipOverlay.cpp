#include "plugins/chat/lib/SnipOverlay.hpp"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QPushButton>

SnipOverlay::SnipOverlay(const QPixmap &screenshot, std::function<void(const QPixmap &)> onSend, QWidget *parent)
    : QWidget(parent), m_screenshot(screenshot), m_onSend(std::move(onSend)) {
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setCursor(Qt::CrossCursor);

    m_sendButton = new QPushButton(QStringLiteral("Send"), this);
    m_sendButton->hide();
    m_sendButton->setCursor(Qt::PointingHandCursor);
    m_sendButton->setStyleSheet(
        QStringLiteral("background-color: #4CAF50; color: white; padding: 4px 10px; border-radius: 6px;")
    );
    connect(m_sendButton, &QPushButton::clicked, this, &SnipOverlay::sendSelection);
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

void SnipOverlay::mousePressEvent(QMouseEvent *event) {
    if (event->button() != Qt::LeftButton)
        return;

    m_isSelecting = true;
    m_startPoint = event->position().toPoint();
    m_endPoint = m_startPoint;
    m_selectionRect = {};
    m_sendButton->hide();
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
        m_sendButton->hide();
        update();
        return;
    }

    placeSendButton();
    m_sendButton->show();
    m_sendButton->raise();
    update();
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
    painter.drawPixmap(rect(), m_screenshot);
    painter.fillRect(rect(), QColor(0, 0, 0, 60));

    if (!m_selectionRect.isNull()) {
        const QRect screenshotRect = selectionToScreenshotRect();
        if (!screenshotRect.isNull()) {
            const QPixmap cropped = m_screenshot.copy(screenshotRect);
            painter.drawPixmap(m_selectionRect, cropped);
        }

        QPen pen(QColor(80, 190, 255), 2);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(m_selectionRect);
    }
}

void SnipOverlay::placeSendButton() {
    const QSize btn = m_sendButton->sizeHint();
    const int pad = 8;

    int x = m_selectionRect.right() - btn.width() - pad;
    int y = m_selectionRect.bottom() - btn.height() - pad;

    x = qMax(m_selectionRect.left() + pad, x);
    y = qMax(m_selectionRect.top() + pad, y);

    x = qBound(0, x, width() - btn.width());
    y = qBound(0, y, height() - btn.height());

    m_sendButton->setGeometry(x, y, btn.width(), btn.height());
}

void SnipOverlay::sendSelection() {
    if (m_selectionRect.isNull())
        return;

    const QRect screenshotRect = selectionToScreenshotRect();
    if (screenshotRect.isNull())
        return;

    const QPixmap cropped = m_screenshot.copy(screenshotRect);
    close();
    if (m_onSend)
        m_onSend(cropped);
}
