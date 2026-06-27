#pragma once

#include "plugin_api/IQuolPlugin.hpp"

#include <QObject>
#include <QStringList>

class QLabel;
class QMovie;
class QPushButton;
class QTimer;

class Chance final : public QObject, public IQuolPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID IQuolPlugin_iid)
    Q_INTERFACES(IQuolPlugin)

public:
    QWidget *createWidget(QWidget *parent = nullptr) override;

    void initialize(const QString &pluginRootPath, const PluginConfig &pluginConfig, QuolServices *services) override;
    void onUpdateConfig(const PluginConfig &pluginConfig) override;
    void shutdown() override;

    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void setCoinMode();
    void setDiceMode();
    void performAction();
    void startFlipAnimation(const QStringList &frames, int totalFlips);
    void showConfetti();
    void hideConfetti();
    void updateModeButtons();

    QString m_pluginRootPath;
    PluginConfig m_cfg;

    bool m_isCoinMode = true;
    bool m_isRunning = false;
    int m_flipCounter = 0;
    int m_totalFlips = 0;
    QStringList m_animFrames;

    QLabel *m_resultLabel = nullptr;
    QPushButton *m_coinButton = nullptr;
    QPushButton *m_diceButton = nullptr;
    QTimer *m_flipTimer = nullptr;
    QLabel *m_confettiLabel = nullptr;
    QMovie *m_confettiMovie = nullptr;
};
