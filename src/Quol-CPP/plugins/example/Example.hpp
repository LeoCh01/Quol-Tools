#pragma once

#include "core/InputManager.hpp"
#include "plugin_api/IQuolPlugin.hpp"

#include <QObject>

class QLabel;

class Example final : public QObject, public IQuolPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID IQuolPlugin_iid)
    Q_INTERFACES(IQuolPlugin)

public:
    QWidget *createWidget(QWidget *parent = nullptr) override;

    void initialize(const QString &pluginRootPath, const PluginConfig &pluginConfig, QuolServices *services) override;
    void onUpdateConfig(const PluginConfig &pluginConfig) override;
    void shutdown() override;

private:
    void refreshLabels();
    void applyHotkeyFromConfig();

    QString m_pluginRootPath;
    PluginConfig m_cfg;
    QuolServices *m_services = nullptr;

    QString m_hotkeyId;
    QString m_keyListenId;
    QString m_mouseListenId;

private:
    void onMouseEvent(InputManager::MouseEvent event);

    QLabel *m_titleLabel = nullptr;
    QLabel *m_mouseLabel = nullptr;
    QLabel *m_valueLabel = nullptr;
    QLabel *m_nestedNoteLabel = nullptr;
    QLabel *m_nestedEnabledLabel = nullptr;
    QLabel *m_nestedModeLabel = nullptr;
    QLabel *m_nestedInnerLabelLabel = nullptr;
    QLabel *m_nestedInnerChoiceLabel = nullptr;
    QLabel *m_pressedLabel = nullptr;
    QLabel *m_releasedLabel = nullptr;
    QLabel *m_triggeredLabel = nullptr;
};
