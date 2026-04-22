#ifndef ROSNODE_H
#define ROSNODE_H

#include <QThread>
#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/int32.hpp>
#include <std_msgs/msg/string.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <string>
#include "sensor_msgs/msg/battery_state.hpp"

class RosNode : public QThread, public rclcpp::Node
{
    Q_OBJECT
public:
    RosNode();
    ~RosNode();

    void publish_servo(const std::string& robot_name, int angle);
    void move_robot(const std::string& robot_name, double linear, double angular);

signals:
    // [추가] 배터리 정보를 Tab1으로 보내줄 신호
    void batteryUpdated(QString robotID, int percentage, double voltage);

protected:
    void run() override;

private:
    // 로봇 A용 Publisher & Subscriber
    rclcpp::Publisher<std_msgs::msg::Int32>::SharedPtr servo_pub_a_;
    rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr cmd_vel_pub_a_;
    rclcpp::Subscription<sensor_msgs::msg::BatteryState>::SharedPtr battery_sub_a_;

    // 로봇 B용 Publisher & Subscriber
    rclcpp::Publisher<std_msgs::msg::Int32>::SharedPtr servo_pub_b_;
    rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr cmd_vel_pub_b_;
    rclcpp::Subscription<sensor_msgs::msg::BatteryState>::SharedPtr battery_sub_b_;

    // [추가] 배터리 콜백 함수
    void battery_callback_a(const sensor_msgs::msg::BatteryState::SharedPtr msg);
    void battery_callback_b(const sensor_msgs::msg::BatteryState::SharedPtr msg);
};

#endif // ROSNODE_H