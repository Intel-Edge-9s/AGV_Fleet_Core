#include "tab1.h"
#include "ui_tab1.h"
#include <QDebug>
#include <QTimer>
#include <QIntValidator>
#include "mapwidget.h"
#include "lidarwidget.h"

namespace {
// 목적지 명칭을 맵 좌표(Point)로 매핑하는 유틸리티 함수
bool mapDestinationToPoint(const QString& destination, Point& out_goal) {
    const QString d = destination.trimmed();

    if (d == "상차장")      { out_goal = {0, 6}; return true; }
    if (d == "하역장X")     { out_goal = {5, 0}; return true; }
    if (d == "하역장Y")     { out_goal = {7, 0}; return true; }
    if (d == "하역장Z")     { out_goal = {3, 0}; return true; }
    if (d == "하역장")      { out_goal = {3, 0}; return true; } 
    if (d == "충전소A")     { out_goal = {7, 6}; return true; }
    if (d == "충전소B")     { out_goal = {7, 5}; return true; }
    if (d == "창고A" || d == "A구역") { out_goal = {1, 4}; return true; }
    if (d == "창고B" || d == "B구역") { out_goal = {4, 4}; return true; }
    if (d == "창고C" || d == "C구역") { out_goal = {6, 2}; return true; }

    return false;
}

// UI 텍스트 기반 로봇 토픽명 변환
std::string robotNameToTopic(const QString& ui_robot_name) {
    if (ui_robot_name.contains("A")) return "tb3_a";
    return "tb3_b";
}

// 자율주행 중 수동 조작을 차단하는 가드 함수
bool blockManualWhenAuto(RosNode* node, const std::string& target) {
    if (!node) return true;
    if (node->isAutoNavigationActive(target)) {
        qDebug() << "자율주행 중에는 수동 입력이 차단됩니다.";
        return true;
    }
    return false;
}
} // namespace

Tab1::Tab1(QWidget *parent) : QWidget(parent), ui(new Ui::Tab1), currentLinear(0.0), currentAngular(0.0)
{
    ui->setupUi(this);

    // 기본 설정: 로봇 A 선택, 수량 입력은 숫자(0~999)만 가능하게 제한
    ui->rbtnTargetA->setChecked(true);
    QIntValidator *validator = new QIntValidator(0, 999, this);
    ui->cbCount->setValidator(validator);

    connect(ui->cbDest, &QComboBox::currentTextChanged, this, &Tab1::on_cbDest_currentTextChanged);

    // 초기 UI 상태 반영
    on_cbDest_currentTextChanged(ui->cbDest->currentText());

    // 주기적인 속도 명령 전송 타이머 (10Hz)
    cmdTimer = new QTimer(this);
    connect(cmdTimer, &QTimer::timeout, this, &Tab1::sendVelocityCommand);
    cmdTimer->start(100);
}

Tab1::~Tab1() { delete ui; }

void Tab1::setRosNode(RosNode* node) { pRosNode = node; }

void Tab1::sendVelocityCommand()
{
    // 수동 명령 주기가 필요할 경우 구현부 (현재는 execute_action 사용 중)
}

// --- 수동 조작 로직 (전진/후진/회전/정지) ---

void Tab1::on_pPBForward_clicked() {
    if(!pRosNode) return;

    bool isA = ui->rbtnTargetA->isChecked();
    std::string target = isA ? "tb3_a" : "tb3_b";
    if (blockManualWhenAuto(pRosNode, target)) return;

    auto& state = isA ? pRosNode->state_a : pRosNode->state_b;

    // 이동 가능 여부 체크 (충돌 방지 로직)
    int next_r = state.row + pRosNode->dr[state.heading];
    int next_c = state.col + pRosNode->dc[state.heading];

    if (next_r < 0 || next_r >= 7 || next_c < 0 || next_c >= 8) {
        qDebug() << "경계 밖으로는 이동할 수 없습니다.";
        return;
    }
    if (pRosNode->world_map[next_r][next_c] == 1) {
        qDebug() << "장애물이 있어 앞으로 갈 수 없습니다!";
        return;
    }

    if (ui->checkBox->isChecked()) { // 수동 활성화 체크 시 실행
        pRosNode->execute_action(target, RosNode::FORWARD);
    }
}

void Tab1::on_pPBBackward_clicked() {
    if(!pRosNode) return;

    bool isA = ui->rbtnTargetA->isChecked();
    std::string target = isA ? "tb3_a" : "tb3_b";
    if (blockManualWhenAuto(pRosNode, target)) return;
    auto& state = isA ? pRosNode->state_a : pRosNode->state_b;

    int next_r = state.row - pRosNode->dr[state.heading];
    int next_c = state.col - pRosNode->dc[state.heading];

    if (next_r < 0 || next_r >= 7 || next_c < 0 || next_c >= 8 || pRosNode->world_map[next_r][next_c] == 1) {
        qDebug() << "뒤쪽에 장애물이 있거나 경계 밖입니다.";
        return;
    }

    if (ui->checkBox->isChecked()) {
        pRosNode->execute_action(target, RosNode::BACKWARD);
    }
}

void Tab1::on_pPBLeft_clicked() {
    if(!pRosNode || !ui->checkBox->isChecked()) return;
    std::string target = ui->rbtnTargetA->isChecked() ? "tb3_a" : "tb3_b";
    if (blockManualWhenAuto(pRosNode, target)) return;
    pRosNode->execute_action(target, RosNode::LEFT);
}

void Tab1::on_pPBRight_clicked() {
    if(!pRosNode || !ui->checkBox->isChecked()) return;
    std::string target = ui->rbtnTargetA->isChecked() ? "tb3_a" : "tb3_b";
    if (blockManualWhenAuto(pRosNode, target)) return;
    pRosNode->execute_action(target, RosNode::RIGHT);
}

void Tab1::on_pPBStop_clicked() {
    if (!pRosNode) return;
    std::string target = ui->rbtnTargetA->isChecked() ? "tb3_a" : "tb3_b";
    if (blockManualWhenAuto(pRosNode, target)) return;
    pRosNode->move_robot(target, 0.0, 0.0);
}

// --- 서보 모터 및 특수 기능 ---

void Tab1::on_pPBServo0_clicked() { // 리프트 상승
    if(!pRosNode || !ui->checkBox->isChecked()) return;
    std::string target = ui->rbtnTargetA->isChecked() ? "tb3_a" : "tb3_b";
    if (blockManualWhenAuto(pRosNode, target)) return;
    pRosNode->publish_servo(target, 0);
}

void Tab1::on_pPBServo30_clicked() { // 리프트 하강
    if(!pRosNode || !ui->checkBox->isChecked()) return;
    std::string target = ui->rbtnTargetA->isChecked() ? "tb3_a" : "tb3_b";
    if (blockManualWhenAuto(pRosNode, target)) return;
    pRosNode->publish_servo(target, 30);
}

// 배터리 상태 UI 업데이트 (잔량별 색상 변경 포함)
void Tab1::updateBatteryStatus(const QString& robotID, int percentage, double voltage) {
    QProgressBar* targetBar = (robotID == "A") ? ui->barBattery : ui->barBattery_2;
    QLabel* targetLabel = (robotID == "A") ? ui->lblVoltageValue : ui->lblVoltageValue_2;

    if (!targetBar || !targetLabel) return;

    targetBar->setValue(percentage);
    targetLabel->setText(QString::number(voltage, 'f', 1) + " V");

    QString style = "QProgressBar { background-color: #121417; border: 1px solid #323842; border-radius: 6px; text-align: center; color: white; font-weight: bold; } ";

    if (percentage <= 20) {
        style += "QProgressBar::chunk { background-color: #ff4d4d; border-radius: 5px; }";
    } else if (percentage <= 50) {
        style += "QProgressBar::chunk { background-color: #ffab00; border-radius: 5px; }";
    } else {
        style += "QProgressBar::chunk { background-color: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #00c6ff, stop:1 #00f2ff); border-radius: 5px; }";
    }
    targetBar->setStyleSheet(style);
}

// --- 자율주행 실행 제어 ---

void Tab1::on_btnStart_clicked()
{
    if (!pRosNode) return;

    QString robot = ui->cbRobot->currentText();
    QString goal = ui->cbDest->currentText();
    std::string target = robotNameToTopic(robot);
    Point goal_point{};

    if (!mapDestinationToPoint(goal, goal_point)) {
        qDebug() << "목적지 좌표 매핑 실패:" << goal;
        return;
    }

    pRosNode->startAutoNavigation(target, goal_point);
    qDebug() << "자율주행 시작:" << robot << "-> (" << goal_point.x << "," << goal_point.y << ")";
}

void Tab1::on_btnStopNavi_clicked()
{
    if (!pRosNode) return;
    std::string target = robotNameToTopic(ui->cbRobot->currentText());
    pRosNode->stopAutoNavigation(target);
}

// 목적지에 따른 입력 필드 활성/비활성 제어
void Tab1::on_cbDest_currentTextChanged(const QString &text)
{
    QString destination = text.trimmed();
    bool isWorkFree = (destination == "상차장" || destination == "충전소A" || destination == "충전소B");

    ui->cbItem->setEnabled(!isWorkFree);
    ui->cbCount->setEnabled(!isWorkFree);

    if (isWorkFree) {
        ui->cbItem->setCurrentText("xxxx");
        ui->cbCount->setText("0");
        ui->cbItem->setStyleSheet("background-color: #2d2d2d; color: #777777;");
        ui->cbCount->setStyleSheet("background-color: #2d2d2d; color: #777777;");
    } else {
        ui->cbItem->setStyleSheet("");
        ui->cbCount->setStyleSheet("");
    }
}

// --- 데이터 시각화 (Map & LiDAR) ---

void Tab1::on_robotPoseReceived(const QString& robotID, int gridX, int gridY) {
    ui->mapWidget->setRobotPosition(robotID, gridX, gridY);
}

void Tab1::on_lidarScanReceived(const QString& robotID, const std::vector<float>& ranges, float angle_min, float angle_increment)
{
    // 현재 UI에서 선택된 로봇의 LiDAR 데이터만 시각화
    QString selected = (ui->cbRobot->currentText() == "터틀봇A") ? "A" : "B";
    if (robotID != selected) return;

    std::vector<LidarPoint> points;
    points.reserve(ranges.size());

    for (size_t i = 0; i < ranges.size(); ++i) {
        LidarPoint p;
        p.range = ranges[i];
        p.angle = angle_min + static_cast<float>(i) * angle_increment;
        points.push_back(p);
    }
    ui->lidarWidget->setScanData(points);
}