#include "plugins/cmd/Cmd.hpp"
#include "plugins/cmd/lib/CommandDialog.hpp"

#include <QFile>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QProcess>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

#include <qt_windows.h>

QWidget *Cmd::createWidget(QWidget *parent) {
    m_widget = new QWidget(parent);
    auto *root = new QVBoxLayout(m_widget);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(6);

    auto *group = new QGroupBox(QStringLiteral("Commands"), m_widget);
    auto *groupLayout = new QVBoxLayout(group);
    groupLayout->setContentsMargins(6, 6, 6, 6);
    groupLayout->setSpacing(6);

    m_commandsLayout = new QVBoxLayout();
    m_commandsLayout->setContentsMargins(0, 0, 0, 0);
    m_commandsLayout->setSpacing(4);
    groupLayout->addLayout(m_commandsLayout);

    root->addWidget(group);

    auto *addBtn = new QPushButton(QStringLiteral("Add Command"), m_widget);
    addBtn->setObjectName(QStringLiteral("btn-primary"));
    QObject::connect(addBtn, &QPushButton::clicked, this, &Cmd::openAddDialog);
    root->addWidget(addBtn);

    rebuildUi();

    return m_widget;
}

void Cmd::initialize(const QString &pluginRootPath, const PluginConfig &pluginConfig, QuolServices *services) {
    Q_UNUSED(pluginConfig)
    Q_UNUSED(services)
    m_commandsPath = pluginRootPath + QStringLiteral("/res/commands.json");

    loadCommands();
}

void Cmd::onUpdateConfig(const PluginConfig &pluginConfig) {
    Q_UNUSED(pluginConfig)
}

void Cmd::shutdown() {
}

void Cmd::openAddDialog() {
    auto *dialog = new CommandDialog(m_widget);
    QObject::connect(dialog, &CommandDialog::accepted, this, [this, dialog]() {
        CommandEntry entry;
        entry.name = dialog->commandName();
        entry.command = dialog->commandText();
        entry.workingDir = dialog->workingDir();
        entry.showOutput = dialog->showOutput();
        addCommand(entry);
    });
    dialog->show();
}

void Cmd::addCommand(const CommandEntry &entry) {
    m_commands.append(entry);
    rebuildUi();
    saveCommands();
}

void Cmd::deleteCommand(int index) {
    if (index < 0 || index >= m_commands.size())
        return;
    m_commands.removeAt(index);
    rebuildUi();
    saveCommands();
}

void Cmd::rebuildUi() {
    if (!m_commandsLayout)
        return;

    while (QLayoutItem *item = m_commandsLayout->takeAt(0)) {
        if (QWidget *w = item->widget())
            w->deleteLater();
        delete item;
    }

    for (int i = 0; i < m_commands.size(); ++i) {
        const CommandEntry &entry = m_commands[i];

        auto *row = new QWidget(m_widget);
        auto *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(0, 0, 0, 0);
        rowLayout->setSpacing(4);

        auto *runBtn = new QPushButton(entry.name, row);
        runBtn->setObjectName(QStringLiteral("btn-toggle"));
        QObject::connect(runBtn, &QPushButton::clicked, this, [this, i]() { runCommand(i); });
        rowLayout->addWidget(runBtn, 1);

        auto *delBtn = new QPushButton(QStringLiteral("\u2716"), row);
        delBtn->setFixedSize(22, 22);
        delBtn->setStyleSheet(QStringLiteral("QPushButton { font-size: 12px; padding: 0; border-radius: 3px; }"));
        QObject::connect(delBtn, &QPushButton::clicked, this, [this, i]() { deleteCommand(i); });
        rowLayout->addWidget(delBtn);

        m_commandsLayout->addWidget(row);
    }
}

void Cmd::runCommand(int index) {
    if (index < 0 || index >= m_commands.size())
        return;

    const CommandEntry &entry = m_commands[index];

    if (entry.showOutput) {
        QString cmd = QStringLiteral("cmd.exe /k ") + entry.command;
        QByteArray cmdBytes = cmd.toLocal8Bit();

        STARTUPINFOA si = {};
        si.cb = sizeof(si);
        PROCESS_INFORMATION pi = {};

        CreateProcessA(
            nullptr,
            cmdBytes.data(),
            nullptr, nullptr, FALSE,
            CREATE_NEW_CONSOLE,
            nullptr,
            entry.workingDir.isEmpty() ? nullptr : entry.workingDir.toLocal8Bit().constData(),
            &si, &pi
        );
        if (pi.hProcess) CloseHandle(pi.hProcess);
        if (pi.hThread) CloseHandle(pi.hThread);
    } else {
        QProcess proc;
        if (!entry.workingDir.isEmpty())
            proc.setWorkingDirectory(entry.workingDir);
        proc.startDetached(QStringLiteral("cmd.exe"), {QStringLiteral("/c"), entry.command});
    }
}

void Cmd::saveCommands() {
    QFile file(m_commandsPath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        file.write(QJsonDocument(serialize()).toJson(QJsonDocument::Indented));
        file.close();
    }
}

void Cmd::loadCommands() {
    QFile file(m_commandsPath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const auto arr = QJsonDocument::fromJson(file.readAll()).array();
        deserialize(arr);
        file.close();
    }
}

QJsonArray Cmd::serialize() const {
    QJsonArray arr;
    for (const auto &entry : m_commands)
        arr.append(QJsonArray{entry.name, entry.command, entry.workingDir, entry.showOutput});
    return arr;
}

void Cmd::deserialize(const QJsonArray &arr) {
    for (const auto &val : arr) {
        const QJsonArray item = val.toArray();
        if (item.size() >= 3) {
            CommandEntry entry;
            entry.name = item.at(0).toString();
            entry.command = item.at(1).toString();
            if (item.size() >= 4) {
                entry.workingDir = item.at(2).toString();
                entry.showOutput = item.at(3).toBool();
            } else {
                entry.showOutput = item.at(2).toBool();
            }
            m_commands.append(entry);
        }
    }
}
