#include "plugins/cmd/lib/CommandDialog.hpp"

#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

CommandDialog::CommandDialog(QWidget *parent) : QuolPopupWindow(QStringLiteral("Add Command"), parent) {
    resize(520, 360);

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

    m_showOutputCheck = new QCheckBox(QStringLiteral("Show Output in Terminal"), content);
    layout->addWidget(m_showOutputCheck);

    auto *saveBtn = new QPushButton(QStringLiteral("Save Command"), content);
    saveBtn->setObjectName(QStringLiteral("btn-primary"));
    QObject::connect(saveBtn, &QPushButton::clicked, this, &CommandDialog::onSave);
    layout->addWidget(saveBtn);

    addContent(content);
}

QString CommandDialog::commandName() const {
    return m_nameInput ? m_nameInput->text().trimmed() : QString();
}

QString CommandDialog::commandText() const {
    return m_commandInput ? m_commandInput->toPlainText().trimmed() : QString();
}

bool CommandDialog::showOutput() const {
    return m_showOutputCheck ? m_showOutputCheck->isChecked() : false;
}

void CommandDialog::onSave() {
    if (commandName().isEmpty() || commandText().isEmpty())
        return;
    emit accepted();
    close();
}
