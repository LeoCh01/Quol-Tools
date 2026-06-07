#include "plugins/macro/Macro.hpp"

#include "core/InputManager.hpp"
#include "plugin_api/QuolServices.hpp"

#include <windows.h>

#include <QCheckBox>
#include <QFileDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSizePolicy>
#include <QVBoxLayout>
#include <QWidget>

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static QString eventTypeToString(MacroEvent::Type t) {
    switch (t) {
        case MacroEvent::Type::KeyDown:
            return "KeyDown";
        case MacroEvent::Type::KeyUp:
            return "KeyUp";
        case MacroEvent::Type::MouseMove:
            return "MouseMove";
        case MacroEvent::Type::MouseLeftDown:
            return "MouseLeftDown";
        case MacroEvent::Type::MouseLeftUp:
            return "MouseLeftUp";
        case MacroEvent::Type::MouseRightDown:
            return "MouseRightDown";
        case MacroEvent::Type::MouseRightUp:
            return "MouseRightUp";
        case MacroEvent::Type::MouseMiddleDown:
            return "MouseMiddleDown";
        case MacroEvent::Type::MouseMiddleUp:
            return "MouseMiddleUp";
        case MacroEvent::Type::MouseWheelUp:
            return "MouseWheelUp";
        case MacroEvent::Type::MouseWheelDown:
            return "MouseWheelDown";
    }
    return "Unknown";
}

static MacroEvent::Type eventTypeFromString(const QString &s) {
    if (s == "KeyDown")
        return MacroEvent::Type::KeyDown;
    if (s == "KeyUp")
        return MacroEvent::Type::KeyUp;
    if (s == "MouseMove")
        return MacroEvent::Type::MouseMove;
    if (s == "MouseLeftDown")
        return MacroEvent::Type::MouseLeftDown;
    if (s == "MouseLeftUp")
        return MacroEvent::Type::MouseLeftUp;
    if (s == "MouseRightDown")
        return MacroEvent::Type::MouseRightDown;
    if (s == "MouseRightUp")
        return MacroEvent::Type::MouseRightUp;
    if (s == "MouseMiddleDown")
        return MacroEvent::Type::MouseMiddleDown;
    if (s == "MouseMiddleUp")
        return MacroEvent::Type::MouseMiddleUp;
    if (s == "MouseWheelUp")
        return MacroEvent::Type::MouseWheelUp;
    if (s == "MouseWheelDown")
        return MacroEvent::Type::MouseWheelDown;
    return MacroEvent::Type::KeyDown;
}

// ─────────────────────────────────────────────────────────────────────────────
// Mouse injection via Win32 SendInput
// ─────────────────────────────────────────────────────────────────────────────

void Macro::injectMouse(int x, int y, MacroEvent::Type type) {
    INPUT inp = {};
    inp.type = INPUT_MOUSE;

    // Normalise coordinates to [0, 65535] for MOUSEEVENTF_ABSOLUTE
    const int screenW = GetSystemMetrics(SM_CXSCREEN);
    const int screenH = GetSystemMetrics(SM_CYSCREEN);
    inp.mi.dx = (screenW > 0) ? (x * 65535 / screenW) : 0;
    inp.mi.dy = (screenH > 0) ? (y * 65535 / screenH) : 0;
    inp.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_VIRTUALDESK;

    switch (type) {
        case MacroEvent::Type::MouseMove:
            inp.mi.dwFlags |= MOUSEEVENTF_MOVE;
            break;
        case MacroEvent::Type::MouseLeftDown:
            inp.mi.dwFlags |= MOUSEEVENTF_LEFTDOWN;
            break;
        case MacroEvent::Type::MouseLeftUp:
            inp.mi.dwFlags |= MOUSEEVENTF_LEFTUP;
            break;
        case MacroEvent::Type::MouseRightDown:
            inp.mi.dwFlags |= MOUSEEVENTF_RIGHTDOWN;
            break;
        case MacroEvent::Type::MouseRightUp:
            inp.mi.dwFlags |= MOUSEEVENTF_RIGHTUP;
            break;
        case MacroEvent::Type::MouseMiddleDown:
            inp.mi.dwFlags |= MOUSEEVENTF_MIDDLEDOWN;
            break;
        case MacroEvent::Type::MouseMiddleUp:
            inp.mi.dwFlags |= MOUSEEVENTF_MIDDLEUP;
            break;
        case MacroEvent::Type::MouseWheelUp:
            inp.mi.dwFlags |= MOUSEEVENTF_WHEEL;
            inp.mi.mouseData = 120;
            break;
        case MacroEvent::Type::MouseWheelDown:
            inp.mi.dwFlags |= MOUSEEVENTF_WHEEL;
            inp.mi.mouseData = static_cast<DWORD>(-120);
            break;
        default:
            return;
    }
    SendInput(1, &inp, sizeof(INPUT));
}

// ─────────────────────────────────────────────────────────────────────────────
// IQuolPlugin
// ─────────────────────────────────────────────────────────────────────────────

void Macro::initialize(const QString &pluginRootPath, const PluginConfig &pluginConfig, QuolServices *services) {
    m_pluginRootPath = pluginRootPath;
    m_cfg = pluginConfig;
    m_services = services;
    m_macroFilePath = pluginRootPath + "/res/macro.json";
    m_hotkeyDebounceTimer.start();

    connect(&m_playTimer, &QTimer::timeout, this, &Macro::scheduleNextEvent);
    applyActionIcons();
    applyHotkeys();
}

QWidget *Macro::createWidget(QWidget *parent) {
    m_rootWidget = new QWidget(parent);
    auto *outerLayout = new QVBoxLayout(m_rootWidget);
    outerLayout->setContentsMargins(0, 0, 0, 0);

    // ── Top status row ───────────────────────────────────────────────────
    auto *statusRow = new QHBoxLayout();

    m_idleLabel = new QLabel("IDLE", m_rootWidget);
    m_idleLabel->setAlignment(Qt::AlignCenter);

    m_eventCountLabel = new QLabel("0", m_rootWidget);
    m_eventCountLabel->setAlignment(Qt::AlignCenter);

    m_hotkeyToggleBtn = new QPushButton("OFF", m_rootWidget);
    m_hotkeyToggleBtn->setObjectName("btn-danger");
    m_hotkeyToggleBtn->setCheckable(true);
    connect(m_hotkeyToggleBtn, &QPushButton::clicked, this, [this]() {
        if (m_hotkeysActive)
            clearHotkeys();
        else
            applyHotkeys();
        updateUi();
    });

    statusRow->addWidget(m_idleLabel, 1);
    statusRow->addWidget(m_eventCountLabel, 1);
    statusRow->addWidget(m_hotkeyToggleBtn, 1);
    outerLayout->addLayout(statusRow);

    // ── Recording inputs ──────────────────────────────────────────────────
    auto *recordBox = new QGroupBox("Recording", m_rootWidget);
    auto *recordLayout = new QHBoxLayout(recordBox);

    m_recordKeysCheck = new QCheckBox("Keys", recordBox);
    m_recordMouseCheck = new QCheckBox("Mouse", recordBox);
    m_recordKeysCheck->setChecked(true);
    m_recordMouseCheck->setChecked(true);
    recordLayout->addWidget(m_recordKeysCheck);
    recordLayout->addWidget(m_recordMouseCheck);

    outerLayout->addWidget(recordBox);

    // ── Playback settings ─────────────────────────────────────────────────
    auto *settingsBox = new QGroupBox("Playback Settings", m_rootWidget);
    auto *settingsLayout = new QVBoxLayout(settingsBox);

    // Speed
    auto *speedRow = new QHBoxLayout();
    speedRow->addWidget(new QLabel("Speed:", settingsBox));
    m_speedEdit = new QLineEdit(settingsBox);
    m_speedEdit->setPlaceholderText("1.0");
    m_speedEdit->setText("1.0");
    speedRow->addWidget(m_speedEdit);
    settingsLayout->addLayout(speedRow);

    // Repeat
    auto *repeatRow = new QHBoxLayout();
    repeatRow->addWidget(new QLabel("Repeat:", settingsBox));
    m_repeatEdit = new QLineEdit(settingsBox);
    m_repeatEdit->setPlaceholderText("1");
    m_repeatEdit->setText("1");
    repeatRow->addWidget(m_repeatEdit);
    settingsLayout->addLayout(repeatRow);

    outerLayout->addWidget(settingsBox);

    // ── Save / Load ───────────────────────────────────────────────────────
    auto *ioBox = new QGroupBox("File", m_rootWidget);
    auto *ioLayout = new QHBoxLayout(ioBox);

    m_saveBtn = new QPushButton("", ioBox);
    m_loadBtn = new QPushButton("", ioBox);
    m_saveBtn->setToolTip("Save Macro");
    m_loadBtn->setToolTip("Load Macro");
    applyActionIcons();

    connect(m_saveBtn, &QPushButton::clicked, this, &Macro::saveMacro);
    connect(m_loadBtn, &QPushButton::clicked, this, &Macro::loadMacro);

    ioLayout->addWidget(m_saveBtn);
    ioLayout->addWidget(m_loadBtn);
    outerLayout->addWidget(ioBox);

    updateUi();
    return m_rootWidget;
}

void Macro::onUpdateConfig(const PluginConfig &pluginConfig) {
    m_cfg = pluginConfig;
    clearHotkeys();
    applyHotkeys();
}

void Macro::shutdown() {
    stopPlaying();
    stopRecording();
    clearHotkeys();
}

// ─────────────────────────────────────────────────────────────────────────────
// Record / Play
// ─────────────────────────────────────────────────────────────────────────────

void Macro::startRecording() {
    if (m_state != MacroState::Idle)
        return;

    m_events.clear();
    m_state = MacroState::Recording;
    m_recordTimer.start();
    m_lastEventMs = 0;

    attachListeners();
    updateUi();
}

void Macro::stopRecording() {
    if (m_state != MacroState::Recording)
        return;

    detachListeners();

    // If stop key leaked into the end of recording, drop trailing control key events.
    while (!m_events.isEmpty()) {
        const MacroEvent &last = m_events.last();
        const bool isKeyEvt = (last.type == MacroEvent::Type::KeyDown || last.type == MacroEvent::Type::KeyUp);
        if (!isKeyEvt || !isControlHotkeyKey(last.key))
            break;
        m_events.removeLast();
    }

    m_state = MacroState::Idle;
    updateUi();
}

void Macro::startPlaying() {
    if (m_state != MacroState::Idle || m_events.isEmpty())
        return;

    m_state = MacroState::Playing;
    m_playIndex = 0;
    m_repeatsDone = 0;
    updateUi();
    scheduleNextEvent();
}

void Macro::stopPlaying() {
    if (m_state != MacroState::Playing)
        return;

    m_playTimer.stop();
    m_state = MacroState::Idle;
    updateUi();
}

void Macro::scheduleNextEvent() {
    if (m_state != MacroState::Playing)
        return;

    const int repeats = playbackRepeat();
    const double speed = playbackSpeed();

    if (m_playIndex >= m_events.size()) {
        // End of one pass
        m_repeatsDone++;
        if (m_repeatsDone < repeats) {
            m_playIndex = 0;
        } else {
            stopPlaying();
            return;
        }
        if (m_events.isEmpty()) {
            stopPlaying();
            return;
        }
    }

    // Dispatch the current event
    const MacroEvent &evt = m_events.at(m_playIndex);
    auto *im = m_services ? m_services->inputManager() : nullptr;

    switch (evt.type) {
        case MacroEvent::Type::KeyDown:
            if (im)
                im->sendPress(evt.key);
            break;
        case MacroEvent::Type::KeyUp:
            if (im)
                im->sendRelease(evt.key);
            break;
        default:
            injectMouse(evt.x, evt.y, evt.type);
            break;
    }

    m_playIndex++;

    // Determine delay before next event
    qint64 nextDelay = 0;
    if (m_playIndex < m_events.size()) {
        nextDelay = static_cast<qint64>(static_cast<double>(m_events.at(m_playIndex).delayMs) / speed);
    }

    m_playTimer.setSingleShot(true);
    m_playTimer.start(static_cast<int>(qMax<qint64>(0, nextDelay)));
}

// ─────────────────────────────────────────────────────────────────────────────
// Hotkeys
// ─────────────────────────────────────────────────────────────────────────────

void Macro::applyHotkeys() {
    if (!m_services)
        return;
    auto *im = m_services->inputManager();

    clearHotkeys();

    const QString hkRecord = m_cfg.get("hotkey_record", "f1").toString().trimmed().toLower();
    const QString hkStopRecord = m_cfg.get("hotkey_stop_record", "f1").toString().trimmed().toLower();
    const QString hkPlay = m_cfg.get("hotkey_play", "f2").toString().trimmed().toLower();
    const QString hkStopPlay = m_cfg.get("hotkey_stop_play", "f3").toString().trimmed().toLower();

    m_controlHotkeyTokens.clear();
    for (const QString &combo : {hkRecord, hkStopRecord, hkPlay, hkStopPlay}) {
        const QStringList parts = combo.split('+', Qt::SkipEmptyParts);
        for (const QString &part : parts)
            m_controlHotkeyTokens.append(part.trimmed().toLower());
    }
    m_controlHotkeyTokens.removeDuplicates();

    if (!hkRecord.isEmpty()) {
        m_hkRecord = im->addHotkey(
            hkRecord,
            [this]() {
                const qint64 nowMs = m_hotkeyDebounceTimer.elapsed();
                if ((nowMs - m_lastRecordToggleMs) < 250)
                    return;
                m_lastRecordToggleMs = nowMs;

                if (m_state == MacroState::Recording)
                    stopRecording();
                else if (m_state == MacroState::Idle)
                    startRecording();
            },
            true
        );
    }
    if (!hkStopRecord.isEmpty() && hkStopRecord != hkRecord)
        m_hkStopRecord = im->addHotkey(hkStopRecord, [this]() { stopRecording(); }, true);
    if (!hkPlay.isEmpty())
        m_hkPlay = im->addHotkey(hkPlay, [this]() { startPlaying(); }, true);
    if (!hkStopPlay.isEmpty())
        m_hkStopPlay = im->addHotkey(hkStopPlay, [this]() { stopPlaying(); }, true);

    m_hotkeysActive =
        !m_hkRecord.isEmpty() || !m_hkStopRecord.isEmpty() || !m_hkPlay.isEmpty() || !m_hkStopPlay.isEmpty();
    updateUi();
}

void Macro::clearHotkeys() {
    if (!m_services)
        return;
    auto *im = m_services->inputManager();

    if (!m_hkRecord.isEmpty()) {
        im->removeHotkey(m_hkRecord);
        m_hkRecord.clear();
    }
    if (!m_hkStopRecord.isEmpty()) {
        im->removeHotkey(m_hkStopRecord);
        m_hkStopRecord.clear();
    }
    if (!m_hkPlay.isEmpty()) {
        im->removeHotkey(m_hkPlay);
        m_hkPlay.clear();
    }
    if (!m_hkStopPlay.isEmpty()) {
        im->removeHotkey(m_hkStopPlay);
        m_hkStopPlay.clear();
    }

    m_hotkeysActive = false;
    m_controlHotkeyTokens.clear();
    updateUi();
}

// ─────────────────────────────────────────────────────────────────────────────
// Listeners (attached only while recording)
// ─────────────────────────────────────────────────────────────────────────────

void Macro::attachListeners() {
    if (m_listenersAttached || !m_services)
        return;
    auto *im = m_services->inputManager();

    m_keyListenId = im->addKeyListener([this](const QString &key, bool pressed) {
        if (m_state != MacroState::Recording)
            return;
        if (!recordingKeysEnabled())
            return;
        if (isControlHotkeyKey(key))
            return;

        MacroEvent evt;
        evt.type = pressed ? MacroEvent::Type::KeyDown : MacroEvent::Type::KeyUp;
        evt.key = key.trimmed().toLower();
        evt.delayMs = m_recordTimer.elapsed() - m_lastEventMs;
        m_lastEventMs = m_recordTimer.elapsed();
        m_events.append(evt);
        updateUi();
    });

    m_mouseListenId = im->addMouseListener([this](const InputManager::MouseEvent &me) {
        if (m_state != MacroState::Recording || !recordingMouseEnabled())
            return;

        MacroEvent::Type type;
        switch (me.type) {
            case InputManager::MouseEvent::Type::Move:
                type = MacroEvent::Type::MouseMove;
                break;
            case InputManager::MouseEvent::Type::LeftDown:
                type = MacroEvent::Type::MouseLeftDown;
                break;
            case InputManager::MouseEvent::Type::LeftUp:
                type = MacroEvent::Type::MouseLeftUp;
                break;
            case InputManager::MouseEvent::Type::RightDown:
                type = MacroEvent::Type::MouseRightDown;
                break;
            case InputManager::MouseEvent::Type::RightUp:
                type = MacroEvent::Type::MouseRightUp;
                break;
            case InputManager::MouseEvent::Type::MiddleDown:
                type = MacroEvent::Type::MouseMiddleDown;
                break;
            case InputManager::MouseEvent::Type::MiddleUp:
                type = MacroEvent::Type::MouseMiddleUp;
                break;
            case InputManager::MouseEvent::Type::WheelUp:
                type = MacroEvent::Type::MouseWheelUp;
                break;
            case InputManager::MouseEvent::Type::WheelDown:
                type = MacroEvent::Type::MouseWheelDown;
                break;
            default:
                return;
        }

        MacroEvent evt;
        evt.type = type;
        evt.x = me.globalPos.x();
        evt.y = me.globalPos.y();
        evt.delayMs = m_recordTimer.elapsed() - m_lastEventMs;

        if (type == MacroEvent::Type::MouseMove) {
            const int dx = evt.x - m_lastMouseX;
            const int dy = evt.y - m_lastMouseY;
            if (dx * dx + dy * dy < 9 && evt.delayMs < 50)
                return;
            m_lastMouseX = evt.x;
            m_lastMouseY = evt.y;
        }

        m_lastEventMs = m_recordTimer.elapsed();
        m_events.append(evt);
        updateUi();
    });

    m_listenersAttached = true;
}

void Macro::detachListeners() {
    if (!m_listenersAttached || !m_services)
        return;
    auto *im = m_services->inputManager();

    if (!m_keyListenId.isEmpty()) {
        im->removeKeyListener(m_keyListenId);
        m_keyListenId.clear();
    }
    if (!m_mouseListenId.isEmpty()) {
        im->removeMouseListener(m_mouseListenId);
        m_mouseListenId.clear();
    }

    m_listenersAttached = false;
}

// ─────────────────────────────────────────────────────────────────────────────
// Save / Load
// ─────────────────────────────────────────────────────────────────────────────

void Macro::saveMacro() {
    const QString path =
        QFileDialog::getSaveFileName(m_rootWidget, "Save Macro", m_macroFilePath, "Macro files (*.json)");
    if (path.isEmpty())
        return;

    QJsonArray arr;
    for (const MacroEvent &evt : std::as_const(m_events)) {
        QJsonObject obj;
        obj["type"] = eventTypeToString(evt.type);
        obj["delayMs"] = evt.delayMs;
        obj["key"] = evt.key;
        obj["x"] = evt.x;
        obj["y"] = evt.y;
        arr.append(obj);
    }

    QJsonDocument doc(arr);
    QFile f(path);
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        f.write(doc.toJson());
}

void Macro::loadMacro() {
    const QString path =
        QFileDialog::getOpenFileName(m_rootWidget, "Load Macro", m_macroFilePath, "Macro files (*.json)");
    if (path.isEmpty())
        return;

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return;

    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isArray())
        return;

    m_events.clear();
    for (const QJsonValue &val : doc.array()) {
        QJsonObject obj = val.toObject();
        MacroEvent evt;
        evt.type = eventTypeFromString(obj["type"].toString());
        evt.delayMs = static_cast<qint64>(obj["delayMs"].toDouble());
        evt.key = obj["key"].toString();
        evt.x = obj["x"].toInt();
        evt.y = obj["y"].toInt();
        m_events.append(evt);
    }

    updateUi();
}

// ─────────────────────────────────────────────────────────────────────────────
// UI helpers
// ─────────────────────────────────────────────────────────────────────────────

void Macro::updateUi() {
    const bool idle = (m_state == MacroState::Idle);
    const bool recording = (m_state == MacroState::Recording);
    const bool playing = (m_state == MacroState::Playing);
    const bool hasEvents = !m_events.isEmpty();

    if (m_idleLabel) {
        if (recording) {
            m_idleLabel->setText("RUNNING");
            m_idleLabel->setStyleSheet(QString());
        } else if (playing) {
            m_idleLabel->setText("RUNNING");
            m_idleLabel->setStyleSheet(QString());
        } else {
            m_idleLabel->setText("IDLE");
            m_idleLabel->setStyleSheet(QString());
        }
    }

    if (m_eventCountLabel)
        m_eventCountLabel->setText(QStringLiteral("%1").arg(m_events.size()));

    if (m_hotkeyToggleBtn) {
        m_hotkeyToggleBtn->setText(m_hotkeysActive ? "ON" : "OFF");
        m_hotkeyToggleBtn->setChecked(m_hotkeysActive);
    }

    if (m_saveBtn)
        m_saveBtn->setEnabled(idle && hasEvents);
    if (m_loadBtn)
        m_loadBtn->setEnabled(idle);
    if (m_speedEdit)
        m_speedEdit->setEnabled(idle);
    if (m_repeatEdit)
        m_repeatEdit->setEnabled(idle);
    if (m_recordKeysCheck)
        m_recordKeysCheck->setEnabled(idle || recording);
    if (m_recordMouseCheck)
        m_recordMouseCheck->setEnabled(idle || recording);
}

void Macro::applyActionIcons() {
    if (m_pluginRootPath.isEmpty())
        return;

    if (m_saveBtn) {
        const QIcon saveIcon(m_pluginRootPath + "/res/img/save.svg");
        if (!saveIcon.isNull())
            m_saveBtn->setIcon(saveIcon);
    }

    if (m_loadBtn) {
        const QIcon loadIcon(m_pluginRootPath + "/res/img/load.svg");
        if (!loadIcon.isNull())
            m_loadBtn->setIcon(loadIcon);
    }
}

double Macro::playbackSpeed() const {
    if (!m_speedEdit)
        return 1.0;
    bool ok = false;
    const double speed = m_speedEdit->text().trimmed().toDouble(&ok);
    if (!ok || speed <= 0.0)
        return 1.0;
    return speed;
}

int Macro::playbackRepeat() const {
    if (!m_repeatEdit)
        return 1;
    bool ok = false;
    const int repeat = m_repeatEdit->text().trimmed().toInt(&ok);
    if (!ok || repeat < 1)
        return 1;
    return repeat;
}

bool Macro::isControlHotkeyKey(const QString &key) const {
    const QString k = key.trimmed().toLower();
    return m_controlHotkeyTokens.contains(k);
}

bool Macro::recordingKeysEnabled() const {
    return !m_recordKeysCheck || m_recordKeysCheck->isChecked();
}

bool Macro::recordingMouseEnabled() const {
    return !m_recordMouseCheck || m_recordMouseCheck->isChecked();
}
