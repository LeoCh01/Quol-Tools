#pragma once

#include "plugin_api/IQuolPlugin.hpp"

#include <QObject>

class QIcon;
class QLabel;
class QPushButton;
class QTimer;

class ColorPicker final : public QObject, public IQuolPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID IQuolPlugin_iid)
    Q_INTERFACES(IQuolPlugin)

public:
    QWidget *createWidget(QWidget *parent = nullptr) override;

    void initialize(const QString &pluginRootPath, const PluginConfig &pluginConfig, QuolServices *services) override;
    void onUpdateConfig(const PluginConfig &pluginConfig) override;
    void shutdown() override;

private:
    static constexpr int kPreviewSize = 50;

    void togglePicking();
    void stopPicking();
    void updateColor();
    void drawFrame(QPixmap &pixmap);
    void applyVisualConfig();

    QString m_pluginRootPath;
    PluginConfig m_cfg;
    QuolServices *m_services = nullptr;
    QString m_escapeHotkeyId;  // handle returned by InputManager::addHotkey
    int m_sampleSize = 7;

    QWidget *m_widget = nullptr;
    qreal m_sf = 1.0;
    bool m_picking = false;

    QLabel *m_previewLabel = nullptr;
    QLabel *m_hexLabel = nullptr;
    QLabel *m_rgbLabel = nullptr;
    QPushButton *m_pickButton = nullptr;
    QTimer *m_timer = nullptr;
};
