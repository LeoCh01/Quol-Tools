#pragma once

#include "plugin_api/IQuolPlugin.hpp"
#include "plugins/chat/lib/providers/ProviderTypes.hpp"

#include <QElapsedTimer>
#include <QMap>
#include <QNetworkReply>
#include <QObject>
#include <QStringList>
#include <QVector>

class QLineEdit;
class QListWidget;
class QNetworkAccessManager;
class QPixmap;
class QPushButton;
class QTextBrowser;
class QTimer;

class SnipOverlay;
class QuolPopupWindow;

class Chat final : public QObject, public IQuolPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID IQuolPlugin_iid)
    Q_INTERFACES(IQuolPlugin)

public:
    QWidget *createWidget(QWidget *parent = nullptr) override;

    void initialize(const QString &pluginRootPath, const PluginConfig &pluginConfig, QuolServices *services) override;
    void onUpdateConfig(const PluginConfig &pluginConfig) override;
    void shutdown() override;

private:
    using HistoryItem = chat::providers::HistoryItem;
    using EndpointConfig = chat::providers::EndpointConfig;

    void applyConfig();
    void applyButtonIcons();
    void showEndpointSelector();
    void activateEndpoint(int index);
    void clearMessage();
    void updateIncludeImageUi();
    void submitPrompt(bool useExistingSnipImage = false);

    void startSnipMode();
    void cancelSnipMode();
    void onSnipSelected(const QPixmap &cropped);

    void ensureOutputWindow();
    void setOutputText(const QString &html);
    QString buildConversationHtml(const QString &pendingAssistantText = QString()) const;

    QString applyCommandTemplate(const QString &rawPrompt) const;
    QPixmap capturePrimaryScreenPixmap() const;
    QString capturePrimaryScreenBase64Png() const;
    static QString pixmapToBase64Png(const QPixmap &pixmap);

    void dispatchProviderRequest(const QString &prompt, const QString &imageBase64);
    void sendJsonRequest(const chat::providers::ProviderRequest &request);
    void onRequestFinished();
    void onRequestError(QNetworkReply::NetworkError code);

    void addHistory(const QString &role, const QString &text, const QString &imageBase64 = QString());
    void trimHistory();
    void appendLog(const QString &provider, bool isUser, const QString &text) const;
    void setControlsEnabled(bool enabled);
    void updatePromptPlaceholder();

    void writeEndpointsToConfig();

    QString m_pluginRootPath;
    PluginConfig m_cfg;
    QuolServices *m_services = nullptr;

    bool m_includeImage = true;
    bool m_historyEnabled = true;
    int m_maxHistory = 10;
    QString m_snipPrompt = "What is this image?";

    QVector<EndpointConfig> m_endpoints;
    int m_activeEndpointIndex = 0;
    QMap<QString, QString> m_commands;

    QVector<HistoryItem> m_history;
    QString m_pendingSnipImageBase64;

    QWidget *m_widget = nullptr;
    QPushButton *m_providerButton = nullptr;
    QPushButton *m_clearButton = nullptr;
    QLineEdit *m_promptEdit = nullptr;
    QPushButton *m_includeImageButton = nullptr;
    QPushButton *m_snipButton = nullptr;

    QuolPopupWindow *m_outputWindow = nullptr;
    QTextBrowser *m_outputBrowser = nullptr;
    SnipOverlay *m_snipOverlay = nullptr;
    QNetworkAccessManager *m_network = nullptr;
    QNetworkReply *m_reply = nullptr;
    QTimer *m_loadingTimer = nullptr;
    QElapsedTimer m_requestTimer;
};
