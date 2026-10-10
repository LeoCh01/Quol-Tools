#include "plugins/chat/lib/providers/GroqProvider.hpp"

#include <QJsonArray>

namespace chat::providers::groq {

ProviderRequest buildRequest(const ProviderConfig &config, const QVector<HistoryItem> &messages) {
    ProviderRequest req;
    req.providerName = QStringLiteral("groq");
    req.url = QUrl(QStringLiteral("https://api.groq.com/openai/v1/chat/completions"));
    req.bearerToken = config.apiKey;

    req.payload.insert("model", config.model);
    req.payload.insert("stream", false);

    QJsonArray out;
    for (const auto &item : messages) {
        if (item.role == "model") {
            out.append(QJsonObject{{"role", "assistant"}, {"content", item.text}});
            continue;
        }

        QJsonArray content;
        content.append(QJsonObject{{"type", "text"}, {"text", item.text}});
        if (!item.imageBase64.isEmpty()) {
            content.append(
                QJsonObject{
                    {"type", "image_url"},
                    {"image_url", QJsonObject{{"url", QString("data:image/png;base64,%1").arg(item.imageBase64)}}}
                }
            );
        }
        out.append(QJsonObject{{"role", "user"}, {"content", content}});
    }

    req.payload.insert("messages", out);
    return req;
}

QString parseResponse(const QJsonObject &response) {
    const QJsonArray choices = response.value("choices").toArray();
    if (choices.isEmpty())
        return {};
    return choices.at(0).toObject().value("message").toObject().value("content").toString();
}

}  // namespace chat::providers::groq
