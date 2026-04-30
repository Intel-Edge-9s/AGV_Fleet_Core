#include "mainwidget.h"
#include "ui_mainwidget.h"
#include <QVBoxLayout>

MainWidget::MainWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MainWidget)
    , m_tab1(nullptr)
    , m_tab2(nullptr)
    , m_rosNode(nullptr)
{
    ui->setupUi(this);

    // ROS 2 스레드 생성 및 시작
    m_rosNode = new RosNode();
    m_rosNode->start();

    // Tab1 객체 생성 및 ROS 노드 주입
    m_tab1 = new Tab1(this);
    m_tab1->setRosNode(m_rosNode);

    // RosNode의 배터리 신호를 Tab1의 업데이트 함수와 연결
    connect(m_rosNode, &RosNode::batteryUpdated, m_tab1, &Tab1::updateBatteryStatus, Qt::QueuedConnection);
    connect(m_rosNode, &RosNode::robotPoseUpdated, m_tab1, &Tab1::on_robotPoseReceived, Qt::QueuedConnection);
    connect(m_rosNode, &RosNode::lidarScanReceived, m_tab1, &Tab1::on_lidarScanReceived, Qt::QueuedConnection);
    connect(m_rosNode, &RosNode::worldMapUpdated, m_tab1, &Tab1::on_worldMapUpdated, Qt::QueuedConnection);

    // Qt Designer에서 만든 빈 pTab1 영역에 m_tab1 위젯을 꽉 차게 삽입
    QVBoxLayout *tab1Layout = new QVBoxLayout(ui->pTab1);
    tab1Layout->setContentsMargins(0, 0, 0, 0);
    tab1Layout->addWidget(m_tab1);
    ui->pTab1->setLayout(tab1Layout);

    // 5. Tab2 생성 및 pTab2 영역에 삽입
    m_tab2 = new Tab2(this);

    QVBoxLayout *tab2Layout = new QVBoxLayout(ui->pTab2);
    tab2Layout->setContentsMargins(0, 0, 0, 0);
    tab2Layout->addWidget(m_tab2);
    ui->pTab2->setLayout(tab2Layout);

    // 6. 초기 지도 위치 수동 반영
    // RosNode 생성자에서 emit한 초기 위치 signal은 connect 전에 발생해서 놓칠 수 있다.
    m_tab1->on_robotPoseReceived("A", m_rosNode->state_a.col, m_rosNode->state_a.row);
    m_tab1->on_robotPoseReceived("B", m_rosNode->state_b.col, m_rosNode->state_b.row);

    RosNode::MapArray arr;
    for (int r = 0; r < 7; ++r)
        for (int c = 0; c < 8; ++c)
            arr[r][c] = m_rosNode->world_map[r][c];
    m_tab1->on_worldMapUpdated(arr);

    ui->tabWidget->setCurrentIndex(0);

    m_rosNode->start();
}

MainWidget::~MainWidget()
{
    // 앱 종료 시 ROS 2 스레드 안전하게 종료
    if (m_rosNode) {
        if (rclcpp::ok()) {
            rclcpp::shutdown();
        }

        m_rosNode->quit();
        m_rosNode->wait();

        delete m_rosNode;
        m_rosNode = nullptr;
    }

    delete ui;
}