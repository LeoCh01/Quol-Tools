#include "plugins/misc/Misc.hpp"
#include "plugins/misc/lib/ToolBase.hpp"
#include "plugins/misc/lib/StopwatchWidget.hpp"

#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

Misc::~Misc() {
    for (auto &entry : m_tools) {
        if (entry.tool)
            entry.tool->stop();
    }
}

QWidget *Misc::createWidget(QWidget *parent) {
    m_widget = new QWidget(parent);
    auto *layout = new QVBoxLayout(m_widget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    for (int i = 0; i < m_tools.size(); ++i) {
        auto &entry = m_tools[i];
        entry.btn = new QPushButton(entry.tool->label(), m_widget);
        entry.btn->setObjectName(QStringLiteral("btn-toggle"));
        entry.btn->setCheckable(true);
        layout->addWidget(entry.btn);

        connect(entry.btn, &QPushButton::clicked, this, [this, i]() {
            toggleTool(i);
        });
    }

    layout->addStretch();
    return m_widget;
}

void Misc::initialize(const QString &pluginRootPath, const PluginConfig &pluginConfig, QuolServices *services) {
    m_pluginRootPath = pluginRootPath;
    m_cfg = pluginConfig;
    m_services = services;

    auto *sw = new StopwatchWidget(pluginRootPath);
    connect(sw, &StopwatchWidget::closed, this, [this]() {
        if (!m_tools.isEmpty() && m_tools[0].btn)
            m_tools[0].btn->setChecked(false);
    });
    m_tools.append({sw, nullptr});
}

void Misc::onUpdateConfig(const PluginConfig &pluginConfig) {
    m_cfg = pluginConfig;
}

void Misc::shutdown() {
    for (auto &entry : m_tools) {
        if (entry.tool) {
            entry.tool->stop();
            delete entry.tool;
        }
        entry.tool = nullptr;
        entry.btn = nullptr;
    }
    m_tools.clear();
}

void Misc::toggleTool(int index) {
    if (index < 0 || index >= m_tools.size())
        return;

    auto &entry = m_tools[index];
    if (!entry.tool || !entry.btn)
        return;

    if (entry.tool->widget()->isVisible()) {
        entry.tool->stop();
        entry.btn->setChecked(false);
    } else {
        entry.tool->start(m_services);
        entry.btn->setChecked(true);
    }
}
