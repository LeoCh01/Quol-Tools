#include "plugins/chat/lib/ChatConfig.hpp"

#include <QJsonArray>

namespace chat::config {

ParsedConfig parse(const QJsonObject &pluginConfig) {
    ParsedConfig out;

    const QJsonObject hidden = pluginConfig.value(QStringLiteral("_")).toObject();

    out.endpointIndex = qMax(0, hidden.value(QStringLiteral("endpoint_index")).toInt(0));
    out.includeImage = hidden.value(QStringLiteral("include_image")).toBool(true);

    const QJsonObject ollamaObj = pluginConfig.value(QStringLiteral("ollama")).toObject();
    out.ollamaEnabled = ollamaObj.value(QStringLiteral("enabled")).toBool(false);
    out.ollamaModel = ollamaObj.value(QStringLiteral("model")).toString();

    const QJsonArray endpointsArr = hidden.value(QStringLiteral("endpoints")).toArray();
    for (const auto &v : endpointsArr) {
        const QJsonObject ep = v.toObject();
        chat::providers::ProviderConfig cfg;
        cfg.model = ep.value(QStringLiteral("model")).toString();
        cfg.apiKey = ep.value(QStringLiteral("apikey")).toString();
        out.endpoints.append(cfg);
    }

    if (out.endpoints.isEmpty())
        out.endpoints.resize(4, {QString(), QString()});

    if (out.endpointIndex >= out.endpoints.size())
        out.endpointIndex = 0;

    const QJsonObject cfg = pluginConfig.value(QStringLiteral("config")).toObject();
    out.historyEnabled = cfg.value(QStringLiteral("history")).toBool(true);
    out.maxHistory = qMax(0, cfg.value(QStringLiteral("max_history")).toInt(10));
    out.snipPrompt = cfg.value(QStringLiteral("snip")).toString(QStringLiteral("What is this image?"));
    out.hideOutputOnToggle = cfg.value(QStringLiteral("hide_output_on_toggle")).toBool(false);

    const QJsonObject commands = pluginConfig.value(QStringLiteral("commands")).toObject();
    for (auto it = commands.begin(); it != commands.end(); ++it)
        out.commands.insert(it.key(), it.value().toString());

    return out;
}

}  // namespace chat::config
