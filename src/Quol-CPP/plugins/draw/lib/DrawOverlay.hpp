#pragma once

#include <QColor>
#include <QPixmap>
#include <QPoint>
#include <QWidget>
#include <deque>
#include <vector>

// ---------------------------------------------------------------------------
// Stroke — one continuous drawn line or a single dot
// ---------------------------------------------------------------------------
struct Stroke {
    int id = 0;  // unique per-stroke ID for undo matching
    QColor color;
    int width = 2;
    std::vector<QPoint> points;
    bool snapActive = false;

    Stroke() = default;
    Stroke(const QColor &c, int w);

    void addPoint(const QPoint &p);
    void finalizeSnap();  // interpolate snap line on release
    void draw(QPainter &painter) const;

private:
    static int s_nextId;
};

// ---------------------------------------------------------------------------
// DrawOverlay — frameless full-screen drawing canvas
// ---------------------------------------------------------------------------
class DrawOverlay : public QWidget {
    Q_OBJECT

public:
    explicit DrawOverlay(QWidget *parent = nullptr);

    // Hide other windows, grab screenshot, then call this.
    void startDrawing(const QPixmap &screenshot);
    void stopDrawing();

    void clearCanvas();
    void undo();

    void setPenColor(const QColor &c) {
        m_penColor = c;
    }
    void setPenWidth(int w) {
        m_penWidth = w;
    }

signals:
    void stopRequested();  // emitted on Escape

protected:
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void paintEvent(QPaintEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;
    void keyReleaseEvent(QKeyEvent *e) override;

private:
    void eraseAt(const QPoint &pos);
    void drawCursorInfo(QPainter &p);
    void drawEraserCircle(QPainter &p);
    void pushUndoAdd(const Stroke &s);
    void pushUndoRemove(const Stroke &s);

    std::vector<Stroke> m_strokes;
    QPixmap m_screenshot;

    // Undo: full stroke copy + flag (true = was added, false = was removed)
    struct UndoEntry {
        Stroke stroke;
        bool added;
    };
    std::deque<UndoEntry> m_undoStack;

    // Index of the in-progress stroke (-1 = none)
    int m_currentIdx = -1;

    QColor m_penColor = Qt::red;
    int m_penWidth = 2;
    bool m_erasing = false;
    bool m_ctrl = false;

    static constexpr int kMaxUndo = 30;
    static constexpr double kEraserMult = 3.0;
};
