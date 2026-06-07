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
    if (snapActive) {
        // Keep only start + current endpoint while snap is active
        while (points.size() >= 2)
            points.pop_back();
    }
    points.push_back(p);
}

void Stroke::finalizeSnap() {
    if (!snapActive || points.size() < 2)
        return;

    snapActive = false;

    QPoint a = points.front();
    QPoint b = points.back();
    double dx = b.x() - a.x();
    double dy = b.y() - a.y();
    int steps = static_cast<int>(std::sqrt(dx * dx + dy * dy) / 10.0);

    points.clear();
    points.push_back(a);
    for (int i = 1; i <= steps; ++i) {
        double t = static_cast<double>(i) / (steps + 1);
        points.push_back(QPoint(static_cast<int>(a.x() + dx * t), static_cast<int>(a.y() + dy * t)));
    }
    points.push_back(b);
}

void Stroke::draw(QPainter &painter) const {
    if (points.empty())
        return;

    if (points.size() == 1) {
        painter.save();
        painter.setBrush(color);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(points[0], width / 2, width / 2);
        painter.restore();
        return;
    }

    QPainterPath path;
    path.moveTo(points[0]);
    for (std::size_t i = 1; i < points.size(); ++i)
        path.lineTo(points[i]);

    QPen pen(color, width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter.save();
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
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
    setCursor(Qt::CrossCursor);
}

void DrawOverlay::clearCanvas() {
    m_strokes.clear();
    m_undoStack.clear();
    m_currentIdx = -1;
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

    update();
}

// ---------------------------------------------------------------------------
// Undo helpers
// ---------------------------------------------------------------------------

void DrawOverlay::pushUndoAdd(const Stroke &s) {
    if (m_undoStack.size() >= static_cast<std::size_t>(kMaxUndo))
        m_undoStack.pop_front();
    m_undoStack.push_back({s, true});
}

void DrawOverlay::pushUndoRemove(const Stroke &s) {
    if (m_undoStack.size() >= static_cast<std::size_t>(kMaxUndo))
        m_undoStack.pop_front();
    m_undoStack.push_back({s, false});
}

// ---------------------------------------------------------------------------
// Mouse events
// ---------------------------------------------------------------------------

void DrawOverlay::mousePressEvent(QMouseEvent *e) {
    const QPoint pos = e->position().toPoint();

    if (e->button() == Qt::LeftButton) {
        m_strokes.emplace_back(m_penColor, m_penWidth);
        m_currentIdx = static_cast<int>(m_strokes.size()) - 1;
        m_strokes[m_currentIdx].snapActive = m_ctrl;
        m_strokes[m_currentIdx].addPoint(pos);
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
        m_strokes[m_currentIdx].snapActive = m_ctrl;
        m_strokes[m_currentIdx].addPoint(pos);
    } else if (e->buttons() & Qt::RightButton) {
        eraseAt(pos);
    }

    update();
}

void DrawOverlay::mouseReleaseEvent(QMouseEvent *e) {
    if (e->button() == Qt::LeftButton && m_currentIdx >= 0) {
        Stroke &s = m_strokes[m_currentIdx];
        s.finalizeSnap();
        // Capture the completed stroke for undo AFTER it is fully drawn
        pushUndoAdd(s);
        m_currentIdx = -1;
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
    if (e->key() == Qt::Key_Control)
        m_ctrl = true;
    else if (e->key() == Qt::Key_Escape)
        emit stopRequested();
    else
        QWidget::keyPressEvent(e);
}

void DrawOverlay::keyReleaseEvent(QKeyEvent *e) {
    if (e->key() == Qt::Key_Control)
        m_ctrl = false;
    else
        QWidget::keyReleaseEvent(e);
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
                pushUndoRemove(m_strokes[i]);
                m_strokes.erase(m_strokes.begin() + i);
                update();
                return;
            }
        }
    }
}

void DrawOverlay::drawCursorInfo(QPainter &p) {
    const QPoint pos = mapFromGlobal(QCursor::pos());
    const QString txt = QStringLiteral("(%1, %2)").arg(pos.x()).arg(pos.y());

    p.save();
    p.setCompositionMode(QPainter::CompositionMode_Difference);
    QFont f = p.font();
    f.setPointSize(8);
    p.setFont(f);
    p.setPen(QColor(255, 255, 255));
    p.drawText(pos + QPoint(12, -8), txt);
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

// ---------------------------------------------------------------------------
// Stroke
