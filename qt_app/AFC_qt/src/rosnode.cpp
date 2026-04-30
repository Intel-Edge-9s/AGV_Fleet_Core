#include "rosnode.h"

namespace {
constexpr int ROWS = 7;
constexpr int COLS = 8;
constexpr double PI = 3.14159265358979323846;

// 격자 범위 내 좌표인지 확인
bool inBounds(int row, int col)
{
    return row >= 0 && row < ROWS && col >= 0 && col < COLS;
}

// 각도를 -π ~ +π 범위로 정규화
double normalizeAngle(double angle)
{
    return std::atan2(std::sin(angle), std::cos(angle));
}

// 현재 yaw와 시작 yaw의 절대 차이 반환
double yawDiff(double current, double start)
{
    return std::abs(normalizeAngle(current - start));
}

// 목표 yaw와 현재 yaw의 부호 있는 오차 반환 (양수=왼쪽, 음수=오른쪽)
double signedYawError(double target, double current)
{
    return normalizeAngle(target - current);
}

// ROS BatteryState의 배터리 퍼센트를 0~100 정수로 정규화
// - 0.0~1.0 범위로 들어오는 경우와 0~100 범위 모두 대응
// - NaN이면 -1 반환
int normalizeBatteryPercentage(double percentage)
{
    if (std::isnan(percentage)) return -1;

    if (percentage >= 0.0 && percentage <= 1.0) {
        return std::clamp(static_cast<int>(percentage * 100.0), 0, 100);
    }

    return std::clamp(static_cast<int>(percentage), 0, 100);
}
}

// 노드 초기화: Publisher/Subscriber 생성, 로봇 초기 상태 설정, 자율주행 타이머 시작
RosNode::RosNode() : rclcpp::Node("qt_ros_node")
{
    move_cmd_pub_a_ = this->create_publisher<std_msgs::msg::String>(
        "/tb3_a/move_command", 10);
    move_cmd_pub_b_ = this->create_publisher<std_msgs::msg::String>(
        "/tb3_b/move_command", 10);
    move_done_sub_a_ = this->create_subscription<std_msgs::msg::String>(
        "/tb3_a/move_done", 10,
        [this](const std_msgs::msg::String::SharedPtr msg) {
            on_move_done("tb3_a", state_a, msg->data);
        });
    move_done_sub_b_ = this->create_subscription<std_msgs::msg::String>(
        "/tb3_b/move_done", 10,
        [this](const std_msgs::msg::String::SharedPtr msg) {
            on_move_done("tb3_b", state_b, msg->data);
        });

    // 초기 로봇 위치 및 방향 설정 (0=UP, 1=RIGHT, 2=DOWN, 3=LEFT)
    state_a.col = 7;
    state_a.row = 4;
    state_a.heading = 3;

    state_b.col = 7;
    state_b.row = 6;
    state_b.heading = 3;

    state_a.init_set = false;
    state_b.init_set = false;
    state_a.current_action = IDLE;
    state_b.current_action = IDLE;

    if (state_a.row >= 0 && state_a.row < 7 && state_a.col >= 0 && state_a.col < 8) {
        world_map[state_a.row][state_a.col] = 3;
    }

    if (state_b.row >= 0 && state_b.row < 7 && state_b.col >= 0 && state_b.col < 8) {
        world_map[state_b.row][state_b.col] = 3;
    }

    auto qos_reliable = rclcpp::QoS(rclcpp::KeepLast(10)).reliable();
    auto qos_vel = rclcpp::QoS(10);

    // [로봇 A] Publisher & Subscriber
    servo_pub_a_ = this->create_publisher<std_msgs::msg::Int32>("/tb3_a/servo_angle", qos_reliable);
    cmd_vel_pub_a_ = this->create_publisher<geometry_msgs::msg::Twist>("/tb3_a/cmd_vel", qos_vel);
    odom_sub_a_ = this->create_subscription<nav_msgs::msg::Odometry>(
        "/tb3_a/odom", qos_vel, [this](const nav_msgs::msg::Odometry::SharedPtr msg) {
            process_odom("tb3_a", msg, state_a);
        });
    scan_sub_a_ = this->create_subscription<sensor_msgs::msg::LaserScan>(
        "/tb3_a/scan", rclcpp::QoS(10).best_effort(), std::bind(&RosNode::scan_callback_a, this, std::placeholders::_1));
    battery_sub_a_ = this->create_subscription<sensor_msgs::msg::BatteryState>(
        "/tb3_a/battery_state", qos_vel, std::bind(&RosNode::battery_callback_a, this, std::placeholders::_1));

    // [로봇 B] Publisher & Subscriber
    servo_pub_b_ = this->create_publisher<std_msgs::msg::Int32>("/tb3_b/servo_angle", qos_reliable);
    cmd_vel_pub_b_ = this->create_publisher<geometry_msgs::msg::Twist>("/tb3_b/cmd_vel", qos_vel);
    odom_sub_b_ = this->create_subscription<nav_msgs::msg::Odometry>(
        "/tb3_b/odom", qos_vel, [this](const nav_msgs::msg::Odometry::SharedPtr msg) {
            process_odom("tb3_b", msg, state_b);
        });
    scan_sub_b_ = this->create_subscription<sensor_msgs::msg::LaserScan>(
        "/tb3_b/scan", rclcpp::QoS(10).best_effort(), std::bind(&RosNode::scan_callback_b, this, std::placeholders::_1));
    battery_sub_b_ = this->create_subscription<sensor_msgs::msg::BatteryState>(
        "/tb3_b/battery_state", qos_vel, std::bind(&RosNode::battery_callback_b, this, std::placeholders::_1));

    // 500ms 주기로 자율주행 루프 실행
    planner_timer_ = new QTimer();
    planner_timer_->setInterval(500);
    connect(planner_timer_, &QTimer::timeout, this, &RosNode::dispatchAutoLoop);
    planner_timer_->start();

    emit robotPoseUpdated("A", state_a.col, state_a.row);
    emit robotPoseUpdated("B", state_b.col, state_b.row);
}

RosNode::~RosNode() {
    if (planner_timer_) {
        planner_timer_->stop();
        delete planner_timer_;
        planner_timer_ = nullptr;
    }
}

// ROS 스핀 루프 실행 (별도 스레드에서 호출)
void RosNode::run()
{
    if (rclcpp::ok()) {
        rclcpp::spin(this->get_node_base_interface());
    }
}

// 현재 world_map 상태를 콘솔에 출력 (R=로봇, #=벽, .=빈칸)
void RosNode::showMap(){
    std::cout << "\n========== GRID MAP ==========\n";
    for (int y = 0; y < 7; y++) {
        for (int x = 0; x < 8; x++) {
            if (world_map[y][x] == 3) std::cout << "R ";
            else if (world_map[y][x] == 1) std::cout << "# ";
            else std::cout << ". ";
        }
        std::cout << std::endl;
    }
    std::cout << "==============================\n";
}

// 로봇이 임계구역(상차지/하역장)에 있을 때 해당 셀을 2(임계구역)로 표시
void RosNode::setCriticalZone(Point p) {
    if (p.y == 6 && (p.x == 0 || p.x == 1)) {
        world_map[6][0] = 2;
        world_map[6][1] = 2;
    }
    if (p.x == 3 && (p.y == 0 || p.y == 1)) {
        world_map[0][3] = 2;
        world_map[1][3] = 2;
    }
}

// 주어진 좌표가 임계구역(상차지/하역장)인지 확인
bool RosNode::isCriticalZone(Point p) const {
    // 상차지
    if (p.y == 6 && (p.x == 0 || p.x == 1)) return true;
    // 하역장 Z
    if (p.x == 3 && (p.y == 0 || p.y == 1)) return true;
    return false;
}

// 시뮬레이션 1스텝 실행: 맵 초기화 → 충돌/양보/임계구역 정책 적용 → 로봇 이동
// 예외 처리 순서: 1.우선순위 양보 → 2.임계구역 통제 → 3.경로 재계획 → 4.대피소 이동
void RosNode::simulateStep(std::vector<Robot>& robots) {
    for (int y = 0; y < 7; y++) {
        for (int x = 0; x < 8; x++) {
            if (world_map[y][x] != 1) world_map[y][x] = 0;
        }
    }

    for (auto& r : robots) {
        setCriticalZone(r.currentPos);
        world_map[r.currentPos.y][r.currentPos.x] = 3;
    }

    showMap();

    for (auto& r : robots) {
        if (r.isFinished) continue;

        if (r.blockedCnt >= 3) {
            // [예외 처리 3: 경로 업데이트]
            r.path = findPath(r.currentPos, r.finalGoal, world_map);
            r.pathIndex = 0;

            if (r.path.empty()) {
                // [예외 처리 4: 대피소 이동]
                std::cout << "Can't Find Path." << std::endl;
                std::cout << "Robot " << r.id << " : Finding Shelter...\n";

                std::vector<Point> shelterPath = findValidShelterPath(r.currentPos, shelters, world_map);
                if (!shelterPath.empty()) {
                    r.path = shelterPath;
                    r.blockedCnt = 0;
                    r.isWaiting = true;
                    std::cout << "Robot " << r.id << " : Moving To Shelter\n";
                    continue;
                } else {
                    std::cout << "Robot " << r.id << " : [WARNING!!] CAN'T FIND ANY SHELTERS\n";
                    continue;
                }
            } else {
                std::cout << "Best Path XY:" << std::endl;
                for (auto p : r.path) {
                    std::cout << "{" << p.x << ", " << p.y << "}, ";
                }
                std::cout << "Way Designation complete\n";
                r.blockedCnt = 0;
                continue;
            }
        }

        Point next = r.getNextMove();

        // [예외 처리 1: 우선순위 양보]
        bool needToYield = false;
        for (auto& other : robots) {
            if (other.id != r.id && !other.isFinished) {
                Point otherNext = other.getNextMove();
                if (otherNext.x == next.x && otherNext.y == next.y) {
                    if (r.priority > other.priority) {
                        needToYield = true;
                        break;
                    }
                }
            }
        }
        if (needToYield) {
            r.blockedCnt++;
            std::cout << "Robot " << r.id << " yields path.\n";
            continue;
        }

        // [예외 처리 2: 임계 구역 진입 통제]
        if (isCriticalZone(next)) {
            bool zoneBlocked = false;
            for (auto& other : robots) {
                if (other.id != r.id && isCriticalZone(other.currentPos)) {
                    if (isCriticalZone(r.finalGoal) || !other.isFinished) {
                        zoneBlocked = true;
                        break;
                    }
                }
            }
            if (zoneBlocked && !isCriticalZone(r.currentPos)) {
                std::cout << "Robot " << r.id << " waits outside Critical Zone.\n";
                r.blockedCnt++;
                continue;
            }
        }

        // 이동 로직
        if (world_map[next.y][next.x] == 0 || world_map[next.y][next.x] == 2 ||
            (next.x == r.currentPos.x && next.y == r.currentPos.y)) {
            world_map[r.currentPos.y][r.currentPos.x] = 0;
            if (!(next.x == r.currentPos.x && next.y == r.currentPos.y)) {
                r.currentPos = next;
                r.pathIndex++;
            }
            world_map[r.currentPos.y][r.currentPos.x] = 3;
            std::cout << "Robot " << r.id << " moved to (" << r.currentPos.x << ", " << r.currentPos.y << ")\n";
        } else {
            r.blockedCnt++;
            std::cout << "Robot " << r.id << " is WAITING (Path Blocked by Robot at "
                      << next.x << "," << next.y << ")\n";
            continue;
        }

        if (!r.path.empty() && r.currentPos.x == r.path.back().x && r.currentPos.y == r.path.back().y) {
            if (!r.isWaiting) {
                r.isFinished = true;
                std::cout << "Robot " << r.id << " arrived at GOAL.\n";
            } else {
                if (world_map[r.finalGoal.y][r.finalGoal.x] == 0) {
                    std::cout << "Robot " << r.id << " : Goal is now free. Re-routing...\n";
                    std::vector<Point> newPath = findPath(r.currentPos, r.finalGoal, world_map);
                    if (!newPath.empty()) {
                        r.path = newPath;
                        r.pathIndex = 0;
                        r.isWaiting = false;
                        r.blockedCnt = 0;
                        std::cout << "Robot " << r.id << " : Leaving shelter and moving to goal.\n";
                    } else {
                        std::cout << "Robot " << r.id << " : Path to goal still blocked.\n";
                    }
                } else {
                    std::cout << "Robot " << r.id << " : Goal still occupied. Waiting...\n";
                }
            }
        }

        r.blockedCnt = 0;
    }
}

// 지정한 로봇의 현재 격자 위치에서 goal까지 경로를 탐색하여 반환
std::vector<Point> RosNode::planPathForRobot(const std::string& robot_name, const Point& goal) {
    Point start{0, 0};
    if (robot_name == "tb3_a") {
        start = {state_a.col, state_a.row};
    } else if (robot_name == "tb3_b") {
        start = {state_b.col, state_b.row};
    } else {
        return {};
    }

    return findPath(start, goal, world_map);
}

// 인접한 두 격자 좌표(from→to)의 이동 방향을 heading 값으로 반환
// heading: 0=UP, 1=RIGHT, 2=DOWN, 3=LEFT / 유효하지 않은 이동이면 -1 반환
int RosNode::headingFromStep(const Point& from, const Point& to) const {
    const int dx = to.x - from.x;
    const int dy = to.y - from.y;

    if (dx == 0 && dy == -1) return 0;
    if (dx == 1 && dy == 0) return 1;
    if (dx == 0 && dy == 1) return 2;
    if (dx == -1 && dy == 0) return 3;
    return -1;
}

// 경로(path)와 시작 heading을 받아 실제 실행할 액션 큐(회전+전진)를 생성
// 방향 전환이 필요한 경우 LEFT/RIGHT를 먼저 삽입하고, 이후 FORWARD를 삽입
std::deque<RosNode::Action> RosNode::buildActionQueueFromPath(const std::vector<Point>& path, int start_heading) const {
    std::deque<Action> actions;
    if (path.size() < 2) return actions;

    int heading = start_heading;

    for (size_t i = 1; i < path.size(); ++i) {
        const int target_heading = headingFromStep(path[i - 1], path[i]);
        if (target_heading < 0) {
            actions.clear();
            return actions;
        }

        int diff = (target_heading - heading + 4) % 4;
        if (diff == 1) {
            actions.push_back(RIGHT);
            heading = (heading + 1) % 4;
        } else if (diff == 3) {
            actions.push_back(LEFT);
            heading = (heading + 3) % 4;
        } else if (diff == 2) {
            actions.push_back(RIGHT);
            actions.push_back(RIGHT);
            heading = (heading + 2) % 4;
        }

        actions.push_back(FORWARD);
    }

    return actions;
}

// 현재 로봇 상태와 액션을 받아 이동 후 예상 격자 좌표를 반환
// 회전 액션(LEFT/RIGHT)은 격자 위치 변화 없음
Point RosNode::predictNextPoint(const RobotState& state, Action action) const {
    Point cur{state.col, state.row};
    if (action == FORWARD) {
        return {cur.x + dc[state.heading], cur.y + dr[state.heading]};
    }
    if (action == BACKWARD) {
        return {cur.x - dc[state.heading], cur.y - dr[state.heading]};
    }
    return cur; // 회전 액션은 격자 위치 변화 없음
}

// robots 벡터가 비어있거나 1개일 때 tb3_a(id=1, priority=1), tb3_b(id=2, priority=2)로 초기화
void RosNode::ensurePolicyRobots() {
    if (robots.size() == 2) return;

    robots.clear();
    Robot a;
    a.id = 1;
    a.priority = 1;
    robots.push_back(a);

    Robot b;
    b.id = 2;
    b.priority = 2;
    robots.push_back(b);
}

// 로봇 이름으로 policy robots 벡터의 해당 Robot 포인터를 반환
RosNode::Robot* RosNode::getPolicyRobot(const std::string& robot_name) {
    ensurePolicyRobots();
    if (robot_name == "tb3_a") return &robots[0];
    if (robot_name == "tb3_b") return &robots[1];
    return nullptr;
}

// 다음 액션을 실행해도 되는지 충돌 정책을 검사
// 경계/장애물 충돌, 우선순위 양보, 임계구역 진입 통제를 순서대로 확인
// 진입 불가 시 blockedCnt를 증가시키고 false 반환
bool RosNode::shouldDispatchByPolicy(Robot& self, const std::string& robot_name, Action next_action) {
    Robot* other = (robot_name == "tb3_a") ? getPolicyRobot("tb3_b") : getPolicyRobot("tb3_a");
    if (!other) return true;

    const RobotState& self_state = (robot_name == "tb3_a") ? state_a : state_b;
    const Point self_next = predictNextPoint(self_state, next_action);
    const Point other_cur = other->currentPos;

    // 경계/장애물 기본 체크
    if (self_next.x < 0 || self_next.x >= 8 || self_next.y < 0 || self_next.y >= 7) {
        self.blockedCnt++;
        return false;
    }
    if (world_map[self_next.y][self_next.x] == 1) {
        self.blockedCnt++;
        return false;
    }

    // 우선순위 양보: 같은 칸으로 진입하려고 할 때 낮은 우선순위(숫자 큰 쪽) 대기
    if (self_next.x == other_cur.x && self_next.y == other_cur.y) {
        if (self.priority > other->priority) {
            self.blockedCnt++;
            return false;
        }
    }

    // 임계구역 진입 통제
    if (isCriticalZone(self_next) && !isCriticalZone(self.currentPos)) {
        if (isCriticalZone(other_cur) && !other->isFinished) {
            self.blockedCnt++;
            return false;
        }
    }

    return true;
}

// 단일 로봇의 자율주행 1사이클 처리
// 목표 도달 확인 → 경로 생성 → 정책 검사 → 액션 디스패치 순으로 진행
// blockedCnt >= 3이면 경로를 초기화하여 다음 사이클에서 재계획
void RosNode::dispatchAutoForRobot(const std::string& robot_name, RobotState& state, bool& auto_mode,
                                   const Point& goal, std::deque<Action>& queue) {
    if (!auto_mode) return;

    Robot* policy_robot = getPolicyRobot(robot_name);
    if (!policy_robot) return;

    const Point current{state.col, state.row};
    policy_robot->currentPos = current;
    policy_robot->finalGoal = goal;
    policy_robot->isFinished = false;

    if (current.x == goal.x && current.y == goal.y) {
        auto_mode = false;
        queue.clear();
        policy_robot->isFinished = true;
        policy_robot->blockedCnt = 0;
        move_robot(robot_name, 0.0, 0.0);
        std::cout << "[AUTO] " << robot_name << " arrived at goal (" << goal.x << ", " << goal.y << ")\n";
        return;
    }

    if (state.current_action != IDLE) return;

    if (queue.empty()) {
        const auto path = planPathForRobot(robot_name, goal);
        if (path.empty()) {
            std::cout << "[AUTO] " << robot_name << " failed to find path to ("
                      << goal.x << ", " << goal.y << ")\n";
            policy_robot->blockedCnt++;
            return;
        }
        queue = buildActionQueueFromPath(path, state.heading);
        if (queue.empty()) {
            std::cout << "[AUTO] " << robot_name << " failed to build action queue\n";
            policy_robot->blockedCnt++;
            return;
        }
        policy_robot->path = path;
        policy_robot->pathIndex = 0;
        std::cout << "[AUTO] " << robot_name << " queued " << queue.size() << " actions\n";
    }

    if (policy_robot->blockedCnt >= 3) {
        // [예외 처리 3] blocked 누적 시 즉시 재계획
        queue.clear();
        policy_robot->path.clear();
        policy_robot->pathIndex = 0;
        policy_robot->blockedCnt = 0;
        std::cout << "[AUTO] " << robot_name << " blocked>=3, forcing replan\n";
        return;
    }

    const Action next = queue.front();
    if (!shouldDispatchByPolicy(*policy_robot, robot_name, next)) {
        return;
    }

    queue.pop_front();
    policy_robot->blockedCnt = 0;
    execute_action(robot_name, next);
}

// 500ms 타이머 콜백: tb3_a, tb3_b 두 로봇의 자율주행 사이클을 순서대로 실행
void RosNode::dispatchAutoLoop() {
    dispatchAutoForRobot("tb3_a", state_a, auto_mode_a_, goal_a_, action_queue_a_);
    dispatchAutoForRobot("tb3_b", state_b, auto_mode_b_, goal_b_, action_queue_b_);
}

// tb3_a의 LaserScan 콜백: 스캔 데이터를 저장하고 UI에 신호 전송
void RosNode::scan_callback_a(const sensor_msgs::msg::LaserScan::SharedPtr msg) {
    latest_scan_a_ = msg->ranges;
    emit lidarScanReceived("A", latest_scan_a_, msg->angle_min, msg->angle_increment);
}

// tb3_b의 LaserScan 콜백: 스캔 데이터를 저장하고 UI에 신호 전송
void RosNode::scan_callback_b(const sensor_msgs::msg::LaserScan::SharedPtr msg) {
    latest_scan_b_ = msg->ranges;
    emit lidarScanReceived("B", latest_scan_b_, msg->angle_min, msg->angle_increment);
}

// Odometry 콜백: 로봇의 현재 x, y, yaw 값을 상태에 갱신 (완료 판정은 on_move_done에서 처리)
void RosNode::process_odom(const std::string& name,
                           const nav_msgs::msg::Odometry::SharedPtr msg, RobotState& state) {
    state.current_x   = msg->pose.pose.position.x;
    state.current_y   = msg->pose.pose.position.y;
    state.current_yaw = get_yaw_from_quaternion(msg->pose.pose.orientation);
}

// LaserScan 데이터를 바탕으로 전/좌/후/우 4방향 장애물을 감지하고 world_map에 반영
// 20cm 이내 감지 시 해당 격자 셀을 1(벽)으로 업데이트
void RosNode::update_grid_map(RobotState& state, const std::vector<float>& scan_data) {
    if (!mapDrawingMod || scan_data.empty()) return;

    // 상대적 센서 방향 인덱스 (0:정면, 90:왼쪽, 180:후면, 270:오른쪽)
    int sensor_angles[4] = {0, 90, 180, 270};
    std::string dir_names[4] = {"FRONT", "LEFT", "BACK", "RIGHT"};

    std::cout << "\n--- Lidar Scan Update (Robot at: " << state.row << ", " << state.col << ") ---" << std::endl;

    for (int i = 0; i < 4; ++i) {
        if (sensor_angles[i] >= static_cast<int>(scan_data.size())) continue;
        float dist = scan_data[sensor_angles[i]];
        if (std::isinf(dist) || std::isnan(dist)) continue;

        std::cout << "[" << dir_names[i] << "] Raw Dist: " << dist << "m";

        if (dist < 0.20 && dist != 0.0) { // 20cm 이내 장애물 감지 시
            int sensor_absolute_dir;
            if (i == 0) sensor_absolute_dir = state.heading;             // 정면
            else if (i == 1) sensor_absolute_dir = (state.heading + 3) % 4; // 로봇기준 왼쪽
            else if (i == 2) sensor_absolute_dir = (state.heading + 2) % 4; // 로봇기준 후면
            else sensor_absolute_dir = (state.heading + 1) % 4;             // 로봇기준 오른쪽

            int obs_row = state.row + dr[sensor_absolute_dir];
            int obs_col = state.col + dc[sensor_absolute_dir];

            std::cout << std::endl << "row, col : " << obs_row << ", " << obs_col << std::endl;

            if (obs_row >= 0 && obs_row < 7 && obs_col >= 0 && obs_col < 8) {
                if (world_map[obs_row][obs_col] == 0) {
                    world_map[obs_row][obs_col] = 1;
                }
            }
        }
    }

    showMap();
}

// 지정한 로봇의 서보 모터 각도 명령을 퍼블리시
void RosNode::publish_servo(const std::string& robot_name, int angle)
{
    if (!rclcpp::ok()) return;

    std_msgs::msg::Int32 msg;
    msg.data = angle;

    if (robot_name == "tb3_a" && servo_pub_a_) servo_pub_a_->publish(msg);
    else if (robot_name == "tb3_b" && servo_pub_b_) servo_pub_b_->publish(msg);

    RCLCPP_INFO(this->get_logger(), "Servo Command Sent: %d degrees", angle);
}

// ===== 온 보딩 코드로 대체 =====
// bool RosNode::move_forward(const std::string& name, RobotState& state,
//         const nav_msgs::msg::Odometry::SharedPtr msg) {
//     double traveled = std::sqrt(std::pow(msg->pose.pose.position.x - state.start_x, 2) +
//              std::pow(msg->pose.pose.position.y - state.start_y, 2));
//     double error = 0.305 - traveled;
//     if (error > 0.01) {
//         double vel = std::clamp(error * Kp_linear, 0.03, 0.18);
//         move_robot(name, vel, 0.0);
//         return false;
//     }
//     return true;

//     // 정지 후 짧게 대기 (관성 제거)
//     move_robot(name, 0.0, 0.0);
//     return true;
// }

// bool RosNode::move_backward(const std::string& name, RobotState& state,
//      const nav_msgs::msg::Odometry::SharedPtr msg) {
//     double traveled = std::sqrt(std::pow(msg->pose.pose.position.x - state.start_x, 2) +
//              std::pow(msg->pose.pose.position.y - state.start_y, 2));
//     double error = 0.3025 - traveled;
//     if (error > 0.01) {
//         // double vel = std::clamp(error * Kp_linear * 0.8, 0.03, 0.24);
//         double vel = std::clamp(error * Kp_linear * 0.8, 0.03, 0.18);
//         move_robot(name, -vel, 0.0);
//         return false;
//     }
//     return true;
// }

// bool RosNode::turn_left(const std::string& name, RobotState& state,
//                         const nav_msgs::msg::Odometry::SharedPtr msg) {
//     double cur_yaw = get_yaw_from_quaternion(msg->pose.pose.orientation);

//     // start_yaw에서 현재까지 왼쪽으로 얼마나 돌았는지 (양수여야 함)
//     double diff = normalizeAngle(cur_yaw - state.start_yaw);
//     if (diff < 0) diff += 2.0 * PI;  // 0~2π로 변환
//     if (diff > PI) diff = 2.0 * PI - diff;  // 180 넘으면 반대방향으로 계산된 것

//     double error = 1.53 - diff;
//     if (error > 0.02) {
//         double vel = std::clamp(error * Kp_angular, 0.10, 0.25);
//         move_robot(name, 0.0, vel);
//         return false;
//     }
//     return true;
// }

// bool RosNode::turn_right(const std::string& name, RobotState& state,
//                          const nav_msgs::msg::Odometry::SharedPtr msg) {
//     double cur_yaw = get_yaw_from_quaternion(msg->pose.pose.orientation);

//     // start_yaw에서 현재까지 오른쪽으로 얼마나 돌았는지 (양수여야 함)
//     double diff = normalizeAngle(state.start_yaw - cur_yaw);
//     if (diff < 0) diff += 2.0 * PI;
//     if (diff > PI) diff = 2.0 * PI - diff;

//     double error = 1.55 - diff;
//     if (error > 0.02) {
//         double vel = std::clamp(error * Kp_angular, 0.10, 0.25);
//         move_robot(name, 0.0, -vel);
//         return false;
//     }
//     return true;
// }
// =====

// 지정한 로봇에 선속도/각속도 명령을 퍼블리시
void RosNode::move_robot(const std::string& robot_name, double linear, double angular) {
    geometry_msgs::msg::Twist msg;
    msg.linear.x = linear;
    msg.angular.z = angular;

    if (robot_name == "tb3_a") cmd_vel_pub_a_->publish(msg);
    else if (robot_name == "tb3_b") cmd_vel_pub_b_->publish(msg);
}

// 로봇에 단일 액션(FORWARD/BACKWARD/LEFT/RIGHT)을 JSON 명령으로 전송
// 이미 액션이 실행 중(non-IDLE)이면 무시
// 각 로봇별로 캘리브레이션된 value 값을 사용
void RosNode::execute_action(const std::string& robot_name, Action action) {
    auto& state = (robot_name == "tb3_a") ? state_a : state_b;
    if (state.current_action != IDLE) return;

    std_msgs::msg::String cmd;
    switch (action) {
    case FORWARD:
        cmd.data = (robot_name == "tb3_a") ? R"({"action":"forward","value":0.295})" : R"({"action":"forward","value":0.295})";
        break;
    case BACKWARD:
        cmd.data = R"({"action":"backward","value":0.30})"; break;
    case LEFT:
        cmd.data = (robot_name == "tb3_a") ? R"({"action":"left","value":1.548})" : R"({"action":"left","value":1.563})";
        break;
    case RIGHT:
        cmd.data = (robot_name == "tb3_a") ? R"({"action":"right","value":1.548})" : R"({"action":"right","value":1.568})";
        break;
    default: return;
    }

    if (robot_name == "tb3_a") {
        move_cmd_pub_a_->publish(cmd);
    } else {
        move_cmd_pub_b_->publish(cmd);
    }

    state.current_action = action;
    std::cout << "[CMD] " << robot_name << " action: " << action << std::endl;
}

// 지정한 로봇의 자율주행을 시작: 목표 설정, 액션 큐 초기화, 상태 리셋
void RosNode::startAutoNavigation(const std::string& robot_name, Point goal) {
    if (robot_name == "tb3_a") {
        auto_mode_a_ = true;
        goal_a_ = goal;
        action_queue_a_.clear();
        state_a.current_action = IDLE;
        state_a.init_set = false;
        std::cout << "[AUTO] Start tb3_a to goal (" << goal.x << ", " << goal.y << ")\n";
    } else if (robot_name == "tb3_b") {
        auto_mode_b_ = true;
        goal_b_ = goal;
        action_queue_b_.clear();
        state_b.current_action = IDLE;
        state_b.init_set = false;
        std::cout << "[AUTO] Start tb3_b to goal (" << goal.x << ", " << goal.y << ")\n";
    }
}

// 지정한 로봇의 자율주행을 즉시 중단: 액션 큐 초기화, 로봇 정지 명령 전송
void RosNode::stopAutoNavigation(const std::string& robot_name) {
    if (robot_name == "tb3_a") {
        auto_mode_a_ = false;
        action_queue_a_.clear();
        state_a.current_action = IDLE;
        state_a.init_set = false;
        move_robot("tb3_a", 0.0, 0.0);
        std::cout << "[AUTO] Stop tb3_a\n";
    } else if (robot_name == "tb3_b") {
        auto_mode_b_ = false;
        action_queue_b_.clear();
        state_b.current_action = IDLE;
        state_b.init_set = false;
        move_robot("tb3_b", 0.0, 0.0);
        std::cout << "[AUTO] Stop tb3_b\n";
    }
}

// 지정한 로봇이 현재 자율주행 중인지 여부를 반환
bool RosNode::isAutoNavigationActive(const std::string& robot_name) const {
    if (robot_name == "tb3_a") return auto_mode_a_;
    if (robot_name == "tb3_b") return auto_mode_b_;
    return false;
}

// ROS Quaternion을 받아 yaw(회전각)를 라디안으로 변환하여 반환
double RosNode::get_yaw_from_quaternion(const geometry_msgs::msg::Quaternion& q)
{
    tf2::Quaternion tf_q(q.x, q.y, q.z, q.w);
    tf2::Matrix3x3 m(tf_q);
    double r, p, y;
    m.getRPY(r, p, y);
    return y;
}

// tb3_a의 배터리 상태 콜백: 퍼센트를 정규화하여 UI에 신호 전송
void RosNode::battery_callback_a(const sensor_msgs::msg::BatteryState::SharedPtr msg) {
    int percentage = normalizeBatteryPercentage(msg->percentage);
    if (percentage < 0) return;
    emit batteryUpdated("A", percentage, msg->voltage);
}

// tb3_b의 배터리 상태 콜백: 퍼센트를 정규화하여 UI에 신호 전송
void RosNode::battery_callback_b(const sensor_msgs::msg::BatteryState::SharedPtr msg) {
    int percentage = normalizeBatteryPercentage(msg->percentage);
    if (percentage < 0) return;
    emit batteryUpdated("B", percentage, msg->voltage);
}

// 로봇 액션 완료 콜백: 격자 좌표/heading 갱신, world_map 업데이트, UI 신호 전송
// 중복 수신 방지를 위해 IDLE 상태에서 들어온 완료 신호는 무시
void RosNode::on_move_done(const std::string& name, RobotState& state, const std::string& action_str) {
    if (state.current_action == IDLE) return; // 중복 수신 방지

    Action completed_action = state.current_action;
    state.current_action = IDLE;
    state.init_set = false;

    std::cout << "[DONE] " << name << " " << action_str << " 완료\n";

    // 그리드 좌표 업데이트
    if (state.row >= 0 && state.row < 7 && state.col >= 0 && state.col < 8) {
        world_map[state.row][state.col] = 0;
    }

    if (completed_action == FORWARD) {
        state.row += dr[state.heading];
        state.col += dc[state.heading];
    } else if (completed_action == BACKWARD) {
        state.row -= dr[state.heading];
        state.col -= dc[state.heading];
    } else if (completed_action == LEFT) {
        state.heading = (state.heading + 3) % 4;
    } else if (completed_action == RIGHT) {
        state.heading = (state.heading + 1) % 4;
    }

    if (state.row >= 0 && state.row < 7 && state.col >= 0 && state.col < 8) {
        world_map[state.row][state.col] = 3;
    }

    QString robotID = (name == "tb3_a") ? "A" : "B";
    emit robotPoseUpdated(robotID, state.col, state.row);

    MapArray arr;
    for (int r = 0; r < 7; ++r)
        for (int c = 0; c < 8; ++c)
            arr[r][c] = world_map[r][c];
    emit worldMapUpdated(arr);
}