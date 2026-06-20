#pragma once

#include "plugin_api/IQuolPlugin.hpp"

#include <QList>
#include <QObject>
#include <QPair>
#include <QString>

class QGroupBox;
class QPushButton;
class QVBoxLayout;
class QWidget;

// A single mapping profile (group).
struct KeymapGroup {
    int id = -1;
    QString name;
    bool enabled = false;
    QList<QPair<QString, QString>> mappings;  // { src, dst }

    // Active remap handle IDs (one per mapping when group is enabled).
    QList<QString> remapIds;

    // UI widgets (owned by the plugin widget tree).
    QWidget *rowWidget = nullptr;
    QPushButton *enableBtn = nullptr;
    QPushButton *nameBtn = nullptr;
};

class Keymap final : public QObject, public IQuolPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID IQuolPlugin_iid)
    Q_INTERFACES(IQuolPlugin)

public:
    QWidget *createWidget(QWidget *parent = nullptr) override;
    void initialize(const QString &pluginRootPath, const PluginConfig &pluginConfig, QuolServices *services) override;
    void onUpdateConfig(const PluginConfig &pluginConfig) override;
    void shutdown() override;

private:
    // UI helpers
    void addGroupRow(
        const QString &name, const QList<QPair<QString, QString>> &mappings, bool enabled = false, int groupId = -1
    );
    int findGroupIndexById(int groupId) const;
    void removeGroup(int groupId);
    void openGroupDialog(int groupId);
    void toggleGroup(int groupId);
    void updateGroupHotkeys(KeymapGroup &group);
    void clearGroupHotkeys(KeymapGroup &group);

    // Persistence
    void saveKeymaps() const;
    void loadKeymaps();

    QString m_pluginRootPath;
    PluginConfig m_cfg;
    QuolServices *m_services = nullptr;

    QList<KeymapGroup> m_groups;
    int m_groupCounter = 0;

    // Top-level widget kept alive for height adjustments
    QWidget *m_rootWidget = nullptr;
    QVBoxLayout *m_rowsLayout = nullptr;

    QString m_keymapsPath;
};
