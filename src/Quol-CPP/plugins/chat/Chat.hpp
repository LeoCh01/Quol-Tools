#pragma once

#include "plugin_api/IQuolPlugin.hpp"
#include "plugins/chat/lib/providers/ProviderTypes.hpp"

#include <QElapsedTimer>
#include <QEvent>
#include <QJsonObject>
#include <QMap>
#include <QObject>
#include <QPointer>
#include <QStringList>
#include <QVector>
#include <functional>

class QLineEdit;
class QNetworkAccessManager;
class QNetworkReply;
class QPixmap;
class QProcess;
class QPushButton;
class QTimer;
class QVBoxLayout;

class SnipOverlay;
class QuolPopupWindow;
class ChatOutput;

class Chat final : public QObject, public IQuolPlugin {
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
    using HistoryItem = chat::providers::HistoryItem;
    using ParseResponse = QString (*)(const QJsonObject &response);

    void applyConfig();
    void migrateLegacyKeys();
    void applyButtonIcons();
    void showProviderSelector();
    void clearMessage();
    void updateIncludeImageUi();
    void submitPrompt(bool useExistingSnipImage = false);

    void startSnipMode();
    void cancelSnipMode();
    void startOverlay(const QString &buttonLabel, std::function<void(const QPixmap &)> onSelected);
    void onSnipSelected(const QPixmap &cropped);

    void startExtractMode();
    void onExtractSelected(const QPixmap &cropped);
    void runOcr(const QString &imagePath);
    void onOcrFinished();

    void ensureOutputWindow();
    void showOutput();

    QString applyCommandTemplate(const QString &rawPrompt) const;
    QPixmap capturePrimaryScreenPixmap() const;
    static QString pixmapToBase64Png(const QPixmap &pixmap);

    chat::providers::ProviderRequest buildRequest(
        int endpointIndex, const QVector<HistoryItem> &messages
    );
    void sendJsonRequest(const chat::providers::ProviderRequest &request);
    void onRequestFinished();

    void addHistory(const QString &role, const QString &text, const QString &imageBase64 = QString());
    void trimHistory();
    void appendLog(const QString &provider, bool isUser, const QString &text) const;
    void setControlsEnabled(bool enabled);
    void updatePromptPlaceholder();

    void writeProviderConfig();
    void rebuildProviderRows();

    QString m_pluginRootPath;
    PluginConfig m_cfg;
    QuolServices *m_services = nullptr;

    QVector<chat::providers::ProviderConfig> m_endpoints;
    int m_endpointIndex = 0;
    bool m_ollamaEnabled = false;
    QString m_ollamaModel;
    bool m_hideOutputOnToggle = false;
    bool m_outputWindowHiddenByToggle = false;
    bool m_includeImage = true;
    bool m_historyEnabled = true;
    int m_maxHistory = 10;
    QString m_snipPrompt = QStringLiteral("What is this image?");

    QMap<QString, QString> m_commands;

    QVector<HistoryItem> m_history;
    QString m_pendingProvider;
    ParseResponse m_pendingParse = nullptr;
    QString m_pendingSnipImageBase64;

    QuolPopupWindow *m_providerPopup = nullptr;
    QVBoxLayout *m_providerScrollLayout = nullptr;

    QWidget *m_widget = nullptr;
    QPushButton *m_providerButton = nullptr;
    QLineEdit *m_promptEdit = nullptr;
    QPushButton *m_includeImageButton = nullptr;
    QPushButton *m_snipButton = nullptr;
    QPushButton *m_extractButton = nullptr;

    QuolPopupWindow *m_outputWindow = nullptr;
    ChatOutput *m_output = nullptr;
    QPointer<SnipOverlay> m_snipOverlay;
    QNetworkAccessManager *m_network = nullptr;
    QNetworkReply *m_reply = nullptr;
    QTimer *m_loadingTimer = nullptr;
    QElapsedTimer m_requestTimer;
    QProcess *m_ocrProcess = nullptr;
    QString m_pendingOcrPath;
};
