#include "plugins/chat/Chat.hpp"

#include "plugins/chat/lib/ChatConfig.hpp"
#include "plugins/chat/lib/SnipOverlay.hpp"
#include "plugins/chat/lib/providers/OpenRouterProvider.hpp"
#include "ui/QuolPopupWindow.hpp"

#include <QBuffer>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QIcon>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QPushButton>
#include <QRegularExpression>
#include <QScreen>
#include <QScrollArea>
#include <QScrollBar>
#include <QTextBrowser>
#include <QTextDocument>
#include <QTimer>
#include <QVBoxLayout>
#include <QUrl>
#include <QWidget>

namespace {
QString markdownToHtmlFragment(const QString &markdown) {
    QTextDocument doc;
    doc.setMarkdown(markdown);

    const QString html = doc.toHtml();
    const int bodyStart = html.indexOf(QStringLiteral("<body"));
    if (bodyStart < 0)
        return markdown.toHtmlEscaped().replace(QStringLiteral("\n"), QStringLiteral("<br/>"));

    const int start = html.indexOf('>', bodyStart);
    const int end = html.lastIndexOf(QStringLiteral("</body>"));
    if (start < 0 || end <= start)
        return markdown.toHtmlEscaped().replace(QStringLiteral("\n"), QStringLiteral("<br/>"));

    return html.mid(start + 1, end - start - 1).trimmed();
}

QString messageHtml(const QString &role, const QString &text, bool hasImage, bool pending = false) {
    const bool isModel = (role == QStringLiteral("model"));
    const QString align = isModel ? QStringLiteral("left") : QStringLiteral("right");
    const QString cls = (isModel ? QStringLiteral("ai-block") : QStringLiteral("user-block"))
                        + (pending ? QStringLiteral(" pending") : QString());
    const QString attach = hasImage ? QStringLiteral("<div class='attachment'>Screenshot attached</div>") : QString();
    const QString content = markdownToHtmlFragment(text);

    return QStringLiteral(
               "<table width='100%'><tr>"
               "<td align='%1' class='%2'><div>%3%4</div></td>"
               "</tr></table>"
    )
        .arg(align, cls, attach, content);
}
}  // namespace

QWidget *Chat::createWidget(QWidget *parent) {
    m_widget = new QWidget(parent);
    auto *layout = new QHBoxLayout(m_widget);
    layout->setContentsMargins(0, 0, 0, 0);

    m_providerButton = new QPushButton(m_widget);
    m_clearButton = new QPushButton(m_widget);
    m_promptEdit = new QLineEdit(m_widget);
    m_includeImageButton = new QPushButton(m_widget);
    m_snipButton = new QPushButton(m_widget);

    m_includeImageButton->setCheckable(true);
    const QSize iconSize(18, 18);

    m_providerButton->setIconSize(iconSize);
    m_clearButton->setIconSize(iconSize);
    m_includeImageButton->setIconSize(iconSize);
    m_snipButton->setIconSize(iconSize);

    m_providerButton->setToolTip(QStringLiteral("Select provider / model"));
    m_clearButton->setToolTip(QStringLiteral("Clear message"));
    m_includeImageButton->setToolTip(QStringLiteral("Include screenshot"));
    m_snipButton->setToolTip(QStringLiteral("Snip mode"));

    layout->addWidget(m_providerButton);
    layout->addWidget(m_clearButton);
    layout->addWidget(m_promptEdit, 1);
    layout->addWidget(m_includeImageButton);
    layout->addWidget(m_snipButton);

    QObject::connect(m_providerButton, &QPushButton::clicked, this, &Chat::showEndpointSelector);
    QObject::connect(m_clearButton, &QPushButton::clicked, this, &Chat::clearMessage);
    QObject::connect(m_includeImageButton, &QPushButton::clicked, this, [this]() {
        m_includeImage = m_includeImageButton->isChecked();
        updateIncludeImageUi();
    });
    QObject::connect(m_snipButton, &QPushButton::clicked, this, [this]() { startSnipMode(); });
    QObject::connect(m_promptEdit, &QLineEdit::returnPressed, this, [this]() { submitPrompt(); });

    if (!m_network)
        m_network = new QNetworkAccessManager(this);

    applyButtonIcons();
    applyConfig();
    return m_widget;
}

void Chat::initialize(const QString &pluginRootPath, const PluginConfig &pluginConfig, QuolServices *services) {
    m_pluginRootPath = pluginRootPath;
    m_cfg = pluginConfig;
    m_services = services;

    applyConfig();
}

void Chat::onUpdateConfig(const PluginConfig &pluginConfig) {
    m_cfg = pluginConfig;
    applyConfig();
}

void Chat::shutdown() {
    cancelSnipMode();

    if (m_loadingTimer) {
        m_loadingTimer->stop();
        delete m_loadingTimer;
        m_loadingTimer = nullptr;
    }

    if (m_reply) {
        m_reply->abort();
        m_reply->deleteLater();
        m_reply = nullptr;
    }

    if (m_outputWindow) {
        m_outputWindow->close();
        m_outputWindow = nullptr;
    }

    m_outputBrowser = nullptr;
    m_services = nullptr;

    m_providerButton = nullptr;
    m_clearButton = nullptr;
    m_promptEdit = nullptr;
    m_includeImageButton = nullptr;
    m_snipButton = nullptr;
    m_widget = nullptr;
}

void Chat::applyConfig() {
    const auto parsed = chat::config::parse(m_cfg.root());

    m_includeImage = parsed.includeImage;
    m_historyEnabled = parsed.historyEnabled;
    m_maxHistory = parsed.maxHistory;
    m_snipPrompt = parsed.snipPrompt;

    m_commands = parsed.commands;
    m_endpoints = parsed.endpoints;
    m_activeEndpointIndex = parsed.activeEndpointIndex;

    applyButtonIcons();
    updatePromptPlaceholder();
    updateIncludeImageUi();
}

void Chat::applyButtonIcons() {
    if (m_pluginRootPath.isEmpty())
        return;

    if (m_providerButton)
        m_providerButton->setIcon(QIcon(m_pluginRootPath + QStringLiteral("/res/img/cycle.svg")));
    if (m_clearButton)
        m_clearButton->setIcon(QIcon(m_pluginRootPath + QStringLiteral("/res/img/clear.svg")));
    if (m_includeImageButton)
        m_includeImageButton->setIcon(QIcon(m_pluginRootPath + QStringLiteral("/res/img/img.svg")));
    if (m_snipButton)
        m_snipButton->setIcon(QIcon(m_pluginRootPath + QStringLiteral("/res/img/snip.svg")));
}

void Chat::writeEndpointsToConfig() {
    QJsonObject endpointsObj;
    QJsonArray order;
    for (const auto &ep : m_endpoints) {
        order.append(ep.name);
        QJsonObject eo;
        eo.insert(QStringLiteral("model"), ep.model);
        eo.insert(QStringLiteral("apikey"), ep.apiKey);
        endpointsObj.insert(ep.name, eo);
    }

    m_cfg.set(QStringLiteral("endpoints"), endpointsObj);
    m_cfg.set(QStringLiteral("endpoint_order"), order);
    m_cfg.set(QStringLiteral("_.active_endpoint"),
              m_endpoints.isEmpty() ? QString() : m_endpoints[m_activeEndpointIndex].name);
    m_cfg.save();
}

void Chat::showEndpointSelector() {
    if (m_endpoints.isEmpty())
        return;

    auto *popup = new QuolPopupWindow(QStringLiteral("Select Provider / Model"), m_widget);

    auto *outerWidget = new QWidget();
    auto *outerLayout = new QVBoxLayout(outerWidget);
    outerLayout->setContentsMargins(0, 0, 0, 0);

    auto *scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    auto *scrollContent = new QWidget();
    auto *scrollLayout = new QVBoxLayout(scrollContent);
    scrollLayout->setContentsMargins(8, 8, 8, 8);
    scrollLayout->setSpacing(8);

    QVector<QWidget *> rows;

    for (int i = 0; i < m_endpoints.size(); ++i) {
        auto &ep = m_endpoints[i];

        auto *row = new QFrame();
        row->setFrameShape(QFrame::StyledPanel);
        row->setStyleSheet(QStringLiteral(
            "QFrame { background:#2d2d2d; border:1px solid #444; border-radius:4px; padding:8px; }"
            "QFrame:hover { border-color:#4CAF50; }"
        ));

        auto *rowLayout = new QVBoxLayout(row);
        rowLayout->setContentsMargins(8, 8, 8, 8);
        rowLayout->setSpacing(6);

        auto *headerLayout = new QHBoxLayout();

        auto *nameLabel = new QLabel(ep.name);
        nameLabel->setStyleSheet(QStringLiteral("font-weight:bold; color:#fff; font-size:13px; border:none; background:transparent;"));
        headerLayout->addWidget(nameLabel);

        headerLayout->addStretch();

        auto *activateBtn = new QPushButton(i == m_activeEndpointIndex ? QStringLiteral("ACTIVE") : QStringLiteral("Activate"));
        activateBtn->setFixedHeight(26);
        if (i == m_activeEndpointIndex) {
            activateBtn->setStyleSheet(QStringLiteral(
                "QPushButton { background:#4CAF50; color:white; border:none; border-radius:3px; padding:2px 12px; font-weight:bold; }"
            ));
            activateBtn->setEnabled(false);
        } else {
            activateBtn->setStyleSheet(QStringLiteral(
                "QPushButton { background:#555; color:#ccc; border:none; border-radius:3px; padding:2px 12px; }"
                "QPushButton:hover { background:#666; color:white; }"
            ));
        }
        headerLayout->addWidget(activateBtn);

        auto *removeBtn = new QPushButton(QStringLiteral("X"));
        removeBtn->setFixedSize(26, 26);
        removeBtn->setToolTip(QStringLiteral("Remove this endpoint"));
        removeBtn->setStyleSheet(QStringLiteral(
            "QPushButton { background:#c0392b; color:white; border:none; border-radius:3px; font-weight:bold; }"
            "QPushButton:hover { background:#e74c3c; }"
        ));
        headerLayout->addWidget(removeBtn);

        rowLayout->addLayout(headerLayout);

        auto *modelEdit = new QLineEdit(ep.model);
        modelEdit->setPlaceholderText(QStringLiteral("Model (e.g. openai/gpt-4o)"));
        modelEdit->setStyleSheet(QStringLiteral(
            "QLineEdit { background:#3a3a3a; color:white; border:1px solid #555; border-radius:3px; padding:4px 6px; }"
            "QLineEdit:focus { border-color:#4CAF50; }"
        ));
        rowLayout->addWidget(modelEdit);

        auto *keyEdit = new QLineEdit(ep.apiKey);
        keyEdit->setPlaceholderText(QStringLiteral("OpenRouter API key"));
        keyEdit->setEchoMode(QLineEdit::Password);
        keyEdit->setStyleSheet(QStringLiteral(
            "QLineEdit { background:#3a3a3a; color:white; border:1px solid #555; border-radius:3px; padding:4px 6px; }"
            "QLineEdit:focus { border-color:#4CAF50; }"
        ));
        rowLayout->addWidget(keyEdit);

        scrollLayout->addWidget(row);
        rows.append(row);

        int idx = i;
        QObject::connect(activateBtn, &QPushButton::clicked, popup, [this, idx, popup]() {
            activateEndpoint(idx);
            writeEndpointsToConfig();
            popup->close();
        });

        QObject::connect(removeBtn, &QPushButton::clicked, popup, [this, idx, popup]() {
            if (m_endpoints.size() <= 1)
                return;
            m_endpoints.removeAt(idx);
            if (m_activeEndpointIndex >= m_endpoints.size())
                m_activeEndpointIndex = m_endpoints.size() - 1;
            writeEndpointsToConfig();
            updatePromptPlaceholder();
            popup->close();
            showEndpointSelector();
        });

        QObject::connect(modelEdit, &QLineEdit::textChanged, popup, [this, i, &ep, modelEdit]() {
            ep.model = modelEdit->text().trimmed();
        });
        QObject::connect(keyEdit, &QLineEdit::textChanged, popup, [this, i, &ep, keyEdit]() {
            ep.apiKey = keyEdit->text();
        });
    }

    scrollLayout->addStretch();
    scrollArea->setWidget(scrollContent);
    outerLayout->addWidget(scrollArea);

    auto *btnLayout = new QHBoxLayout();
    auto *addBtn = new QPushButton(QStringLiteral("+ Add endpoint"));
    addBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background:#2d7d46; color:white; border:none; border-radius:3px; padding:6px 16px; font-weight:bold; }"
        "QPushButton:hover { background:#3a9d56; }"
    ));
    QObject::connect(addBtn, &QPushButton::clicked, popup, [this, popup]() {
        int n = 1;
        while (true) {
            QString name = QStringLiteral("endpoint-%1").arg(n);
            bool exists = false;
            for (const auto &ep : m_endpoints) {
                if (ep.name == name) { exists = true; break; }
            }
            if (!exists) break;
            ++n;
        }
        m_endpoints.append({QStringLiteral("endpoint-%1").arg(n), QString(), QString()});
        writeEndpointsToConfig();
        popup->close();
        showEndpointSelector();
    });
    btnLayout->addWidget(addBtn);
    btnLayout->addStretch();
    outerLayout->addLayout(btnLayout);

    popup->addContent(outerWidget);
    popup->resize(420, 400);
    popup->show();
}

void Chat::activateEndpoint(int index) {
    if (index < 0 || index >= m_endpoints.size())
        return;
    m_activeEndpointIndex = index;
    updatePromptPlaceholder();
}

void Chat::clearMessage() {
    if (m_promptEdit)
        m_promptEdit->clear();

    m_history.clear();

    if (m_outputWindow) {
        m_outputWindow->close();
    }
}

void Chat::updateIncludeImageUi() {
    if (!m_includeImageButton)
        return;

    m_includeImageButton->setChecked(m_includeImage);
    m_includeImageButton->setStyleSheet(
        m_includeImage ? QStringLiteral("background-color: #4CAF50;") : QStringLiteral("background-color: #F44336;")
    );

    m_includeImageButton->setToolTip(
        m_includeImage ? QStringLiteral("Include screenshot: ON") : QStringLiteral("Include screenshot: OFF")
    );
}

void Chat::submitPrompt(bool useExistingSnipImage) {
    if (!m_promptEdit || m_endpoints.isEmpty())
        return;

    QString prompt = m_promptEdit->text().trimmed();
    if (prompt.isEmpty())
        return;

    prompt = applyCommandTemplate(prompt);

    QString imageBase64;
    if (m_includeImage) {
        if (useExistingSnipImage) {
            imageBase64 = m_pendingSnipImageBase64;
        } else {
            imageBase64 = capturePrimaryScreenBase64Png();
        }
    }

    addHistory(QStringLiteral("user"), prompt, imageBase64);
    appendLog(m_endpoints[m_activeEndpointIndex].name, true, prompt);

    setControlsEnabled(false);
    m_requestTimer.start();
    if (!m_loadingTimer) {
        m_loadingTimer = new QTimer(this);
        m_loadingTimer->setInterval(100);
        QObject::connect(m_loadingTimer, &QTimer::timeout, this, [this]() {
            if (m_outputBrowser) {
                const double elapsed = m_requestTimer.elapsed() / 1000.0;
                const QString text = QStringLiteral("Loading... (%1 s)").arg(elapsed, 0, 'f', 1);
                m_outputBrowser->setHtml(buildConversationHtml(text));
                auto *sb = m_outputBrowser->verticalScrollBar();
                sb->setValue(sb->maximum());
            }
        });
    }
    m_loadingTimer->start();
    setOutputText(buildConversationHtml(QStringLiteral("Loading... (0.0 s)")));

    dispatchProviderRequest(prompt, imageBase64);

    m_promptEdit->clear();
}

void Chat::startSnipMode() {
    const QPixmap screenshot = capturePrimaryScreenPixmap();
    if (screenshot.isNull())
        return;

    cancelSnipMode();

    m_snipOverlay = new SnipOverlay(screenshot, [this](const QPixmap &cropped) { onSnipSelected(cropped); });
    QObject::connect(m_snipOverlay, &QObject::destroyed, this, [this]() { m_snipOverlay = nullptr; });
    m_snipOverlay->showFullScreen();
    m_snipOverlay->raise();
    m_snipOverlay->activateWindow();
}

void Chat::cancelSnipMode() {
    if (m_snipOverlay) {
        m_snipOverlay->close();
        m_snipOverlay->deleteLater();
        m_snipOverlay = nullptr;
    }
}

void Chat::onSnipSelected(const QPixmap &cropped) {
    if (cropped.isNull())
        return;

    m_pendingSnipImageBase64 = pixmapToBase64Png(cropped);

    if (m_promptEdit && m_promptEdit->text().trimmed().isEmpty())
        m_promptEdit->setText(m_snipPrompt);

    submitPrompt(true);
}

void Chat::ensureOutputWindow() {
    if (m_outputWindow)
        return;

    m_outputWindow = new QuolPopupWindow(QStringLiteral("Chat Output"), m_widget);
    QObject::connect(m_outputWindow, &QObject::destroyed, this, [this]() {
        m_outputWindow = nullptr;
        m_outputBrowser = nullptr;
        m_history.clear();
    });
    m_outputWindow->resize(500, 600);

    m_outputBrowser = new QTextBrowser(m_outputWindow);
    m_outputBrowser->setOpenExternalLinks(true);
    m_outputBrowser->document()->setDocumentMargin(0);
    m_outputWindow->addContent(m_outputBrowser);
}

void Chat::setOutputText(const QString &html) {
    ensureOutputWindow();
    if (!m_outputWindow || !m_outputBrowser)
        return;

    m_outputBrowser->setHtml(html);

    m_outputWindow->show();
    m_outputWindow->raise();
    m_outputWindow->activateWindow();
    auto *sb = m_outputBrowser->verticalScrollBar();
    sb->setValue(sb->maximum());
}

QString Chat::buildConversationHtml(const QString &pendingAssistantText) const {
    QString html = QStringLiteral(
        "<html>"
        "<head>"
        "<style>"
        "body { background:#333; color:white; font-size:13px; letter-spacing:0.2px; line-height:1.4;"
        "       margin:0; padding:0; font-family:'Segoe UI',sans-serif; }"
        "table { border-collapse:collapse; margin:0; padding:0; }"
        ".ai-block { padding:10px 15px; background:#2d2d2d; word-wrap:break-word; white-space:pre-wrap; }"
        ".user-block { padding:10px 15px; background:#3a3a3a; word-wrap:break-word; white-space:pre-wrap; }"
        ".pending { opacity:0.75; }"
        "p { margin:0.3em 0; }"
        "p:first-child { margin-top:0; }"
        "p:last-child  { margin-bottom:0; }"
        "h1,h2,h3,h4 { margin:0.4em 0 0.2em 0; }"
        "ul,ol { margin:0.3em 0 0.3em 1.4em; }"
        "li { margin:0.15em 0; }"
        "pre { background:#262626; line-height:1.3; word-wrap:break-word; white-space:pre-wrap;"
        "      padding:8px 10px; margin:6px 0; border-radius:4px; font-size:12px; }"
        "code { background:#262626; padding:1px 4px; border-radius:3px; font-size:12px; }"
        "pre code { background:transparent; padding:0; }"
        "blockquote { margin:0.5em 0; padding:4px 10px; border-left:3px solid #888; color:#ccc; }"
        "a { color:#7ab8f5; }"
        ".attachment { color:#aaa; font-size:11px; }"
        "</style>"
        "</head>"
        "<body>"
    );

    for (const auto &item : m_history)
        html += messageHtml(item.role, item.text, !item.imageBase64.isEmpty());

    if (!pendingAssistantText.isEmpty())
        html += messageHtml(QStringLiteral("model"), pendingAssistantText, false, true);

    html += QStringLiteral("</body></html>");
    return html;
}

QString Chat::applyCommandTemplate(const QString &rawPrompt) const {
    const QStringList tokens = rawPrompt.split(' ', Qt::SkipEmptyParts);
    if (tokens.isEmpty())
        return rawPrompt;

    const QString cmd = tokens.first();
    if (!m_commands.contains(cmd))
        return rawPrompt;

    QString result = m_commands.value(cmd);
    for (int i = 1; i < tokens.size(); ++i) {
        result.replace(QString(QStringLiteral("{%1}")).arg(i - 1), tokens.at(i));
    }

    QRegularExpression optionalExpr(R"(\{(\d+):([^}]+)\})");
    auto it = optionalExpr.globalMatch(result);
    while (it.hasNext()) {
        const auto m = it.next();
        result.replace(m.captured(0), m.captured(2));
    }

    QRegularExpression bareExpr(R"(\{\d+\})");
    result.remove(bareExpr);
    return result.trimmed();
}

QString Chat::capturePrimaryScreenBase64Png() const {
    return pixmapToBase64Png(capturePrimaryScreenPixmap());
}

QPixmap Chat::capturePrimaryScreenPixmap() const {
    QScreen *screen = QGuiApplication::primaryScreen();
    if (!screen)
        return {};

    if (m_services)
        m_services->hideAllPluginWindows();

    const QPixmap screenshot = screen->grabWindow(0);

    if (m_services)
        m_services->showAllPluginWindows();

    return screenshot;
}

QString Chat::pixmapToBase64Png(const QPixmap &pixmap) {
    if (pixmap.isNull())
        return {};

    QByteArray bytes;
    QBuffer buffer(&bytes);
    if (!buffer.open(QIODevice::WriteOnly))
        return {};

    pixmap.toImage().save(&buffer, "PNG");
    return QString::fromLatin1(bytes.toBase64());
}

void Chat::dispatchProviderRequest(const QString &prompt, const QString &imageBase64) {
    const auto &ep = m_endpoints[m_activeEndpointIndex];
    chat::providers::ProviderConfig config{ep.model, ep.apiKey};
    chat::providers::ProviderRequest request =
        chat::providers::openrouter::buildRequest(config, m_history, prompt, imageBase64, m_historyEnabled);

    sendJsonRequest(request);
}

void Chat::sendJsonRequest(const chat::providers::ProviderRequest &request) {
    if (!m_network)
        return;

    if (m_reply) {
        m_reply->deleteLater();
        m_reply = nullptr;
    }

    QNetworkRequest req(request.url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

    // OpenRouter expects API key in Authorization header
    if (!request.bearerToken.trimmed().isEmpty()) {
        req.setRawHeader("Authorization", QByteArray("Bearer ") + request.bearerToken.toUtf8());
    }

    // OpenRouter optional headers
    req.setRawHeader("HTTP-Referer", QByteArray("https://quol.app"));
    req.setRawHeader("X-Title", QByteArray("Quol Chat"));

    m_reply = m_network->post(req, QJsonDocument(request.payload).toJson(QJsonDocument::Compact));
    QObject::connect(m_reply, &QNetworkReply::finished, this, &Chat::onRequestFinished);
    QObject::connect(m_reply, &QNetworkReply::errorOccurred, this, &Chat::onRequestError);
}

void Chat::onRequestFinished() {
    if (!m_reply)
        return;

    const QByteArray raw = m_reply->readAll();
    const QJsonDocument doc = QJsonDocument::fromJson(raw);
    QString answer;

    if (doc.isObject()) {
        const QJsonObject obj = doc.object();
        if (obj.contains(QStringLiteral("error"))) {
            answer = obj.value(QStringLiteral("error")).toObject().value(QStringLiteral("message")).toString();
            if (answer.isEmpty())
                answer = QStringLiteral("Unknown provider error");
            answer = QStringLiteral("Error: ") + answer;
        } else {
            answer = chat::providers::openrouter::parseResponse(obj);
        }
    } else {
        answer = QString::fromUtf8(raw);
    }

    if (m_loadingTimer)
        m_loadingTimer->stop();

    if (answer.trimmed().isEmpty())
        answer = QStringLiteral("(no response)");

    const QString epName = m_endpoints.isEmpty() ? QStringLiteral("openrouter") : m_endpoints[m_activeEndpointIndex].name;
    addHistory(QStringLiteral("model"), answer);
    appendLog(epName, false, answer);
    setOutputText(buildConversationHtml());
    setControlsEnabled(true);

    m_reply->deleteLater();
    m_reply = nullptr;
}

void Chat::onRequestError(QNetworkReply::NetworkError code) {
    Q_UNUSED(code)
    if (m_loadingTimer)
        m_loadingTimer->stop();

    const QString msg = m_reply ? m_reply->errorString() : QStringLiteral("Unknown network error");
    setOutputText(buildConversationHtml(QStringLiteral("Error: ") + msg));
    setControlsEnabled(true);

    if (m_reply) {
        m_reply->deleteLater();
        m_reply = nullptr;
    }
}

void Chat::addHistory(const QString &role, const QString &text, const QString &imageBase64) {
    if (!m_historyEnabled)
        return;
    m_history.push_back({role, text, imageBase64});
    trimHistory();
}

void Chat::trimHistory() {
    const int limit = qMax(0, m_maxHistory) * 2;
    if (limit == 0) {
        m_history.clear();
        return;
    }
    while (m_history.size() > limit)
        m_history.removeFirst();
}

void Chat::appendLog(const QString &provider, bool isUser, const QString &text) const {
    const QString logsDir = m_pluginRootPath + QStringLiteral("/res/logs");
    QDir().mkpath(logsDir);

    QFile f(logsDir + QStringLiteral("/") + provider + QStringLiteral(".log"));
    if (!f.open(QIODevice::Append | QIODevice::Text))
        return;

    const QString ts = QDateTime::currentDateTime().toString(Qt::ISODate);
    const QString line =
        isUser ? QString(QStringLiteral("%1\nQ: %2\n")).arg(ts, text) : QString(QStringLiteral("A: %1\n\n")).arg(text);
    f.write(line.toUtf8());
    f.close();
}

void Chat::setControlsEnabled(bool enabled) {
    if (m_promptEdit)
        m_promptEdit->setEnabled(enabled);
    if (m_providerButton)
        m_providerButton->setEnabled(enabled);
    if (m_clearButton)
        m_clearButton->setEnabled(enabled);
    if (m_includeImageButton)
        m_includeImageButton->setEnabled(enabled);
    if (m_snipButton)
        m_snipButton->setEnabled(enabled);
}

void Chat::updatePromptPlaceholder() {
    if (!m_promptEdit)
        return;

    if (m_endpoints.isEmpty()) {
        m_promptEdit->setPlaceholderText(QStringLiteral("Prompt..."));
        return;
    }

    const auto &ep = m_endpoints[m_activeEndpointIndex];
    const QString model = ep.model.isEmpty() ? QStringLiteral("?") : ep.model;
    m_promptEdit->setPlaceholderText(
        QStringLiteral("Prompt for %1 (%2)...").arg(ep.name, model)
    );
}
