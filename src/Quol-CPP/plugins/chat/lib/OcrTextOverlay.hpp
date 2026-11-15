#pragma once

#include <QPixmap>
#include <QPoint>
#include <QRect>
#include <QSet>
#include <QString>
#include <QWidget>
#include <QVector>

class QLabel;
class QPushButton;

// Full-screen overlay that keeps the screenshot visible after an Extract and lets the
// user select recognized words in the snipped region, Snipping Tool "text actions" style.
class OcrTextOverlay final : public QWidget {
    Q_OBJECT

public:
    struct Word {
        QString text;
        int line = 0;
        QRect rect;  // screenshot pixel coordinates
    };

    explicit OcrTextOverlay(const QPixmap &screenshot, const QRect &cropRect, QWidget *parent = nullptr);

    void setResult(const QVector<Word> &words);
    void setFailed(const QString &message);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    QRectF cropWidgetRect() const;
    QRect wordWidgetRect(int index) const;
    int wordAt(const QPoint &pos) const;
    void applySelection();
    void selectAll();
    void clearSelection();
    void refreshStatus();
    QString selectedText() const;
    void copySelection();
    void placeToolbar();
    void positionTip();

    QPixmap m_screenshot;
    QRect m_cropRect;  // screenshot pixel coordinates
    QVector<Word> m_words;
    QSet<int> m_selected;
    QSet<int> m_pressedSelection;  // selection when the current drag started (shift = additive)
    int m_hoverIndex = -1;

    QPoint m_startPoint;
    QRect m_selectionRect;  // widget coordinates
    bool m_isDragging = false;

    QWidget *m_toolbar = nullptr;
    QLabel *m_statusLabel = nullptr;
    QPushButton *m_copyButton = nullptr;
    QPushButton *m_cancelButton = nullptr;
    QLabel *m_tipLabel = nullptr;
};
