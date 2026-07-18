#pragma once

#include "plugins/misc/lib/ToolBase.hpp"

#include <QPixmap>
#include <QPoint>
#include <QRect>
#include <QWidget>

class ShaderWidget final : public QWidget, public ToolBase {
    Q_OBJECT

public:
    explicit ShaderWidget(QWidget *parent = nullptr);

    QString label() const override { return QStringLiteral("Shader"); }
    void start(QuolServices *services) override;
    void stop() override;
    QWidget *widget() override;

public slots:
    void openSettings();

signals:
    void closed();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private:
    enum class Edge { None, Left, Right, Top, Bottom, TopLeft, TopRight, BottomLeft, BottomRight };

    Edge edgeAtPos(const QPoint &pos) const;
    void applyEdgeCursor(Edge edge);
    void captureBackground();

    QPixmap m_bgCapture;

    // Drag
    QPoint m_dragOffset;
    bool m_dragging = false;

    // Resize
    Edge m_resizeEdge = Edge::None;
    QRect m_resizeStartGeom;
    QPoint m_resizeStartPos;
    bool m_resizing = false;

    // Settings popup (singleton)
    QWidget *m_settingsPopup = nullptr;

    static constexpr int kHandleMargin = 8;
    static constexpr int kMinSize = 60;
};
