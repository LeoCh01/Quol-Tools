#include "plugins/chat/lib/SnipOverlay.hpp"

#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QPropertyAnimation>
#include <QPushButton>

namespace {
const QColor kAccent(86, 156, 255, 235);
const QColor kDim(0, 0, 0, 90);

QString buttonStyle(const QString &background, const QString &hover, bool darkText = false) {
    const QString color = darkText ? QStringLiteral("#1c1c1e") : QStringLiteral("white");
    return QStringLiteral(
               "QPushButton { background:%1; color:%2; border:none; padding:6px 16px;"
               " border-radius:6px; font-size:12px; font-weight:600; }"
               "QPushButton:hover { background:%3; }"
               "QPushButton:pressed { opacity:0.85; }")
        .arg(background, color, hover);
}
}  // namespace

SnipOverlay::SnipOverlay(const QPixmap &screenshot, std::function<void(const QPixmap &)> onSend,
                         const QString &buttonLabel, QWidget *parent)
    : QWidget(parent), m_screenshot(screenshot), m_onSend(std::move(onSend)) {
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setCursor(Qt::CrossCursor);
    setWindowOpacity(0.0);
    setAttribute(Qt::WA_DeleteOnClose);

    m_toolbar = new QWidget(this);
    m_toolbar->setObjectName(QStringLiteral("snip-toolbar"));
    m_toolbar->setStyleSheet(QStringLiteral(
        "#snip-toolbar { background: rgba(24,24,26,0.92); border: 1px solid rgba(255,255,255,0.12);"
        " border-radius: 10px; }"
        "QLabel { color: #e6e6e6; font-size: 11px; padding-left: 4px; }"
    ));
    m_toolbar->hide();

    auto *toolbarLayout = new QHBoxLayout(m_toolbar);
    toolbarLayout->setContentsMargins(12, 6, 8, 6);
    toolbarLayout->setSpacing(8);

    m_sizeLabel = new QLabel(QStringLiteral("0 × 0 px"), m_toolbar);
    m_sizeLabel->setAlignment(Qt::AlignCenter);
    toolbarLayout->addWidget(m_sizeLabel);

m_sendButton = new QPushButton(buttonLabel, m_toolbar);
    m_sendButton->setCursor(Qt::PointingHandCursor);
    m_sendButton->setStyleSheet(buttonStyle(QStringLiteral("#2e7d32"), QStringLiteral("#388e3c")));
    toolbarLayout->addWidget(m_sendButton);

    m_cancelButton = new QPushButton(QStringLiteral("Cancel"), m_toolbar);
    m_cancelButton->setCursor(Qt::PointingHandCursor);
    m_cancelButton->setStyleSheet(buttonStyle(QStringLiteral("#3a3a3e"), QStringLiteral("#4a4a4f")));
    toolbarLayout->addWidget(m_cancelButton);

    connect(m_sendButton, &QPushButton::clicked, this, &SnipOverlay::sendSelection);
    connect(m_cancelButton, &QPushButton::clicked, this, &SnipOverlay::close);

    m_tipLabel = new QLabel(QStringLiteral("Drag to select area  ·  Esc to cancel"), this);
    m_tipLabel->setStyleSheet(QStringLiteral(
        "QLabel { background: rgba(24,24,26,0.85); color: #dcdcdc; border: 1px solid rgba(255,255,255,0.12);"
        " border-radius: 8px; padding: 8px 18px; font-size: 12px; }"
    ));
    m_tipLabel->setCursor(Qt::CrossCursor);
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
    positionTip();

    QPropertyAnimation *fade = new QPropertyAnimation(this, "windowOpacity", this);
    fade->setDuration(160);
    fade->setStartValue(0.0);
    fade->setEndValue(1.0);
    fade->start(QAbstractAnimation::DeleteWhenStopped);
}

void SnipOverlay::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    positionTip();
}

void SnipOverlay::positionTip() {
    m_tipLabel->adjustSize();
    m_tipLabel->move((width() - m_tipLabel->width()) / 2, 26);
    m_tipLabel->raise();
}

void SnipOverlay::mousePressEvent(QMouseEvent *event) {
    if (event->button() != Qt::LeftButton)
        return;

    m_isSelecting = true;
    m_startPoint = event->position().toPoint();
    m_endPoint = m_startPoint;
    m_selectionRect = {};
    m_toolbar->hide();
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
        m_toolbar->hide();
        update();
        return;
    }

    placeToolbar();
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

void SnipOverlay::placeToolbar() {
    m_sizeLabel->setText(QStringLiteral("%1 × %2 px").arg(m_selectionRect.width()).arg(m_selectionRect.height()));

    m_toolbar->adjustSize();
    const int pad = 14;
    int x = m_selectionRect.right() - m_toolbar->width() + pad;
    int y = m_selectionRect.bottom() + 10;

    x = qBound(0, x, width() - m_toolbar->width());
    y = qBound(0, y, height() - m_toolbar->height());

    m_toolbar->move(x, y);
    m_toolbar->show();
    m_toolbar->raise();
    m_tipLabel->raise();
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