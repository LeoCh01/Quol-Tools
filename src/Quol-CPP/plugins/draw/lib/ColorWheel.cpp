#include "plugins/draw/lib/ColorWheel.hpp"

#include <QImage>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <algorithm>
#include <cmath>

ColorWheel::ColorWheel(int circleRadius, int squareSize, int thickness, QWidget *parent)
    : QWidget(parent), m_circleRadius(circleRadius), m_thickness(thickness) {
    // Fit the SV square inside the inner edge of the ring
    const int innerRadius = m_circleRadius - m_thickness;
    const int autoSquare = static_cast<int>(innerRadius / 1.2);
    m_squareHalf = (squareSize > 0) ? squareSize / 2 : autoSquare;
    const int side = circleRadius * 2 + 16;
    setFixedSize(side, side);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setCursor(Qt::CrossCursor);
}

QColor ColorWheel::color() const {
    return QColor::fromHsv(m_hue, qRound(m_saturation * 255), qRound(m_value * 255));
}

void ColorWheel::setColor(const QColor &c, bool emitSignal) {
    if (!c.isValid())
        return;
    int h, s, v, a;
    c.getHsv(&h, &s, &v, &a);
    if (h != -1)
        m_hue = h;  // -1 for achromatic colours
    m_saturation = s / 255.0;
    m_value = v / 255.0;
    update();
    if (emitSignal)
        emit colorChanged(color());
}

// ---------------------------------------------------------------------------
// Paint
// ---------------------------------------------------------------------------

void ColorWheel::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QPoint c = center();
    const int r = ringRadius();
    const int sh = squareHalf();
    const QRectF ring(c.x() - r, c.y() - r, r * 2, r * 2);

    // ── Hue ring ──────────────────────────────────────────────────────────
    QPen pen;
    pen.setWidth(m_thickness);
    for (int deg = 0; deg < 360; ++deg) {
        pen.setColor(QColor::fromHsv(deg, 255, 255));
        p.setPen(pen);
        // Qt arc: angle in 1/16 degree, counter-clockwise, 0 = 3 o'clock
        p.drawArc(ring, deg * 16, 16);
    }

    // ── SV square ─────────────────────────────────────────────────────────
    // Build with a QImage for pixel accuracy
    const int sz = sh * 2;
    if (sz > 0) {
        QImage img(sz, sz, QImage::Format_RGB32);
        for (int x = 0; x < sz; ++x) {
            for (int y = 0; y < sz; ++y) {
                double s = double(x) / sz;
                double v = 1.0 - double(y) / sz;
                img.setPixel(x, y, QColor::fromHsv(m_hue, qRound(s * 255), qRound(v * 255)).rgb());
            }
        }
        p.drawImage(c.x() - sh, c.y() - sh, img);
    }

    // ── Indicators ────────────────────────────────────────────────────────
    p.setBrush(Qt::NoBrush);

    // Hue indicator on ring
    {
        const double rad = qDegreesToRadians(double(m_hue));
        const int ix = qRound(c.x() + r * std::cos(rad));
        const int iy = qRound(c.y() - r * std::sin(rad));
        p.setPen(QPen(Qt::white, 2));
        p.drawEllipse(QPoint(ix, iy), 5, 5);
    }

    // SV indicator inside square
    {
        const int ix = qRound(c.x() - sh + m_saturation * sz);
        const int iy = qRound(c.y() - sh + (1.0 - m_value) * sz);
        p.setPen(QPen(Qt::white, 2));
        p.drawEllipse(QPoint(ix, iy), 5, 5);
    }
}

// ---------------------------------------------------------------------------
// Mouse
// ---------------------------------------------------------------------------

void ColorWheel::mousePressEvent(QMouseEvent *e) {
    if (e->button() != Qt::LeftButton)
        return;

    const QPoint c = center();
    const double dx = e->position().x() - c.x();
    const double dy = e->position().y() - c.y();
    const double dist = std::hypot(dx, dy);
    const int r = ringRadius();

    // Decide whether dragging the ring or the square
    m_onRing = (dist >= r - m_thickness - 2 && dist <= r + 4);
    m_dragging = true;
    handleMouse(e->position());
}

void ColorWheel::mouseMoveEvent(QMouseEvent *e) {
    if (m_dragging)
        handleMouse(e->position());
}

void ColorWheel::mouseReleaseEvent(QMouseEvent *e) {
    if (e->button() == Qt::LeftButton)
        m_dragging = false;
}

void ColorWheel::handleMouse(const QPointF &pos) {
    const QPoint c = center();
    const double dx = pos.x() - c.x();
    const double dy = pos.y() - c.y();

    if (m_onRing) {
        double angle = std::fmod(qRadiansToDegrees(std::atan2(-dy, dx)), 360.0);
        if (angle < 0)
            angle += 360.0;
        m_hue = qRound(angle);
    } else {
        const int sh = squareHalf();
        const int sz = sh * 2;
        if (sz <= 0)
            return;
        double x = pos.x() - (c.x() - sh);
        double y = pos.y() - (c.y() - sh);
        m_saturation = qBound(0.0, x / sz, 1.0);
        m_value = qBound(0.0, 1.0 - y / sz, 1.0);
    }

    update();
    emitColor();
}

void ColorWheel::emitColor() {
    emit colorChanged(color());
}
