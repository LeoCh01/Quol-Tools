#include "plugins/draw/lib/DrawOverlay.hpp"

#include <QApplication>
#include <QKeyEvent>
#include <QKeySequence>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QScreen>
#include <QShortcut>
#include <cmath>

// ---------------------------------------------------------------------------
// Stroke
// ---------------------------------------------------------------------------

int Stroke::s_nextId = 1;

Stroke::Stroke(const QColor &c, int w) : id(s_nextId++), color(c), width(w) {
}

void Stroke::addPoint(const QPoint &p) {
    if (mode != Mode::Free) {
        // Keep only start + current endpoint while a shape mode is active
        while (points.size() >= 2)
            points.pop_back();
    }
    points.push_back(p);
}

void Stroke::finalize() {
    if (points.size() < 2)
        return;

    const QPoint a = points.front();
    const QPoint b = points.back();

    if (mode == Mode::Line) {
        // Interpolate the straight segment so erase hit-testing works along it
        const double dx = b.x() - a.x();
        const double dy = b.y() - a.y();
        const int steps = static_cast<int>(std::sqrt(dx * dx + dy * dy) / 10.0);

        points.clear();
        points.push_back(a);
        for (int i = 1; i <= steps; ++i) {
            const double t = static_cast<double>(i) / (steps + 1);
            points.push_back(QPoint(static_cast<int>(a.x() + dx * t), static_cast<int>(a.y() + dy * t)));
        }
        points.push_back(b);

    } else if (mode == Mode::Box) {
        // Sample the rectangle perimeter so erase and draw work like any stroke
        const QRect r = QRect(a, b).normalized();
        const int perimeter = 2 * (r.width() + r.height());
        const int steps = perimeter / 10;

        points.clear();
        for (int i = 0; i <= steps; ++i) {
            const double t = static_cast<double>(i) / std::max(steps, 1);
            const double dist = t * perimeter;
            QPoint p;
            if (dist < r.width()) {  // top edge, left → right
                p = QPoint(r.left() + static_cast<int>(dist), r.top());
            } else if (dist < r.width() + r.height()) {  // right edge, top → bottom
                p = QPoint(r.right(), r.top() + static_cast<int>(dist - r.width()));
            } else if (dist < 2 * r.width() + r.height()) {  // bottom edge, right → left
                p = QPoint(r.right() - static_cast<int>(dist - r.width() - r.height()), r.bottom());
            } else {  // left edge, bottom → top
                p = QPoint(r.left(), r.bottom() - static_cast<int>(dist - 2 * r.width() - r.height()));
            }
            points.push_back(p);
        }
    }
}

void Stroke::draw(QPainter &painter) const {
    if (points.empty())
        return;

    const QPen pen(color, width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);

    if (points.size() == 1) {
        painter.save();
        painter.setBrush(color);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(points[0], width / 2, width / 2);
        painter.restore();
        return;
    }

    // In-progress box: two points are opposite corners, not yet finalized
    if (mode == Mode::Box && points.size() == 2) {
        painter.save();
        painter.setPen(pen);
        painter.setBrush(color);
        painter.drawRect(QRect(points[0], points[1]).normalized());
        painter.restore();
        return;
    }

    QPainterPath path;
    path.moveTo(points[0]);
    for (std::size_t i = 1; i < points.size(); ++i)
        path.lineTo(points[i]);

    painter.save();
    painter.setPen(pen);
    // Finalized boxes are closed perimeters, so they fill their interior
    if (mode == Mode::Box)
        painter.setBrush(color);
    painter.drawPath(path);
    painter.restore();
}

// ---------------------------------------------------------------------------
// DrawOverlay
// ---------------------------------------------------------------------------

DrawOverlay::DrawOverlay(QWidget *parent) : QWidget(parent, Qt::Window) {
    setAttribute(Qt::WA_StaticContents);
    setWindowFlags(Qt::FramelessWindowHint);
    setMouseTracking(true);
    setCursor(Qt::CrossCursor);

    auto *escSc = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    connect(escSc, &QShortcut::activated, this, &DrawOverlay::stopRequested);
}

void DrawOverlay::startDrawing(const QPixmap &screenshot) {
    m_screenshot = screenshot;
    m_screenshotImg = screenshot.toImage();
    m_sceneImg = m_screenshotImg;
    QScreen *screen = QApplication::primaryScreen();
    const QRect g = screen->availableGeometry();
    setGeometry(g);
    update();
    show();
}

void DrawOverlay::stopDrawing() {
    hide();
    m_currentIdx = -1;
    m_erasing = false;
    m_lineDown = false;
    m_boxDown = false;
    m_pickDown = false;
    setCursor(Qt::CrossCursor);
}

void DrawOverlay::clearCanvas() {
    m_strokes.clear();
    m_undoStack.clear();
    m_currentIdx = -1;
    rebuildScene();
    update();
}

void DrawOverlay::undo() {
    if (m_undoStack.empty())
        return;

    UndoEntry entry = m_undoStack.back();
    m_undoStack.pop_back();

    if (entry.added) {
        // Find stroke by ID and remove it
        for (int i = static_cast<int>(m_strokes.size()) - 1; i >= 0; --i) {
            if (m_strokes[i].id == entry.stroke.id) {
                if (m_currentIdx == i)
                    m_currentIdx = -1;
                else if (m_currentIdx > i)
                    --m_currentIdx;
                m_strokes.erase(m_strokes.begin() + i);
                break;
            }
        }
    } else {
        // Re-insert the removed stroke
        m_strokes.push_back(entry.stroke);
    }

    rebuildScene();
    update();
}

// ---------------------------------------------------------------------------
// Undo helpers
// ---------------------------------------------------------------------------

void DrawOverlay::pushUndo(const Stroke &s, bool added) {
    if (m_undoStack.size() >= static_cast<std::size_t>(kMaxUndo))
        m_undoStack.pop_front();
    m_undoStack.push_back({s, added});
}

// ---------------------------------------------------------------------------
// Mouse events
// ---------------------------------------------------------------------------

Stroke::Mode DrawOverlay::activeMode() const {
    if (m_boxDown)
        return Stroke::Mode::Box;
    if (m_lineDown)
        return Stroke::Mode::Line;
    return Stroke::Mode::Free;
}

QColor DrawOverlay::pixelAt(const QPoint &pos) const {
    // The scene is in physical pixels; widget/mouse coordinates are logical
    const qreal dpr = m_screenshot.devicePixelRatio();
    const QPoint imgPos = (QPointF(pos) * dpr).toPoint();
    return m_sceneImg.pixelColor(
        std::clamp(imgPos.x(), 0, m_sceneImg.width() - 1),
        std::clamp(imgPos.y(), 0, m_sceneImg.height() - 1)
    );
}

void DrawOverlay::rebuildScene() {
    m_sceneImg = m_screenshotImg;
    if (m_strokes.empty())
        return;
    QPainter p(&m_sceneImg);
    for (const Stroke &s : m_strokes)
        s.draw(p);
}

void DrawOverlay::mousePressEvent(QMouseEvent *e) {
    const QPoint pos = e->position().toPoint();

    if (e->button() == Qt::LeftButton && m_pickDown) {
        // Eyedropper: sample the screenshot under the cursor, no stroke
        emit colorPicked(pixelAt(pos));
        return;
    }

    if (e->button() == Qt::LeftButton) {
        m_strokes.emplace_back(m_penColor, m_penWidth);
        m_currentIdx = static_cast<int>(m_strokes.size()) - 1;
        Stroke &s = m_strokes[m_currentIdx];
        s.mode = activeMode();
        if (s.mode == Stroke::Mode::Box && m_lineDown)
            s.color = Qt::white;  // ctrl+shift draws a white box regardless of pen colour
        s.addPoint(pos);
        update();

    } else if (e->button() == Qt::RightButton) {
        m_erasing = true;
        m_currentIdx = -1;
        setCursor(Qt::BlankCursor);
        eraseAt(pos);
    }
}

void DrawOverlay::mouseMoveEvent(QMouseEvent *e) {
    const QPoint pos = e->position().toPoint();

    if ((e->buttons() & Qt::LeftButton) && m_currentIdx >= 0) {
        m_strokes[m_currentIdx].mode = activeMode();
        m_strokes[m_currentIdx].addPoint(pos);
    } else if (e->buttons() & Qt::RightButton) {
        eraseAt(pos);
    }

    update();
}

void DrawOverlay::mouseReleaseEvent(QMouseEvent *e) {
    if (e->button() == Qt::LeftButton && m_currentIdx >= 0) {
        Stroke &s = m_strokes[m_currentIdx];
        s.finalize();
        // Capture the completed stroke for undo AFTER it is fully drawn
        pushUndo(s, true);
        m_currentIdx = -1;
        rebuildScene();
    } else if (e->button() == Qt::RightButton) {
        m_erasing = false;
        setCursor(Qt::CrossCursor);
        update();
    }
}

// ---------------------------------------------------------------------------
// Paint
// ---------------------------------------------------------------------------

void DrawOverlay::paintEvent(QPaintEvent *) {
    QPainter painter(this);

    painter.drawPixmap(rect(), m_screenshot);

    for (const Stroke &s : m_strokes)
        s.draw(painter);

    if (m_erasing)
        drawEraserCircle(painter);
    else
        drawCursorInfo(painter);
}

// ---------------------------------------------------------------------------
// Keyboard
// ---------------------------------------------------------------------------

void DrawOverlay::keyPressEvent(QKeyEvent *e) {
    if (e->key() == Qt::Key_Escape) {
        emit stopRequested();
        return;
    }
    const int presetIndex = e->key() - Qt::Key_1;
    if (presetIndex >= 0 && presetIndex < m_presetColors.size()
        && m_presetColors[presetIndex].isValid()) {
        emit colorPicked(m_presetColors[presetIndex]);
        return;
    }
    if (setModifierState(e->key(), true))
        update();
    else
        QWidget::keyPressEvent(e);
}

void DrawOverlay::keyReleaseEvent(QKeyEvent *e) {
    if (setModifierState(e->key(), false))
        update();
    else
        QWidget::keyReleaseEvent(e);
}

bool DrawOverlay::setModifierState(int key, bool down) {
    if (key == m_lineKey)
        m_lineDown = down;
    else if (key == m_boxKey)
        m_boxDown = down;
    else if (key == m_pickKey)
        m_pickDown = down;
    else
        return false;
    return true;
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

void DrawOverlay::eraseAt(const QPoint &pos) {
    const double threshold = (3.0 + std::pow(m_penWidth, 0.8)) * kEraserMult;

    for (int i = static_cast<int>(m_strokes.size()) - 1; i >= 0; --i) {
        const Stroke &s = m_strokes[i];
        for (const QPoint &p : s.points) {
            double dx = p.x() - pos.x();
            double dy = p.y() - pos.y();
            if (std::sqrt(dx * dx + dy * dy) <= threshold + s.width / 2.0) {
                pushUndo(m_strokes[i], false);
                m_strokes.erase(m_strokes.begin() + i);
                rebuildScene();
                update();
                return;
            }
        }
    }
}

void DrawOverlay::drawCursorInfo(QPainter &p) {
    const QPoint pos = mapFromGlobal(QCursor::pos());

    QFont f = p.font();
    f.setPointSize(8);

    // Live swatch for the eyedropper: solid true-colour fill with a white outline
    if (m_pickDown) {
        const QColor c = pixelAt(pos);
        const QRect swatch(pos + QPoint(12, 4), QSize(14, 14));
        p.save();
        p.setPen(QPen(QColor(255, 255, 255), 1));
        p.setBrush(c);
        p.drawRect(swatch);
        p.restore();
    }

    p.save();
    p.setCompositionMode(QPainter::CompositionMode_Difference);
    p.setFont(f);
    p.setPen(QColor(255, 255, 255));
    p.drawText(pos + QPoint(12, -8), QStringLiteral("(%1, %2)").arg(pos.x()).arg(pos.y()));
    if (m_pickDown)
        p.drawText(pos + QPoint(32, 15), pixelAt(pos).name().toUpper());
    p.restore();
}

void DrawOverlay::drawEraserCircle(QPainter &p) {
    const double r = (3.0 + std::pow(m_penWidth, 0.8)) * kEraserMult;
    const QPoint pos = mapFromGlobal(QCursor::pos());

    p.save();
    p.setCompositionMode(QPainter::CompositionMode_Difference);
    p.setPen(QPen(QColor(255, 255, 255), 2));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(pos, static_cast<int>(r), static_cast<int>(r));
    p.restore();
}
