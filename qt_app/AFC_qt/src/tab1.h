#ifndef TAB1_H
#define TAB1_H

#include <QWidget>
#include <QTimer>
#include <vector>
#include "rosnode.h"
#include "navi.h"
#include "simulation.h"

namespace Ui { class Tab1; }

enum class GridState { IDLE, MOVING, TURNING };

class Tab1 : public QWidget {
    Q_OBJECT
public:
    explicit Tab1(QWidget *parent = nullptr);
    ~Tab1();

    void setRosNode(RosNode* node);
    void on_robotPoseReceived(const QString& robotID, int gridX, int gridY);
    void on_lidarDataReceived(const std::vector<float>& ranges, float angle_min, float angle_increment);
    void updateBatteryStatus(const QString& robotID, int percentage, double voltage);

private slots:
    void sendVelocityCommand();
    void on_pPBForward_clicked();
    void on_pPBBackward_clicked();
    void on_pPBLeft_clicked();
    void on_pPBRight_clicked();
    void on_pPBStop_clicked();
    void on_pPBServo0_clicked();
    void on_pPBServo30_clicked();

    // 자율주행 모드 제어
    void on_btnStart_clicked();
    void on_btnStopNavi_clicked();
    void on_cbDest_currentTextChanged(const QString &text);

private:
    void startMove(bool reverse);
    void startTurn(int dir);
    void stopRobot();
    void nextStep(Robot& r);
    void startAutoNav(Point goal);
    Point getGoalFromDest(const QString& dest);

    Ui::Tab1  *ui;
    RosNode   *pRosNode{nullptr};
    QTimer    *cmdTimer;

    double currentLinear{0.0};
    double currentAngular{0.0};

    GridState gridState_{GridState::IDLE};
    int       moveTick_{0};
    int       turnTick_{0};
    int       turnDir_{1};
    bool      reverse_{false};

    Direction currentDir_{Direction::WEST};

    // 자율주행
    Robot robot_a_;
    Robot robot_b_;
    int   selectedRobot_{0};
    bool  isAutoRunning_{false};

    static const int MOVE_TICKS       = 62;
    static const int TURN_TICKS_LEFT  = 53;
    static const int TURN_TICKS_RIGHT = 52;
};

#endif