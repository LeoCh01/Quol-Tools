#include "plugins/api/Api.hpp"

#include "ui/QuolPopupWindow.hpp"

#include <QComboBox>
#include <QEventLoop>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

QWidget *Api::createWidget(QWidget *parent) {
    auto *widget = new QWidget(parent);
    auto *layout = new QVBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);

    auto *methodRow = new QHBoxLayout();
    methodRow->addWidget(new QLabel(QStringLiteral("HTTP Method:"), widget));
    m_methodDropdown = new QComboBox(widget);
    m_methodDropdown->addItems(
        {QStringLiteral("GET"),
         QStringLiteral("POST"),
         QStringLiteral("PUT"),
         QStringLiteral("DELETE"),
         QStringLiteral("PATCH")}
    );
    methodRow->addWidget(m_methodDropdown);
    layout->addLayout(methodRow);

    m_urlInput = new QLineEdit(widget);
    m_urlInput->setPlaceholderText(QStringLiteral("API url..."));
    layout->addWidget(m_urlInput);

    m_bodyInput = new QPlainTextEdit(widget);
    m_bodyInput->setPlaceholderText(QStringLiteral("Body (JSON format)"));
    m_bodyInput->setVisible(false);
    layout->addWidget(m_bodyInput);

    auto *headersBtn = new QPushButton(QStringLiteral("Headers"), widget);
    layout->addWidget(headersBtn);

    m_sendButton = new QPushButton(QStringLiteral("Send Request"), widget);
    layout->addWidget(m_sendButton);

    QObject::connect(m_methodDropdown, &QComboBox::currentTextChanged, this, &Api::toggleBodyInput);
    QObject::connect(headersBtn, &QPushButton::clicked, this, &Api::openHeadersDialog);
    QObject::connect(m_sendButton, &QPushButton::clicked, this, &Api::sendRequest);

    return widget;
}

void Api::initialize(const QString &pluginRootPath, const PluginConfig &pluginConfig, QuolServices *services) {
    Q_UNUSED(services)
    m_pluginRootPath = pluginRootPath;
    m_cfg = pluginConfig;
}

void Api::onUpdateConfig(const PluginConfig &pluginConfig) {
    m_cfg = pluginConfig;
}

void Api::shutdown() {
    m_methodDropdown = nullptr;
    m_urlInput = nullptr;
    m_bodyInput = nullptr;
    m_sendButton = nullptr;
}

void Api::toggleBodyInput() {
    const QString method = m_methodDropdown->currentText();
    const bool hasBody =
        (method == QStringLiteral("POST") || method == QStringLiteral("PUT") || method == QStringLiteral("PATCH"));
    m_bodyInput->setVisible(hasBody);
}

static QPair<int, QString> doSyncRequest(
    const QString &url, const QString &method, const QString &body, const QJsonObject &headers
) {
    auto *manager = new QNetworkAccessManager();

    QNetworkRequest request{QUrl(url)};
    request.setTransferTimeout(30000);

    for (auto it = headers.begin(); it != headers.end(); ++it)
        request.setRawHeader(it.key().toUtf8(), it.value().toString().toUtf8());

    QNetworkReply *reply = nullptr;
    const QByteArray bodyData = body.toUtf8();

    if (method == QStringLiteral("GET"))
        reply = manager->get(request);
    else if (method == QStringLiteral("POST"))
        reply = manager->post(request, bodyData);
    else if (method == QStringLiteral("PUT"))
        reply = manager->put(request, bodyData);
    else if (method == QStringLiteral("DELETE"))
        reply = manager->deleteResource(request);
    else if (method == QStringLiteral("PATCH"))
        reply = manager->sendCustomRequest(request, "PATCH", bodyData);
    else {
        delete manager;
        return {-1, QStringLiteral("Unknown method: ") + method};
    }

    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QString responseText = QString::fromUtf8(reply->readAll());

    reply->deleteLater();
    manager->deleteLater();

    return {statusCode, responseText};
}

void Api::sendRequest() {
    const QString url = m_urlInput->text().trimmed();
    if (!url.startsWith(QStringLiteral("http"), Qt::CaseInsensitive))
        return;

    const QString method = m_methodDropdown->currentText();
    const QString body = m_bodyInput->toPlainText().trimmed();
    const auto [statusCode, responseText] = doSyncRequest(url, method, body, m_headers);
    showResponse(statusCode, responseText, url, method, body, m_headers);
}

void Api::openHeadersDialog() {
    auto *popup = new QuolPopupWindow(QStringLiteral("Edit Headers"), m_sendButton ? m_sendButton->window() : nullptr);
    popup->resize(400, 300);

    auto *editor = new QPlainTextEdit(popup);
    editor->setPlainText(QString::fromUtf8(QJsonDocument(m_headers).toJson(QJsonDocument::Indented)));
    popup->addContent(editor);

    auto *saveBtn = new QPushButton(QStringLiteral("Save Headers"), popup);
    QObject::connect(saveBtn, &QPushButton::clicked, popup, [this, popup, editor]() {
        const QJsonDocument doc = QJsonDocument::fromJson(editor->toPlainText().toUtf8());
        if (doc.isObject())
            m_headers = doc.object();
        popup->close();
    });
    popup->addContent(saveBtn);

    popup->show();
}

void Api::showResponse(
    int statusCode,
    const QString &responseText,
    const QString &url,
    const QString &method,
    const QString &body,
    const QJsonObject &headers
) {
    auto *popup = new QuolPopupWindow(QString::number(statusCode), m_sendButton ? m_sendButton->window() : nullptr);
    popup->resize(600, 400);

    auto *textEdit = new QPlainTextEdit(popup);
    textEdit->setPlainText(responseText);
    textEdit->setReadOnly(true);
    popup->addContent(textEdit);

    auto *buttonRow = new QHBoxLayout();

    auto *resendBtn = new QPushButton(QStringLiteral("Resend Request"), popup);
    auto *editBtn = new QPushButton(QStringLiteral("Edit Request"), popup);

    QObject::connect(resendBtn, &QPushButton::clicked, popup, [popup, textEdit, url, method, body, headers]() {
        const auto [code, text] = doSyncRequest(url, method, body, headers);
        popup->titleBar()->setTitle(QString::number(code));
        textEdit->setPlainText(text);
    });

    QObject::connect(editBtn, &QPushButton::clicked, popup, [this, url, method, body, headers]() {
        m_headers = headers;
        m_urlInput->setText(url);
        m_methodDropdown->setCurrentText(method);
        m_bodyInput->setPlainText(body);
    });

    buttonRow->addWidget(resendBtn);
    buttonRow->addWidget(editBtn);
    popup->addContent(buttonRow);

    popup->show();
}
