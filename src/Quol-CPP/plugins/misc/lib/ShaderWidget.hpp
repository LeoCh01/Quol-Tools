#pragma once

#include "plugins/misc/lib/ToolBase.hpp"

#include <QImage>
#include <QPainter>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QPoint>
#include <QRect>
#include <QString>
#include <QTextBlock>
#include <QTimer>
#include <QWidget>

// ---------------------------------------------------------------------------
// Line-numbered shader editor widget
// ---------------------------------------------------------------------------

class LineNumberArea;

class ShaderEditor : public QPlainTextEdit {
    Q_OBJECT

public:
    explicit ShaderEditor(QWidget *parent = nullptr);
    void lineNumberAreaPaintEvent(QPaintEvent *event);
    int lineNumberAreaWidth() const;

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void updateLineNumberArea(const QRect &rect, int dy);

private:
    QWidget *m_lineNumberArea = nullptr;
};

class LineNumberArea : public QWidget {
public:
    explicit LineNumberArea(ShaderEditor *editor);
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    ShaderEditor *m_editor = nullptr;
};

// ---------------------------------------------------------------------------
// Shader tool widget
// ---------------------------------------------------------------------------

class ShaderWidget final : public QWidget, public ToolBase {
    Q_OBJECT

public:
    explicit ShaderWidget(QWidget *parent = nullptr);
    ~ShaderWidget() override;

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
    struct GLCache;

    enum class Edge { None, Left, Right, Top, Bottom, TopLeft, TopRight, BottomLeft, BottomRight };

    Edge edgeAtPos(const QPoint &pos) const;
    void applyEdgeCursor(Edge edge);
    void captureBackground();
    void applyShaderToCapture();
    static QImage renderShader(const QImage &source, const QString &fragSrc, float time, GLCache *cache, QString *errorLog = nullptr);
    void onAnimTick();

    QImage m_rawCapture;
    QPixmap m_bgCapture;
    QString m_shaderSource;
    qreal m_captureDpr = 1.0;

    QTimer *m_animTimer = nullptr;
    float m_animTime = 0.0f;
    GLCache *m_gl = nullptr;

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
