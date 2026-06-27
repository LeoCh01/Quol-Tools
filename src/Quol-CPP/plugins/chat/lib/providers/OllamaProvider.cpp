#include "plugins/chat/lib/providers/OllamaProvider.hpp"

#include <QJsonArray>

namespace chat::providers::ollama {

ProviderRequest buildRequest(
    const ProviderConfig &config,
    const QVector<HistoryItem> &history,
    const QString &prompt,
    const QString &imageBase64,
    bool includeHistory
) {
    Q_UNUSED(history)
    Q_UNUSED(includeHistory)

    ProviderRequest req;
    req.providerName = QStringLiteral("ollama");
    req.url = QUrl(QStringLiteral("http://localhost:11434/api/chat"));

    req.payload.insert(QStringLiteral("model"), config.model);
    req.payload.insert(QStringLiteral("stream"), false);

    QJsonObject user;
    user.insert(QStringLiteral("role"), QStringLiteral("user"));
    user.insert(QStringLiteral("content"), prompt);
    if (!imageBase64.isEmpty()) {
        QJsonArray images;
        images.append(imageBase64);
        user.insert(QStringLiteral("images"), images);
    }

    QJsonArray messages;
    messages.append(user);
    req.payload.insert(QStringLiteral("messages"), messages);
    return req;
}

QString parseResponse(const QJsonObject &response) {
    return response.value(QStringLiteral("message")).toObject().value(QStringLiteral("content")).toString();
}

}  // namespace chat::providers::ollama
