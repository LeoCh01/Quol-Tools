#include "plugins/screen/lib/SnipItem.hpp"

#include <QGuiApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QScreen>
#include <QtMath>

SnipItem::SnipItem(const QPixmap &pixmap, QWidget *parent) : QWidget(parent), m_pixmap(pixmap) {
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setMouseTracking(true);
    setMinimumSize(kMinSize, kMinSize);

    if (QScreen *screen = QGuiApplication::primaryScreen())
        m_dpr = screen->devicePixelRatio();
    if (m_dpr <= 0.0)
        m_dpr = 1.0;

    resize(originalSize());
}

QSize SnipItem::originalSize() const {
    return {
        qMax(kMinSize, static_cast<int>(m_pixmap.width() / m_dpr)),
        qMax(kMinSize, static_cast<int>(m_pixmap.height() / m_dpr))
    };
}

QSize SnipItem::contentSize() const {
    return m_rotation % 180 == 0 ? size() : size().transposed();
}

void SnipItem::rotate() {
    const QSize cs = contentSize();
    m_rotation = (m_rotation + 90) % 360;
    resize(m_rotation % 180 == 0 ? cs : cs.transposed());
}

void SnipItem::paintEvent(QPaintEvent *) {
    if (m_pixmap.isNull())
        return;

    const QSize cs = contentSize();
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    p.translate(rect().center());
    p.rotate(m_rotation);
    p.drawPixmap(-cs.width() / 2, -cs.height() / 2, cs.width(), cs.height(), m_pixmap);
}

int SnipItem::edgeAt(const QPoint &pos) const {
    int edge = NoEdge;
    if (pos.x() <= kResizeMargin)
        edge |= Left;
    if (pos.x() >= width() - 1 - kResizeMargin)
        edge |= Right;
    if (pos.y() <= kResizeMargin)
        edge |= Top;
    if (pos.y() >= height() - 1 - kResizeMargin)
        edge |= Bottom;
    return edge;
}

void SnipItem::applyEdgeCursor(int edge) {
    const bool horizontal = edge & (Left | Right);
    const bool vertical = edge & (Top | Bottom);

    if (horizontal && vertical) {
        const bool fdiag = (edge & Left && edge & Top) || (edge & Right && edge & Bottom);
        setCursor(fdiag ? Qt::SizeFDiagCursor : Qt::SizeBDiagCursor);
    } else if (horizontal) {
        setCursor(Qt::SizeHorCursor);
    } else if (vertical) {
        setCursor(Qt::SizeVerCursor);
    } else {
        setCursor(Qt::ArrowCursor);
    }
}

QRect SnipItem::resizedRect(const QPoint &delta, bool lockAspect) const {
    int dw = 0;
    int dh = 0;
    if (m_edge & Right)
        dw += delta.x();
    if (m_edge & Left)
        dw -= delta.x();
    if (m_edge & Bottom)
        dh += delta.y();
    if (m_edge & Top)
        dh -= delta.y();

    int w = m_startGeometry.width() + dw;
    int h = m_startGeometry.height() + dh;

    const bool affectsX = m_edge & (Left | Right);

    if (lockAspect) {
        const QSize orig = originalSize();
        const qreal aspect = static_cast<qreal>(orig.width()) / orig.height();
        const qreal widgetAspect = (m_rotation % 180) ? 1.0 / aspect : aspect;
        if (affectsX && (!(m_edge & (Top | Bottom)) || qAbs(dw) >= qAbs(dh)))
            h = qRound(w / widgetAspect);
        else
            w = qRound(h * widgetAspect);
    }

    w = qMax(kMinSize, w);
    h = qMax(kMinSize, h);

    QRect r = m_startGeometry;
    if (m_edge & Left)
        r.setLeft(r.right() - w + 1);
    else if (m_edge & Right)
        r.setRight(r.left() + w - 1);

    if (m_edge & Top)
        r.setTop(r.bottom() - h + 1);
    else if (m_edge & Bottom)
        r.setBottom(r.top() + h - 1);
    else
        r.setHeight(h);

    return r;
}

void SnipItem::mousePressEvent(QMouseEvent *event) {
    if (event->button() != Qt::LeftButton)
        return;

    setFocus();
    m_edge = edgeAt(event->position().toPoint());
    m_startGeometry = geometry();
    if (m_edge) {
        m_mode = Resizing;
        m_dragStart = event->globalPosition().toPoint();
    } else {
        m_mode = Moving;
        m_dragStart = event->globalPosition().toPoint() - pos();
    }
    event->accept();
}

void SnipItem::mouseMoveEvent(QMouseEvent *event) {
    if (m_mode == Moving) {
        move(event->globalPosition().toPoint() - m_dragStart);
    } else if (m_mode == Resizing) {
        const bool lockAspect = (event->modifiers() & Qt::ShiftModifier) != 0;
        setGeometry(resizedRect(event->globalPosition().toPoint() - m_dragStart, lockAspect));
    } else {
        applyEdgeCursor(edgeAt(event->position().toPoint()));
    }
    event->accept();
}

void SnipItem::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() != Qt::LeftButton)
        return;
    m_mode = Idle;
    m_edge = NoEdge;
    event->accept();
}

void SnipItem::keyPressEvent(QKeyEvent *event) {
    switch (event->key()) {
        case Qt::Key_R:
            rotate();
            update();
            break;
        case Qt::Key_Delete:
            emit deleted(this);
            break;
        case Qt::Key_Backspace:
            m_rotation = 0;
            resize(originalSize());
            update();
            break;
        default:
            QWidget::keyPressEvent(event);
            return;
    }
    event->accept();
}