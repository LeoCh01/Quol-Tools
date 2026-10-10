#include "plugins/chat/lib/providers/OllamaProvider.hpp"

#include <QJsonArray>

namespace chat::providers::ollama {

ProviderRequest buildRequest(const ProviderConfig &config, const QVector<HistoryItem> &messages) {
    ProviderRequest req;
    req.providerName = QStringLiteral("ollama");
    req.url = QUrl(QStringLiteral("http://localhost:11434/api/chat"));

    req.payload.insert(QStringLiteral("model"), config.model);
    req.payload.insert(QStringLiteral("stream"), false);

    QJsonArray out;
    for (const auto &item : messages) {
        QJsonObject message;
        message.insert(QStringLiteral("role"), item.role == QStringLiteral("model") ? QStringLiteral("assistant")
                                                                                    : QStringLiteral("user"));
        message.insert(QStringLiteral("content"), item.text);
        if (!item.imageBase64.isEmpty()) {
            QJsonArray images;
            images.append(item.imageBase64);
            message.insert(QStringLiteral("images"), images);
        }
        out.append(message);
    }

    req.payload.insert(QStringLiteral("messages"), out);
    return req;
}

QString parseResponse(const QJsonObject &response) {
    return response.value(QStringLiteral("message")).toObject().value(QStringLiteral("content")).toString();
}

}  // namespace chat::providers::ollama
