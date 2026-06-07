#pragma once

#include <QJsonObject>
#include <QJsonValue>
#include <QString>

namespace examplelib {
QString selectedArrayOption(const QJsonValue &value);
QString calculateFromConfig(const QJsonObject &config);
QString nestedNoteLine(const QJsonObject &config);
QString nestedEnabledLine(const QJsonObject &config);
QString nestedModeLine(const QJsonObject &config);
QString nestedInnerLabelLine(const QJsonObject &config);
QString nestedInnerChoiceLine(const QJsonObject &config);
}  // namespace examplelib
