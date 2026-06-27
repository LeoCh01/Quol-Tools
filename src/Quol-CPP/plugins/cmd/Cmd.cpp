#include "plugins/cmd/Cmd.hpp"
#include "plugins/cmd/lib/CommandDialog.hpp"

#include <QFile>
#include <QFontDatabase>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonValue>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

#include "ui/QuolPopupWindow.hpp"

Cmd::~Cmd() {
    shutdown();
}

QWidget *Cmd::createWidget(QWidget *parent) {
    m_widget = new QWidget(parent);
    auto *root = new QVBoxLayout(m_widget);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(6);

    auto *group = new QGroupBox(QStringLiteral("Commands"), m_widget);
    auto *groupLayout = new QVBoxLayout(group);
    groupLayout->setContentsMargins(6, 6, 6, 6);
    groupLayout->setSpacing(6);

    m_commandsContainer = new QWidget(group);
    m_commandsLayout = new QVBoxLayout(m_commandsContainer);
    m_commandsLayout->setContentsMargins(0, 0, 0, 0);
    m_commandsLayout->setSpacing(4);
    groupLayout->addWidget(m_commandsContainer);

    root->addWidget(group);

    auto *addBtn = new QPushButton(QStringLiteral("Add Command"), m_widget);
    addBtn->setObjectName(QStringLiteral("btn-primary"));
    QObject::connect(addBtn, &QPushButton::clicked, this, &Cmd::openAddDialog);
    root->addWidget(addBtn);

    rebuildUi();

    return m_widget;
}

void Cmd::initialize(const QString &pluginRootPath, const PluginConfig &pluginConfig, QuolServices *services) {
    Q_UNUSED(services)
    m_pluginRootPath = pluginRootPath;
    m_cfg = pluginConfig;
    m_commandsPath = pluginRootPath + QStringLiteral("/res/commands.json");

    loadCommands();
}

void Cmd::onUpdateConfig(const PluginConfig &pluginConfig) {
    m_cfg = pluginConfig;
}

void Cmd::shutdown() {
    if (m_currentProcess) {
        m_currentProcess->kill();
        m_currentProcess->waitForFinished(3000);
        m_currentProcess->deleteLater();
        m_currentProcess = nullptr;
    }
    if (m_outputWindow) {
        m_outputWindow->close();
        m_outputWindow = nullptr;
    }
    m_widget = nullptr;
    m_commandsLayout = nullptr;
    m_commandsContainer = nullptr;
}

void Cmd::openAddDialog() {
    auto *dialog = new CommandDialog(m_widget);
    QObject::connect(dialog, &CommandDialog::accepted, this, [this, dialog]() {
        CommandEntry entry;
        entry.name = dialog->commandName();
        entry.command = dialog->commandText();
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

        auto *row = new QWidget(m_commandsContainer);
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

    if (m_currentProcess) {
        m_currentProcess->kill();
        m_currentProcess->waitForFinished(3000);
        m_currentProcess->deleteLater();
        m_currentProcess = nullptr;
    }

    m_currentProcess = new QProcess(this);
    m_currentProcess->setProcessChannelMode(QProcess::MergedChannels);

    QObject::connect(
        m_currentProcess,
        QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
        this,
        [this, entry](int exitCode, QProcess::ExitStatus) {
            const QString output = QString::fromLocal8Bit(m_currentProcess->readAllStandardOutput());
            if (entry.showOutput) {
                QString text = output;
                if (exitCode != 0)
                    text += QStringLiteral("\n\nExit code: %1").arg(exitCode);
                showOutput(text);
            }
            m_currentProcess->deleteLater();
            m_currentProcess = nullptr;
        }
    );

    m_currentProcess->start(QStringLiteral("cmd.exe"), {QStringLiteral("/c"), entry.command});
}

void Cmd::showOutput(const QString &text) {
    if (!m_outputWindow) {
        m_outputWindow = new QuolPopupWindow(QStringLiteral("Command Output"), m_widget);
        QObject::connect(m_outputWindow, &QObject::destroyed, this, [this]() {
            m_outputWindow = nullptr;
            m_outputBrowser = nullptr;
        });
        m_outputWindow->resize(600, 400);

        m_outputBrowser = new QPlainTextEdit(m_outputWindow);
        m_outputBrowser->setReadOnly(true);
        m_outputBrowser->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
        m_outputWindow->addContent(m_outputBrowser);
    }

    m_outputBrowser->setPlainText(text);
    m_outputWindow->show();
    m_outputWindow->raise();
    m_outputWindow->activateWindow();
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
        arr.append(QJsonArray{entry.name, entry.command, entry.showOutput});
    return arr;
}

void Cmd::deserialize(const QJsonArray &arr) {
    for (const auto &val : arr) {
        const QJsonArray item = val.toArray();
        if (item.size() >= 3) {
            CommandEntry entry;
            entry.name = item.at(0).toString();
            entry.command = item.at(1).toString();
            entry.showOutput = item.at(2).toBool();
            m_commands.append(entry);
        }
    }
}
