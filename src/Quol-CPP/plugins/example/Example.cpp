#include "plugins/example/Example.hpp"
#include "plugins/example/lib/adder.hpp"

#include "core/InputManager.hpp"
#include "plugin_api/QuolServices.hpp"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMetaObject>
#include <QVBoxLayout>
#include <QWidget>

QWidget *Example::createWidget(QWidget *parent) {
    auto *widget = new QWidget(parent);
    auto *layout = new QVBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);

    m_titleLabel = new QLabel(widget);
    m_titleLabel->setWordWrap(true);
    m_titleLabel->setAlignment(Qt::AlignCenter);

    m_valueLabel = new QLabel(widget);
    m_valueLabel->setWordWrap(true);
    m_valueLabel->setAlignment(Qt::AlignCenter);

    auto *headlineRow = new QHBoxLayout();
    headlineRow->addWidget(m_titleLabel, 1);
    headlineRow->addWidget(m_valueLabel, 1);
    layout->addLayout(headlineRow);

    auto *configGroup = new QGroupBox(QStringLiteral("Config Preview"), widget);
    auto *configLayout = new QVBoxLayout(configGroup);

    m_nestedNoteLabel = new QLabel(configGroup);
    m_nestedNoteLabel->setWordWrap(true);
    configLayout->addWidget(m_nestedNoteLabel);

    m_nestedEnabledLabel = new QLabel(configGroup);
    m_nestedEnabledLabel->setWordWrap(true);

    m_nestedModeLabel = new QLabel(configGroup);
    m_nestedModeLabel->setWordWrap(true);

    auto *modeRow = new QHBoxLayout();
    modeRow->addWidget(m_nestedEnabledLabel, 1);
    modeRow->addWidget(m_nestedModeLabel, 1);
    configLayout->addLayout(modeRow);

    auto *innerGroup = new QGroupBox(QStringLiteral("Nested Inner"), configGroup);
    auto *innerLayout = new QVBoxLayout(innerGroup);

    m_nestedInnerLabelLabel = new QLabel(innerGroup);
    m_nestedInnerLabelLabel->setWordWrap(true);
    innerLayout->addWidget(m_nestedInnerLabelLabel);

    m_nestedInnerChoiceLabel = new QLabel(innerGroup);
    m_nestedInnerChoiceLabel->setWordWrap(true);
    innerLayout->addWidget(m_nestedInnerChoiceLabel);

    configLayout->addWidget(innerGroup);
    layout->addWidget(configGroup);

    m_pressedLabel = new QLabel("Key down: (none)", widget);
    m_releasedLabel = new QLabel("Key up: (none)", widget);
    m_triggeredLabel = new QLabel("Triggered: (none)", widget);
    m_mouseLabel = new QLabel("Mouse: (none)", widget);

    auto *eventRow = new QHBoxLayout();
    eventRow->addWidget(m_pressedLabel, 1);
    eventRow->addWidget(m_releasedLabel, 1);
    layout->addLayout(eventRow);

    layout->addWidget(m_triggeredLabel);
    layout->addWidget(m_mouseLabel);

    refreshLabels();

    return widget;
}

void Example::initialize(const QString &pluginRootPath, const PluginConfig &pluginConfig, QuolServices *services) {
    m_pluginRootPath = pluginRootPath;
    m_cfg = pluginConfig;
    m_services = services;

    if (m_services && m_services->inputManager()) {
        m_keyListenId = m_services->inputManager()->addKeyListener([this](const QString &key, bool pressed) {
            if (pressed) {
                if (m_pressedLabel)
                    m_pressedLabel->setText(QStringLiteral("Key down: ") + key);
            } else {
                if (m_releasedLabel)
                    m_releasedLabel->setText(QStringLiteral("Key up: ") + key);
            }
        });

        m_mouseListenId = m_services->inputManager()->addMouseListener([this](const InputManager::MouseEvent &event) {
            QMetaObject::invokeMethod(this, [this, event]() { onMouseEvent(event); }, Qt::QueuedConnection);
        });
    }

    refreshLabels();
    applyHotkeyFromConfig();
}

void Example::onUpdateConfig(const PluginConfig &pluginConfig) {
    m_cfg = pluginConfig;
    refreshLabels();
    applyHotkeyFromConfig();
}

void Example::shutdown() {
    if (m_services && m_services->inputManager()) {
        if (!m_hotkeyId.isEmpty()) {
            m_services->inputManager()->removeHotkey(m_hotkeyId);
            m_hotkeyId.clear();
        }
        if (!m_keyListenId.isEmpty()) {
            m_services->inputManager()->removeKeyListener(m_keyListenId);
            m_keyListenId.clear();
        }
        if (!m_mouseListenId.isEmpty()) {
            m_services->inputManager()->removeMouseListener(m_mouseListenId);
            m_mouseListenId.clear();
        }
    }
}

void Example::applyHotkeyFromConfig() {
    if (!m_services || !m_services->inputManager())
        return;

    const QString combo = m_cfg.get("send_combo").toString().trimmed().toLower();

    if (!m_hotkeyId.isEmpty()) {
        m_services->inputManager()->removeHotkey(m_hotkeyId);
        m_hotkeyId.clear();
    }

    if (combo.isEmpty())
        return;

    m_hotkeyId = m_services->inputManager()->addHotkey(
        combo,
        [this, combo]() {
            if (m_triggeredLabel)
                m_triggeredLabel->setText(QStringLiteral("Triggered: ") + combo);
        },
        true
    );
}

void Example::onMouseEvent(InputManager::MouseEvent event) {
    if (!m_mouseLabel)
        return;
    const QString typeStr = [&]() -> QString {
        switch (event.type) {
            case InputManager::MouseEvent::Type::Move:
                return QStringLiteral("Move");
            case InputManager::MouseEvent::Type::LeftDown:
                return QStringLiteral("LDown");
            case InputManager::MouseEvent::Type::LeftUp:
                return QStringLiteral("LUp");
            case InputManager::MouseEvent::Type::RightDown:
                return QStringLiteral("RDown");
            case InputManager::MouseEvent::Type::RightUp:
                return QStringLiteral("RUp");
            case InputManager::MouseEvent::Type::MiddleDown:
                return QStringLiteral("MDown");
            case InputManager::MouseEvent::Type::MiddleUp:
                return QStringLiteral("MUp");
            case InputManager::MouseEvent::Type::WheelUp:
                return QStringLiteral("WheelUp");
            case InputManager::MouseEvent::Type::WheelDown:
                return QStringLiteral("WheelDown");
            default:
                return QStringLiteral("?");
        }
    }();
    m_mouseLabel->setText(
        QStringLiteral("Mouse: %1 (%2, %3)").arg(typeStr).arg(event.globalPos.x()).arg(event.globalPos.y())
    );
}

void Example::refreshLabels() {
    if (m_titleLabel) {
        const QString title = m_cfg.get("sss").toString();
        m_titleLabel->setText(title);
    }

    if (m_valueLabel) {
        m_valueLabel->setText(examplelib::calculateFromConfig(m_cfg.root()));
    }

    if (m_nestedNoteLabel) {
        m_nestedNoteLabel->setText(examplelib::nestedNoteLine(m_cfg.root()));
    }

    if (m_nestedEnabledLabel) {
        m_nestedEnabledLabel->setText(examplelib::nestedEnabledLine(m_cfg.root()));
    }

    if (m_nestedModeLabel) {
        m_nestedModeLabel->setText(examplelib::nestedModeLine(m_cfg.root()));
    }

    if (m_nestedInnerLabelLabel) {
        m_nestedInnerLabelLabel->setText(examplelib::nestedInnerLabelLine(m_cfg.root()));
    }

    if (m_nestedInnerChoiceLabel) {
        m_nestedInnerChoiceLabel->setText(examplelib::nestedInnerChoiceLine(m_cfg.root()));
    }
}
