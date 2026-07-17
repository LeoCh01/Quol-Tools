#pragma once

#include "ui/QuolPopupWindow.hpp"

#include <QList>
#include <QPair>
#include <QString>

class QLineEdit;
class QScrollArea;
class QVBoxLayout;
class QWidget;

class KeymapGroupDialog : public QuolPopupWindow {
    Q_OBJECT
public:
    explicit KeymapGroupDialog(QWidget *parent, const QString &name, const QList<QPair<QString, QString>> &mappings);

    void setGroupId(int id);
    int groupId() const;
    QString groupName() const;
    QList<QPair<QString, QString>> mappings() const;

signals:
    void accepted();
    void deleteRequested(int groupId);

private:
    void addRow(const QString &src = {}, const QString &dst = {});
    void removeRow(QWidget *row);

    int m_groupId = -1;
    QLineEdit *m_nameEdit = nullptr;
    QVBoxLayout *m_rowsLayout = nullptr;
    QWidget *m_rowsContainer = nullptr;
    QScrollArea *m_scrollArea = nullptr;

    QList<QWidget *> m_rows;
};
