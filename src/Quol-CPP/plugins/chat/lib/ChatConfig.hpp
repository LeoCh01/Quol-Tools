#pragma once

#include <QJsonObject>
#include <QMap>
#include <QStringList>

namespace chat::config {

struct ParsedConfig {
    QStringList providers = {"groq", "gemini", "ollama"};
    int providerIndex = 0;
    bool includeImage = true;
    bool historyEnabled = true;
    int maxHistory = 10;
    QString snipPrompt = "What is this image?";

    QMap<QString, QString> models;
    QMap<QString, QString> apiKeys;
    QMap<QString, QString> commands;
};

ParsedConfig parse(const QJsonObject &pluginConfig);

}  // namespace chat::config
