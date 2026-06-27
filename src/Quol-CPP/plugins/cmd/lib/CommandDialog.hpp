#pragma once

#include "ui/QuolPopupWindow.hpp"

class QCheckBox;
class QLineEdit;
class QPlainTextEdit;

class CommandDialog : public QuolPopupWindow {
    Q_OBJECT

public:
    explicit CommandDialog(QWidget *parent = nullptr);

    QString commandName() const;
    QString commandText() const;
    bool showOutput() const;

signals:
    void accepted();

private:
    void onSave();

    QLineEdit *m_nameInput = nullptr;
    QPlainTextEdit *m_commandInput = nullptr;
    QCheckBox *m_showOutputCheck = nullptr;
};
