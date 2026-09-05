#include "plugins/cmd/lib/CommandDialog.hpp"

#include <QCheckBox>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

CommandDialog::CommandDialog(QWidget *parent) : QuolPopupWindow(QStringLiteral("Add Command"), parent) {
    resize(520, 420);

    auto *content = new QWidget(this);
    auto *layout = new QVBoxLayout(content);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    layout->addWidget(new QLabel(QStringLiteral("Command Name:"), content));

    m_nameInput = new QLineEdit(content);
    m_nameInput->setPlaceholderText(QStringLiteral("Enter command name..."));
    layout->addWidget(m_nameInput);

    layout->addWidget(new QLabel(QStringLiteral("Command:"), content));

    m_commandInput = new QPlainTextEdit(content);
    m_commandInput->setPlaceholderText(
        QStringLiteral("Add terminal command...\n\nExample: start https://www.google.com\n")
    );
    m_commandInput->setFixedHeight(120);
    layout->addWidget(m_commandInput);

    layout->addWidget(new QLabel(QStringLiteral("Working Directory (optional):"), content));

    auto *pathRow = new QWidget(content);
    auto *pathLayout = new QHBoxLayout(pathRow);
    pathLayout->setContentsMargins(0, 0, 0, 0);
    pathLayout->setSpacing(4);

    m_pathInput = new QLineEdit(pathRow);
    m_pathInput->setPlaceholderText(QStringLiteral("Default: current directory"));
    pathLayout->addWidget(m_pathInput, 1);

    auto *browseBtn = new QPushButton(QStringLiteral("Browse"), pathRow);
    browseBtn->setObjectName(QStringLiteral("btn-toggle"));
    QObject::connect(browseBtn, &QPushButton::clicked, this, &CommandDialog::browseDirectory);
    pathLayout->addWidget(browseBtn);

    layout->addWidget(pathRow);

    m_showOutputCheck = new QCheckBox(QStringLiteral("Open in CMD Terminal"), content);
    layout->addWidget(m_showOutputCheck);

    auto *saveBtn = new QPushButton(QStringLiteral("Save Command"), content);
    saveBtn->setObjectName(QStringLiteral("btn-primary"));
    QObject::connect(saveBtn, &QPushButton::clicked, this, &CommandDialog::onSave);
    layout->addWidget(saveBtn);

    addContent(content);
}

QString CommandDialog::commandName() const {
    return m_nameInput->text().trimmed();
}

QString CommandDialog::commandText() const {
    return m_commandInput->toPlainText().trimmed();
}

QString CommandDialog::workingDir() const {
    return m_pathInput->text().trimmed();
}

bool CommandDialog::showOutput() const {
    return m_showOutputCheck->isChecked();
}

void CommandDialog::browseDirectory() {
    const QString dir = QFileDialog::getExistingDirectory(this, QStringLiteral("Select Working Directory"));
    if (!dir.isEmpty())
        m_pathInput->setText(dir);
}

void CommandDialog::onSave() {
    if (commandName().isEmpty() || commandText().isEmpty())
        return;
    emit accepted();
    close();
}
