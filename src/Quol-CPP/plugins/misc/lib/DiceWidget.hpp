#pragma once

#include "plugins/misc/lib/ToolBase.hpp"

#include <QLabel>
#include <QPoint>
#include <QTimer>
#include <QWidget>

class DiceWidget final : public QWidget, public ToolBase {
    Q_OBJECT

public:
    explicit DiceWidget(const QString &pluginRootPath, QWidget *parent = nullptr);

    QString label() const override { return QStringLiteral("Dice Roll"); }
    void start(QuolServices *services) override;
    void stop() override;
    QWidget *widget() override;

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    void updatePhysics();
    void setRandomFace();
    QString diceImagePath(int face) const;

    QString m_rootPath;

    QLabel *m_diceLabel = nullptr;
    QTimer *m_physicsTimer = nullptr;

    // Drag state
    QPoint m_dragOffset;
    QPoint m_lastMousePos;
    double m_lastVelX = 0.0;
    double m_lastVelY = 0.0;
    bool m_dragging = false;

    // Physics
    double m_vx = 0.0;
    double m_vy = 0.0;
    double m_px = 0.0;
    double m_py = 0.0;
    double m_distSinceLastFace = 0.0;

    int m_currentFace = 0; // 0 = dice-x, 1-6 = dice faces
};
