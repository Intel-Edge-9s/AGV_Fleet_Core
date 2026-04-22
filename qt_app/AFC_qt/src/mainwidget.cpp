#include "mainwidget.h"
#include "ui_mainwidget.h"
#include <QVBoxLayout>

MainWidget::MainWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MainWidget)
{
    ui->setupUi(this);

    // 1. ROS 2 스레드 엔진 시작
    m_rosNode = new RosNode();
    m_rosNode->start();

    // 2. 각 탭 객체 생성
    m_tab1 = new Tab1(this);
    m_tab1->setRosNode(m_rosNode);
    m_tab2 = new Tab2(this);

    // 3. 신호 연결 (ROS 배터리 정보 -> Tab1 업데이트)
    connect(m_rosNode, &RosNode::batteryUpdated, m_tab1, &Tab1::updateBatteryStatus);

    // 4. 레이아웃 설정: Tab1을 ui->pTab1에 삽입
    if (ui->pTab1) {
        QVBoxLayout *tab1Layout = new QVBoxLayout(ui->pTab1);
        tab1Layout->setContentsMargins(0, 0, 0, 0);
        tab1Layout->addWidget(m_tab1);
    }

    // 5. 레이아웃 설정: Tab2를 ui->pTab2에 삽입
    if (ui->pTab2) {
        QVBoxLayout *tab2Layout = new QVBoxLayout(ui->pTab2);
        tab2Layout->setContentsMargins(0, 0, 0, 0);
        tab2Layout->addWidget(m_tab2);
    }

    // 6. 버튼 클릭 시 화면 전환
    // 버튼 이름(objectName)이 Designer와 일치하는지 확인하세요.
    // connect(ui->pTab1, &QPushButton::clicked, [=](){
    //     ui->tabWidget->setCurrentIndex(0); // 1페이지: 로봇 관제
    // });
    // connect(ui->pTab2, &QPushButton::clicked, [=](){
    //     ui->tabWidget->setCurrentIndex(1); // 2페이지: DB/재고
    // });

    // 시작 시 첫 번째 페이지 보여주기
    ui->tabWidget->setCurrentIndex(0);
}

MainWidget::~MainWidget()
{
    if (m_rosNode) {
        rclcpp::shutdown();
        m_rosNode->quit();
        m_rosNode->wait();
        delete m_rosNode;
    }
    delete ui;
}