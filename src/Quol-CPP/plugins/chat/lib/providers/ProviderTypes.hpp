#pragma once

#include <QJsonObject>
#include <QString>
#include <QUrl>
#include <QVector>

namespace chat::providers {

struct HistoryItem {
    QString role;
    QString text;
    QString imageBase64;
};

struct ProviderConfig {
    QString model;
    QString apiKey;
};

struct ProviderRequest {
    QString providerName;
    QUrl url;
    QJsonObject payload;
    QString bearerToken;
};

}  // namespace chat::providers
