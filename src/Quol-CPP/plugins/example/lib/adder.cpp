#include "plugins/example/lib/adder.hpp"

#include <QJsonArray>

namespace examplelib {
QString selectedArrayOption(const QJsonValue &value) {
    if (!value.isArray()) {
        return {};
    }

    const QJsonArray arr = value.toArray();
    if (arr.size() != 2 || !arr.at(0).isArray()) {
        return {};
    }

    const QJsonArray options = arr.at(0).toArray();
    const int index = arr.at(1).toInt(-1);
    if (index < 0 || index >= options.size()) {
        return {};
    }

    return options.at(index).toString();
}

QString calculateFromConfig(const QJsonObject &config) {
    const int a = config.value(QStringLiteral("a")).toInt(0);
    const int b = config.value(QStringLiteral("b")).toInt(0);
    const QString op = selectedArrayOption(config.value(QStringLiteral("op")));

    if (op == QStringLiteral("+")) {
        return QString(QStringLiteral("%1 + %2 = %3")).arg(a).arg(b).arg(a + b);
    }
    if (op == QStringLiteral("-")) {
        return QString(QStringLiteral("%1 - %2 = %3")).arg(a).arg(b).arg(a - b);
    }
    if (op == QStringLiteral("*")) {
        return QString(QStringLiteral("%1 * %2 = %3")).arg(a).arg(b).arg(a * b);
    }
    if (op == QStringLiteral("/")) {
        if (b == 0) {
            return QString(QStringLiteral("%1 / %2 = undefined")).arg(a).arg(b);
        }
        return QString(QStringLiteral("%1 / %2 = %3"))
            .arg(a)
            .arg(b)
            .arg(static_cast<double>(a) / static_cast<double>(b));
    }

    return QString(QStringLiteral("a=%1, b=%2, op=(invalid)")).arg(a).arg(b);
}

QString nestedNoteLine(const QJsonObject &config) {
    const QJsonObject nested = config.value(QStringLiteral("nested")).toObject();
    const QString note = nested.value(QStringLiteral("note")).toString();

    return QString(QStringLiteral("nested.note=%1")).arg(note);
}

QString nestedEnabledLine(const QJsonObject &config) {
    const QJsonObject nested = config.value(QStringLiteral("nested")).toObject();
    const bool enabled = nested.value(QStringLiteral("enabled")).toBool(false);

    return QString(QStringLiteral("nested.enabled=%1")).arg(enabled ? QStringLiteral("true") : QStringLiteral("false"));
}

QString nestedModeLine(const QJsonObject &config) {
    const QJsonObject nested = config.value(QStringLiteral("nested")).toObject();
    const QString mode = selectedArrayOption(nested.value(QStringLiteral("mode")));

    return QString(QStringLiteral("nested.mode=%1")).arg(mode);
}

QString nestedInnerLabelLine(const QJsonObject &config) {
    const QJsonObject nested = config.value(QStringLiteral("nested")).toObject();
    const QJsonObject inner = nested.value(QStringLiteral("inner")).toObject();
    const QString label = inner.value(QStringLiteral("label")).toString();

    return QString(QStringLiteral("nested.inner.label=%1")).arg(label);
}

QString nestedInnerChoiceLine(const QJsonObject &config) {
    const QJsonObject nested = config.value(QStringLiteral("nested")).toObject();
    const QJsonObject inner = nested.value(QStringLiteral("inner")).toObject();
    const QString choice = selectedArrayOption(inner.value(QStringLiteral("choice")));

    return QString(QStringLiteral("nested.inner.choice=%1")).arg(choice);
}
}  // namespace examplelib
