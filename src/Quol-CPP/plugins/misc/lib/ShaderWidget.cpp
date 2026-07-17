#include "plugins/misc/lib/ShaderWidget.hpp"
#include "plugin_api/QuolServices.hpp"
#include "ui/QuolPopupWindow.hpp"

#include <QCloseEvent>
#include <QFile>
#include <QFileDialog>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QMoveEvent>
#include <QPainter>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScreen>
#include <QVBoxLayout>

ShaderWidget::ShaderWidget(const QString &pluginRootPath, QWidget *parent)
    : QWidget(parent), m_rootPath(pluginRootPath) {
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setMouseTracking(true);
    resize(200, 150);
    setMinimumSize(kMinSize, kMinSize);
}

void ShaderWidget::start(QuolServices *services) {
    m_services = services;
    if (QScreen *screen = QGuiApplication::primaryScreen()) {
        const QRect g = screen->availableGeometry();
        move(g.center().x() - width() / 2, g.center().y() - height() / 2);
    }
    show();
    raise();
    activateWindow();
}

void ShaderWidget::stop() {
    if (m_settingsPopup) {
        m_settingsPopup->close();
        m_settingsPopup = nullptr;
    }
    hide();
}

QWidget *ShaderWidget::widget() {
    return this;
}

void ShaderWidget::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
}

void ShaderWidget::moveEvent(QMoveEvent *event) {
    QWidget::moveEvent(event);
}

void ShaderWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // Nearly-transparent fill so mouse events are captured by the OS.
    // Fully transparent (alpha=0) pixels on a layered window pass through.
    p.fillRect(rect(), QColor(0, 0, 0, 1));

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
    popup->resize(500, 400);

    auto *outerLayout = new QVBoxLayout();
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(6);

    auto *editor = new QPlainTextEdit();
    editor->setPlaceholderText(QStringLiteral("Shader editor (coming soon)"));
    editor->setStyleSheet(QStringLiteral(
        "QPlainTextEdit {"
        "  background: #1E1E1E;"
        "  color: #D4D4D4;"
        "  font-family: 'Consolas', monospace;"
        "  font-size: 13px;"
        "}"
    ));
    outerLayout->addWidget(editor, 1);

    auto *btnRow = new QHBoxLayout();
    btnRow->setSpacing(6);

    auto *importBtn = new QPushButton(QStringLiteral("Import"));
    auto *exportBtn = new QPushButton(QStringLiteral("Export"));
    btnRow->addWidget(importBtn);
    btnRow->addWidget(exportBtn);
    btnRow->addStretch();

    auto *saveBtn = new QPushButton(QStringLiteral("Save"));
    auto *cancelBtn = new QPushButton(QStringLiteral("Cancel"));
    btnRow->addWidget(saveBtn);
    btnRow->addWidget(cancelBtn);
    outerLayout->addLayout(btnRow);

    auto *content = new QWidget();
    content->setLayout(outerLayout);
    popup->addContent(content);

    connect(importBtn, &QPushButton::clicked, this, [editor]() {
        QString path = QFileDialog::getOpenFileName(
            nullptr, QStringLiteral("Import Shader"), QString(),
            QStringLiteral("All Files (*)"));
        if (!path.isEmpty()) {
            QFile file(path);
            if (file.open(QIODevice::ReadOnly | QIODevice::Text))
                editor->setPlainText(QString::fromUtf8(file.readAll()));
        }
    });

    connect(exportBtn, &QPushButton::clicked, this, [editor]() {
        QString path = QFileDialog::getSaveFileName(
            nullptr, QStringLiteral("Export Shader"), QStringLiteral("shader.glsl"),
            QStringLiteral("All Files (*)"));
        if (!path.isEmpty()) {
            QFile file(path);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text))
                file.write(editor->toPlainText().toUtf8());
        }
    });

    connect(saveBtn, &QPushButton::clicked, popup, &QWidget::close);
    connect(cancelBtn, &QPushButton::clicked, popup, &QWidget::close);

    popup->show();
    popup->raise();
    popup->activateWindow();
}

// ---------------------------------------------------------------------------
// Mouse handling – drag & resize
// ---------------------------------------------------------------------------

ShaderWidget::Edge ShaderWidget::edgeAtPos(const QPoint &pos) const {
    const QRect r = rect();
    const bool left   = pos.x() <= r.left()   + kHandleMargin;
    const bool right  = pos.x() >= r.right()  - kHandleMargin;
    const bool top    = pos.y() <= r.top()    + kHandleMargin;
    const bool bottom = pos.y() >= r.bottom() - kHandleMargin;

    if (top && left)   return Edge::TopLeft;
    if (top && right)  return Edge::TopRight;
    if (bottom && left)  return Edge::BottomLeft;
    if (bottom && right) return Edge::BottomRight;
    if (left)   return Edge::Left;
    if (right)  return Edge::Right;
    if (top)    return Edge::Top;
    if (bottom) return Edge::Bottom;
    return Edge::None;
}

void ShaderWidget::applyEdgeCursor(Edge edge) {
    switch (edge) {
    case Edge::TopLeft:
    case Edge::BottomRight:   setCursor(Qt::SizeFDiagCursor); break;
    case Edge::TopRight:
    case Edge::BottomLeft:    setCursor(Qt::SizeBDiagCursor); break;
    case Edge::Left:
    case Edge::Right:         setCursor(Qt::SizeHorCursor);   break;
    case Edge::Top:
    case Edge::Bottom:        setCursor(Qt::SizeVerCursor);   break;
    default:                  setCursor(Qt::ArrowCursor);      break;
    }
}

void ShaderWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::MiddleButton) {
        hide();
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
    if (m_resizing) {
        const QPoint delta = event->globalPosition().toPoint() - m_resizeStartPos;
        QRect g = m_resizeStartGeom;

        if (m_resizeEdge == Edge::Left   || m_resizeEdge == Edge::TopLeft  || m_resizeEdge == Edge::BottomLeft)  g.setLeft(g.left() + delta.x());
        if (m_resizeEdge == Edge::Right  || m_resizeEdge == Edge::TopRight || m_resizeEdge == Edge::BottomRight) g.setRight(g.right() + delta.x());
        if (m_resizeEdge == Edge::Top    || m_resizeEdge == Edge::TopLeft  || m_resizeEdge == Edge::TopRight)    g.setTop(g.top() + delta.y());
        if (m_resizeEdge == Edge::Bottom || m_resizeEdge == Edge::BottomLeft|| m_resizeEdge == Edge::BottomRight) g.setBottom(g.bottom() + delta.y());

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
        m_dragging = false;
        m_resizing = false;
        m_resizeEdge = Edge::None;
        event->accept();
    }
}

void ShaderWidget::closeEvent(QCloseEvent *event) {
    emit closed();
    QWidget::closeEvent(event);
}
