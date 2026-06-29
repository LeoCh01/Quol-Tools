#pragma once

#include "plugin_api/IQuolPlugin.hpp"

#include <QMediaPlayer>
#include <QObject>
#include <QStringList>

class QAudioOutput;
class QLabel;
class QPushButton;

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
    void onMediaError(QMediaPlayer::Error error);
    void onPlaybackStateChanged(QMediaPlayer::PlaybackState state);
    void onMediaStatusChanged(QMediaPlayer::MediaStatus status);

    QString m_pluginRootPath;
    PluginConfig m_cfg;

    QMediaPlayer *m_player = nullptr;
    QAudioOutput *m_audioOutput = nullptr;

    QPushButton *m_prevBtn = nullptr;
    QPushButton *m_playPauseBtn = nullptr;
    QPushButton *m_nextBtn = nullptr;
    QLabel *m_songLabel = nullptr;
    QPushButton *m_manageBtn = nullptr;

    QStringList m_songList;
    int m_currentIndex = -1;
};
