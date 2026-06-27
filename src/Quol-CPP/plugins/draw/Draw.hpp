#pragma once

#include "plugin_api/IQuolPlugin.hpp"

#include <QColor>
#include <QIcon>
#include <QObject>

class ColorWheel;
class DrawOverlay;
class QLabel;
class QLineEdit;
class QPushButton;
class QSlider;

class Draw final : public QObject, public IQuolPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID IQuolPlugin_iid)
    Q_INTERFACES(IQuolPlugin)

public:
    QWidget *createWidget(QWidget *parent = nullptr) override;
    void initialize(const QString &pluginRootPath, const PluginConfig &pluginConfig, QuolServices *services) override;
    void onUpdateConfig(const PluginConfig &pluginConfig) override;
    void shutdown() override;

private:
    void toggleDrawing();
    void applyHotkeys();  // re-register draw_toggle + undo hotkeys
    void onHexInputChanged();
    void onColorChanged(const QColor &color);

    QString m_pluginRootPath;
    PluginConfig m_cfg;
    QuolServices *m_services = nullptr;
    QString m_toggleHotkeyId;
    QString m_undoHotkeyId;

    QWidget *m_widget = nullptr;
    ColorWheel *m_colorWheel = nullptr;
    QLineEdit *m_hexInput = nullptr;
    QPushButton *m_clearButton = nullptr;
    QPushButton *m_startButton = nullptr;
    QSlider *m_sizeSlider = nullptr;
    QLabel *m_sizeLabel = nullptr;

    QIcon m_drawIcon;
    QIcon m_stopIcon;

    DrawOverlay *m_overlay = nullptr;
    QColor m_penColor = Qt::red;
    bool m_drawing = false;
};
