#pragma once

#include "plugin_api/IQuolPlugin.hpp"

#include <QObject>
#include <QStringList>

#include <memory>

class QLabel;
class QPushButton;
class QSlider;
class QTimer;

struct ma_engine;
struct ma_sound;

namespace music {

// RAII owners for the miniaudio C API. Fully defined in this header (they only
// hold raw pointers, so the incomplete `ma_engine`/`ma_sound` types are fine);
// the actual init/teardown lives in MusicPlayer.cpp where miniaudio.h is
// included. Each tracks whether the underlying object was successfully
// initialized so teardown never calls ma_*_uninit on a struct that failed
// initialization (which would dereference a null engine).
struct EngineOwner {
    explicit EngineOwner();
    ~EngineOwner();

    bool init();

    ma_engine *engine = nullptr;
    bool initialized = false;
};

struct SoundOwner {
    explicit SoundOwner();
    ~SoundOwner();

    bool init(ma_engine *engine, const QString &filePath);

    ma_sound *sound = nullptr;
    bool initialized = false;
};

}  // namespace music

class MusicPlayer final : public QObject, public IQuolPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID IQuolPlugin_iid)
    Q_INTERFACES(IQuolPlugin)

public:
    ~MusicPlayer() override;

    QWidget *createWidget(QWidget *parent = nullptr) override;
    void initialize(const QString &pluginRootPath, const PluginConfig &pluginConfig, QuolServices *services) override;
    void onUpdateConfig(const PluginConfig &pluginConfig) override;
    void shutdown() override;

private:
    void playPause();
    void playNext();
    void playPrev();
    void playFile(int index);
    void openManageDialog();
    void scanDirectories();
    void updateSongLabel();
    void updateTimeLabel();
    void setElidedText(const QString &text);
    bool eventFilter(QObject *obj, QEvent *event) override;
    void onTick();
    void setPlayPauseIcon(bool playing);

    static void setToggleStyle(QPushButton *btn, bool active);
    static qint64 framesToMs(quint64 frames, quint32 sampleRate);
    static quint64 msToFrames(qint64 ms, quint32 sampleRate);

    QString m_pluginRootPath;
    PluginConfig m_cfg;

    std::unique_ptr<music::EngineOwner> m_engine;
    std::unique_ptr<music::SoundOwner> m_sound;

    QLabel *m_songLabel = nullptr;
    QPushButton *m_prevBtn = nullptr;
    QPushButton *m_playPauseBtn = nullptr;
    QPushButton *m_nextBtn = nullptr;
    QPushButton *m_shuffleBtn = nullptr;
    QPushButton *m_repeatBtn = nullptr;
    QSlider *m_seekSlider = nullptr;
    QLabel *m_timeLabel = nullptr;
    QSlider *m_volumeSlider = nullptr;
    QPushButton *m_manageBtn = nullptr;
    QTimer *m_timer = nullptr;

    QStringList m_songList;
    int m_currentIndex = -1;
    bool m_shuffle = false;
    bool m_repeat = false;
    bool m_seekSliderPressed = false;
    quint64 m_soundLengthFrames = 0;
    quint32 m_sampleRate = 48000;
};
