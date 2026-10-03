#include "plugins/screen/Screen.hpp"
#include "plugins/screen/lib/ShaderWidget.hpp"
#include "plugins/screen/lib/SnipItem.hpp"
#include "plugins/screen/lib/SnipOverlay.hpp"

#include <QEvent>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QIcon>
#include <QPixmap>
#include <QPushButton>
#include <QScreen>
#include <QWidget>

QWidget *Screen::createWidget(QWidget *parent) {
    m_widget = new QWidget(parent);
    m_widget->installEventFilter(this);
    auto *layout = new QHBoxLayout(m_widget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    auto *snipBtn = new QPushButton(QStringLiteral("Snip"), m_widget);
    snipBtn->setObjectName(QStringLiteral("btn-snip"));
    connect(snipBtn, &QPushButton::clicked, this, &Screen::startSnip);
    layout->addWidget(snipBtn);

    m_shaderBtn = new QPushButton(QStringLiteral("Shader"), m_widget);
    m_shaderBtn->setObjectName(QStringLiteral("btn-toggle"));
    m_shaderBtn->setCheckable(true);
    connect(m_shaderBtn, &QPushButton::clicked, this, &Screen::toggleShader);
    layout->addWidget(m_shaderBtn);

    auto *settingsBtn = new QPushButton(m_widget);
    settingsBtn->setIcon(QIcon(m_pluginRootPath + QStringLiteral("/res/img/setting.svg")));
    settingsBtn->setFixedSize(24, 24);
    settingsBtn->setObjectName(QStringLiteral("btn-settings"));
    connect(settingsBtn, &QPushButton::clicked, m_shaderWidget, &ShaderWidget::openSettings);
    layout->addWidget(settingsBtn);

    return m_widget;
}

void Screen::initialize(const QString &pluginRootPath, const PluginConfig &pluginConfig, QuolServices *services) {
    m_pluginRootPath = pluginRootPath;
    m_cfg = pluginConfig;
    m_services = services;
    applyConfig();

    m_shaderWidget = new ShaderWidget();
    m_shaderWidget->setPluginRootPath(pluginRootPath);
    connect(m_shaderWidget, &ShaderWidget::closed, this, [this]() {
        m_shaderActive = false;
        m_shaderBtn->setChecked(false);
    });
}

void Screen::onUpdateConfig(const PluginConfig &pluginConfig) {
    m_cfg = pluginConfig;
    applyConfig();
}

void Screen::applyConfig() {
    m_hideImgOnToggle = m_cfg.get(QStringLiteral("hide_img_on_toggle")).toBool(false);
}

void Screen::shutdown() {
    if (m_shaderWidget) {
        m_shaderWidget->stop();
        delete m_shaderWidget;
        m_shaderWidget = nullptr;
    }
    qDeleteAll(m_snipItems);
    m_snipItems.clear();
    m_snipOverlay = nullptr;
}

bool Screen::eventFilter(QObject *obj, QEvent *event) {
    if (obj != m_widget || !m_hideImgOnToggle)
        return QObject::eventFilter(obj, event);

    if (event->type() == QEvent::Hide)
        for (SnipItem *item : m_snipItems)
            item->hide();
    else if (event->type() == QEvent::Show)
        for (SnipItem *item : m_snipItems)
            item->show();

    return QObject::eventFilter(obj, event);
}

void Screen::toggleShader() {
    m_shaderActive = !m_shaderActive;
    m_shaderBtn->setChecked(m_shaderActive);
    if (m_shaderActive)
        m_shaderWidget->start(m_services);
    else
        m_shaderWidget->stop();
}

void Screen::startSnip() {
    if (m_snipOverlay)
        return;

    m_services->hideAllPluginWindows();
    QPixmap screenshot;
    if (QScreen *screen = QGuiApplication::primaryScreen())
        screenshot = screen->grabWindow(0);
    m_services->showAllPluginWindows();

    if (screenshot.isNull())
        return;

    m_snipOverlay = new SnipOverlay(screenshot);
    connect(m_snipOverlay, &SnipOverlay::snipReady, this, &Screen::onSnipReady);
    connect(m_snipOverlay, &QWidget::destroyed, this, [this]() { m_snipOverlay = nullptr; });
    m_snipOverlay->showFullScreen();
    m_snipOverlay->raise();
    m_snipOverlay->activateWindow();
}

void Screen::onSnipReady(const QPixmap &cropped) {
    auto *item = new SnipItem(cropped);
    connect(item, &SnipItem::deleted, this, &Screen::deleteSnipItem);
    m_snipItems.append(item);
    item->show();
    item->raise();
    item->activateWindow();
}

void Screen::deleteSnipItem(SnipItem *item) {
    m_snipItems.removeOne(item);
    item->deleteLater();
}