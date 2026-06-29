#include "plugins/musicPlayer/MusicPlayer.hpp"

#include "ui/QuolPopupWindow.hpp"

#include <QAudioOutput>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QIcon>
#include <QJsonArray>
#include <QLabel>
#include <QListWidget>
#include <QMediaPlayer>
#include <QPushButton>
#include <QSize>
#include <QVBoxLayout>
#include <QWidget>

QWidget *MusicPlayer::createWidget(QWidget *parent) {
    auto *widget = new QWidget(parent);
    auto *layout = new QVBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);

    m_songLabel = new QLabel(QStringLiteral("No song loaded"), widget);
    m_songLabel->setAlignment(Qt::AlignCenter);
    m_songLabel->setWordWrap(true);
    layout->addWidget(m_songLabel);

    auto *btnRow = new QHBoxLayout();
    btnRow->setSpacing(2);

    const int iconSize = 16;

    m_prevBtn = new QPushButton(widget);
    m_prevBtn->setObjectName(QStringLiteral("btn-prev"));
    m_prevBtn->setIcon(QIcon(m_pluginRootPath + QStringLiteral("/res/img/prev.svg")));
    m_prevBtn->setIconSize(QSize(iconSize, iconSize));
    m_prevBtn->setFixedSize(24, 24);
    m_prevBtn->setToolTip(QStringLiteral("Previous"));
    connect(m_prevBtn, &QPushButton::clicked, this, &MusicPlayer::playPrev);

    m_playPauseBtn = new QPushButton(widget);
    m_playPauseBtn->setObjectName(QStringLiteral("btn-play-pause"));
    m_playPauseBtn->setIcon(QIcon(m_pluginRootPath + QStringLiteral("/res/img/play.svg")));
    m_playPauseBtn->setIconSize(QSize(iconSize, iconSize));
    m_playPauseBtn->setFixedSize(24, 24);
    m_playPauseBtn->setToolTip(QStringLiteral("Play / Pause"));
    connect(m_playPauseBtn, &QPushButton::clicked, this, &MusicPlayer::playPause);

    m_nextBtn = new QPushButton(widget);
    m_nextBtn->setObjectName(QStringLiteral("btn-next"));
    m_nextBtn->setIcon(QIcon(m_pluginRootPath + QStringLiteral("/res/img/next.svg")));
    m_nextBtn->setIconSize(QSize(iconSize, iconSize));
    m_nextBtn->setFixedSize(24, 24);
    m_nextBtn->setToolTip(QStringLiteral("Next"));
    connect(m_nextBtn, &QPushButton::clicked, this, &MusicPlayer::playNext);

    btnRow->addStretch();
    btnRow->addWidget(m_prevBtn);
    btnRow->addWidget(m_playPauseBtn);
    btnRow->addWidget(m_nextBtn);
    btnRow->addStretch();
    layout->addLayout(btnRow);

    m_manageBtn = new QPushButton(QStringLiteral("Manage Directories"), widget);
    m_manageBtn->setObjectName(QStringLiteral("btn-manage"));
    connect(m_manageBtn, &QPushButton::clicked, this, &MusicPlayer::openManageDialog);
    layout->addWidget(m_manageBtn);

    updateSongLabel();

    return widget;
}

void MusicPlayer::initialize(const QString &pluginRootPath, const PluginConfig &pluginConfig, QuolServices *services) {
    Q_UNUSED(services)
    m_pluginRootPath = pluginRootPath;
    m_cfg = pluginConfig;

    m_player = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    m_player->setAudioOutput(m_audioOutput);

    connect(m_player, &QMediaPlayer::errorOccurred, this, &MusicPlayer::onMediaError);
    connect(m_player, &QMediaPlayer::playbackStateChanged, this, &MusicPlayer::onPlaybackStateChanged);
    connect(m_player, &QMediaPlayer::mediaStatusChanged, this, &MusicPlayer::onMediaStatusChanged);

    scanDirectories();

    if (!m_songList.isEmpty())
        m_currentIndex = 0;
}

void MusicPlayer::onUpdateConfig(const PluginConfig &pluginConfig) {
    m_cfg = pluginConfig;
    scanDirectories();
}

void MusicPlayer::shutdown() {
    if (m_player) {
        m_player->stop();
        m_player = nullptr;
    }
    m_audioOutput = nullptr;
    m_prevBtn = nullptr;
    m_playPauseBtn = nullptr;
    m_nextBtn = nullptr;
    m_songLabel = nullptr;
    m_manageBtn = nullptr;
    m_songList.clear();
    m_currentIndex = -1;
}

MusicPlayer::~MusicPlayer() {
    shutdown();
}

void MusicPlayer::playFile(int index) {
    if (index < 0 || index >= m_songList.size() || !m_player)
        return;

    m_currentIndex = index;
    m_player->setSource(QUrl::fromLocalFile(m_songList.at(index)));
    m_player->play();
}

void MusicPlayer::playPause() {
    if (m_songList.isEmpty()) {
        m_songLabel->setText(QStringLiteral("No songs found - add directories first"));
        return;
    }
    if (!m_player)
        return;

    if (m_player->playbackState() == QMediaPlayer::PlayingState) {
        m_player->pause();
    } else {
        if (m_player->playbackState() == QMediaPlayer::StoppedState)
            playFile(m_currentIndex >= 0 ? m_currentIndex : 0);
        else
            m_player->play();
    }
}

void MusicPlayer::playNext() {
    if (m_songList.isEmpty() || !m_player)
        return;

    int next = m_currentIndex + 1;
    if (next >= m_songList.size())
        next = 0;
    playFile(next);
}

void MusicPlayer::playPrev() {
    if (m_songList.isEmpty() || !m_player)
        return;

    int prev = m_currentIndex - 1;
    if (prev < 0)
        prev = m_songList.size() - 1;
    playFile(prev);
}

void MusicPlayer::onMediaError(QMediaPlayer::Error error) {
    if (!m_songLabel)
        return;
    switch (error) {
    case QMediaPlayer::FormatError:
        m_songLabel->setText(QStringLiteral("Audio format not supported"));
        break;
    case QMediaPlayer::ResourceError:
        m_songLabel->setText(QStringLiteral("File not found or inaccessible"));
        break;
    case QMediaPlayer::AccessDeniedError:
        m_songLabel->setText(QStringLiteral("Access denied"));
        break;
    default:
        m_songLabel->setText(QStringLiteral("Playback error"));
        break;
    }
}

void MusicPlayer::onPlaybackStateChanged(QMediaPlayer::PlaybackState state) {
    if (!m_playPauseBtn)
        return;
    switch (state) {
    case QMediaPlayer::PlayingState:
        m_playPauseBtn->setIcon(QIcon(m_pluginRootPath + QStringLiteral("/res/img/pause.svg")));
        break;
    case QMediaPlayer::PausedState:
    case QMediaPlayer::StoppedState:
        m_playPauseBtn->setIcon(QIcon(m_pluginRootPath + QStringLiteral("/res/img/play.svg")));
        break;
    }
}

void MusicPlayer::onMediaStatusChanged(QMediaPlayer::MediaStatus status) {
    if (status == QMediaPlayer::LoadedMedia && m_songLabel)
        updateSongLabel();
    if (status == QMediaPlayer::EndOfMedia && !m_songList.isEmpty())
        playNext();
}

void MusicPlayer::updateSongLabel() {
    if (!m_songLabel)
        return;

    if (m_currentIndex >= 0 && m_currentIndex < m_songList.size()) {
        const QFileInfo fi(m_songList.at(m_currentIndex));
        m_songLabel->setText(fi.fileName());
    } else {
        m_songLabel->setText(QStringLiteral("No song loaded"));
    }
}

void MusicPlayer::scanDirectories() {
    m_songList.clear();

    const QJsonArray dirs = m_cfg.get("directories").toArray();
    for (const auto &val : dirs) {
        QDir dir(val.toString());
        if (!dir.exists())
            continue;

        const QStringList filters = { QStringLiteral("*.mp3") };
        const auto files = dir.entryList(filters, QDir::Files, QDir::Name);
        for (const auto &file : files) {
            m_songList.append(dir.absoluteFilePath(file));
        }
    }
}

void MusicPlayer::openManageDialog() {
    auto *popup = new QuolPopupWindow(QStringLiteral("Manage Music Directories"), m_manageBtn->window());
    popup->resize(400, 300);

    auto *content = new QWidget(popup);
    auto *layout = new QVBoxLayout(content);
    layout->setContentsMargins(0, 0, 0, 0);

    auto *listWidget = new QListWidget(content);
    const QJsonArray dirs = m_cfg.get("directories").toArray();
    for (const auto &val : dirs) {
        listWidget->addItem(val.toString());
    }

    auto *addBtn = new QPushButton(QStringLiteral("Add Directory"), content);
    connect(addBtn, &QPushButton::clicked, this, [this, listWidget]() {
        const QString dir = QFileDialog::getExistingDirectory(listWidget, QStringLiteral("Select Music Directory"));
        if (dir.isEmpty())
            return;
        listWidget->addItem(dir);
    });

    auto *removeBtn = new QPushButton(QStringLiteral("Remove Selected"), content);
    connect(removeBtn, &QPushButton::clicked, this, [this, listWidget]() {
        auto items = listWidget->selectedItems();
        for (auto *item : items) {
            delete listWidget->takeItem(listWidget->row(item));
        }
    });

    auto *saveBtn = new QPushButton(QStringLiteral("Save"), content);
    connect(saveBtn, &QPushButton::clicked, this, [this, listWidget, popup]() {
        QJsonArray dirs;
        for (int i = 0; i < listWidget->count(); ++i) {
            dirs.append(listWidget->item(i)->text());
        }
        m_cfg.set("directories", dirs);
        m_cfg.save();
        scanDirectories();
        popup->close();
    });

    auto *btnLayout = new QHBoxLayout();
    btnLayout->addWidget(addBtn);
    btnLayout->addWidget(removeBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(saveBtn);

    layout->addWidget(listWidget, 1);
    layout->addLayout(btnLayout);

    popup->addContent(content);
    popup->show();
}
