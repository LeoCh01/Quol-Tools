#pragma once

#include <QColor>
#include <QWidget>

// HSV colour wheel: outer hue ring + inner saturation/value square.
// Emits colorChanged whenever the user adjusts the colour.
class ColorWheel : public QWidget {
    Q_OBJECT

public:
    // circleRadius: outer radius of the hue ring (px)
    // squareSize:   side length of the SV square (px); -1 = auto-fit inside the ring
    // thickness:    width of the hue ring (px)
    explicit ColorWheel(int circleRadius = 45, int squareSize = -1, int thickness = 14, QWidget *parent = nullptr);

    QColor color() const;
    void setColor(const QColor &color, bool emitSignal = false);

signals:
    void colorChanged(const QColor &color);

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;

private:
    void handleMouse(const QPointF &pos);
    void emitColor();
    QPoint center() const {
        return {width() / 2, height() / 2};
    }
    int ringRadius() const {
        return m_circleRadius;
    }
    int squareHalf() const {
        return m_squareHalf;
    }

    int m_hue = 0;
    double m_saturation = 1.0;
    double m_value = 1.0;

    int m_circleRadius;
    int m_squareHalf;
    int m_thickness;
    bool m_onRing = false;  // which region the current drag is in
    bool m_dragging = false;
};
