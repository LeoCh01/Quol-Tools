#pragma once

#include "plugins/chat/lib/providers/ProviderTypes.hpp"

namespace chat::providers::groq {

ProviderRequest buildRequest(
    const ProviderConfig &config,
    const QVector<HistoryItem> &history,
    const QString &prompt,
    const QString &imageBase64,
    bool includeHistory
);

QString parseResponse(const QJsonObject &response);

}  // namespace chat::providers::groq
