#include "plugins/chat/lib/providers/GeminiProvider.hpp"

#include <QJsonArray>

namespace chat::providers::gemini {

ProviderRequest buildRequest(
    const ProviderConfig &config,
    const QVector<HistoryItem> &history,
    const QString &prompt,
    const QString &imageBase64,
    bool includeHistory
) {
    ProviderRequest req;
    req.providerName = QStringLiteral("gemini");
    req.url = QUrl(
        QString(QStringLiteral("https://generativelanguage.googleapis.com/v1beta/models/%1:generateContent?key=%2"))
            .arg(config.model, config.apiKey)
    );

    QJsonArray contents;

    if (includeHistory) {
        for (const auto &item : history) {
            QJsonObject c;
            c.insert(
                QStringLiteral("role"),
                item.role == QStringLiteral("model") ? QStringLiteral("model") : QStringLiteral("user")
            );
            QJsonArray parts;
            parts.append(QJsonObject{{QStringLiteral("text"), item.text}});
            if (!item.imageBase64.isEmpty()) {
                parts.append(
                    QJsonObject{
                        {QStringLiteral("inline_data"),
                         QJsonObject{
                             {QStringLiteral("mime_type"), QStringLiteral("image/png")},
                             {QStringLiteral("data"), item.imageBase64}
                         }}
                    }
                );
            }
            c.insert(QStringLiteral("parts"), parts);
            contents.append(c);
        }
    }

    QJsonObject cur;
    cur.insert(QStringLiteral("role"), QStringLiteral("user"));
    QJsonArray parts;
    parts.append(QJsonObject{{QStringLiteral("text"), prompt}});
    if (!imageBase64.isEmpty()) {
        parts.append(
            QJsonObject{
                {QStringLiteral("inline_data"),
                 QJsonObject{
                     {QStringLiteral("mime_type"), QStringLiteral("image/png")}, {QStringLiteral("data"), imageBase64}
                 }}
            }
        );
    }
    cur.insert(QStringLiteral("parts"), parts);
    contents.append(cur);

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
