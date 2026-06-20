#include "plugins/draw/Draw.hpp"
#include "core/InputManager.hpp"
#include "plugins/draw/lib/ColorWheel.hpp"
#include "plugins/draw/lib/DrawOverlay.hpp"

#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScreen>
#include <QSlider>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

// ---------------------------------------------------------------------------
// IQuolPlugin lifecycle
// ---------------------------------------------------------------------------

void Draw::initialize(const QString &rootPath, const PluginConfig &pluginConfig, QuolServices *services) {
    m_pluginRootPath = rootPath;
    m_cfg = pluginConfig;
    m_services = services;

    m_drawIcon = QIcon(rootPath + "/res/img/draw.svg");
    m_stopIcon = QIcon(rootPath + "/res/img/stop.svg");

    m_overlay = new DrawOverlay();
    connect(m_overlay, &DrawOverlay::stopRequested, this, &Draw::toggleDrawing);

    applyHotkeys();
}

QWidget *Draw::createWidget(QWidget *parent) {
    m_widget = new QWidget(parent);
    auto *root = new QVBoxLayout(m_widget);
    root->setContentsMargins(0, 0, 0, 0);

    auto *topRow = new QHBoxLayout();

    m_colorWheel = new ColorWheel(28, -1, 12, m_widget);
    m_colorWheel->setColor(m_penColor);
    connect(m_colorWheel, &ColorWheel::colorChanged, this, &Draw::onColorChanged);

    auto *controlCol = new QVBoxLayout();
    controlCol->setAlignment(Qt::AlignTop);

    // Buttons row (right side)
    auto *btnRow = new QHBoxLayout();

    m_clearButton = new QPushButton(m_widget);
    m_clearButton->setIcon(QIcon(m_pluginRootPath + QStringLiteral("/res/img/clear.svg")));
    m_clearButton->setToolTip(QStringLiteral("Clear canvas"));
    connect(m_clearButton, &QPushButton::clicked, m_overlay, &DrawOverlay::clearCanvas);

    m_startButton = new QPushButton(m_widget);
    m_startButton->setIcon(m_drawIcon);
    m_startButton->setToolTip(QStringLiteral("Start / stop drawing"));
    connect(m_startButton, &QPushButton::clicked, this, &Draw::toggleDrawing);

    btnRow->addWidget(m_clearButton);
    btnRow->addWidget(m_startButton);
    controlCol->addLayout(btnRow);

    // Hex input (right side)
    m_hexInput = new QLineEdit(m_widget);
    m_hexInput->setPlaceholderText(QStringLiteral("#RRGGBB"));
    m_hexInput->setMaxLength(7);
    m_hexInput->setToolTip(QStringLiteral("Hex colour"));
    connect(m_hexInput, &QLineEdit::editingFinished, this, &Draw::onHexInputChanged);
    controlCol->addWidget(m_hexInput);

    // Stroke row (right side)
    auto *sizeRow = new QHBoxLayout();
    sizeRow->setSpacing(6);

    m_sizeLabel = new QLabel("2", m_widget);
    m_sizeLabel->setFixedWidth(22);
    m_sizeLabel->setAlignment(Qt::AlignCenter);

    m_sizeSlider = new QSlider(Qt::Horizontal, m_widget);
    m_sizeSlider->setRange(1, 30);
    m_sizeSlider->setValue(2);
    connect(m_sizeSlider, &QSlider::valueChanged, this, [this](int v) {
        m_sizeLabel->setText(QString::number(v));
        m_overlay->setPenWidth(v);
    });

    sizeRow->addWidget(m_sizeLabel);
    sizeRow->addWidget(m_sizeSlider, 1);
    controlCol->addLayout(sizeRow);

    topRow->addWidget(m_colorWheel);
    topRow->addLayout(controlCol, 1);
    root->addLayout(topRow);

    // Sync initial colour
    onColorChanged(m_penColor);

    return m_widget;
}

void Draw::onUpdateConfig(const PluginConfig &pluginConfig) {
    m_cfg = pluginConfig;
    applyHotkeys();
}

void Draw::shutdown() {
    if (m_drawing && m_overlay)
        m_overlay->stopDrawing();
    m_drawing = false;

    delete m_overlay;
    m_overlay = nullptr;

    auto *im = m_services ? m_services->inputManager() : nullptr;
    if (im) {
        if (!m_toggleHotkeyId.isEmpty()) {
            im->removeHotkey(m_toggleHotkeyId);
            m_toggleHotkeyId.clear();
        }
        if (!m_undoHotkeyId.isEmpty()) {
            im->removeHotkey(m_undoHotkeyId);
            m_undoHotkeyId.clear();
        }
    }
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

void Draw::applyHotkeys() {
    if (!m_services)
        return;
    auto *im = m_services->inputManager();

    // Toggle hotkey
    if (!m_toggleHotkeyId.isEmpty()) {
        im->removeHotkey(m_toggleHotkeyId);
        m_toggleHotkeyId.clear();
    }
    const QString toggleCombo = m_cfg.get(QStringLiteral("draw_toggle")).toString().trimmed().toLower();
    if (!toggleCombo.isEmpty())
        m_toggleHotkeyId = im->addHotkey(toggleCombo, [this]() { toggleDrawing(); }, true);

    // Undo hotkey is registered in toggleDrawing() when drawing starts
}

void Draw::toggleDrawing() {
    if (!m_drawing) {
        m_drawing = true;
        if (m_startButton)
            m_startButton->setIcon(m_stopIcon);

        // Register undo hotkey while drawing
        if (m_services && m_undoHotkeyId.isEmpty()) {
            const QString undoCombo = m_cfg.get(QStringLiteral("undo")).toString().trimmed().toLower();
            if (!undoCombo.isEmpty())
                m_undoHotkeyId = m_services->inputManager()->addHotkey(
                    undoCombo,
                    [this]() {
                        if (m_overlay)
                            m_overlay->undo();
                    },
                    true
                );
        }

        m_services->hideAllPluginWindows();
        QScreen *screen = QApplication::primaryScreen();
        const QRect g = screen->availableGeometry();
        const QPixmap screenshot = screen->grabWindow(0, g.x(), g.y(), g.width(), g.height());
        m_services->showAllPluginWindows();
        m_overlay->startDrawing(screenshot);

    } else {
        m_drawing = false;
        if (m_startButton)
            m_startButton->setIcon(m_drawIcon);

        // Remove undo hotkey
        if (m_services && !m_undoHotkeyId.isEmpty()) {
            m_services->inputManager()->removeHotkey(m_undoHotkeyId);
            m_undoHotkeyId.clear();
        }

        m_overlay->stopDrawing();
    }
}

void Draw::onHexInputChanged() {
    if (!m_hexInput)
        return;
    const QString text = m_hexInput->text().trimmed();
    if (QColor::isValidColorName(text))
        onColorChanged(QColor(text));
    m_hexInput->clearFocus();
}

void Draw::onColorChanged(const QColor &color) {
    m_penColor = color;
    if (m_overlay)
        m_overlay->setPenColor(color);
    if (m_colorWheel)
        m_colorWheel->setColor(color, false);

    if (m_hexInput) {
        const QString hex = color.name().toUpper();
        if (m_hexInput->text().toUpper() != hex)
            m_hexInput->setText(hex);
    }
}
