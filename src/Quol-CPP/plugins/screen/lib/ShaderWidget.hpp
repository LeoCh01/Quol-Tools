#pragma once

#include <QImage>
#include <QPainter>
#include <QPlainTextEdit>
#include <QPoint>
#include <QPointF>
#include <QRect>
#include <QString>
#include <QTextBlock>
#include <QTimer>
#include <QWidget>

class LineNumberArea;
class QuolServices;

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

class ShaderWidget final : public QWidget {
    Q_OBJECT

public:
    explicit ShaderWidget(QWidget *parent = nullptr);
    ~ShaderWidget() override;

    void start(QuolServices *services);
    void stop();
    void setPluginRootPath(const QString &path) { m_pluginRootPath = path; }

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
    QPointF physMouse() const;
    void releaseGles();
    static bool renderShader(const QImage &source, const QString &fragSrc, float time, const QPointF &mousePos, GLCache *cache, QImage &out, QString *errorLog = nullptr);
    void onAnimTick();

    QImage m_rawCapture;
    QImage m_bgImage;
    QString m_shaderSource;
    QString m_pluginRootPath;
    qreal m_captureDpr = 1.0;
    QPointF m_mousePos;

    QTimer *m_animTimer = nullptr;
    float m_animTime = 0.0f;
    GLCache *m_gl = nullptr;

    QPoint m_dragOffset;
    bool m_dragging = false;

    Edge m_resizeEdge = Edge::None;
    QRect m_resizeStartGeom;
    QPoint m_resizeStartPos;
    bool m_resizing = false;

    QWidget *m_settingsPopup = nullptr;

    static constexpr int kHandleMargin = 8;
    static constexpr int kMinSize = 60;
};
