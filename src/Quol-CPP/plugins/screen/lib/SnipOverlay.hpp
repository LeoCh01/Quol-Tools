#pragma once

#include <QPoint>
#include <QRect>
#include <QString>
#include <QWidget>

class QPixmap;

class SnipOverlay final : public QWidget {
    Q_OBJECT

public:
    explicit SnipOverlay(const QPixmap &screenshot, QWidget *parent = nullptr);

signals:
    void snipReady(const QPixmap &cropped);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    QRect selectionToScreenshotRect() const;

    QPixmap m_screenshot;

    QPoint m_startPoint;
    QPoint m_endPoint;
    QRect m_selectionRect;
    bool m_isSelecting = false;
};
