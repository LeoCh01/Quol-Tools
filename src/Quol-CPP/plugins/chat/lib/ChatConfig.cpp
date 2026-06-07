#include "plugins/chat/lib/ChatConfig.hpp"

#include <QJsonArray>

namespace chat::config {

ParsedConfig parse(const QJsonObject &pluginConfig) {
    ParsedConfig out;

    const QJsonObject hidden = pluginConfig.value(QStringLiteral("_")).toObject();
    if (hidden.contains(QStringLiteral("providers")) && hidden.value(QStringLiteral("providers")).isArray()) {
        QStringList providers;
        const QJsonArray arr = hidden.value(QStringLiteral("providers")).toArray();
        for (const auto &v : arr) {
            const QString p = v.toString().trimmed().toLower();
            if (!p.isEmpty())
                providers.append(p);
        }
        if (!providers.isEmpty())
            out.providers = providers;
    }

    out.providerIndex = hidden.value(QStringLiteral("provider_index")).toInt(0);
    if (out.providerIndex < 0 || out.providerIndex >= out.providers.size())
        out.providerIndex = 0;

    out.includeImage = hidden.value(QStringLiteral("include_image")).toBool(true);

    const QJsonObject cfg = pluginConfig.value(QStringLiteral("config")).toObject();
    out.historyEnabled = cfg.value(QStringLiteral("history")).toBool(true);
    out.maxHistory = qMax(0, cfg.value(QStringLiteral("max_history")).toInt(10));
    out.snipPrompt = cfg.value(QStringLiteral("snip")).toString(QStringLiteral("What is this image?"));

    const QJsonObject commands = pluginConfig.value(QStringLiteral("commands")).toObject();
    for (auto it = commands.begin(); it != commands.end(); ++it)
        out.commands.insert(it.key(), it.value().toString());

    out.models[QStringLiteral("ollama")] =
        pluginConfig.value(QStringLiteral("ollama")).toObject().value(QStringLiteral("model")).toString();
    out.models[QStringLiteral("gemini")] =
        pluginConfig.value(QStringLiteral("gemini")).toObject().value(QStringLiteral("model")).toString();
    out.models[QStringLiteral("groq")] =
        pluginConfig.value(QStringLiteral("groq")).toObject().value(QStringLiteral("model")).toString();

    out.apiKeys[QStringLiteral("gemini")] =
        pluginConfig.value(QStringLiteral("gemini")).toObject().value(QStringLiteral("apikey")).toString();
    out.apiKeys[QStringLiteral("groq")] =
        pluginConfig.value(QStringLiteral("groq")).toObject().value(QStringLiteral("apikey")).toString();

    return out;
}

}  // namespace chat::config
