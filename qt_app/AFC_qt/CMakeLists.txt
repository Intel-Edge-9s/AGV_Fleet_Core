cmake_minimum_required(VERSION 3.8)
project(AFC_qt)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Widgets Core Gui)
find_package(geometry_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(rclcpp_action REQUIRED)
find_package(nav2_msgs REQUIRED)
find_package(tf2 REQUIRED)
# 사용할 Qt 컴포넌트
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Sql
    Charts
)

# Set Qt
set(CMAKE_AUTOMOC on)
set(CMAKE_AUTORCC on)
set(CMAKE_AUTOUIC on)

#============ turtlebot_node ==============
# Build
add_executable(turtlebot_node
  src/main.cpp
  src/mainwidget.cpp
  src/mainwidget.h
  src/mainwidget.ui
  src/tab1.h
  src/tab1.cpp
  src/tab1.ui
  src/tab2.h
  src/tab2.cpp
  src/tab2.ui
  src/rosnode.h
  src/rosnode.cpp
  src/pathfinding.h
  src/pathfinding.cpp
  src/simulation.h
  src/simulation.cpp
  src/navi.h
  src/navi.cpp
  src/mapwidget.h src/mapwidget.cpp
  src/lidarwidget.h src/lidarwidget.cpp
)

ament_target_dependencies(turtlebot_node
  rclcpp
  std_msgs
  geometry_msgs
  sensor_msgs
  rclcpp_action
  nav2_msgs
  tf2
)

# include 폴더를 삭제했으므로 src 폴더만 포함 경로로 설정합니다.
target_include_directories(turtlebot_node PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Qt6 라이브러리 링크
target_link_libraries(turtlebot_node
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Sql
    Qt6::Charts
)


# install
install(TARGETS
  turtlebot_node
  DESTINATION lib/${PROJECT_NAME}
)

# test
if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  set(ament_cmake_copyright_FOUND TRUE)
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()