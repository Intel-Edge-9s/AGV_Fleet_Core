#ifndef ROSNODE_H
#define ROSNODE_H

#include <QThread>
#include <QTimer>
#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/int32.hpp>
#include <std_msgs/msg/string.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <sensor_msgs/msg/battery_state.hpp>
#include <sensor_msgs/msg/laser_scan.hpp>
#include <nav_msgs/msg/odometry.hpp>
#include <tf2/LinearMath/Quaternion.h>
#include <tf2/LinearMath/Matrix3x3.h>

#include <algorithm>
#include <deque>
#include <memory>
#include <vector>
#include <array>
#include <cmath>

#include "pathfinding.h"

class RosNode : public QThread, public rclcpp::Node
{
    Q_OBJECT
public:
    enum Action { IDLE, FORWARD, BACKWARD, LEFT, RIGHT }; // 로봇 동작 정의

    RosNode();
    ~RosNode();

    // 로봇 제어 명령 관련
    void publish_servo(const std::string& robot_name, int angle);
    void move_robot(const std::string& robot_name, double linear, double angular);
    void execute_action(const std::string& robot_name, Action action);
    
    // 자율 주행 제어
    void startAutoNavigation(const std::string& robot_name, Point goal);
    void stopAutoNavigation(const std::string& robot_name);
    bool isAutoNavigationActive(const std::string& robot_name) const;
    void showMap();

    // 맵 데이터 구조 (8x7 그리드)
    using MapArray = std::array<std::array<int,8>,7>;
    int world_map[7][8] = {
        {1, 1, 1, 0, 1, 0, 1, 0},
        {1, 1, 1, 0, 0, 0, 0, 0},
        {1, 1, 1, 0, 0, 0, 0, 0},
        {1, 0, 0, 0, 1, 1, 1, 1},
        {1, 0, 0, 0, 0, 0, 0, 0},
        {1, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0}
    };
    bool mapDrawingMod = false;

    // 방향별 행/열 이동 오프셋 (Heading 인덱스: 0북, 1동, 2남, 3서)
    int dr[4] = {-1, 0, 1, 0};
    int dc[4] = {0, 1, 0, -1};

    // 로봇의 물리적/논리적 상태 정보
    struct RobotState {
        int row = 0;
        int col = 0;
        int heading = 0;

        double start_x = 0.0;
        double start_y = 0.0;
        double start_yaw = 0.0;

        double current_x = 0.0;
        double current_y = 0.0;
        double current_yaw = 0.0;

        bool init_set = false;
        Action current_action = IDLE;
    };

    // 멀티 로봇 경로 계획용 구조체
    struct Robot {
        int id = 0;
        int priority = 0;
        Point currentPos{0, 0};
        Point finalGoal{0, 0};
        std::vector<Point> path;
        int pathIndex = 0;
        int blockedCnt = 0;
        bool isFinished = false;
        bool isWaiting = false;

        Point getNextMove() const {
            if (pathIndex + 1 < static_cast<int>(path.size())) return path[pathIndex + 1];
            return currentPos;
        }
    };

    RobotState state_a, state_b;
    std::vector<Robot> robots;
    std::vector<Point> shelters = {{2, 5}, {4, 2}, {4, 3}, {4, 4}};

    // 자율주행 상태 및 액션 큐
    bool auto_mode_a_ = false;
    bool auto_mode_b_ = false;
    Point goal_a_{0, 0};
    Point goal_b_{0, 0};
    std::deque<Action> action_queue_a_;
    std::deque<Action> action_queue_b_;

signals:
    // UI 업데이트를 위한 신호들
    void batteryUpdated(QString robotID, int percentage, double voltage);
    void robotPoseUpdated(QString robotID, int col, int row);
    void lidarScanReceived(QString robotID, std::vector<float> ranges, float angle_min, float angle_increment);
    void worldMapUpdated(MapArray map);

protected:
    void run() override;

private:
    // ROS 2 통신 객체 (Robot A & B)
    rclcpp::Publisher<std_msgs::msg::Int32>::SharedPtr servo_pub_a_, servo_pub_b_;
    rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr cmd_vel_pub_a_, cmd_vel_pub_b_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr move_cmd_pub_a_, move_cmd_pub_b_;
    
    rclcpp::Subscription<sensor_msgs::msg::BatteryState>::SharedPtr battery_sub_a_, battery_sub_b_;
    rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr odom_sub_a_, odom_sub_b_;
    rclcpp::Subscription<sensor_msgs::msg::LaserScan>::SharedPtr scan_sub_a_, scan_sub_b_;
    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr move_done_sub_a_, move_done_sub_b_;

    QTimer* planner_timer_ = nullptr;

    // 콜백 및 내부 처리 함수
    void battery_callback_a(const sensor_msgs::msg::BatteryState::SharedPtr msg);
    void battery_callback_b(const sensor_msgs::msg::BatteryState::SharedPtr msg);
    void odom_callback(const nav_msgs::msg::Odometry::SharedPtr msg);
    void scan_callback_a(const sensor_msgs::msg::LaserScan::SharedPtr msg);
    void scan_callback_b(const sensor_msgs::msg::LaserScan::SharedPtr msg);
    void on_move_done(const std::string& name, RobotState& state, const std::string& action_str);

    // 유틸리티 및 알고리즘 함수
    double get_yaw_from_quaternion(const geometry_msgs::msg::Quaternion& q);
    void update_grid_map(RobotState& state, const std::vector<float>& scan_data);
    void process_odom(const std::string& name, const nav_msgs::msg::Odometry::SharedPtr msg, RobotState& state);
    
    // 경로 계획 및 자율 주행 로직
    void setCriticalZone(Point p);
    bool isCriticalZone(Point p) const;
    void simulateStep(std::vector<Robot>& robots);
    std::vector<Point> planPathForRobot(const std::string& robot_name, const Point& goal);
    std::deque<Action> buildActionQueueFromPath(const std::vector<Point>& path, int start_heading) const;
    int headingFromStep(const Point& from, const Point& to) const;
    Point predictNextPoint(const RobotState& state, Action action) const;
    
    // 멀티 로봇 정책 제어
    void ensurePolicyRobots();
    Robot* getPolicyRobot(const std::string& robot_name);
    bool shouldDispatchByPolicy(Robot& self, const std::string& robot_name, Action next_action);
    void dispatchAutoLoop();
    void dispatchAutoForRobot(const std::string& robot_name, RobotState& state, bool& auto_mode,
                              const Point& goal, std::deque<Action>& queue);

    // 온보딩 코드로 대체
    // bool move_forward (const std::string& name, RobotState& state, const nav_msgs::msg::Odometry::SharedPtr msg);
    // bool move_backward(const std::string& name, RobotState& state, const nav_msgs::msg::Odometry::SharedPtr msg);
    // bool turn_left    (const std::string& name, RobotState& state, const nav_msgs::msg::Odometry::SharedPtr msg);
    // bool turn_right   (const std::string& name, RobotState& state, const nav_msgs::msg::Odometry::SharedPtr msg);
    
    std::vector<float> latest_scan_a_;
    std::vector<float> latest_scan_b_;

    // 제어 파라미터
    const double TARGET_ANGLE = 1.52; // 약 90도 (라디안)
    const double Kp_linear = 0.5;
    const double Kp_angular = 0.8;
};

#endif // ROSNODE_H