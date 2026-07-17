#include "plugins/keymap/lib/KeymapGroupDialog.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSizePolicy>
#include <QVBoxLayout>
#include <QWidget>

KeymapGroupDialog::KeymapGroupDialog(
    QWidget *parent, const QString &name, const QList<QPair<QString, QString>> &mappings
)
    : QuolPopupWindow(name.isEmpty() ? "New Mapping Group" : "Edit: " + name, parent) {
    resize(420, 460);

    auto *panel = new QWidget(this);
    auto *mainLayout = new QVBoxLayout(panel);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(10);

    // --- Name row ---
    auto *nameRow = new QHBoxLayout();
    nameRow->addWidget(new QLabel("Group name:", panel));
    m_nameEdit = new QLineEdit(name, panel);
    m_nameEdit->setPlaceholderText("e.g. Caps → Ctrl");
    nameRow->addWidget(m_nameEdit, 1);
    mainLayout->addLayout(nameRow);

    // --- Column header ---
    auto *header = new QHBoxLayout();
    auto *srcLabel = new QLabel("Source", panel);
    header->addWidget(srcLabel, 1);
    auto *arrowLabel = new QLabel(QStringLiteral("→"), panel);
    arrowLabel->setAlignment(Qt::AlignCenter);
    header->addWidget(arrowLabel);
    auto *dstLabel = new QLabel("Target", panel);
    header->addWidget(dstLabel, 1);
    header->addSpacing(28);
    mainLayout->addLayout(header);

    // --- Scrollable rows ---
    m_rowsContainer = new QWidget(panel);
    m_rowsLayout = new QVBoxLayout(m_rowsContainer);
    m_rowsLayout->setContentsMargins(0, 0, 0, 0);
    m_rowsLayout->setSpacing(6);
    m_rowsLayout->setAlignment(Qt::AlignTop);
    m_rowsLayout->addStretch();

    m_scrollArea = new QScrollArea(panel);
    m_scrollArea->setWidget(m_rowsContainer);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    mainLayout->addWidget(m_scrollArea, 1);

    for (const auto &pair : mappings)
        addRow(pair.first, pair.second);

    // --- Add row button ---
    auto *addBtn = new QPushButton("+ Add Mapping", panel);
    connect(addBtn, &QPushButton::clicked, this, [this]() { addRow(); });
    mainLayout->addWidget(addBtn);

    // --- Delete group + Save / Cancel ---
    auto *actions = new QHBoxLayout();

    auto *deleteBtn = new QPushButton("Delete Group", panel);
    deleteBtn->setObjectName("btn-danger");
    actions->addWidget(deleteBtn);

    actions->addStretch();

    auto *cancelBtn = new QPushButton("Cancel", panel);
    auto *saveBtn = new QPushButton("Save", panel);
    actions->addWidget(cancelBtn);
    actions->addWidget(saveBtn);
    mainLayout->addLayout(actions);

    connect(cancelBtn, &QPushButton::clicked, this, [this]() { close(); });
    connect(saveBtn, &QPushButton::clicked, this, [this]() {
        emit accepted();
        close();
    });
    connect(deleteBtn, &QPushButton::clicked, this, [this]() {
        emit deleteRequested(m_groupId);
        close();
    });

    addContent(panel);
}

void KeymapGroupDialog::setGroupId(int id) {
    m_groupId = id;
}

int KeymapGroupDialog::groupId() const {
    return m_groupId;
}

QString KeymapGroupDialog::groupName() const {
    return m_nameEdit->text().trimmed();
}

QList<QPair<QString, QString>> KeymapGroupDialog::mappings() const {
    QList<QPair<QString, QString>> result;
    for (auto *row : m_rows) {
        auto *srcEdit = row->findChild<QLineEdit *>("src");
        auto *dstEdit = row->findChild<QLineEdit *>("dst");
        if (!srcEdit || !dstEdit)
            continue;
        const QString src = srcEdit->text().trimmed().toLower();
        const QString dst = dstEdit->text().trimmed().toLower();
        if (!src.isEmpty() && !dst.isEmpty())
            result.append({src, dst});
    }
    return result;
}

void KeymapGroupDialog::addRow(const QString &src, const QString &dst) {
    auto *row = new QWidget(m_rowsContainer);

    auto *hl = new QHBoxLayout(row);
    hl->setContentsMargins(4, 2, 4, 2);
    hl->setSpacing(8);

    auto *srcEdit = new QLineEdit(src, row);
    srcEdit->setObjectName("src");
    srcEdit->setPlaceholderText("e.g. caps");

    auto *arrow = new QLabel(QStringLiteral("→"), row);
    arrow->setAlignment(Qt::AlignCenter);

    auto *dstEdit = new QLineEdit(dst, row);
    dstEdit->setObjectName("dst");
    dstEdit->setPlaceholderText("e.g. ctrl");

    auto *delBtn = new QPushButton("✕", row);
    delBtn->setObjectName("btn-danger");
    delBtn->setToolTip("Remove mapping");
    delBtn->setFixedSize(28, 28);
    connect(delBtn, &QPushButton::clicked, this, [this, row]() { removeRow(row); });

    hl->addWidget(srcEdit, 1);
    hl->addWidget(arrow);
    hl->addWidget(dstEdit, 1);
    hl->addWidget(delBtn);

    m_rowsLayout->insertWidget(m_rowsLayout->count() - 1, row);
    m_rows.append(row);
    row->show();
}

void KeymapGroupDialog::removeRow(QWidget *row) {
    m_rows.removeOne(row);
    m_rowsLayout->removeWidget(row);
    row->deleteLater();
}
