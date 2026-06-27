#include "plugins/colorPicker/ColorPicker.hpp"

#include <QColor>
#include <QCursor>
#include <QGridLayout>
#include <QGuiApplication>
#include <QIcon>
#include <QLabel>
#include <QPainter>
#include <QPen>
#include <QPixmap>
#include <QPushButton>
#include <QScreen>
#include <QTimer>
#include <QWidget>
#include <algorithm>

#include "core/InputManager.hpp"

// ---------------------------------------------------------------------------
QWidget *ColorPicker::createWidget(QWidget *parent) {
    m_widget = new QWidget(parent);
    auto *gridLayout = new QGridLayout(m_widget);
    gridLayout->setContentsMargins(0, 0, 0, 0);

    // Preview label (fixed size — zoom adapts to sample size)
    m_previewLabel = new QLabel(m_widget);
    m_previewLabel->setFixedSize(kPreviewSize, kPreviewSize);
    m_previewLabel->setAlignment(Qt::AlignCenter);
    gridLayout->addWidget(m_previewLabel, 0, 0, 3, 1);

    // Hex value (selectable so user can copy)
    m_hexLabel = new QLabel(QStringLiteral("#------"), m_widget);
    m_hexLabel->setAlignment(Qt::AlignCenter);
    m_hexLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    gridLayout->addWidget(m_hexLabel, 0, 1);

    // RGB value (selectable)
    m_rgbLabel = new QLabel(QStringLiteral("r, g, b"), m_widget);
    m_rgbLabel->setAlignment(Qt::AlignCenter);
    m_rgbLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    gridLayout->addWidget(m_rgbLabel, 1, 1);

    // Pick button with eyedropper icon
    m_pickButton = new QPushButton(m_widget);
    m_pickButton->setCheckable(true);
    if (!m_pluginRootPath.isEmpty()) {
        m_pickButton->setIcon(QIcon(m_pluginRootPath + QStringLiteral("/res/img/pick.svg")));
    }
    gridLayout->addWidget(m_pickButton, 2, 1);

    QObject::connect(m_pickButton, &QPushButton::clicked, this, &ColorPicker::togglePicking);

    // Screen scale factor
    if (QScreen *screen = QGuiApplication::primaryScreen())
        m_sf = screen->devicePixelRatio();

    applyVisualConfig();

    // Update color once on creation so the preview isn't blank
    updateColor();

    return m_widget;
}

// ---------------------------------------------------------------------------
void ColorPicker::initialize(const QString &pluginRootPath, const PluginConfig &pluginConfig, QuolServices *services) {
    m_pluginRootPath = pluginRootPath;
    m_cfg = pluginConfig;
    m_services = services;
}

void ColorPicker::onUpdateConfig(const PluginConfig &pluginConfig) {
    m_cfg = pluginConfig;
    applyVisualConfig();
    updateColor();
}

void ColorPicker::shutdown() {
    stopPicking();
    m_previewLabel = nullptr;
    m_hexLabel = nullptr;
    m_rgbLabel = nullptr;
    m_pickButton = nullptr;
    m_widget = nullptr;
}

void ColorPicker::applyVisualConfig() {
    int sampleSize = m_cfg.get(QStringLiteral("sample_size")).toInt();
    sampleSize = std::clamp(sampleSize, 1, 31);
    if ((sampleSize % 2) == 0)
        sampleSize += 1;
    m_sampleSize = sampleSize;
}

// ---------------------------------------------------------------------------
void ColorPicker::togglePicking() {
    if (m_picking) {
        stopPicking();
        return;
    }

    m_picking = true;

    // Start polling
    if (!m_timer) {
        m_timer = new QTimer(this);
        QObject::connect(m_timer, &QTimer::timeout, this, &ColorPicker::updateColor);
    }
    m_timer->start(60);

    if (m_services && m_services->inputManager()) {
        m_escapeHotkeyId = m_services->inputManager()->addHotkey(
            "esc",
            [this]() {
                if (m_picking)
                    stopPicking();
            },
            true
        );
    }

    if (m_pickButton) {
        m_pickButton->setText(QStringLiteral("Stop (ESC)"));
        m_pickButton->setIcon(QIcon{});
        m_pickButton->setStyleSheet(QStringLiteral("background-color: #eee; color: #000;"));
        m_pickButton->setChecked(true);
    }
}

void ColorPicker::stopPicking() {
    if (m_timer)
        m_timer->stop();

    m_picking = false;

    if (m_services && m_services->inputManager() && !m_escapeHotkeyId.isEmpty()) {
        m_services->inputManager()->removeHotkey(m_escapeHotkeyId);
        m_escapeHotkeyId.clear();
    }

    if (m_pickButton) {
        m_pickButton->setText(QString{});
        if (!m_pluginRootPath.isEmpty())
            m_pickButton->setIcon(QIcon(m_pluginRootPath + QStringLiteral("/res/img/pick.svg")));
        m_pickButton->setStyleSheet(QString{});
        m_pickButton->setChecked(false);
    }
}

// ---------------------------------------------------------------------------
void ColorPicker::updateColor() {
    QScreen *screen = QGuiApplication::primaryScreen();
    if (!screen)
        return;

    const QPoint pos = QCursor::pos();
    const int ps = m_sampleSize;

    // Grab a small region around the cursor (in physical pixels on HiDPI)
    const int x = pos.x() - ps / 2;
    const int y = pos.y() - ps / 2;

    QPixmap grabbed = screen->grabWindow(0, x, y, ps, ps);
    if (grabbed.isNull())
        return;

    QImage image = grabbed.toImage();
    if (image.isNull())
        return;

    // Normalize to exactly ps×ps logical samples regardless of HiDPI scale factor
    if (image.width() != ps || image.height() != ps)
        image = image.scaled(ps, ps, Qt::IgnoreAspectRatio, Qt::FastTransformation);

    // Scale up to fill the fixed preview label (physical pixels)
    const int physicalPreview = static_cast<int>(kPreviewSize * m_sf);
    QPixmap scaled = QPixmap::fromImage(image).scaled(
        QSize(physicalPreview, physicalPreview), Qt::IgnoreAspectRatio, Qt::FastTransformation
    );
    scaled.setDevicePixelRatio(m_sf);

    drawFrame(scaled);

    if (m_previewLabel)
        m_previewLabel->setPixmap(scaled);

    // Center pixel color
    const QColor center(image.pixel(ps / 2, ps / 2));

    if (m_hexLabel)
        m_hexLabel->setText(center.name());

    if (m_rgbLabel)
        m_rgbLabel->setText(
            QString::number(center.red()) + QStringLiteral(",") + QString::number(center.green()) + QStringLiteral(",")
            + QString::number(center.blue())
        );
}

// ---------------------------------------------------------------------------
void ColorPicker::drawFrame(QPixmap &pixmap) {
    QPainter painter(&pixmap);
    if (!painter.isActive())
        return;

    const qreal sq = pixmap.width() / m_sf;
    const qreal cell = sq / m_sampleSize;
    const qreal center = (m_sampleSize / 2.0) * cell;

    QPen pen;
    pen.setColor(QColor(255, 255, 255, 160));
    pen.setWidth(1);
    painter.setPen(pen);

    // Outer border
    painter.drawRect(0, 0, static_cast<int>(sq), static_cast<int>(sq));

    for (int i = 1; i < m_sampleSize; ++i) {
        const qreal p = i * cell;
        painter.drawLine(QPointF(p, 0), QPointF(p, sq));
        painter.drawLine(QPointF(0, p), QPointF(sq, p));
    }

    pen.setColor(QColor(255, 255, 255, 220));
    pen.setWidth(2);
    painter.setPen(pen);
    painter.drawRect(QRectF(center - cell / 2.0, center - cell / 2.0, cell, cell));

    painter.end();
}
