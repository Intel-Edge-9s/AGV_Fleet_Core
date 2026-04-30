#include "mainwidget.h"
#include "rosnode.h"

#include <QApplication>
#include <QMetaType>

#include <rclcpp/rclcpp.hpp>

#include <vector>

int main(int argc, char *argv[])
{
    // Qt queued signal/slot에서 std::vector<float> 전달을 위한 메타타입 등록
    qRegisterMetaType<std::vector<float>>("std::vector<float>");
    qRegisterMetaType<RosNode::MapArray>("MapArray");

    // ROS 2 초기화
    rclcpp::init(argc, argv);

    QApplication app(argc, argv);

    MainWidget w;
    w.show();

    int result = app.exec();

    // ROS 2 종료
    if (rclcpp::ok()) {
        rclcpp::shutdown();
    }

    return result;
}