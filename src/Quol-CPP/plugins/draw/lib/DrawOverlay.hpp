#pragma once

#include <QColor>
#include <QList>
#include <QPixmap>
#include <QPoint>
#include <QWidget>
#include <deque>
#include <vector>

// ---------------------------------------------------------------------------
// Stroke — one continuous drawn line, dot, or rectangle
// ---------------------------------------------------------------------------
struct Stroke {
    enum class Mode { Free, Line, Box };

    int id = 0;  // unique per-stroke ID for undo matching
    QColor color;
    int width = 2;
    Mode mode = Mode::Free;
    std::vector<QPoint> points;

    Stroke() = default;
    Stroke(const QColor &c, int w);

    void addPoint(const QPoint &p);
    void finalize();  // densify constrained shapes on release
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

    // Which modifier keys (if held) constrain a drag or pick a colour.
    // Qt::Key_unknown disables a feature.
    void setModifierKeys(Qt::Key line, Qt::Key box, Qt::Key pick) {
        m_lineKey = line;
        m_boxKey = box;
        m_pickKey = pick;
    }

    // Colours selectable with keys 1..N while the overlay is open.
    void setPresetColors(const QList<QColor> &colors) {
        m_presetColors = colors;
    }

signals:
    void stopRequested();  // emitted on Escape
    void colorPicked(const QColor &color);

protected:
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void paintEvent(QPaintEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;
    void keyReleaseEvent(QKeyEvent *e) override;

private:
    Stroke::Mode activeMode() const;
    bool setModifierState(int key, bool down);
    QColor pixelAt(const QPoint &pos) const;
    void rebuildScene();
    void eraseAt(const QPoint &pos);
    void drawCursorInfo(QPainter &p);
    void drawEraserCircle(QPainter &p);
    void pushUndo(const Stroke &s, bool added);

    std::vector<Stroke> m_strokes;
    QPixmap m_screenshot;
    QImage m_screenshotImg;  // base layer for the scene composite
    QImage m_sceneImg;       // screenshot + strokes; what the eyedropper samples

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

    Qt::Key m_lineKey = Qt::Key_Shift;
    Qt::Key m_boxKey = Qt::Key_Control;
    Qt::Key m_pickKey = Qt::Key_Alt;
    QList<QColor> m_presetColors;
    bool m_lineDown = false;
    bool m_boxDown = false;
    bool m_pickDown = false;

    static constexpr int kMaxUndo = 30;
    static constexpr double kEraserMult = 3.0;
};
