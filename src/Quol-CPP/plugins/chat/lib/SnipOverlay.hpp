#pragma once

#include <QPoint>
#include <QRect>
#include <QWidget>
#include <functional>

class QPushButton;
class QPixmap;

class SnipOverlay final : public QWidget {
    Q_OBJECT

public:
    explicit SnipOverlay(
        const QPixmap &screenshot, std::function<void(const QPixmap &)> onSend, QWidget *parent = nullptr
    );

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    QRect selectionToScreenshotRect() const;
    void placeSendButton();
    void sendSelection();

    QPixmap m_screenshot;
    std::function<void(const QPixmap &)> m_onSend;

    QPoint m_startPoint;
    QPoint m_endPoint;
    QRect m_selectionRect;
    bool m_isSelecting = false;

    QPushButton *m_sendButton = nullptr;
};
