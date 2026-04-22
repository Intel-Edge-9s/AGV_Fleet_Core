#include "tab1.h"
#include "ui_tab1.h"
#include <QDebug>
#include <QTimer>
#include <QIntValidator>
#include "mapwidget.h"
#include "lidarwidget.h"

Tab1::Tab1(QWidget *parent) : QWidget(parent), ui(new Ui::Tab1), currentLinear(0.0), currentAngular(0.0)
{
    ui->setupUi(this);

    // 기본 타겟을 A로 설정하여 에러 방지
    ui->rbtnTargetA->setChecked(true);

    // cbCount(QLineEdit)에 숫자만 입력 가능하도록 제한 (0~999개)
    QIntValidator *validator = new QIntValidator(0, 999, this);
    ui->cbCount->setValidator(validator);

    connect(ui->cbDest, &QComboBox::currentTextChanged, this, &Tab1::on_cbDest_currentTextChanged);

    // 프로그램 시작 시 현재 목적지 상태를 즉시 UI에 반영
    on_cbDest_currentTextChanged(ui->cbDest->currentText());

    cmdTimer = new QTimer(this);
    connect(cmdTimer, &QTimer::timeout, this, &Tab1::sendVelocityCommand);
    cmdTimer->start(100);
}

Tab1::~Tab1() { delete ui; }

void Tab1::setRosNode(RosNode* node) { pRosNode = node; }

// 수동 조작 및 주행 제어 로직
void Tab1::sendVelocityCommand()
{
    if (!pRosNode) return;

    if (ui->checkBox->isChecked()) { // 수동조작 모드 ON
        // 어떤 라디오 버튼이 눌려있는지 확인하여 타겟 로봇 결정
        std::string target_robot = ui->rbtnTargetA->isChecked() ? "robot_a" : "robot_b";
        pRosNode->move_robot(target_robot, currentLinear, currentAngular);
    } else { // 수동조작 모드 OFF
        if (currentLinear != 0.0 || currentAngular != 0.0) {
            currentLinear = 0.0; currentAngular = 0.0;
            // 모드가 꺼지면 두 로봇 모두 정지 명령 전송
            pRosNode->move_robot("robot_a", 0.0, 0.0);
            pRosNode->move_robot("robot_b", 0.0, 0.0);
        }
    }
}

// 방향 버튼 슬롯들 (속도 설정만 담당, 실제 전송은 타이머가 처리)
void Tab1::on_pPBForward_clicked() { currentLinear = 0.15; currentAngular = 0.0; }
void Tab1::on_pPBBackward_clicked() { currentLinear = -0.15; currentAngular = 0.0; }
void Tab1::on_pPBLeft_clicked() { currentLinear = 0.0; currentAngular = 0.5; }
void Tab1::on_pPBRight_clicked() { currentLinear = 0.0; currentAngular = -0.5; }
void Tab1::on_pPBStop_clicked() { currentLinear = 0.0; currentAngular = 0.0; }


// 서보 모터(상승/하강) 제어 로직
void Tab1::on_pPBServo0_clicked() { // 상승 (대기/이동)
    if(!pRosNode || !ui->checkBox->isChecked()) return; // 수동모드일때만 작동

    std::string target = ui->rbtnTargetA->isChecked() ? "robot_a" : "robot_b";
    pRosNode->publish_servo(target, 0);
}

void Tab1::on_pPBServo30_clicked() { // 하강 (작업/적재)
    if(!pRosNode || !ui->checkBox->isChecked()) return;

    std::string target = ui->rbtnTargetA->isChecked() ? "robot_a" : "robot_b";
    pRosNode->publish_servo(target, 30);
}

// 자율주행 및 UI 상태 업데이트 로직
void Tab1::updateBatteryStatus(const QString& robotID, int percentage, double voltage) {
    QProgressBar* targetBar = nullptr;
    QLabel* targetLabel = nullptr;

    // 1. 대상 위젯 결정
    if (robotID == "A") {
        targetBar = ui->barBattery;
        targetLabel = ui->lblVoltageValue;
    } else if (robotID == "B") {
        targetBar = ui->barBattery_2;
        targetLabel = ui->lblVoltageValue_2;
    }

    if (!targetBar || !targetLabel) return;

    // 배터리 잔량, 전압값 설정
    targetBar->setValue(percentage);
    targetLabel->setText(QString::number(voltage, 'f', 1) + " V");

    // 잔량에 따른 색상 변경
    QString style = "QProgressBar { background-color: #121417; border: 1px solid #323842; border-radius: 6px; text-align: center; color: white; font-weight: bold; } ";

    if (percentage <= 20) {
        style += "QProgressBar::chunk { background-color: #ff4d4d; border-radius: 5px; }"; // 빨강
    } else if (percentage <= 50) {
        style += "QProgressBar::chunk { background-color: #ffab00; border-radius: 5px; }"; // 주황
    } else {
        style += "QProgressBar::chunk { background-color: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #00c6ff, stop:1 #00f2ff); border-radius: 5px; }"; // 기본 사이언
    }

    targetBar->setStyleSheet(style);
}

void Tab1::on_btnStart_clicked()
{
    QString robot = ui->cbRobot->currentText();
    QString goal = ui->cbDest->currentText();
    QString item = ui->cbItem->currentText();
    QString count = ui->cbCount->text();

    qDebug() << "명령 수신: " << robot << " 가 " << goal << " 로 이동합니다.";

    bool isNoItemLocation = (goal == "상차장" || goal == "충전소A" || goal == "충전소B");

    if (!isNoItemLocation) {
        qDebug() << "운반 물품: " << item << " 개수: " << count;
    }

    // if(pRosNode) pRosNode->sendGoal(goal); // ROS 명령 전달 시 사용
}

void Tab1::on_cbDest_currentTextChanged(const QString &text)
{
    QString destination = text.trimmed();

    if (destination == "상차장" || destination == "충전소A" || destination == "충전소B") {
        ui->cbItem->setCurrentText("xxxx");
        ui->cbCount->setText("0");

        ui->cbItem->setEnabled(false);
        ui->cbCount->setEnabled(false);

        ui->cbItem->setStyleSheet("background-color: #2d2d2d; color: #777777;");
        ui->cbCount->setStyleSheet("background-color: #2d2d2d; color: #777777;");
    }
    else {
        ui->cbItem->setEnabled(true);
        ui->cbCount->setEnabled(true);

        ui->cbItem->setStyleSheet("");
        ui->cbCount->setStyleSheet("");
    }
}


// 지도 및 라이다(센서) 데이터 시각화 업데이트
void Tab1::on_robotPoseReceived(const QString& robotID, int gridX, int gridY) {
    // UI의 mapWidget에 로봇 ID(A/B)와 위치 전달
    ui->mapWidget->setRobotPosition(robotID, gridX, gridY);
}

void Tab1::on_lidarDataReceived(const std::vector<float>& ranges, float angle_min, float angle_increment) {
    std::vector<LidarPoint> points;
    for (size_t i = 0; i < ranges.size(); ++i) {
        LidarPoint p;
        p.range = ranges[i];
        p.angle = angle_min + (i * angle_increment);
        points.push_back(p);
    }
    ui->lidarWidget->setScanData(points);
}

void Tab1::on_btnStopNavi_clicked()
{
    if (!pRosNode) return;

    // 현재 선택된 로봇 확인
    QString selectedRobot = ui->cbRobot->currentText();
    std::string target = (selectedRobot == "터틀봇A") ? "robot_a" : "robot_b";

    // 즉시 정지 명령 전송 (속도 0)
    pRosNode->move_robot(target, 0.0, 0.0);

    // (로그 출력)
    qDebug() << "자율주행 중단 명령: " << selectedRobot << " 정지합니다.";
}