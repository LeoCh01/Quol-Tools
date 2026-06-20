#pragma once

#include "plugin_api/IQuolPlugin.hpp"

#include <QJsonObject>
#include <QObject>

class QComboBox;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;

class Api final : public QObject, public IQuolPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID IQuolPlugin_iid)
    Q_INTERFACES(IQuolPlugin)

public:
    QWidget *createWidget(QWidget *parent = nullptr) override;
    void initialize(const QString &pluginRootPath, const PluginConfig &pluginConfig, QuolServices *services) override;
    void onUpdateConfig(const PluginConfig &pluginConfig) override;
    void shutdown() override;

private:
    void toggleBodyInput();
    void sendRequest();
    void openHeadersDialog();
    void showResponse(
        int statusCode,
        const QString &responseText,
        const QString &url,
        const QString &method,
        const QString &body,
        const QJsonObject &headers
    );

    QString m_pluginRootPath;
    PluginConfig m_cfg;

    QComboBox *m_methodDropdown = nullptr;
    QLineEdit *m_urlInput = nullptr;
    QPlainTextEdit *m_bodyInput = nullptr;
    QPushButton *m_sendButton = nullptr;
    QJsonObject m_headers;
};
