#include "plugins/misc/lib/ShaderWidget.hpp"
#include "ui/QuolPopupWindow.hpp"

#include <cstring>

#include <QCloseEvent>
#include <QFile>
#include <QFileDialog>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QOffscreenSurface>
#include <QOpenGLExtraFunctions>
#include <QOpenGLFramebufferObject>
#include <QOpenGLShaderProgram>
#include <QPainter>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScreen>
#include <QSurfaceFormat>
#include <QTextBlock>
#include <QVBoxLayout>
#include <QVector2D>

#include <memory>

// ---------------------------------------------------------------------------
// RAII wrappers for raw GL handles. They call glDelete* on the *current*
// GL context, so they must only be destroyed while a context is current.
// ---------------------------------------------------------------------------

namespace gl {

class Texture {
public:
    Texture() = default;
    ~Texture() { release(); }

    Texture(const Texture &) = delete;
    Texture &operator=(const Texture &) = delete;

    void create() {
        release();
        QOpenGLContext::currentContext()->extraFunctions()->glGenTextures(1, &m_id);
    }

    void release() {
        if (QOpenGLContext *ctx = QOpenGLContext::currentContext()) {
            if (m_id != 0) {
                ctx->extraFunctions()->glDeleteTextures(1, &m_id);
                m_id = 0;
            }
        }
    }

    unsigned int id() const { return m_id; }

private:
    unsigned int m_id = 0;
};

class Quad {
public:
    Quad() = default;
    ~Quad() { release(); }

    Quad(const Quad &) = delete;
    Quad &operator=(const Quad &) = delete;

    void create() {
        release();
        auto *f = QOpenGLContext::currentContext()->extraFunctions();
        f->glGenVertexArrays(1, &m_vao);
        f->glGenBuffers(1, &m_vbo);
        f->glGenBuffers(1, &m_ebo);
    }

    void release() {
        if (QOpenGLContext *ctx = QOpenGLContext::currentContext()) {
            if (m_vao != 0) {
                auto *f = ctx->extraFunctions();
                f->glDeleteVertexArrays(1, &m_vao);
                f->glDeleteBuffers(1, &m_vbo);
                f->glDeleteBuffers(1, &m_ebo);
                m_vao = m_vbo = m_ebo = 0;
            }
        }
    }

    unsigned int vao() const { return m_vao; }
    unsigned int vbo() const { return m_vbo; }
    unsigned int ebo() const { return m_ebo; }

private:
    unsigned int m_vao = 0;
    unsigned int m_vbo = 0;
    unsigned int m_ebo = 0;
};

}  // namespace gl

// ---------------------------------------------------------------------------
// Default (passthrough) shader shown when no shader is applied
// ---------------------------------------------------------------------------

const QString kDefaultShaderSrc = QStringLiteral(
    "#version 330 core\n"
    "in vec2 v_texCoord;\n"
    "out vec4 fragColor;\n"
    "uniform sampler2D u_texture;\n"
    "uniform sampler2D u_prevFrame;\n"
    "uniform float u_time;\n"
    "uniform vec2  u_resolution;\n"
    "uniform vec2  u_mouse;\n"
    "\n"
    "void main() {\n"
    "    vec4 color = texture(u_texture, v_texCoord);\n"
    "    fragColor = color;\n"
    "}\n"
);

namespace {
const QString kLogNeutral = QStringLiteral("#888888");
const QString kLogOk = QStringLiteral("#98C379");
const QString kLogWarn = QStringLiteral("#E5C07B");
const QString kLogError = QStringLiteral("#E06C75");

void setStatusLog(QPlainTextEdit *log, const QString &color, const QString &text) {
    log->setStyleSheet(QStringLiteral(
        "QPlainTextEdit { background: #1E1E1E; color: %1; font-family: 'Consolas', monospace;"
        " font-size: 12px; border: none; }"
        "QPlainTextEdit:focus { border: none; }"
    ).arg(color));
    log->setPlainText(text);
}
}  // namespace

// ---------------------------------------------------------------------------
// ShaderEditor implementation (line-numbered code editor)
// ---------------------------------------------------------------------------

ShaderEditor::ShaderEditor(QWidget *parent) : QPlainTextEdit(parent) {
    m_lineNumberArea = new LineNumberArea(this);
    connect(this, &QPlainTextEdit::blockCountChanged, this, &ShaderEditor::updateLineNumberAreaWidth);
    connect(this, &QPlainTextEdit::updateRequest, this, &ShaderEditor::updateLineNumberArea);
    updateLineNumberAreaWidth(0);
}

int ShaderEditor::lineNumberAreaWidth() const {
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }
    return 10 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
}

void ShaderEditor::updateLineNumberAreaWidth(int) {
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void ShaderEditor::updateLineNumberArea(const QRect &rect, int dy) {
    if (dy)
        m_lineNumberArea->scroll(0, dy);
    else
        m_lineNumberArea->update(0, rect.y(), m_lineNumberArea->width(), rect.height());
    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void ShaderEditor::resizeEvent(QResizeEvent *event) {
    QPlainTextEdit::resizeEvent(event);
    const QRect cr = contentsRect();
    m_lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void ShaderEditor::lineNumberAreaPaintEvent(QPaintEvent *event) {
    QPainter painter(m_lineNumberArea);
    painter.fillRect(event->rect(), QColor("#252526"));

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            painter.setPen(QColor("#858585"));
            painter.drawText(
                0,
                top,
                m_lineNumberArea->width(),
                fontMetrics().height(),
                Qt::AlignRight,
                QString::number(blockNumber + 1)
            );
        }
        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

// ---------------------------------------------------------------------------
// LineNumberArea
// ---------------------------------------------------------------------------

LineNumberArea::LineNumberArea(ShaderEditor *editor) : QWidget(editor), m_editor(editor) {
}

QSize LineNumberArea::sizeHint() const {
    return QSize(m_editor->lineNumberAreaWidth(), 0);
}

void LineNumberArea::paintEvent(QPaintEvent *event) {
    m_editor->lineNumberAreaPaintEvent(event);
}

// ---------------------------------------------------------------------------
// Cached OpenGL resources for animated shader rendering
// ---------------------------------------------------------------------------

struct ShaderWidget::GLCache {
    QOpenGLContext *ctx = nullptr;
    QOffscreenSurface *surface = nullptr;
    gl::Texture texture;
    gl::Quad quad;
    std::unique_ptr<QOpenGLShaderProgram> prog;
    QString cachedFragSrc;
    int cachedW = 0;
    int cachedH = 0;
    bool initialized = false;
    QOpenGLFramebufferObject *fbo[2] = {nullptr, nullptr};
    int fboWriteIdx = 0;
    QImage readback;
};

ShaderWidget::ShaderWidget(QWidget *parent) : QWidget(parent) {
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setMouseTracking(true);
    resize(200, 150);
    setMinimumSize(kMinSize, kMinSize);

    m_animTimer = new QTimer(this);
    m_animTimer->setInterval(33);
    connect(m_animTimer, &QTimer::timeout, this, &ShaderWidget::onAnimTick);

    m_gl = new GLCache();
}

ShaderWidget::~ShaderWidget() {
    releaseGles();
    delete m_gl;
}

void ShaderWidget::start(QuolServices *services) {
    (void) services;
    resize(200, 150);
    if (QScreen *screen = QGuiApplication::primaryScreen()) {
        const QRect g = screen->availableGeometry();
        move(g.center().x() - width() / 2, g.center().y() - height() / 2);
    }
    captureBackground();
    show();
    raise();
    activateWindow();
}

void ShaderWidget::stop() {
    m_animTimer->stop();
    if (m_settingsPopup) {
        m_settingsPopup->close();
        m_settingsPopup = nullptr;
    }
    m_rawCapture = QImage();
    m_bgImage = QImage();
    releaseGles();
    hide();
}

void ShaderWidget::releaseGles() {
    if (!m_gl || !m_gl->initialized)
        return;

    if (m_gl->ctx && m_gl->surface) {
        if (m_gl->ctx->makeCurrent(m_gl->surface)) {
            m_gl->texture.release();
            m_gl->quad.release();
            delete m_gl->fbo[0];
            delete m_gl->fbo[1];
        }
        m_gl->ctx->doneCurrent();
    }

    m_gl->fbo[0] = nullptr;
    m_gl->fbo[1] = nullptr;
    m_gl->prog.reset();
    m_gl->cachedFragSrc.clear();
    m_gl->cachedW = 0;
    m_gl->cachedH = 0;
    m_gl->fboWriteIdx = 0;
    m_gl->readback = QImage();

    delete m_gl->surface;
    m_gl->surface = nullptr;
    delete m_gl->ctx;
    m_gl->ctx = nullptr;
    m_gl->initialized = false;
}

QWidget *ShaderWidget::widget() {
    return this;
}

void ShaderWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    if (!m_bgImage.isNull())
        p.drawImage(0, 0, m_bgImage);

    QPen borderPen(QColor(100, 180, 255), 2);
    p.setPen(borderPen);
    p.drawRect(QRectF(rect()).adjusted(1, 1, -1, -1));
}

void ShaderWidget::openSettings() {
    if (m_settingsPopup) {
        m_settingsPopup->raise();
        m_settingsPopup->activateWindow();
        return;
    }

    auto *popup = new QuolPopupWindow(QStringLiteral("Shader Settings"), this);
    m_settingsPopup = popup;
    connect(popup, &QObject::destroyed, this, [this]() { m_settingsPopup = nullptr; });
    popup->resize(600, 600);

    auto *outerLayout = new QVBoxLayout();
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(6);

    auto *editor = new ShaderEditor();
    editor->setPlaceholderText(QStringLiteral("Write a GLSL fragment shader..."));
    editor->setStyleSheet(QStringLiteral(
        "QPlainTextEdit {"
        "  background: #1E1E1E;"
        "  color: #D4D4D4;"
        "  font-family: 'Consolas', monospace;"
        "  font-size: 13px;"
        "}"
    ));
    editor->setPlainText(m_shaderSource.isEmpty() ? kDefaultShaderSrc : m_shaderSource);
    outerLayout->addWidget(editor, 1);

    auto *statusLog = new QPlainTextEdit();
    statusLog->setReadOnly(true);
    statusLog->setFixedHeight(60);
    statusLog->setTextInteractionFlags(Qt::TextSelectableByMouse);
    setStatusLog(statusLog, kLogNeutral, QString());
    outerLayout->addWidget(statusLog);

    auto *btnRow = new QHBoxLayout();
    btnRow->setSpacing(6);

    auto *importBtn = new QPushButton(QStringLiteral("Import"));
    auto *exportBtn = new QPushButton(QStringLiteral("Export"));
    auto *resetBtn = new QPushButton(QStringLiteral("Reset"));
    btnRow->addWidget(importBtn);
    btnRow->addWidget(exportBtn);
    btnRow->addWidget(resetBtn);
    btnRow->addStretch();

    auto *saveBtn = new QPushButton(QStringLiteral("Save"));
    auto *cancelBtn = new QPushButton(QStringLiteral("Cancel"));
    btnRow->addWidget(saveBtn);
    btnRow->addWidget(cancelBtn);
    outerLayout->addLayout(btnRow);

    auto *content = new QWidget();
    content->setLayout(outerLayout);
    popup->addContent(content);

    connect(importBtn, &QPushButton::clicked, this, [this, editor, popup]() {
        QString path = QFileDialog::getOpenFileName(
            popup, QStringLiteral("Import Shader"),
            m_pluginRootPath + QStringLiteral("/res/shaders"),
            QStringLiteral("GLSL files (*.glsl);;All Files (*)")
        );
        if (!path.isEmpty()) {
            QFile file(path);
            if (file.open(QIODevice::ReadOnly | QIODevice::Text))
                editor->setPlainText(QString::fromUtf8(file.readAll()));
        }
    });

    connect(exportBtn, &QPushButton::clicked, this, [this, editor, popup, statusLog]() {
        QString path = QFileDialog::getSaveFileName(
            popup, QStringLiteral("Export Shader"),
            m_pluginRootPath + QStringLiteral("/res/shaders"),
            QStringLiteral("GLSL files (*.glsl);;All Files (*)")
        );
        if (!path.isEmpty()) {
            QFile file(path);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                file.write(editor->toPlainText().toUtf8());
            } else {
                setStatusLog(statusLog, kLogError, QStringLiteral("Error: cannot write to ") + path);
            }
        }
    });

    connect(saveBtn, &QPushButton::clicked, this, [this, editor, statusLog]() {
        if (m_rawCapture.isNull()) {
            setStatusLog(statusLog, kLogError, QStringLiteral("Error: no captured area. Open the shader area first."));
            return;
        }
        QImage compiled;
        QString error;
        if (renderShader(m_rawCapture, editor->toPlainText(), 0.0f, physMouse(), m_gl, compiled, &error)) {
            m_animTime = 0.0f;
            m_shaderSource = editor->toPlainText();
            m_bgImage = compiled;
            m_bgImage.setDevicePixelRatio(m_captureDpr);
            update();
            if (!m_shaderSource.trimmed().isEmpty())
                m_animTimer->start();
            setStatusLog(statusLog, kLogOk, QStringLiteral("Shader compiled successfully."));
        } else {
            setStatusLog(statusLog, kLogError, error);
        }
    });

    connect(resetBtn, &QPushButton::clicked, this, [this, editor, statusLog]() {
        m_shaderSource.clear();
        m_animTime = 0.0f;
        m_rawCapture = QImage();
        m_bgImage = QImage();
        editor->setPlainText(kDefaultShaderSrc);
        releaseGles();
        update();
        setStatusLog(statusLog, kLogWarn, QStringLiteral("Shader reset to default (none)."));
    });

    connect(cancelBtn, &QPushButton::clicked, popup, &QWidget::close);

    popup->show();
    popup->raise();
    popup->activateWindow();
}

// ---------------------------------------------------------------------------
// Background capture
// ---------------------------------------------------------------------------

void ShaderWidget::captureBackground() {
    const QRect geom = geometry();
    const bool wasVisible = isVisible();
    if (wasVisible)
        hide();

    if (QScreen *screen = QGuiApplication::primaryScreen()) {
        QPixmap px = screen->grabWindow(0, geom.x(), geom.y(), geom.width(), geom.height());
        if (!px.isNull()) {
            m_captureDpr = px.devicePixelRatio();
            m_rawCapture = px.toImage();
            // Force GL texture re-upload on next render
            if (m_gl) {
                m_gl->cachedW = 0;
                m_gl->cachedH = 0;
            }
            applyShaderToCapture();
        }
    }

    if (wasVisible) {
        show();
        raise();
        activateWindow();
    }
}

void ShaderWidget::applyShaderToCapture() {
    if (m_shaderSource.trimmed().isEmpty() || m_rawCapture.isNull()) {
        m_bgImage = m_rawCapture;
        m_animTimer->stop();
    } else if (!renderShader(m_rawCapture, m_shaderSource, m_animTime, physMouse(), m_gl, m_bgImage)) {
        m_bgImage = m_rawCapture;
        m_animTimer->stop();
    } else if (!m_animTimer->isActive()) {
        m_animTimer->start();
    }
    m_bgImage.setDevicePixelRatio(m_captureDpr);
}

QPointF ShaderWidget::physMouse() const {
    return QPointF(m_mousePos.x() * m_captureDpr,
                   m_rawCapture.height() - m_mousePos.y() * m_captureDpr);
}

void ShaderWidget::onAnimTick() {
    m_animTime += 0.033f;
    applyShaderToCapture();
    update();
}

// ---------------------------------------------------------------------------
// Offscreen GLSL shader rendering (with static cache for animation)
// ---------------------------------------------------------------------------

static const QString kVertexSrc = QStringLiteral(
    "#version 330 core\n"
    "layout(location = 0) in vec2 a_pos;\n"
    "layout(location = 1) in vec2 a_tex;\n"
    "out vec2 v_texCoord;\n"
    "void main() {\n"
    "    gl_Position = vec4(a_pos, 0.0, 1.0);\n"
    "    v_texCoord = a_tex;\n"
    "}\n"
);

bool ShaderWidget::renderShader(
    const QImage &source, const QString &fragSrc, float time, const QPointF &mousePos,
    GLCache *cache, QImage &out, QString *errorLog
) {
    out = QImage();
    if (fragSrc.trimmed().isEmpty()) {
        out = source;
        return true;
    }

    // First-time GL setup
    if (!cache->initialized) {
        QSurfaceFormat fmt;
        fmt.setVersion(3, 3);
        fmt.setProfile(QSurfaceFormat::CoreProfile);

        cache->ctx = new QOpenGLContext();
        cache->ctx->setFormat(fmt);
        if (!cache->ctx->create()) {
            if (errorLog)
                *errorLog = QStringLiteral("Failed to create OpenGL context.");
            return false;
        }

        cache->surface = new QOffscreenSurface();
        cache->surface->setFormat(cache->ctx->format());
        cache->surface->create();

        cache->initialized = true;
    }

    if (!cache->ctx->makeCurrent(cache->surface)) {
        if (errorLog)
            *errorLog = QStringLiteral("Failed to make OpenGL context current.");
        return false;
    }

    auto *f = cache->ctx->extraFunctions();

    // Recompile shader if source changed
    if (fragSrc != cache->cachedFragSrc) {
        cache->prog = std::make_unique<QOpenGLShaderProgram>();

        if (!cache->prog->addShaderFromSourceCode(QOpenGLShader::Vertex, kVertexSrc)) {
            if (errorLog)
                *errorLog = cache->prog->log();
            cache->prog.reset();
            cache->cachedFragSrc.clear();
            cache->ctx->doneCurrent();
            return false;
        }
        if (!cache->prog->addShaderFromSourceCode(QOpenGLShader::Fragment, fragSrc)) {
            if (errorLog)
                *errorLog = cache->prog->log();
            cache->prog.reset();
            cache->cachedFragSrc.clear();
            cache->ctx->doneCurrent();
            return false;
        }
        if (!cache->prog->link()) {
            if (errorLog)
                *errorLog = cache->prog->log();
            cache->prog.reset();
            cache->cachedFragSrc.clear();
            cache->ctx->doneCurrent();
            return false;
        }
        cache->cachedFragSrc = fragSrc;
    }

    if (!cache->prog) {
        if (errorLog)
            *errorLog = QStringLiteral("No valid shader program.");
        return false;
    }

    const int w = source.width();
    const int h = source.height();

    // Upload texture (or re-upload if dimensions changed)
    if (w != cache->cachedW || h != cache->cachedH) {
        QImage texImg = source.convertToFormat(QImage::Format_RGBA8888);

        cache->texture.create();
        f->glBindTexture(GL_TEXTURE_2D, cache->texture.id());
        f->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, texImg.constBits());
        f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // Recreate quad geometry
        cache->quad.create();

        const float verts[] = {
            -1.0f, -1.0f, 0.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f,
             1.0f,  1.0f, 1.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f
        };
        const unsigned int idxs[] = {0, 1, 2, 0, 2, 3};

        f->glBindVertexArray(cache->quad.vao());
        f->glBindBuffer(GL_ARRAY_BUFFER, cache->quad.vbo());
        f->glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
        f->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cache->quad.ebo());
        f->glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idxs), idxs, GL_STATIC_DRAW);

        f->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * (int) sizeof(float), nullptr);
        f->glEnableVertexAttribArray(0);
        f->glVertexAttribPointer(
            1, 2, GL_FLOAT, GL_FALSE, 4 * (int) sizeof(float),
            reinterpret_cast<const void *>(2 * sizeof(float))
        );
        f->glEnableVertexAttribArray(1);

        // Resize or create FBOs for ping-pong
        delete cache->fbo[0];
        delete cache->fbo[1];
        cache->fbo[0] = new QOpenGLFramebufferObject(w, h);
        cache->fbo[1] = new QOpenGLFramebufferObject(w, h);
        cache->fboWriteIdx = 0;

        cache->fbo[0]->bind();
        f->glClear(GL_COLOR_BUFFER_BIT);
        cache->fbo[1]->bind();
        f->glClear(GL_COLOR_BUFFER_BIT);

        cache->cachedW = w;
        cache->cachedH = h;
    }

    // Per-frame render
    int curIdx = cache->fboWriteIdx;
    int prevIdx = 1 - curIdx;

    cache->fbo[curIdx]->bind();
    f->glViewport(0, 0, w, h);
    f->glClear(GL_COLOR_BUFFER_BIT);

    cache->prog->bind();

    // u_texture = current capture (unit 0)
    f->glActiveTexture(GL_TEXTURE0);
    f->glBindTexture(GL_TEXTURE_2D, cache->texture.id());
    cache->prog->setUniformValue("u_texture", 0);

    // u_prevFrame = previous frame output (unit 1)
    f->glActiveTexture(GL_TEXTURE1);
    f->glBindTexture(GL_TEXTURE_2D, cache->fbo[prevIdx]->texture());
    cache->prog->setUniformValue("u_prevFrame", 1);

    cache->prog->setUniformValue("u_time", time);
    cache->prog->setUniformValue("u_resolution", QVector2D(w, h));
    cache->prog->setUniformValue("u_mouse", QVector2D(mousePos.x(), mousePos.y()));

    f->glBindVertexArray(cache->quad.vao());
    f->glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

    // Read back into a cached scratch buffer, then flip into out
    if (cache->readback.size() != source.size())
        cache->readback = QImage(source.size(), QImage::Format_RGBA8888);
    if (out.size() != source.size())
        out = QImage(source.size(), QImage::Format_RGBA8888);
    f->glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, cache->readback.bits());
    const int stride = cache->readback.bytesPerLine();
    for (int y = 0; y < h; ++y)
        memcpy(out.scanLine(h - 1 - y), cache->readback.constScanLine(y), stride);

    // Swap FBOs for next frame
    cache->fboWriteIdx = prevIdx;

    return true;
}

// ---------------------------------------------------------------------------
// Mouse handling – drag & resize
// ---------------------------------------------------------------------------

ShaderWidget::Edge ShaderWidget::edgeAtPos(const QPoint &pos) const {
    const QRect r = rect();
    const bool left = pos.x() <= r.left() + kHandleMargin;
    const bool right = pos.x() >= r.right() - kHandleMargin;
    const bool top = pos.y() <= r.top() + kHandleMargin;
    const bool bottom = pos.y() >= r.bottom() - kHandleMargin;

    if (top && left)
        return Edge::TopLeft;
    if (top && right)
        return Edge::TopRight;
    if (bottom && left)
        return Edge::BottomLeft;
    if (bottom && right)
        return Edge::BottomRight;
    if (left)
        return Edge::Left;
    if (right)
        return Edge::Right;
    if (top)
        return Edge::Top;
    if (bottom)
        return Edge::Bottom;
    return Edge::None;
}

void ShaderWidget::applyEdgeCursor(Edge edge) {
    switch (edge) {
        case Edge::TopLeft:
        case Edge::BottomRight:
            setCursor(Qt::SizeFDiagCursor);
            break;
        case Edge::TopRight:
        case Edge::BottomLeft:
            setCursor(Qt::SizeBDiagCursor);
            break;
        case Edge::Left:
        case Edge::Right:
            setCursor(Qt::SizeHorCursor);
            break;
        case Edge::Top:
        case Edge::Bottom:
            setCursor(Qt::SizeVerCursor);
            break;
        default:
            setCursor(Qt::ArrowCursor);
            break;
    }
}

void ShaderWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::MiddleButton) {
        stop();
        emit closed();
        return;
    }

    if (event->button() != Qt::LeftButton)
        return;

    const QPoint local = event->position().toPoint();
    Edge edge = edgeAtPos(local);

    if (edge != Edge::None) {
        m_resizing = true;
        m_resizeEdge = edge;
        m_resizeStartGeom = geometry();
        m_resizeStartPos = event->globalPosition().toPoint();
    } else {
        m_dragging = true;
        m_dragOffset = event->globalPosition().toPoint() - pos();
    }
    event->accept();
}

void ShaderWidget::mouseMoveEvent(QMouseEvent *event) {
    m_mousePos = event->position();

    if (m_resizing) {
        const QPoint delta = event->globalPosition().toPoint() - m_resizeStartPos;
        QRect g = m_resizeStartGeom;

        if (m_resizeEdge == Edge::Left || m_resizeEdge == Edge::TopLeft || m_resizeEdge == Edge::BottomLeft)
            g.setLeft(g.left() + delta.x());
        if (m_resizeEdge == Edge::Right || m_resizeEdge == Edge::TopRight || m_resizeEdge == Edge::BottomRight)
            g.setRight(g.right() + delta.x());
        if (m_resizeEdge == Edge::Top || m_resizeEdge == Edge::TopLeft || m_resizeEdge == Edge::TopRight)
            g.setTop(g.top() + delta.y());
        if (m_resizeEdge == Edge::Bottom || m_resizeEdge == Edge::BottomLeft || m_resizeEdge == Edge::BottomRight)
            g.setBottom(g.bottom() + delta.y());

        if (g.width() >= kMinSize && g.height() >= kMinSize)
            setGeometry(g);

        event->accept();
        return;
    }

    if (m_dragging) {
        move(event->globalPosition().toPoint() - m_dragOffset);
        event->accept();
        return;
    }

    applyEdgeCursor(edgeAtPos(event->position().toPoint()));
    event->accept();
}

void ShaderWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        const bool changed = m_dragging || m_resizing;
        m_dragging = false;
        m_resizing = false;
        m_resizeEdge = Edge::None;
        if (changed) {
            captureBackground();
            update();
        }
        event->accept();
    }
}

void ShaderWidget::closeEvent(QCloseEvent *event) {
    stop();
    emit closed();
    QWidget::closeEvent(event);
}
