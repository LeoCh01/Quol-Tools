#pragma once

#include "plugin_api/IQuolPlugin.hpp"

#include <QObject>
#include <QString>
#include <QVector>

class QHBoxLayout;
class QPushButton;
class ToolBase;

class Misc final : public QObject, public IQuolPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID IQuolPlugin_iid)
    Q_INTERFACES(IQuolPlugin)

public:
    ~Misc() override;

    QWidget *createWidget(QWidget *parent = nullptr) override;
    void initialize(const QString &pluginRootPath, const PluginConfig &pluginConfig, QuolServices *services) override;
    void onUpdateConfig(const PluginConfig &pluginConfig) override;
    void shutdown() override;

private:
    void toggleTool(int index);

    struct ToolEntry {
        ToolBase *tool = nullptr;
        QPushButton *btn = nullptr;
    };

    QString m_pluginRootPath;
    PluginConfig m_cfg;
    QWidget *m_widget = nullptr;
    QuolServices *m_services = nullptr;

    QVector<ToolEntry> m_tools;
};
