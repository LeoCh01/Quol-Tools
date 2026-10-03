#pragma once

#include "plugin_api/IQuolPlugin.hpp"

#include <QList>
#include <QObject>

class QPushButton;
class QWidget;
class ShaderWidget;
class SnipItem;
class SnipOverlay;

class Screen final : public QObject, public IQuolPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID IQuolPlugin_iid)
    Q_INTERFACES(IQuolPlugin)

public:
    QWidget *createWidget(QWidget *parent = nullptr) override;
    void initialize(const QString &pluginRootPath, const PluginConfig &pluginConfig, QuolServices *services) override;
    void onUpdateConfig(const PluginConfig &pluginConfig) override;
    void shutdown() override;

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void applyConfig();
    void toggleShader();
    void startSnip();
    void onSnipReady(const QPixmap &cropped);
    void deleteSnipItem(SnipItem *item);

    QString m_pluginRootPath;
    PluginConfig m_cfg;
    QuolServices *m_services = nullptr;
    QWidget *m_widget = nullptr;
    bool m_hideImgOnToggle = false;

    ShaderWidget *m_shaderWidget = nullptr;
    bool m_shaderActive = false;
    QPushButton *m_shaderBtn = nullptr;

    SnipOverlay *m_snipOverlay = nullptr;
    QList<SnipItem *> m_snipItems;
};