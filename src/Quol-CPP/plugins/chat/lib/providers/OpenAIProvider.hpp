#pragma once

#include "plugins/chat/lib/providers/ProviderTypes.hpp"

namespace chat::providers::openai {

ProviderRequest buildRequest(const ProviderConfig &config, const QVector<HistoryItem> &messages);

QString parseResponse(const QJsonObject &response);

}  // namespace chat::providers::openai
