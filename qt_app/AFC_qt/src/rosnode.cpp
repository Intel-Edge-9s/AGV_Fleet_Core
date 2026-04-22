#include "rosnode.h"

RosNode::RosNode() : rclcpp::Node("qt_ros_node")
{
    auto qos_reliable = rclcpp::QoS(rclcpp::KeepLast(10)).reliable();
    auto qos_vel = rclcpp::QoS(10);

    // 로봇 A와 B의 네임스페이스를 분리하여 Publisher 생성
    servo_pub_a_ = this->create_publisher<std_msgs::msg::Int32>("/robot_a/servo_angle", qos_reliable);
    cmd_vel_pub_a_ = this->create_publisher<geometry_msgs::msg::Twist>("/cmd_vel", qos_vel);

    servo_pub_b_ = this->create_publisher<std_msgs::msg::Int32>("/robot_b/servo_angle", qos_reliable);
    cmd_vel_pub_b_ = this->create_publisher<geometry_msgs::msg::Twist>("/robot_b/cmd_vel", qos_vel);

    // 배터리 Subscriber 생성
    battery_sub_a_ = this->create_subscription<sensor_msgs::msg::BatteryState>(
        "/robot_a/battery_state", 10, std::bind(&RosNode::battery_callback_a, this, std::placeholders::_1));

    battery_sub_b_ = this->create_subscription<sensor_msgs::msg::BatteryState>(
        "/robot_b/battery_state", 10, std::bind(&RosNode::battery_callback_b, this, std::placeholders::_1));
}

RosNode::~RosNode()
{
}

void RosNode::run()
{
    if (rclcpp::ok()) {
        rclcpp::spin(this->get_node_base_interface());
    }
}

void RosNode::publish_servo(const std::string& robot_name, int angle)
{
    if (!rclcpp::ok()) return;

    std_msgs::msg::Int32 msg;
    msg.data = angle;

    if (robot_name == "robot_a" && servo_pub_a_) servo_pub_a_->publish(msg);
    else if (robot_name == "robot_b" && servo_pub_b_) servo_pub_b_->publish(msg);

    RCLCPP_INFO(this->get_logger(), "Servo Command Sent: %d degrees", angle);
}

void RosNode::move_robot(const std::string& robot_name, double linear, double angular)
{
    if (!rclcpp::ok()) return;

    geometry_msgs::msg::Twist msg;
    msg.linear.x = linear;
    msg.angular.z = angular;

    if (robot_name == "robot_a" && cmd_vel_pub_a_) cmd_vel_pub_a_->publish(msg);
    else if (robot_name == "robot_b" && cmd_vel_pub_b_) cmd_vel_pub_b_->publish(msg);
}

// 배터리 콜백 함수 구현
void RosNode::battery_callback_a(const sensor_msgs::msg::BatteryState::SharedPtr msg) {
    if (std::isnan(msg->percentage)) return;
    emit batteryUpdated("A", static_cast<int>(msg->percentage), msg->voltage);
}

void RosNode::battery_callback_b(const sensor_msgs::msg::BatteryState::SharedPtr msg) {
    if (std::isnan(msg->percentage)) return;
    emit batteryUpdated("B", static_cast<int>(msg->percentage), msg->voltage);
}