#pragma once

#include "ui/QuolPopupWindow.hpp"

#include <QList>
#include <QPair>
#include <QString>

class QLineEdit;
class QScrollArea;
class QVBoxLayout;
class QWidget;

// Popup window for creating / editing a mapping group.
// Shows a name field and a scrollable list of src → dst key-pair rows.
class KeymapGroupDialog : public QuolPopupWindow {
    Q_OBJECT
public:
    explicit KeymapGroupDialog(QWidget *parent, const QString &name, const QList<QPair<QString, QString>> &mappings);

    QString groupName() const;
    QList<QPair<QString, QString>> mappings() const;

signals:
    void accepted();
    void rejected();

private:
    void addRow(const QString &src = {}, const QString &dst = {});
    void removeRow(QWidget *row);

    QLineEdit *m_nameEdit = nullptr;
    QVBoxLayout *m_rowsLayout = nullptr;
    QWidget *m_rowsContainer = nullptr;
    QScrollArea *m_scrollArea = nullptr;

    QList<QWidget *> m_rows;
};
