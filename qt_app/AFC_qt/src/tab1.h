#ifndef TAB1_H
#define TAB1_H

#include <QWidget>
#include "rosnode.h"
#include "lidarwidget.h"

namespace Ui {
class Tab1;
}

class Tab1 : public QWidget
{
    Q_OBJECT

public:
    explicit Tab1(QWidget *parent = nullptr);
    ~Tab1();

    void setRosNode(RosNode* node);
    void on_robotPoseReceived(const QString& robotID, int gridX, int gridY);
    void on_lidarDataReceived(const std::vector<float>& ranges, float angle_min, float angle_increment);
    void on_lidarScanReceived(const QString& robotID,
                                const std::vector<float>& ranges,
                                float angle_min,
                                float angle_increment);
    void updateBatteryStatus(const QString& robotID, int percentage, double voltage);
    void on_worldMapUpdated(RosNode::MapArray map);

private slots:
    void sendVelocityCommand(); // 타이머에 의해 호출되는 속도 발행 함수

    // 주행 제어 관련
    void on_pPBForward_clicked();
    void on_pPBBackward_clicked();
    void on_pPBLeft_clicked();
    void on_pPBRight_clicked();
    void on_pPBStop_clicked();

    // 서보 모터
    void on_pPBServo0_clicked();
    void on_pPBServo30_clicked();

    // 자율주행 모드 제어
    void on_btnStart_clicked();
    void on_btnStopNavi_clicked();
    void on_cbDest_currentTextChanged(const QString &text);

private:
    Ui::Tab1 *ui;
    RosNode *pRosNode;

    QTimer *cmdTimer;     // 10Hz 발행용 타이머
    double currentLinear;  // 현재 목표 선속도
    double currentAngular; // 현재 목표 각속도
};

#endif // TAB1_H
