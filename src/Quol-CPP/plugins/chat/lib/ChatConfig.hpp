#pragma once

#include "plugins/chat/lib/providers/ProviderTypes.hpp"

#include <QJsonObject>
#include <QMap>
#include <QVector>

namespace chat::config {

struct ParsedConfig {
    QVector<chat::providers::ProviderConfig> endpoints;
    int endpointIndex = 0;
    bool includeImage = true;
    bool historyEnabled = true;
    int maxHistory = 10;
    QString snipPrompt = QStringLiteral("What is this image?");

    bool ollamaEnabled = false;
    QString ollamaModel;
    bool hideOutputOnToggle = false;

    QMap<QString, QString> commands;
};

ParsedConfig parse(const QJsonObject &pluginConfig);

}  // namespace chat::config
