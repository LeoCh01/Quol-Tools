#include "plugins/musicPlayer/MusicPlayer.hpp"

#include "ui/QuolPopupWindow.hpp"

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include <QDir>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QIcon>
#include <QJsonArray>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QRandomGenerator>
#include <QSize>
#include <QSlider>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

static QString formatTime(qint64 ms) {
    int totalSec = static_cast<int>(ms / 1000);
    int min = totalSec / 60;
    int sec = totalSec % 60;
    return QStringLiteral("%1:%2").arg(min).arg(sec, 2, 10, QLatin1Char('0'));
}

qint64 MusicPlayer::framesToMs(quint64 frames, quint32 sampleRate) {
    if (sampleRate == 0) return 0;
    return static_cast<qint64>(frames) * 1000 / sampleRate;
}

quint64 MusicPlayer::msToFrames(qint64 ms, quint32 sampleRate) {
    if (sampleRate == 0) return 0;
    return static_cast<quint64>(ms) * sampleRate / 1000;
}

QWidget *MusicPlayer::createWidget(QWidget *parent) {
    auto *widget = new QWidget(parent);
    auto *layout = new QVBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);

    m_songLabel = new QLabel(QStringLiteral("No song loaded"), widget);
    m_songLabel->setAlignment(Qt::AlignCenter);
    m_songLabel->setWordWrap(true);
    m_songLabel->setMinimumWidth(0);
    m_songLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    m_songLabel->setFixedHeight(m_songLabel->fontMetrics().lineSpacing() * 2);
    m_songLabel->installEventFilter(this);
    layout->addWidget(m_songLabel);

    const int iconSize = 16;
    auto *btnRow = new QHBoxLayout();
    btnRow->setSpacing(2);

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

    btnRow->addWidget(m_prevBtn);
    btnRow->addWidget(m_playPauseBtn);
    btnRow->addWidget(m_nextBtn);
    btnRow->addStretch();

    m_repeatBtn = new QPushButton(widget);
    m_repeatBtn->setObjectName(QStringLiteral("btn-repeat"));
    m_repeatBtn->setIcon(QIcon(m_pluginRootPath + QStringLiteral("/res/img/repeat.svg")));
    m_repeatBtn->setIconSize(QSize(iconSize, iconSize));
    m_repeatBtn->setFixedSize(24, 24);
    m_repeatBtn->setCheckable(true);
    m_repeatBtn->setToolTip(QStringLiteral("Repeat"));
    setToggleStyle(m_repeatBtn, false);
    connect(m_repeatBtn, &QPushButton::toggled, this, [this](bool checked) {
        m_repeat = checked;
        setToggleStyle(m_repeatBtn, checked);
    });

    btnRow->addWidget(m_repeatBtn);

    m_shuffleBtn = new QPushButton(widget);
    m_shuffleBtn->setObjectName(QStringLiteral("btn-shuffle"));
    m_shuffleBtn->setIcon(QIcon(m_pluginRootPath + QStringLiteral("/res/img/shuffle.svg")));
    m_shuffleBtn->setIconSize(QSize(iconSize, iconSize));
    m_shuffleBtn->setFixedSize(24, 24);
    m_shuffleBtn->setCheckable(true);
    m_shuffleBtn->setToolTip(QStringLiteral("Shuffle"));
    setToggleStyle(m_shuffleBtn, false);
    connect(m_shuffleBtn, &QPushButton::toggled, this, [this](bool checked) {
        m_shuffle = checked;
        setToggleStyle(m_shuffleBtn, checked);
    });

    btnRow->addWidget(m_shuffleBtn);

    m_manageBtn = new QPushButton(widget);
    m_manageBtn->setObjectName(QStringLiteral("btn-manage"));
    m_manageBtn->setIcon(QIcon(m_pluginRootPath + QStringLiteral("/res/img/manage.svg")));
    m_manageBtn->setIconSize(QSize(iconSize, iconSize));
    m_manageBtn->setFixedSize(24, 24);
    m_manageBtn->setToolTip(QStringLiteral("Manage Directories"));
    connect(m_manageBtn, &QPushButton::clicked, this, &MusicPlayer::openManageDialog);
    btnRow->addWidget(m_manageBtn);

    layout->addLayout(btnRow);

    m_seekSlider = new QSlider(Qt::Horizontal, widget);
    m_seekSlider->setObjectName(QStringLiteral("seek-slider"));
    m_seekSlider->setRange(0, 0);
    m_seekSlider->setEnabled(false);
    m_seekSlider->setToolTip(QStringLiteral("Seek"));
    connect(m_seekSlider, &QSlider::sliderPressed, this, [this]() {
        m_seekSliderPressed = true;
    });
    connect(m_seekSlider, &QSlider::sliderReleased, this, [this]() {
        m_seekSliderPressed = false;
        if (m_sound)
            ma_sound_seek_to_pcm_frame(m_sound, msToFrames(m_seekSlider->value(), m_sampleRate));
    });
    layout->addWidget(m_seekSlider);

    auto *bottomRow = new QHBoxLayout();
    bottomRow->setSpacing(4);

    m_timeLabel = new QLabel(QStringLiteral("0:00 / 0:00"), widget);
    m_timeLabel->setObjectName(QStringLiteral("time-label"));
    bottomRow->addWidget(m_timeLabel);

    bottomRow->addStretch();

    auto *volLabel = new QLabel(widget);
    volLabel->setPixmap(QIcon(m_pluginRootPath + QStringLiteral("/res/img/volume.svg")).pixmap(14, 14));
    bottomRow->addWidget(volLabel);

    m_volumeSlider = new QSlider(Qt::Horizontal, widget);
    m_volumeSlider->setRange(0, 100);
    m_volumeSlider->setValue(m_cfg.get("_.volume", 50).toInt());
    m_volumeSlider->setFixedWidth(80);
    m_volumeSlider->setToolTip(QStringLiteral("Volume"));
    connect(m_volumeSlider, &QSlider::valueChanged, this, [this](int value) {
        if (m_engine)
            ma_engine_set_volume(m_engine, value / 100.0f);
        m_cfg.set("_.volume", value);
        m_cfg.save();
    });
    bottomRow->addWidget(m_volumeSlider);
    layout->addLayout(bottomRow);

    updateSongLabel();

    return widget;
}

void MusicPlayer::initialize(const QString &pluginRootPath, const PluginConfig &pluginConfig, QuolServices *services) {
    Q_UNUSED(services)
    m_pluginRootPath = pluginRootPath;
    m_cfg = pluginConfig;

    m_engine = new ma_engine();
    ma_engine_config engineConfig = ma_engine_config_init();
    if (ma_engine_init(&engineConfig, m_engine) != MA_SUCCESS) {
        m_songLabel->setText(QStringLiteral("Failed to initialize audio engine"));
        return;
    }

    m_sampleRate = ma_engine_get_sample_rate(m_engine);
    float vol = m_cfg.get("_.volume", 50).toInt() / 100.0f;
    ma_engine_set_volume(m_engine, vol);

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &MusicPlayer::onTick);
    m_timer->start(200);

    scanDirectories();

    if (!m_songList.isEmpty()) {
        m_currentIndex = m_cfg.get("_.last_index", 0).toInt();
        if (m_currentIndex < 0 || m_currentIndex >= m_songList.size())
            m_currentIndex = 0;
    }
}

void MusicPlayer::onUpdateConfig(const PluginConfig &pluginConfig) {
    m_cfg = pluginConfig;
    scanDirectories();
}

void MusicPlayer::shutdown() {
    if (m_timer)
        m_timer->stop();

    if (m_sound) {
        ma_sound_stop(m_sound);
        ma_sound_uninit(m_sound);
        delete m_sound;
        m_sound = nullptr;
    }
    if (m_engine) {
        ma_engine_uninit(m_engine);
        delete m_engine;
        m_engine = nullptr;
    }
}

MusicPlayer::~MusicPlayer() {
    shutdown();
}

void MusicPlayer::playFile(int index) {
    if (index < 0 || index >= m_songList.size() || !m_engine)
        return;

    m_currentIndex = index;
    m_cfg.set("_.last_index", index);
    m_cfg.save();

    if (m_sound) {
        ma_sound_stop(m_sound);
        ma_sound_uninit(m_sound);
        delete m_sound;
        m_sound = nullptr;
    }

    m_sound = new ma_sound();
    QByteArray pathBytes = m_songList.at(index).toUtf8();
    ma_result result = ma_sound_init_from_file(m_engine, pathBytes.constData(),
        MA_SOUND_FLAG_NO_SPATIALIZATION | MA_SOUND_FLAG_NO_PITCH, nullptr, nullptr, m_sound);

    if (result != MA_SUCCESS) {
        delete m_sound;
        m_sound = nullptr;
        m_soundLengthFrames = 0;
        if (m_songLabel)
            m_songLabel->setText(QStringLiteral("Failed to load file"));
        return;
    }

    ma_sound_get_length_in_pcm_frames(m_sound, &m_soundLengthFrames);
    ma_sound_start(m_sound);

    int durMs = static_cast<int>(framesToMs(m_soundLengthFrames, m_sampleRate));
    m_seekSlider->setRange(0, durMs);
    m_seekSlider->setEnabled(true);

    setPlayPauseIcon(true);
    updateSongLabel();
}

void MusicPlayer::playPause() {
    if (m_songList.isEmpty()) {
        if (m_songLabel)
            m_songLabel->setText(QStringLiteral("No songs found - add directories first"));
        return;
    }

    if (!m_sound) {
        playFile(m_currentIndex >= 0 ? m_currentIndex : 0);
        return;
    }

    if (ma_sound_is_playing(m_sound)) {
        ma_sound_stop(m_sound);
        setPlayPauseIcon(false);
    } else {
        ma_sound_start(m_sound);
        setPlayPauseIcon(true);
    }
}

void MusicPlayer::playNext() {
    if (m_songList.isEmpty())
        return;

    int next;
    if (m_shuffle) {
        do {
            next = QRandomGenerator::global()->bounded(m_songList.size());
        } while (next == m_currentIndex && m_songList.size() > 1);
    } else {
        next = m_currentIndex + 1;
        if (next >= m_songList.size()) {
            if (m_repeat)
                next = 0;
            else
                return;
        }
    }
    playFile(next);
}

void MusicPlayer::playPrev() {
    if (m_songList.isEmpty())
        return;

    int prev;
    if (m_shuffle) {
        do {
            prev = QRandomGenerator::global()->bounded(m_songList.size());
        } while (prev == m_currentIndex && m_songList.size() > 1);
    } else {
        prev = m_currentIndex - 1;
        if (prev < 0) {
            if (m_repeat)
                prev = m_songList.size() - 1;
            else
                return;
        }
    }
    playFile(prev);
}

void MusicPlayer::onTick() {
    if (!m_sound || !m_engine)
        return;

    if (m_soundLengthFrames > 0) {
        ma_uint64 pos = ma_sound_get_time_in_pcm_frames(m_sound);
        if (pos >= m_soundLengthFrames) {
            ma_sound_stop(m_sound);
            ma_sound_uninit(m_sound);
            delete m_sound;
            m_sound = nullptr;
            m_soundLengthFrames = 0;

            if (m_repeat) {
                playFile(m_currentIndex);
            } else {
                playNext();
            }

            if (!m_sound) {
                m_seekSlider->setRange(0, 0);
                m_seekSlider->setEnabled(false);
                setPlayPauseIcon(false);
                updateTimeLabel();
            }
            return;
        }

        if (!m_seekSliderPressed && m_seekSlider) {
            int posMs = static_cast<int>(framesToMs(pos, m_sampleRate));
            m_seekSlider->setValue(posMs);
        }
    }
    updateTimeLabel();
}

void MusicPlayer::setPlayPauseIcon(bool playing) {
    if (!m_playPauseBtn)
        return;
    QString iconName = playing ? QStringLiteral("pause.svg") : QStringLiteral("play.svg");
    m_playPauseBtn->setIcon(QIcon(m_pluginRootPath + QStringLiteral("/res/img/") + iconName));
}

bool MusicPlayer::eventFilter(QObject *obj, QEvent *event) {
    if (obj == m_songLabel && event->type() == QEvent::Resize)
        updateSongLabel();
    return QObject::eventFilter(obj, event);
}

void MusicPlayer::setToggleStyle(QPushButton *btn, bool active) {
    if (active)
        btn->setStyleSheet(QStringLiteral("background-color: #2d7d2d;"));
    else
        btn->setStyleSheet(QString());
}

void MusicPlayer::updateTimeLabel() {
    if (!m_timeLabel || !m_sound || m_soundLengthFrames <= 0) {
        m_timeLabel->setText(QStringLiteral("0:00 / 0:00"));
        return;
    }
    ma_uint64 pos = ma_sound_get_time_in_pcm_frames(m_sound);
    m_timeLabel->setText(formatTime(framesToMs(pos, m_sampleRate))
                         + QStringLiteral(" / ")
                         + formatTime(framesToMs(m_soundLengthFrames, m_sampleRate)));
}

void MusicPlayer::updateSongLabel() {
    if (!m_songLabel)
        return;

    if (m_currentIndex < 0 || m_currentIndex >= m_songList.size()) {
        m_songLabel->setText(QStringLiteral("No song loaded"));
        return;
    }

    setElidedText(QFileInfo(m_songList.at(m_currentIndex)).fileName());
}

void MusicPlayer::setElidedText(const QString &text) {
    QFontMetrics fm(m_songLabel->font());
    int lineH = fm.lineSpacing();
    int maxH = lineH * 2;
    int w = m_songLabel->width();

    if (w <= 0) {
        m_songLabel->setText(text);
        return;
    }

    QRect r = fm.boundingRect(QRect(0, 0, w, maxH), Qt::AlignLeft | Qt::TextWordWrap, text);
    if (r.height() <= maxH) {
        m_songLabel->setText(text);
        return;
    }

    int lo = 0, hi = text.length();
    while (lo < hi) {
        int mid = (lo + hi + 1) / 2;
        QRect rr = fm.boundingRect(QRect(0, 0, w, maxH), Qt::AlignLeft | Qt::TextWordWrap,
                                    text.left(mid) + QStringLiteral("..."));
        if (rr.height() <= maxH)
            lo = mid;
        else
            hi = mid - 1;
    }
    m_songLabel->setText(text.left(lo) + QStringLiteral("..."));
}

void MusicPlayer::scanDirectories() {
    m_songList.clear();

    QJsonArray dirs = m_cfg.get("_.directories").toArray();
    if (dirs.isEmpty()) {
        dirs = m_cfg.get("directories").toArray();
        if (!dirs.isEmpty()) {
            m_cfg.set("_.directories", dirs);
            m_cfg.set("directories", QJsonValue());
            m_cfg.save();
        }
    }
    for (const auto &val : dirs) {
        QDir dir(val.toString());
        if (!dir.exists())
            continue;

        const auto files = dir.entryList({ QStringLiteral("*.mp3") }, QDir::Files, QDir::Name);
        for (const auto &file : files)
            m_songList.append(dir.absoluteFilePath(file));
    }
}

void MusicPlayer::openManageDialog() {
    auto *popup = new QuolPopupWindow(QStringLiteral("Manage Music Directories"), m_manageBtn->window());
    popup->resize(400, 300);

    auto *content = new QWidget(popup);
    auto *layout = new QVBoxLayout(content);
    layout->setContentsMargins(0, 0, 0, 0);

    auto *listWidget = new QListWidget(content);
    const QJsonArray dirs = m_cfg.get("_.directories").toArray();
    for (const auto &val : dirs)
        listWidget->addItem(val.toString());

    auto *addBtn = new QPushButton(QStringLiteral("Add Directory"), content);
    connect(addBtn, &QPushButton::clicked, this, [this, listWidget]() {
        const QString dir = QFileDialog::getExistingDirectory(listWidget, QStringLiteral("Select Music Directory"));
        if (dir.isEmpty())
            return;
        listWidget->addItem(dir);
    });

    auto *removeBtn = new QPushButton(QStringLiteral("Remove Selected"), content);
    connect(removeBtn, &QPushButton::clicked, this, [this, listWidget]() {
        for (auto *item : listWidget->selectedItems())
            delete listWidget->takeItem(listWidget->row(item));
    });

    auto *saveBtn = new QPushButton(QStringLiteral("Save"), content);
    connect(saveBtn, &QPushButton::clicked, this, [this, listWidget, popup]() {
        QJsonArray dirs;
        for (int i = 0; i < listWidget->count(); ++i)
            dirs.append(listWidget->item(i)->text());
        m_cfg.set("_.directories", dirs);
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
