#pragma once

#include <QString>

class QWidget;
class QuolServices;

class ToolBase {
public:
    virtual ~ToolBase() = default;

    virtual QString label() const = 0;
    virtual void start(QuolServices *services) = 0;
    virtual void stop() = 0;
    virtual QWidget *widget() = 0;
};
