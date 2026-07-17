#include "plugins/chat/lib/ChatConfig.hpp"

#include <QJsonArray>

namespace chat::config {

ParsedConfig parse(const QJsonObject &pluginConfig) {
    ParsedConfig out;

    const QJsonObject hidden = pluginConfig.value(QStringLiteral("_")).toObject();
    out.includeImage = hidden.value(QStringLiteral("include_image")).toBool(true);

    const QString activeEndpointName =
        hidden.value(QStringLiteral("active_endpoint")).toString();

    const QJsonObject cfg = pluginConfig.value(QStringLiteral("config")).toObject();
    out.historyEnabled = cfg.value(QStringLiteral("history")).toBool(true);
    out.maxHistory = qMax(0, cfg.value(QStringLiteral("max_history")).toInt(10));
    out.snipPrompt = cfg.value(QStringLiteral("snip")).toString(QStringLiteral("What is this image?"));

    const QJsonObject commands = pluginConfig.value(QStringLiteral("commands")).toObject();
    for (auto it = commands.begin(); it != commands.end(); ++it)
        out.commands.insert(it.key(), it.value().toString());

    const QJsonObject endpoints = pluginConfig.value(QStringLiteral("endpoints")).toObject();
    const QJsonArray order = pluginConfig.value(QStringLiteral("endpoint_order")).toArray();

    if (!order.isEmpty()) {
        for (const auto &v : order) {
            const QString name = v.toString().trimmed();
            if (name.isEmpty())
                continue;

            const QJsonObject ep = endpoints.value(name).toObject();
            chat::providers::EndpointConfig ec;
            ec.name = name;
            ec.model = ep.value(QStringLiteral("model")).toString();
            ec.apiKey = ep.value(QStringLiteral("apikey")).toString();
            out.endpoints.append(ec);
        }
    } else {
        const QStringList fallbackKeys = {QStringLiteral("groq"), QStringLiteral("gemini"), QStringLiteral("ollama")};
        for (const auto &name : fallbackKeys) {
            const QJsonObject ep = pluginConfig.value(name).toObject();
            if (ep.isEmpty())
                continue;
            chat::providers::EndpointConfig ec;
            ec.name = name;
            ec.model = ep.value(QStringLiteral("model")).toString();
            ec.apiKey = ep.value(QStringLiteral("apikey")).toString();
            out.endpoints.append(ec);
        }
    }

    out.activeEndpointIndex = 0;
    if (!activeEndpointName.isEmpty()) {
        for (int i = 0; i < out.endpoints.size(); ++i) {
            if (out.endpoints[i].name == activeEndpointName) {
                out.activeEndpointIndex = i;
                break;
            }
        }
    }

    if (out.endpoints.isEmpty()) {
        out.endpoints.append({QStringLiteral("openrouter"), QString(), QString()});
    }

    if (out.activeEndpointIndex < 0 || out.activeEndpointIndex >= out.endpoints.size())
        out.activeEndpointIndex = 0;

    return out;
}

}  // namespace chat::config
