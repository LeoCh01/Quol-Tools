#include "plugins/chance/Chance.hpp"

#include <QEvent>
#include <QGridLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QMovie>
#include <QPushButton>
#include <QRandomGenerator>
#include <QTimer>
#include <QWidget>

static const QString COIN_PLACEHOLDER = QStringLiteral("coin-x.png");
static const QString DICE_PLACEHOLDER = QStringLiteral("dice-x.png");
static const QString CONFETTI = QStringLiteral("confetti2.gif");

static const QStringList COIN_FRAMES = {
    QStringLiteral("coin-h.png"),
    QStringLiteral("coin-t.png"),
};
static const QStringList DICE_FRAMES = {
    QStringLiteral("dice-1.png"),
    QStringLiteral("dice-2.png"),
    QStringLiteral("dice-3.png"),
    QStringLiteral("dice-4.png"),
    QStringLiteral("dice-5.png"),
    QStringLiteral("dice-6.png"),
};

// ---------------------------------------------------------------------------
QWidget *Chance::createWidget(QWidget *parent) {
    auto *widget = new QWidget(parent);
    auto *gridLayout = new QGridLayout(widget);
    gridLayout->setContentsMargins(0, 0, 0, 0);

    // --- Image label (click to flip/roll) ---
    m_resultLabel = new QLabel(widget);
    m_resultLabel->setAlignment(Qt::AlignCenter);
    m_resultLabel->setCursor(Qt::PointingHandCursor);
    m_resultLabel->setPixmap(QPixmap(m_pluginRootPath + QStringLiteral("/res/img/") + COIN_PLACEHOLDER));
    m_resultLabel->installEventFilter(this);
    gridLayout->addWidget(m_resultLabel, 0, 0, 1, 2);

    // --- Mode buttons ---
    m_coinButton = new QPushButton(QStringLiteral("Coin"), widget);
    m_coinButton->setObjectName("btn-toggle");
    m_coinButton->setCheckable(true);
    m_diceButton = new QPushButton(QStringLiteral("Dice"), widget);
    m_diceButton->setObjectName("btn-toggle");
    m_diceButton->setCheckable(true);
    gridLayout->addWidget(m_coinButton, 1, 0);
    gridLayout->addWidget(m_diceButton, 1, 1);

    QObject::connect(m_coinButton, &QPushButton::clicked, this, &Chance::setCoinMode);
    QObject::connect(m_diceButton, &QPushButton::clicked, this, &Chance::setDiceMode);

    updateModeButtons();

    return widget;
}

// ---------------------------------------------------------------------------
void Chance::initialize(const QString &pluginRootPath, const PluginConfig &pluginConfig, QuolServices *services) {
    Q_UNUSED(services)
    m_pluginRootPath = pluginRootPath;
    m_cfg = pluginConfig;
}

void Chance::onUpdateConfig(const PluginConfig &pluginConfig) {
    m_cfg = pluginConfig;
}

void Chance::shutdown() {
    if (m_flipTimer) {
        m_flipTimer->stop();
        m_flipTimer = nullptr;
    }
    if (m_confettiMovie) {
        m_confettiMovie->stop();
        m_confettiMovie = nullptr;
    }
    m_resultLabel = nullptr;
    m_coinButton = nullptr;
    m_diceButton = nullptr;
    m_confettiLabel = nullptr;
    m_isRunning = false;
}

// ---------------------------------------------------------------------------
bool Chance::eventFilter(QObject *obj, QEvent *event) {
    if (obj == m_resultLabel && event->type() == QEvent::MouseButtonPress) {
        performAction();
        return true;
    }
    return QObject::eventFilter(obj, event);
}

// ---------------------------------------------------------------------------
void Chance::setCoinMode() {
    if (m_isRunning)
        return;
    m_isCoinMode = true;
    updateModeButtons();
}

void Chance::setDiceMode() {
    if (m_isRunning)
        return;
    m_isCoinMode = false;
    updateModeButtons();
}

void Chance::updateModeButtons() {
    if (!m_coinButton || !m_diceButton || !m_resultLabel)
        return;

    m_coinButton->setChecked(m_isCoinMode);
    m_diceButton->setChecked(!m_isCoinMode);
    m_resultLabel->setPixmap(
        QPixmap(m_pluginRootPath + QStringLiteral("/res/img/") + (m_isCoinMode ? COIN_PLACEHOLDER : DICE_PLACEHOLDER))
    );
}

void Chance::performAction() {
    if (m_isRunning)
        return;
    m_isRunning = true;

    if (m_isCoinMode) {
        // Shuffle the 2 coin frames and pick a random total flip count (14-15)
        QStringList frames = COIN_FRAMES;
        if (QRandomGenerator::global()->bounded(2))
            std::reverse(frames.begin(), frames.end());
        int total = 14 + static_cast<int>(QRandomGenerator::global()->bounded(2));
        startFlipAnimation(frames, total);
    } else {
        // Shuffle dice frames and pick a random total (12-16)
        QStringList frames = DICE_FRAMES;
        for (int i = frames.size() - 1; i > 0; --i) {
            int j = static_cast<int>(QRandomGenerator::global()->bounded(i + 1));
            frames.swapItemsAt(i, j);
        }
        int total = 12 + static_cast<int>(QRandomGenerator::global()->bounded(5));
        startFlipAnimation(frames, total);
    }
}

void Chance::startFlipAnimation(const QStringList &frames, int totalFlips) {
    m_animFrames = frames;
    m_totalFlips = totalFlips;
    m_flipCounter = 0;

    if (m_flipTimer) {
        m_flipTimer->stop();
        m_flipTimer->deleteLater();
    }

    m_flipTimer = new QTimer(this);
    m_flipTimer->setInterval(10);

    QObject::connect(m_flipTimer, &QTimer::timeout, this, [this]() {
        if (!m_resultLabel) {
            m_flipTimer->stop();
            return;
        }

        if (m_flipCounter < m_totalFlips) {
            const QString &frame = m_animFrames[m_flipCounter % m_animFrames.size()];
            m_resultLabel->setPixmap(QPixmap(m_pluginRootPath + QStringLiteral("/res/img/") + frame));
            ++m_flipCounter;
            // Progressively slow down
            m_flipTimer->setInterval(10 + m_flipCounter * 20);
        } else {
            m_flipTimer->stop();
            showConfetti();
        }
    });

    m_flipTimer->start();
}

void Chance::showConfetti() {
    if (!m_resultLabel)
        return;

    m_confettiLabel = new QLabel(m_resultLabel->parentWidget());
    m_confettiLabel->setAlignment(Qt::AlignCenter);
    m_confettiLabel->setGeometry(m_resultLabel->geometry());
    m_confettiLabel->raise();

    m_confettiMovie = new QMovie(m_pluginRootPath + QStringLiteral("/res/img/") + CONFETTI);
    m_confettiMovie->setSpeed(150);
    m_confettiLabel->setMovie(m_confettiMovie);
    m_confettiLabel->show();
    m_confettiMovie->start();

    QTimer::singleShot(1700, this, &Chance::hideConfetti);
}

void Chance::hideConfetti() {
    m_isRunning = false;

    if (m_confettiMovie) {
        m_confettiMovie->stop();
        m_confettiMovie->deleteLater();
        m_confettiMovie = nullptr;
    }

    if (m_confettiLabel) {
        m_confettiLabel->deleteLater();
        m_confettiLabel = nullptr;
    }
}
