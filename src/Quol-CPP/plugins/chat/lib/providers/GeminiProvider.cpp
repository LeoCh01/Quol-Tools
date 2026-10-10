#include "plugins/chat/lib/providers/GeminiProvider.hpp"

#include <QJsonArray>

namespace chat::providers::gemini {

ProviderRequest buildRequest(const ProviderConfig &config, const QVector<HistoryItem> &messages) {
    ProviderRequest req;
    req.providerName = QStringLiteral("gemini");
    req.url = QUrl(
        QString(QStringLiteral("https://generativelanguage.googleapis.com/v1beta/models/%1:generateContent?key=%2"))
            .arg(config.model, config.apiKey)
    );

    QJsonArray contents;
    for (const auto &item : messages) {
        QJsonArray parts;
        parts.append(QJsonObject{{"text", item.text}});
        if (!item.imageBase64.isEmpty()) {
            parts.append(
                QJsonObject{
                    {"inline_data",
                     QJsonObject{
                         {"mime_type", QStringLiteral("image/png")}, {"data", item.imageBase64}
                     }}
                }
            );
        }

        QJsonObject content;
        content.insert(QStringLiteral("role"), item.role == QStringLiteral("model") ? QStringLiteral("model")
                                                                                   : QStringLiteral("user"));
        content.insert(QStringLiteral("parts"), parts);
        contents.append(content);
    }

    req.payload.insert(QStringLiteral("contents"), contents);
    return req;
}

QString parseResponse(const QJsonObject &response) {
    const QJsonArray candidates = response.value(QStringLiteral("candidates")).toArray();
    if (candidates.isEmpty())
        return {};
    const QJsonObject content = candidates.at(0).toObject().value(QStringLiteral("content")).toObject();
    const QJsonArray parts = content.value(QStringLiteral("parts")).toArray();
    if (parts.isEmpty())
        return {};
    return parts.at(0).toObject().value(QStringLiteral("text")).toString();
}

}  // namespace chat::providers::gemini
