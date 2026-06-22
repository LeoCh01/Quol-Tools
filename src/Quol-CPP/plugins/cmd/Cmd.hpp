#pragma once

#include "plugin_api/IQuolPlugin.hpp"

#include <QJsonArray>
#include <QObject>
#include <QString>
#include <QVector>

class QPlainTextEdit;
class QProcess;
class QVBoxLayout;
class QWidget;
class QuolPopupWindow;

struct CommandEntry {
    QString name;
    QString command;
    bool showOutput = false;
};

class Cmd final : public QObject, public IQuolPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID IQuolPlugin_iid)
    Q_INTERFACES(IQuolPlugin)

public:
    ~Cmd() override;

    QWidget *createWidget(QWidget *parent = nullptr) override;
    void initialize(const QString &pluginRootPath, const PluginConfig &pluginConfig, QuolServices *services) override;
    void onUpdateConfig(const PluginConfig &pluginConfig) override;
    void shutdown() override;

private:
    void openAddDialog();
    void addCommand(const CommandEntry &entry);
    void deleteCommand(int index);
    void runCommand(int index);
    void showOutput(const QString &text);
    void rebuildUi();

    void saveCommands();
    void loadCommands();

    QJsonArray serialize() const;
    void deserialize(const QJsonArray &arr);

    QString m_pluginRootPath;
    PluginConfig m_cfg;
    QString m_commandsPath;

    QWidget *m_widget = nullptr;
    QVBoxLayout *m_commandsLayout = nullptr;
    QVector<CommandEntry> m_commands;
    QWidget *m_commandsContainer = nullptr;

    QProcess *m_currentProcess = nullptr;
    QuolPopupWindow *m_outputWindow = nullptr;
    QPlainTextEdit *m_outputBrowser = nullptr;
};
