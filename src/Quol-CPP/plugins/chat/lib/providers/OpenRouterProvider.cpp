#include "plugins/chat/lib/providers/OpenRouterProvider.hpp"

#include <QJsonArray>

namespace chat::providers::openrouter {

ProviderRequest buildRequest(
    const ProviderConfig &config,
    const QVector<HistoryItem> &history,
    const QString &prompt,
    const QString &imageBase64,
    bool includeHistory
) {
    ProviderRequest req;
    req.providerName = QStringLiteral("openrouter");
    req.url = QUrl(QStringLiteral("https://openrouter.ai/api/v1/chat/completions"));
    req.bearerToken = config.apiKey;

    req.payload.insert(QStringLiteral("model"), config.model);
    req.payload.insert(QStringLiteral("stream"), false);

    QJsonArray messages;

    if (includeHistory) {
        for (const auto &item : history) {
            if (item.role == QStringLiteral("model")) {
                messages.append(QJsonObject{{QStringLiteral("role"), QStringLiteral("assistant")}, {QStringLiteral("content"), item.text}});
            } else {
                QJsonArray content;
                content.append(QJsonObject{{QStringLiteral("type"), QStringLiteral("text")}, {QStringLiteral("text"), item.text}});
                if (!item.imageBase64.isEmpty()) {
                    content.append(
                        QJsonObject{
                            {QStringLiteral("type"), QStringLiteral("image_url")},
                            {QStringLiteral("image_url"),
                             QJsonObject{{QStringLiteral("url"), QString("data:image/png;base64,%1").arg(item.imageBase64)}}}
                        }
                    );
                }
                messages.append(QJsonObject{{QStringLiteral("role"), QStringLiteral("user")}, {QStringLiteral("content"), content}});
            }
        }
    }

    QJsonArray cur;
    cur.append(QJsonObject{{QStringLiteral("type"), QStringLiteral("text")}, {QStringLiteral("text"), prompt}});
    if (!imageBase64.isEmpty()) {
        cur.append(
            QJsonObject{
                {QStringLiteral("type"), QStringLiteral("image_url")},
                {QStringLiteral("image_url"), QJsonObject{{QStringLiteral("url"), QString("data:image/png;base64,%1").arg(imageBase64)}}}
            }
        );
    }
    messages.append(QJsonObject{{QStringLiteral("role"), QStringLiteral("user")}, {QStringLiteral("content"), cur}});

    req.payload.insert(QStringLiteral("messages"), messages);
    return req;
}

QString parseResponse(const QJsonObject &response) {
    const QJsonArray choices = response.value(QStringLiteral("choices")).toArray();
    if (choices.isEmpty())
        return {};
    return choices.at(0).toObject().value(QStringLiteral("message")).toObject().value(QStringLiteral("content")).toString();
}

}  // namespace chat::providers::openrouter
