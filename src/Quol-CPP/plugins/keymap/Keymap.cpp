#include "plugins/keymap/Keymap.hpp"
#include "plugins/keymap/lib/KeymapGroupDialog.hpp"

#include "core/InputManager.hpp"

#include <QFile>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPushButton>
#include <QSizePolicy>
#include <QVBoxLayout>
#include <QWidget>

// ─────────────────────────────────────────────────────────────────────────────
// IQuolPlugin interface
// ─────────────────────────────────────────────────────────────────────────────

QWidget *Keymap::createWidget(QWidget *parent) {
    m_rootWidget = new QWidget(parent);
    auto *outerLayout = new QVBoxLayout(m_rootWidget);
    outerLayout->setContentsMargins(0, 0, 0, 0);

    // GroupBox containing the mapping group rows
    auto *groupBox = new QGroupBox("Key Mappings", m_rootWidget);
    auto *groupBoxLayout = new QVBoxLayout(groupBox);
    groupBoxLayout->setContentsMargins(4, 4, 4, 4);
    groupBoxLayout->setSpacing(4);
    groupBoxLayout->addStretch();

    m_rowsLayout = groupBoxLayout;
    outerLayout->addWidget(groupBox);

    // "Add Mapping Group" button
    auto *addBtn = new QPushButton("+ Add Mapping Group", m_rootWidget);
    connect(addBtn, &QPushButton::clicked, this, [this]() {
        addGroupRow("Unnamed", {});
        saveKeymaps();
    });
    outerLayout->addWidget(addBtn);

    // Load saved keymaps now that the layout is ready
    loadKeymaps();

    return m_rootWidget;
}

void Keymap::initialize(const QString &pluginRootPath, const PluginConfig &pluginConfig, QuolServices *services) {
    m_pluginRootPath = pluginRootPath;
    m_cfg = pluginConfig;
    m_services = services;
    m_keymapsPath = pluginRootPath + "/res/keymaps.json";
}

void Keymap::onUpdateConfig(const PluginConfig &pluginConfig) {
    m_cfg = pluginConfig;
}

void Keymap::shutdown() {
    for (auto &g : m_groups)
        clearGroupHotkeys(g);
    m_groups.clear();
}

int Keymap::findGroupIndexById(int groupId) const {
    for (int i = 0; i < m_groups.size(); ++i) {
        if (m_groups[i].id == groupId)
            return i;
    }
    return -1;
}

// ─────────────────────────────────────────────────────────────────────────────
// UI
// ─────────────────────────────────────────────────────────────────────────────

void Keymap::addGroupRow(
    const QString &name, const QList<QPair<QString, QString>> &mappings, bool enabled, int groupId
) {
    if (!m_rowsLayout)
        return;

    const int index = m_groups.size();
    if (groupId < 0)
        groupId = m_groupCounter++;
    else if (groupId >= m_groupCounter)
        m_groupCounter = groupId + 1;

    KeymapGroup group;
    group.id = groupId;
    group.name = name;
    group.mappings = mappings;
    group.enabled = enabled;
    m_groups.append(group);

    // --- Row widget ---
    auto *row = new QWidget();
    auto *hl = new QHBoxLayout(row);
    hl->setContentsMargins(0, 0, 0, 0);
    hl->setSpacing(4);

    auto *nameBtn = new QPushButton(name, row);
    nameBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    auto *enableBtn = new QPushButton("✔", row);
    enableBtn->setObjectName("btn-toggle");
    enableBtn->setFixedSize(24, 24);
    enableBtn->setCheckable(true);
    enableBtn->setChecked(enabled);

    hl->addWidget(nameBtn);
    hl->addWidget(enableBtn);

    m_rowsLayout->insertWidget(m_rowsLayout->count() - 1, row);

    // Store pointers back into the group
    m_groups[index].rowWidget = row;
    m_groups[index].nameBtn = nameBtn;
    m_groups[index].enableBtn = enableBtn;

    // Register hotkeys if enabled
    if (enabled)
        updateGroupHotkeys(m_groups[index]);

    // --- Connections ---
    connect(enableBtn, &QPushButton::clicked, this, [this, groupId]() { toggleGroup(groupId); });
    connect(nameBtn, &QPushButton::clicked, this, [this, groupId]() { openGroupDialog(groupId); });
}

void Keymap::removeGroup(int groupId) {
    const int index = findGroupIndexById(groupId);
    if (index < 0 || index >= m_groups.size())
        return;

    auto &g = m_groups[index];
    clearGroupHotkeys(g);

    if (g.rowWidget) {
        m_rowsLayout->removeWidget(g.rowWidget);
        g.rowWidget->deleteLater();
    }
    m_groups.removeAt(index);
}

void Keymap::openGroupDialog(int groupId) {
    const int index = findGroupIndexById(groupId);
    if (index < 0 || index >= m_groups.size())
        return;

    auto *dialog = new KeymapGroupDialog(m_rootWidget, m_groups[index].name, m_groups[index].mappings);
    dialog->setGroupId(groupId);

    connect(dialog, &KeymapGroupDialog::accepted, this, [this, dialog, groupId]() {
        const int i = findGroupIndexById(groupId);
        if (i < 0 || i >= m_groups.size())
            return;

        const QString newName = dialog->groupName();
        if (newName.isEmpty())
            return;

        auto &grp = m_groups[i];
        grp.name = newName;
        grp.mappings = dialog->mappings();

        if (grp.nameBtn)
            grp.nameBtn->setText(newName);
        if (grp.enabled)
            updateGroupHotkeys(grp);

        saveKeymaps();
    });

    connect(dialog, &KeymapGroupDialog::deleteRequested, this, [this, groupId]() {
        removeGroup(groupId);
        saveKeymaps();
    });

    dialog->show();
}

void Keymap::toggleGroup(int groupId) {
    const int index = findGroupIndexById(groupId);
    if (index < 0 || index >= m_groups.size())
        return;

    auto &g = m_groups[index];
    g.enabled = !g.enabled;

    if (g.enableBtn)
        g.enableBtn->setChecked(g.enabled);

    if (g.enabled)
        updateGroupHotkeys(g);
    else
        clearGroupHotkeys(g);

    saveKeymaps();
}

// ─────────────────────────────────────────────────────────────────────────────
// Hotkey management
// ─────────────────────────────────────────────────────────────────────────────

void Keymap::updateGroupHotkeys(KeymapGroup &group) {
    if (!m_services || !m_services->inputManager())
        return;

    clearGroupHotkeys(group);

    auto *im = m_services->inputManager();
    for (const auto &pair : group.mappings) {
        const QString src = pair.first;
        const QString dst = pair.second;
        if (src.isEmpty() || dst.isEmpty())
            continue;

        const QString id = im->addKeyRemap(src, dst);
        if (!id.isEmpty())
            group.remapIds.append(id);
    }
}

void Keymap::clearGroupHotkeys(KeymapGroup &group) {
    if (!m_services || !m_services->inputManager())
        return;

    auto *im = m_services->inputManager();
    for (const QString &id : group.remapIds)
        im->removeKeyRemap(id);

    group.remapIds.clear();
}

// ─────────────────────────────────────────────────────────────────────────────
// Persistence
// ─────────────────────────────────────────────────────────────────────────────

void Keymap::saveKeymaps() const {
    QJsonObject root;
    for (const auto &g : m_groups) {
        QJsonArray arr;
        for (const auto &pair : g.mappings) {
            QJsonArray entry;
            entry.append(pair.first);
            entry.append(pair.second);
            arr.append(entry);
        }
        QJsonObject groupObj;
        groupObj["mappings"] = arr;
        groupObj["enabled"] = g.enabled;
        root[g.name] = groupObj;
    }

    QFile f(m_keymapsPath);
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        f.write(QJsonDocument(root).toJson());
}

void Keymap::loadKeymaps() {
    QFile f(m_keymapsPath);
    if (!f.open(QIODevice::ReadOnly))
        return;

    const QJsonObject root = QJsonDocument::fromJson(f.readAll()).object();
    for (auto it = root.begin(); it != root.end(); ++it) {
        const QString name = it.key();
        const QJsonObject obj = it.value().toObject();
        const bool enabled = obj.value("enabled").toBool(false);
        const QJsonArray arr = obj.value("mappings").toArray();

        QList<QPair<QString, QString>> mappings;
        for (const auto &val : arr) {
            const QJsonArray entry = val.toArray();
            if (entry.size() >= 2)
                mappings.append({entry[0].toString(), entry[1].toString()});
        }
        addGroupRow(name, mappings, enabled);
    }
}
