#pragma once

#include "plugin_api/IQuolPlugin.hpp"

#include <QJsonArray>
#include <QObject>
#include <QString>
#include <QVector>

class QVBoxLayout;
class QWidget;

struct CommandEntry {
    QString name;
    QString command;
    QString workingDir;
    bool showOutput = false;
};

class Cmd final : public QObject, public IQuolPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID IQuolPlugin_iid)
    Q_INTERFACES(IQuolPlugin)

public:
    QWidget *createWidget(QWidget *parent = nullptr) override;
    void initialize(const QString &pluginRootPath, const PluginConfig &pluginConfig, QuolServices *services) override;
    void onUpdateConfig(const PluginConfig &pluginConfig) override;
    void shutdown() override;

private:
    void openAddDialog();
    void addCommand(const CommandEntry &entry);
    void deleteCommand(int index);
    void runCommand(int index);
    void rebuildUi();

    void saveCommands();
    void loadCommands();

    QJsonArray serialize() const;
    void deserialize(const QJsonArray &arr);

    QString m_commandsPath;

    QWidget *m_widget = nullptr;
    QVBoxLayout *m_commandsLayout = nullptr;
    QVector<CommandEntry> m_commands;
};
