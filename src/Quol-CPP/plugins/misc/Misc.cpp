#include "plugins/misc/Misc.hpp"
#include "plugins/misc/lib/ToolBase.hpp"
#include "plugins/misc/lib/StopwatchWidget.hpp"
#include "plugins/misc/lib/DiceWidget.hpp"
#include "plugins/misc/lib/ShaderWidget.hpp"

#include <QHBoxLayout>
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

        connect(entry.btn, &QPushButton::clicked, this, [this, i]() {
            toggleTool(i);
        });

        if (auto *shader = dynamic_cast<ShaderWidget *>(entry.tool)) {
            auto *row = new QHBoxLayout();
            row->setContentsMargins(0, 0, 0, 0);
            row->setSpacing(4);

            auto *settingsBtn = new QPushButton(QStringLiteral("..."), m_widget);
            settingsBtn->setFixedSize(24, 24);
            settingsBtn->setObjectName(QStringLiteral("btn-settings"));
            connect(settingsBtn, &QPushButton::clicked, shader, &ShaderWidget::openSettings);

            row->addWidget(entry.btn, 1);
            row->addWidget(settingsBtn);
            layout->addLayout(row);
        } else {
            layout->addWidget(entry.btn);
        }
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

    auto *dw = new DiceWidget(pluginRootPath);
    m_tools.append({dw, nullptr});

    auto *shader = new ShaderWidget(pluginRootPath);
    connect(shader, &ShaderWidget::closed, this, [this, shader]() {
        for (int i = 0; i < m_tools.size(); ++i) {
            if (m_tools[i].tool == shader && m_tools[i].btn) {
                m_tools[i].btn->setChecked(false);
                break;
            }
        }
    });
    m_tools.append({shader, nullptr});
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
