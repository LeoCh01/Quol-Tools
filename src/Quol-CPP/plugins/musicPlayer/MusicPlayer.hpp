#pragma once

#include "plugin_api/IQuolPlugin.hpp"

#include <QObject>
#include <QStringList>

class QLabel;
class QPushButton;
class QSlider;
class QTimer;

struct ma_engine;
struct ma_sound;

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

    ma_engine *m_engine = nullptr;
    ma_sound *m_sound = nullptr;

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
