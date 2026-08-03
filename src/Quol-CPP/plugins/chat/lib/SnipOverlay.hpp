#pragma once

#include <QPoint>
#include <QRect>
#include <QString>
#include <QWidget>
#include <functional>

class QLabel;
class QPushButton;
class QPixmap;

class SnipOverlay final : public QWidget {
    Q_OBJECT

public:
    explicit SnipOverlay(
        const QPixmap &screenshot,
        std::function<void(const QPixmap &)> onSend,
        const QString &buttonLabel = QStringLiteral("Send"),
        QWidget *parent = nullptr
    );

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    QRect selectionToScreenshotRect() const;
    void placeToolbar();
    void positionTip();
    void sendSelection();

    QPixmap m_screenshot;
    std::function<void(const QPixmap &)> m_onSend;

    QPoint m_startPoint;
    QPoint m_endPoint;
    QRect m_selectionRect;
    bool m_isSelecting = false;

    QPushButton *m_sendButton = nullptr;
    QPushButton *m_cancelButton = nullptr;
    QWidget *m_toolbar = nullptr;
    QLabel *m_sizeLabel = nullptr;
    QLabel *m_tipLabel = nullptr;
};