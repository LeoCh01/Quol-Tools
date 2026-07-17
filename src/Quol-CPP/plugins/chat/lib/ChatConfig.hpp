#pragma once

#include "plugins/chat/lib/providers/ProviderTypes.hpp"

#include <QJsonObject>
#include <QMap>
#include <QStringList>
#include <QVector>

namespace chat::config {

struct ParsedConfig {
    bool includeImage = true;
    bool historyEnabled = true;
    int maxHistory = 10;
    QString snipPrompt = "What is this image?";

    QMap<QString, QString> commands;

    QVector<chat::providers::EndpointConfig> endpoints;
    int activeEndpointIndex = 0;
};

ParsedConfig parse(const QJsonObject &pluginConfig);

}  // namespace chat::config
