#pragma once

#include <QPixmap>
#include <QPoint>
#include <QSize>
#include <QWidget>

// A snipped image kept on screen as its own always-on-top window.
// Click to focus, drag to move, drag an edge/corner to resize (Shift locks
// aspect), R to rotate 90 degrees, Backspace to reset, Del to delete.
class SnipItem final : public QWidget {
    Q_OBJECT

public:
    explicit SnipItem(const QPixmap &pixmap, QWidget *parent = nullptr);

signals:
    void deleted(SnipItem *item);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    enum Mode { Idle, Moving, Resizing };
    // Edge is a bit set so corners are just two flags.
    enum Edge : int { NoEdge = 0, Left = 1, Right = 2, Top = 4, Bottom = 8 };

    [[nodiscard]] int edgeAt(const QPoint &pos) const;
    void applyEdgeCursor(int edge);
    [[nodiscard]] QRect resizedRect(const QPoint &delta, bool lockAspect) const;

    // Rotation is always a multiple of 90, so the widget holds the image's
    // bounding box: axis-aligned content unless the item is on its side.
    [[nodiscard]] QSize contentSize() const;
    [[nodiscard]] QSize originalSize() const;
    void rotate();

    QPixmap m_pixmap;
    qreal m_dpr = 1.0;
    int m_rotation = 0;

    Mode m_mode = Idle;
    int m_edge = NoEdge;
    QPoint m_dragStart;
    QRect m_startGeometry;

    static constexpr int kResizeMargin = 8;
    static constexpr int kMinSize = 20;
};