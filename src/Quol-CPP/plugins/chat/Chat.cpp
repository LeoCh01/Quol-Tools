#include "plugins/chat/Chat.hpp"

#include "plugins/chat/lib/ChatConfig.hpp"
#include "plugins/chat/lib/ChatOutput.hpp"
#include "plugins/chat/lib/SnipOverlay.hpp"
#include "plugins/chat/lib/providers/DeepSeekProvider.hpp"
#include "plugins/chat/lib/providers/GeminiProvider.hpp"
#include "plugins/chat/lib/providers/GroqProvider.hpp"
#include "plugins/chat/lib/providers/OllamaProvider.hpp"
#include "plugins/chat/lib/providers/OpenAIProvider.hpp"
#include "ui/QuolPopupWindow.hpp"

#include <QBuffer>
#include <QClipboard>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFrame>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QIcon>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPixmap>
#include <QProcess>
#include <QPushButton>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QScreen>
#include <QScrollArea>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>

namespace {

struct ProviderMeta {
    const char *id;
    const char *displayName;
};

constexpr int kProviderCount = 4;
constexpr ProviderMeta kProviders[kProviderCount] = {
    {"groq", "Groq"},
    {"gemini", "Gemini"},
    {"openai", "OpenAI"},
    {"deepseek", "DeepSeek"},
};

QString providerIdForIndex(int index) {
    return QString::fromLatin1(kProviders[qBound(0, index, kProviderCount - 1)].id);
}

QString revealKeyButtonStyle(bool revealed) {
    return revealed ? QStringLiteral(
                          "QPushButton { background:#4CAF50; border:1px solid #4CAF50; border-radius:4px; }"
                          "QPushButton:hover { background:#43a047; }"
                      )
                    : QStringLiteral(
                          "QPushButton { background:transparent; border:1px solid #444; border-radius:4px; }"
                          "QPushButton:hover { background:#333; border-color:#555; }"
                      );
}

}  // namespace

QWidget *Chat::createWidget(QWidget *parent) {
    m_widget = new QWidget(parent);
    auto *layout = new QHBoxLayout(m_widget);
    layout->setContentsMargins(0, 0, 0, 0);

    m_providerButton = new QPushButton(m_widget);
    m_promptEdit = new QLineEdit(m_widget);
    m_includeImageButton = new QPushButton(m_widget);
    m_snipButton = new QPushButton(m_widget);
    m_extractButton = new QPushButton(m_widget);

    m_includeImageButton->setCheckable(true);
    const QSize iconSize(18, 18);

    m_providerButton->setIconSize(iconSize);
    m_includeImageButton->setIconSize(iconSize);
    m_snipButton->setIconSize(iconSize);
    m_extractButton->setIconSize(iconSize);

    m_providerButton->setToolTip(QStringLiteral("Select provider / model"));
    m_includeImageButton->setToolTip(QStringLiteral("Include screenshot"));
    m_snipButton->setToolTip(QStringLiteral("Snip mode"));
    m_extractButton->setToolTip(QStringLiteral("Extract text from screen"));

    layout->addWidget(m_providerButton);
    layout->addWidget(m_promptEdit, 1);
    layout->addWidget(m_includeImageButton);
    layout->addWidget(m_snipButton);
    layout->addWidget(m_extractButton);

    QObject::connect(m_providerButton, &QPushButton::clicked, this, &Chat::showProviderSelector);
    QObject::connect(m_includeImageButton, &QPushButton::clicked, this, [this]() {
        m_includeImage = m_includeImageButton->isChecked();
        updateIncludeImageUi();
    });
    QObject::connect(m_snipButton, &QPushButton::clicked, this, [this]() { startSnipMode(); });
    QObject::connect(m_extractButton, &QPushButton::clicked, this, [this]() { startExtractMode(); });
    QObject::connect(m_promptEdit, &QLineEdit::returnPressed, this, [this]() { submitPrompt(); });

    m_widget->installEventFilter(this);

    if (!m_network)
        m_network = new QNetworkAccessManager(this);

    m_loadingTimer = new QTimer(this);
    m_loadingTimer->setInterval(100);
    QObject::connect(m_loadingTimer, &QTimer::timeout, this, [this]() {
        if (m_output) {
            const double elapsed = m_requestTimer.elapsed() / 1000.0;
            m_output->setPending(QStringLiteral("Loading... (%1 s)").arg(elapsed, 0, 'f', 1));
        }
    });

    applyButtonIcons();
    updatePromptPlaceholder();
    updateIncludeImageUi();

    return m_widget;
}

void Chat::initialize(const QString &pluginRootPath, const PluginConfig &pluginConfig, QuolServices *services) {
    m_pluginRootPath = pluginRootPath;
    m_cfg = pluginConfig;
    m_services = services;

    migrateLegacyKeys();

    if (!m_pluginRootPath.isEmpty())
        QDir().mkpath(m_pluginRootPath + QStringLiteral("/res/logs"));

    applyConfig();
}

void Chat::onUpdateConfig(const PluginConfig &pluginConfig) {
    m_cfg = pluginConfig;
    applyConfig();
}

void Chat::shutdown() {
    cancelSnipMode();

    if (m_ocrProcess) {
        m_ocrProcess->kill();
        m_ocrProcess->deleteLater();
        m_ocrProcess = nullptr;
    }

    if (!m_pendingOcrPath.isEmpty()) {
        QFile::remove(m_pendingOcrPath);
        m_pendingOcrPath.clear();
    }

    if (m_loadingTimer) {
        m_loadingTimer->stop();
        delete m_loadingTimer;
        m_loadingTimer = nullptr;
    }

    if (m_reply) {
        QNetworkReply *reply = m_reply;
        m_reply = nullptr;
        reply->abort();
        reply->deleteLater();
    }

    if (m_providerPopup) {
        m_providerPopup->close();
        m_providerPopup = nullptr;
        m_providerScrollLayout = nullptr;
    }

    if (m_outputWindow) {
        m_outputWindow->close();
        m_outputWindow = nullptr;
    }
}

void Chat::applyConfig() {
    const auto parsed = chat::config::parse(m_cfg.root());

    m_endpoints = parsed.endpoints;
    m_endpointIndex = parsed.endpointIndex;
    m_includeImage = parsed.includeImage;
    m_historyEnabled = parsed.historyEnabled;
    m_maxHistory = parsed.maxHistory;
    m_snipPrompt = parsed.snipPrompt;
    m_ollamaEnabled = parsed.ollamaEnabled;
    m_ollamaModel = parsed.ollamaModel;
    m_hideOutputOnToggle = parsed.hideOutputOnToggle;

    m_commands = parsed.commands;

    updatePromptPlaceholder();
    updateIncludeImageUi();
}

// legacy per-provider config keys from before endpoints[] existed
void Chat::migrateLegacyKeys() {
    static const QStringList kLegacyKeys = {
        QStringLiteral("_.ollama_enabled"),
        QStringLiteral("_.ollama_model"),
        QStringLiteral("_.active_provider"),
        QStringLiteral("_.providers"),
        QStringLiteral("_.provider_index"),
        QStringLiteral("_.active_endpoint"),
        QStringLiteral("groq"),
        QStringLiteral("gemini"),
        QStringLiteral("providers"),
        QStringLiteral("endpoint_order"),
        QStringLiteral("endpoints"),
        QStringLiteral("openrouter"),
    };

    bool removedAny = false;
    for (const QString &key : kLegacyKeys) {
        if (!m_cfg.get(key).isUndefined()) {
            m_cfg.remove(key);
            removedAny = true;
        }
    }

    if (removedAny)
        m_cfg.save();
}

void Chat::applyButtonIcons() {
    if (m_pluginRootPath.isEmpty())
        return;

    const QVector<QPair<QPushButton *, QString>> icons = {
        {m_providerButton, QStringLiteral("cycle")},
        {m_includeImageButton, QStringLiteral("img")},
        {m_snipButton, QStringLiteral("snip")},
        {m_extractButton, QStringLiteral("extract")},
    };

    for (const auto &[button, name] : icons) {
        if (button)
            button->setIcon(QIcon(m_pluginRootPath + QStringLiteral("/res/img/%1.svg").arg(name)));
    }
}

void Chat::writeProviderConfig() {
    QJsonArray arr;
    for (const auto &ep : m_endpoints) {
        QJsonObject obj;
        obj[QStringLiteral("model")] = ep.model;
        obj[QStringLiteral("apikey")] = ep.apiKey;
        arr.append(obj);
    }
    m_cfg.set(QStringLiteral("_.endpoints"), arr);

    m_cfg.set(QStringLiteral("ollama.enabled"), m_ollamaEnabled);
    m_cfg.set(QStringLiteral("ollama.model"), m_ollamaModel);

    if (m_ollamaEnabled)
        m_cfg.remove(QStringLiteral("_.endpoint_index"));
    else
        m_cfg.set(QStringLiteral("_.endpoint_index"), m_endpointIndex);

    m_cfg.save();
}

void Chat::showProviderSelector() {
    if (m_providerPopup) {
        m_providerPopup->raise();
        m_providerPopup->activateWindow();
        return;
    }

    m_providerPopup = new QuolPopupWindow(QStringLiteral("Select Provider / Model"), m_widget);

    auto *outerWidget = new QWidget();
    auto *outerLayout = new QVBoxLayout(outerWidget);
    outerLayout->setContentsMargins(0, 0, 0, 0);

    auto *scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    auto *scrollContent = new QWidget();
    m_providerScrollLayout = new QVBoxLayout(scrollContent);
    m_providerScrollLayout->setContentsMargins(8, 8, 8, 8);
    m_providerScrollLayout->setSpacing(10);

    scrollArea->setWidget(scrollContent);
    outerLayout->addWidget(scrollArea);

    m_providerPopup->addContent(outerWidget);
    m_providerPopup->resize(420, 520);

    QObject::connect(m_providerPopup, &QObject::destroyed, this, [this]() {
        writeProviderConfig();
        m_providerPopup = nullptr;
        m_providerScrollLayout = nullptr;
    });

    rebuildProviderRows();
    m_providerPopup->show();
}

void Chat::rebuildProviderRows() {
    if (!m_providerScrollLayout)
        return;

    while (auto *item = m_providerScrollLayout->takeAt(0)) {
        if (auto *w = item->widget())
            w->deleteLater();
        delete item;
    }

    for (int i = 0; i < kProviderCount && i < m_endpoints.size(); ++i) {
        const bool isActive = !m_ollamaEnabled && i == m_endpointIndex;

        auto *row = new QFrame();
        row->setFrameShape(QFrame::StyledPanel);
        if (isActive) {
            row->setObjectName(QStringLiteral("active-row"));
            row->setStyleSheet(QStringLiteral(
                "QFrame#active-row { background:#2d2d2d; border:1px solid #4CAF50; border-radius:4px; padding:8px; }"
            ));
        } else {
            row->setStyleSheet(
                QStringLiteral("QFrame { background:#2d2d2d; border:1px solid #444; border-radius:4px; padding:8px; }")
            );
        }

        auto *rowLayout = new QVBoxLayout(row);
        rowLayout->setContentsMargins(10, 8, 10, 8);
        rowLayout->setSpacing(6);

        auto *headerLayout = new QHBoxLayout();

        auto *nameLabel = new QLabel(QString::fromLatin1(kProviders[i].displayName));
        nameLabel->setStyleSheet(
            QStringLiteral("font-weight:bold; color:#fff; font-size:13px; background:transparent;")
        );
        headerLayout->addWidget(nameLabel);

        headerLayout->addStretch();

        auto *activateBtn = new QPushButton(isActive ? QStringLiteral("ACTIVE") : QStringLiteral("Activate"));
        activateBtn->setObjectName(QStringLiteral("btn-toggle"));
        activateBtn->setCheckable(true);
        activateBtn->setChecked(isActive);
        activateBtn->setFixedHeight(26);
        headerLayout->addWidget(activateBtn);

        rowLayout->addLayout(headerLayout);

        auto *modelEdit = new QLineEdit(m_endpoints[i].model);
        modelEdit->setPlaceholderText(QStringLiteral("Model (e.g. meta-llama/llama-4-scout-17b-16e-instruct)"));
        rowLayout->addWidget(modelEdit);

        auto *keyEdit = new QLineEdit(m_endpoints[i].apiKey);
        keyEdit->setPlaceholderText(QStringLiteral("API key"));
        keyEdit->setEchoMode(QLineEdit::Password);

        auto *revealBtn = new QPushButton();
        revealBtn->setCheckable(true);
        revealBtn->setFocusPolicy(Qt::NoFocus);
        revealBtn->setCursor(Qt::PointingHandCursor);
        revealBtn->setFixedSize(28, 28);
        revealBtn->setIcon(QIcon(m_pluginRootPath + QStringLiteral("/res/img/eye.svg")));
        revealBtn->setIconSize(QSize(16, 16));
        revealBtn->setToolTip(QStringLiteral("Show API key"));
        revealBtn->setStyleSheet(revealKeyButtonStyle(false));

        QObject::connect(revealBtn, &QPushButton::toggled, this, [keyEdit, revealBtn](bool revealed) {
            keyEdit->setEchoMode(revealed ? QLineEdit::Normal : QLineEdit::Password);
            revealBtn->setStyleSheet(revealKeyButtonStyle(revealed));
        });

        auto *keyLayout = new QHBoxLayout();
        keyLayout->setContentsMargins(0, 0, 0, 0);
        keyLayout->setSpacing(4);
        keyLayout->addWidget(keyEdit, 1);
        keyLayout->addWidget(revealBtn, 0, Qt::AlignVCenter);
        rowLayout->addLayout(keyLayout);

        m_providerScrollLayout->addWidget(row);

        QObject::connect(activateBtn, &QPushButton::clicked, this, [this, i, activateBtn]() {
            if (!activateBtn->isChecked())
                activateBtn->setChecked(true);
            m_ollamaEnabled = false;
            m_endpointIndex = i;
            writeProviderConfig();
            updatePromptPlaceholder();

            if (m_providerPopup)
                rebuildProviderRows();
        });

        QObject::connect(modelEdit, &QLineEdit::textChanged, this, [this, i, modelEdit]() {
            if (i < m_endpoints.size())
                m_endpoints[i].model = modelEdit->text().trimmed();
            writeProviderConfig();
            updatePromptPlaceholder();
        });

        QObject::connect(keyEdit, &QLineEdit::textChanged, this, [this, i, keyEdit]() {
            if (i < m_endpoints.size())
                m_endpoints[i].apiKey = keyEdit->text();
            writeProviderConfig();
        });
    }

    m_providerScrollLayout->addStretch();
}

void Chat::clearMessage() {
    if (m_promptEdit)
        m_promptEdit->clear();

    m_history.clear();

    if (m_output)
        m_output->clear();

    if (m_outputWindow)
        m_outputWindow->close();
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

bool Chat::eventFilter(QObject *obj, QEvent *event) {
    if (obj != m_widget || !m_hideOutputOnToggle)
        return QObject::eventFilter(obj, event);

    if (event->type() == QEvent::Hide) {
        if (m_providerPopup)
            m_providerPopup->hide();
        if (m_outputWindow) {
            m_outputWindow->hide();
            m_outputWindowHiddenByToggle = true;
        }
    } else if (event->type() == QEvent::Show && m_outputWindowHiddenByToggle) {
        m_outputWindow->show();
        m_outputWindowHiddenByToggle = false;
    }

    return QObject::eventFilter(obj, event);
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
        if (useExistingSnipImage)
            imageBase64 = m_pendingSnipImageBase64;
        else
            imageBase64 = pixmapToBase64Png(capturePrimaryScreenPixmap());
    }

    const HistoryItem turn{QStringLiteral("user"), prompt, imageBase64};
    addHistory(turn.role, turn.text, turn.imageBase64);

    // the current turn is already the last history entry; fall back to it alone
    // when history is off or max_history trims it away
    QVector<HistoryItem> messages;
    if (m_historyEnabled)
        messages = m_history;
    if (messages.isEmpty())
        messages = {turn};

    appendLog(m_ollamaEnabled ? QStringLiteral("ollama") : providerIdForIndex(m_endpointIndex), true, prompt);

    setControlsEnabled(false);
    m_requestTimer.start();
    m_loadingTimer->start();
    showOutput();
    m_output->addMessage(QStringLiteral("user"), prompt, !imageBase64.isEmpty());
    m_output->setPending(QStringLiteral("Loading... (0.0 s)"));

    sendJsonRequest(buildRequest(m_endpointIndex, messages));

    m_promptEdit->clear();
}

void Chat::startSnipMode() {
    startOverlay(QStringLiteral("Send"), [this](const QPixmap &cropped) { onSnipSelected(cropped); });
}

void Chat::cancelSnipMode() {
    if (m_snipOverlay)
        m_snipOverlay->close();
}

void Chat::startOverlay(const QString &buttonLabel, std::function<void(const QPixmap &)> onSelected) {
    const QPixmap screenshot = capturePrimaryScreenPixmap();
    if (screenshot.isNull())
        return;

    cancelSnipMode();

    m_snipOverlay = new SnipOverlay(screenshot, std::move(onSelected), buttonLabel);
    m_snipOverlay->showFullScreen();
    m_snipOverlay->raise();
    m_snipOverlay->activateWindow();
}

void Chat::onSnipSelected(const QPixmap &cropped) {
    if (cropped.isNull())
        return;

    m_pendingSnipImageBase64 = pixmapToBase64Png(cropped);

    if (m_promptEdit && m_promptEdit->text().trimmed().isEmpty())
        m_promptEdit->setText(m_snipPrompt);

    submitPrompt(true);
}

void Chat::startExtractMode() {
    startOverlay(QStringLiteral("Extract"), [this](const QPixmap &cropped) { onExtractSelected(cropped); });
}

void Chat::onExtractSelected(const QPixmap &cropped) {
    if (cropped.isNull())
        return;

    const QString path = QDir::tempPath() + QStringLiteral("/quol_ocr_")
                         + QString::number(QRandomGenerator::global()->generate()) + QStringLiteral(".png");
    if (!cropped.save(path, "PNG"))
        return;

    m_pendingOcrPath = path;
    runOcr(path);
}

void Chat::runOcr(const QString &imagePath) {
    if (m_ocrProcess)
        return;

    m_ocrProcess = new QProcess(this);
    m_ocrProcess->setProgram(QStringLiteral("powershell.exe"));
    m_ocrProcess->setArguments(
        QStringList{
            QStringLiteral("-NoProfile"),
            QStringLiteral("-NoLogo"),
            QStringLiteral("-ExecutionPolicy"),
            QStringLiteral("Bypass"),
            QStringLiteral("-File"),
            m_pluginRootPath + QStringLiteral("/res/ocr/ocr.ps1"),
            QStringLiteral("-ImagePath"),
            imagePath,
        }
    );
    QObject::connect(
        m_ocrProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &Chat::onOcrFinished
    );
    m_ocrProcess->start();
}

void Chat::onOcrFinished() {
    QProcess *process = m_ocrProcess;
    m_ocrProcess = nullptr;

    const bool ok = process && process->exitStatus() == QProcess::NormalExit && process->exitCode() == 0;
    const QString text = ok ? QString::fromUtf8(process->readAllStandardOutput()).trimmed() : QString();

    if (process)
        process->deleteLater();

    if (!m_pendingOcrPath.isEmpty()) {
        QFile::remove(m_pendingOcrPath);
        m_pendingOcrPath.clear();
    }

    if (!text.isEmpty())
        QGuiApplication::clipboard()->setText(text);
}

void Chat::ensureOutputWindow() {
    if (m_outputWindow)
        return;

    m_outputWindow = new QuolPopupWindow(QStringLiteral("Chat Output"), m_widget);
    QObject::connect(m_outputWindow, &QObject::destroyed, this, [this]() {
        m_outputWindow = nullptr;
        m_output = nullptr;
        m_history.clear();
    });
    m_outputWindow->resize(520, 640);

    m_output = new ChatOutput(m_outputWindow);
    m_outputWindow->addContent(m_output);
}

void Chat::showOutput() {
    ensureOutputWindow();
    if (!m_outputWindow)
        return;

    m_outputWindow->show();
    m_outputWindow->raise();
    m_outputWindow->activateWindow();
}

QString Chat::applyCommandTemplate(const QString &rawPrompt) const {
    const QStringList tokens = rawPrompt.split(' ', Qt::SkipEmptyParts);
    if (tokens.isEmpty())
        return rawPrompt;

    const QString cmd = tokens.first();
    if (!m_commands.contains(cmd))
        return rawPrompt;

    // {0} {1} -> positional args; {1:english} -> arg with fallback; bare {2} with no arg -> empty
    static const QRegularExpression placeholder(R"(\{(\d+)(?::([^}]*))?\})");

    const QString tmpl = m_commands.value(cmd);
    QString result;
    int cursor = 0;

    for (auto it = placeholder.globalMatch(tmpl); it.hasNext();) {
        const QRegularExpressionMatch match = it.next();
        const int index = match.captured(1).toInt() - 1;
        result += tmpl.mid(cursor, match.capturedStart() - cursor);
        result += index >= 0 && index < tokens.size() ? tokens.at(index) : match.captured(2);
        cursor = match.capturedEnd();
    }
    result += tmpl.mid(cursor);

    return result.trimmed();
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

chat::providers::ProviderRequest Chat::buildRequest(int endpointIndex, const QVector<HistoryItem> &messages) {
    if (m_ollamaEnabled) {
        m_pendingParse = chat::providers::ollama::parseResponse;

        chat::providers::ProviderConfig cfg;
        cfg.model = m_ollamaModel;
        return chat::providers::ollama::buildRequest(cfg, messages);
    }

    if (endpointIndex < 0 || endpointIndex >= m_endpoints.size())
        return {};

    const chat::providers::ProviderConfig &config = m_endpoints.at(endpointIndex);

    switch (endpointIndex) {
        case 1:
            m_pendingParse = chat::providers::gemini::parseResponse;
            return chat::providers::gemini::buildRequest(config, messages);
        case 2:
            m_pendingParse = chat::providers::openai::parseResponse;
            return chat::providers::openai::buildRequest(config, messages);
        case 3:
            m_pendingParse = chat::providers::deepseek::parseResponse;
            return chat::providers::deepseek::buildRequest(config, messages);
        default:
            m_pendingParse = chat::providers::groq::parseResponse;
            return chat::providers::groq::buildRequest(config, messages);
    }
}

void Chat::sendJsonRequest(const chat::providers::ProviderRequest &request) {
    if (!m_network || request.url.isEmpty())
        return;

    if (m_reply) {
        m_reply->deleteLater();
        m_reply = nullptr;
    }

    QNetworkRequest req(request.url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    if (!request.bearerToken.trimmed().isEmpty())
        req.setRawHeader("Authorization", QByteArray("Bearer ") + request.bearerToken.toUtf8());

    m_pendingProvider = request.providerName;
    m_reply = m_network->post(req, QJsonDocument(request.payload).toJson(QJsonDocument::Compact));
    QObject::connect(m_reply, &QNetworkReply::finished, this, &Chat::onRequestFinished);
}

void Chat::onRequestFinished() {
    QNetworkReply *reply = m_reply;
    if (!reply)
        return;

    m_reply = nullptr;

    if (m_loadingTimer)
        m_loadingTimer->stop();

    const QByteArray raw = reply->readAll();
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        const QString message = QStringLiteral("Error: ") + reply->errorString();
        if (m_output)
            m_output->setPending(message, true);
        appendLog(m_pendingProvider, false, message);
        setControlsEnabled(true);
        return;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(raw);
    QString answer;

    if (doc.isObject()) {
        const QJsonObject obj = doc.object();
        const QJsonValue error = obj.value(QStringLiteral("error"));

        if (!error.isUndefined()) {
            QString message = error.toObject().value(QStringLiteral("message")).toString();
            if (message.isEmpty())
                message = error.toString();
            answer = message.isEmpty() ? QStringLiteral("Unknown provider error") : QStringLiteral("Error: ") + message;
        } else if (m_pendingParse) {
            answer = m_pendingParse(obj);
        }
    } else {
        answer = QString::fromUtf8(raw);
    }

    if (answer.trimmed().isEmpty())
        answer = QStringLiteral("(no response)");

    addHistory(QStringLiteral("model"), answer);
    appendLog(m_pendingProvider, false, answer);
    if (m_output) {
        m_output->setPending(QString());
        m_output->addMessage(QStringLiteral("model"), answer);
    }
    setControlsEnabled(true);
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
    QFile f(m_pluginRootPath + QStringLiteral("/res/logs/") + provider + QStringLiteral(".log"));
    if (!f.open(QIODevice::Append | QIODevice::Text))
        return;

    const QString ts = QDateTime::currentDateTime().toString(Qt::ISODate);
    if (isUser)
        f.write(QStringLiteral("%1\nQ: %2\n").arg(ts, text).toUtf8());
    else
        f.write(QStringLiteral("%1\nA: %2\n\n").arg(ts, text).toUtf8());
    f.close();
}

void Chat::setControlsEnabled(bool enabled) {
    if (m_widget)
        m_widget->setEnabled(enabled);
}

void Chat::updatePromptPlaceholder() {
    if (!m_promptEdit)
        return;

    const QString label = m_ollamaEnabled ? QStringLiteral("ollama") : providerIdForIndex(m_endpointIndex);

    const QString model = m_ollamaEnabled
                              ? m_ollamaModel
                              : (m_endpointIndex < m_endpoints.size() ? m_endpoints[m_endpointIndex].model : QString());

    const QString modelLabel = model.isEmpty() ? QStringLiteral("?") : model;
    m_promptEdit->setPlaceholderText(QStringLiteral("Prompt for %1 (%2)...").arg(label, modelLabel));
}
