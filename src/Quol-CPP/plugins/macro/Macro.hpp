#pragma once

#include "core/InputManager.hpp"
#include "plugin_api/IQuolPlugin.hpp"
#include "plugin_api/PluginConfig.hpp"

#include <QElapsedTimer>
#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QString>
#include <QTimer>

class QCheckBox;
class QGroupBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QVBoxLayout;
class QWidget;

// ─────────────────────────────────────────────────────────────────────────────
// Data types
// ─────────────────────────────────────────────────────────────────────────────

struct MacroEvent {
    enum class Type {
        KeyDown,
        KeyUp,
        MouseMove,
        MouseLeftDown,
        MouseLeftUp,
        MouseRightDown,
        MouseRightUp,
        MouseMiddleDown,
        MouseMiddleUp,
        MouseWheelUp,
        MouseWheelDown,
    };
    Type type = Type::KeyDown;
    qint64 delayMs = 0;  // delay from the previous event

    // Key events
    QString key;

    // Mouse events
    int x = 0;
    int y = 0;
};

enum class MacroState { Idle, Recording, Playing };

// ─────────────────────────────────────────────────────────────────────────────
// Plugin class
// ─────────────────────────────────────────────────────────────────────────────

class Macro final : public QObject, public IQuolPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID IQuolPlugin_iid)
    Q_INTERFACES(IQuolPlugin)

public:
    QWidget *createWidget(QWidget *parent = nullptr) override;
    void initialize(const QString &pluginRootPath, const PluginConfig &pluginConfig, QuolServices *services) override;
    void onUpdateConfig(const PluginConfig &pluginConfig) override;
    void shutdown() override;

private:
    // ── Record / play ──────────────────────────────────────────────────────
    void startRecording();
    void stopRecording();
    void startPlaying();
    void stopPlaying();
    void scheduleNextEvent();

    // ── Hotkeys ────────────────────────────────────────────────────────────
    void applyHotkeys();
    void clearHotkeys();

    // ── Listeners (used during recording) ─────────────────────────────────
    void attachListeners();
    void detachListeners();

    // ── Save / Load ────────────────────────────────────────────────────────
    void saveMacro();
    void loadMacro();

    // ── UI ─────────────────────────────────────────────────────────────────
    void updateUi();
    void applyActionIcons();
    double playbackSpeed() const;
    int playbackRepeat() const;
    bool isControlHotkeyKey(const QString &key) const;
    bool recordingKeysEnabled() const;
    bool recordingMouseEnabled() const;

    // ── Helpers ────────────────────────────────────────────────────────────
    static void injectMouse(int x, int y, MacroEvent::Type type);

    // ── State ──────────────────────────────────────────────────────────────
    MacroState m_state = MacroState::Idle;
    QList<MacroEvent> m_events;
    int m_playIndex = 0;
    int m_repeatsDone = 0;

    QElapsedTimer m_recordTimer;
    qint64 m_lastEventMs = 0;
    QElapsedTimer m_hotkeyDebounceTimer;
    qint64 m_lastRecordToggleMs = -100000;
    int m_lastMouseX = 0;
    int m_lastMouseY = 0;

    QTimer m_playTimer;

    // ── Services / config ──────────────────────────────────────────────────
    QuolServices *m_services = nullptr;
    PluginConfig m_cfg;
    QString m_pluginRootPath;
    QString m_macroFilePath;

    // ── Hotkey handles ─────────────────────────────────────────────────────
    QString m_hkRecord;
    QString m_hkStopRecord;
    QString m_hkPlay;
    QString m_hkStopPlay;
    bool m_hotkeysActive = false;
    QStringList m_controlHotkeyTokens;

    // ── Listener handles ───────────────────────────────────────────────────
    QString m_keyListenId;
    QString m_mouseListenId;
    bool m_listenersAttached = false;

    // ── UI widgets ─────────────────────────────────────────────────────────
    QWidget *m_rootWidget = nullptr;
    QLabel *m_idleLabel = nullptr;
    QPushButton *m_hotkeyToggleBtn = nullptr;
    QPushButton *m_saveBtn = nullptr;
    QPushButton *m_loadBtn = nullptr;
    QLineEdit *m_speedEdit = nullptr;
    QLineEdit *m_repeatEdit = nullptr;
    QCheckBox *m_recordKeysCheck = nullptr;
    QCheckBox *m_recordMouseCheck = nullptr;
    QLabel *m_eventCountLabel = nullptr;
};
