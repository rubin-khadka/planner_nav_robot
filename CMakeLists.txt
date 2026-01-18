cmake_minimum_required(VERSION 3.8)
project(planner_nav_robot)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(OpenCV REQUIRED)
find_package(cv_bridge REQUIRED)
find_package(rclcpp REQUIRED)
find_package(plansys2_pddl_parser REQUIRED)
find_package(plansys2_msgs REQUIRED)
find_package(plansys2_domain_expert REQUIRED)
find_package(plansys2_planner REQUIRED)
find_package(plansys2_problem_expert REQUIRED)
find_package(plansys2_executor REQUIRED)
find_package(rclcpp_action REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(nav2_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(image_transport REQUIRED)
find_package(tf2 REQUIRED)
find_package(tf2_geometry_msgs REQUIRED)
find_package(std_msgs REQUIRED)

find_package(rosidl_default_generators REQUIRED)

rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/MarkerList.msg"  
)

rosidl_get_typesupport_target(cpp_typesupport_target
  ${PROJECT_NAME} rosidl_typesupport_cpp
)

ament_export_dependencies(rosidl_default_runtime)

include_directories(
  include
)
# uncomment the following section in order to fill in
# further dependencies manually.
# find_package(<dependency> REQUIRED)


add_executable(get_plan_and_execute src/get_plan_and_execute.cpp)
add_executable(navigate_to_waypoint src/navigate_to_waypoint.cpp)
add_executable(navigate_to_marker src/navigate_to_marker.cpp)
add_executable(detect_marker_action src/detect_marker_action.cpp)
add_executable(image_process_action src/image_process_action.cpp)
add_executable(sorting_marker_action src/sorting_marker_action.cpp)

ament_target_dependencies(get_plan_and_execute
  rclcpp
  plansys2_pddl_parser
  plansys2_msgs
  plansys2_domain_expert
  plansys2_planner
  plansys2_problem_expert
  plansys2_executor
  rclcpp_action
)

ament_target_dependencies(navigate_to_waypoint
  rclcpp
  rclcpp_action
  plansys2_pddl_parser
  plansys2_executor
  plansys2_problem_expert 
  plansys2_msgs
  geometry_msgs
  nav2_msgs
  lifecycle_msgs
  tf2
  tf2_geometry_msgs
)

ament_target_dependencies(navigate_to_marker
  rclcpp
  rclcpp_action
  plansys2_pddl_parser
  plansys2_executor
  plansys2_problem_expert 
  plansys2_msgs
  geometry_msgs
  nav2_msgs
  lifecycle_msgs
  tf2
  tf2_geometry_msgs
)

ament_target_dependencies(detect_marker_action
  rclcpp
  rclcpp_action
  plansys2_pddl_parser
  plansys2_problem_expert
  plansys2_executor
  plansys2_msgs
  geometry_msgs
  nav2_msgs
  lifecycle_msgs
  sensor_msgs
  cv_bridge
  image_transport
  OpenCV
)

ament_target_dependencies(image_process_action
  rclcpp
  rclcpp_action
  plansys2_pddl_parser
  plansys2_problem_expert
  plansys2_executor
  plansys2_msgs
  geometry_msgs
  nav2_msgs
  lifecycle_msgs
  sensor_msgs
  cv_bridge
  image_transport
  OpenCV
)

ament_target_dependencies(sorting_marker_action
  rclcpp
  rclcpp_action
  plansys2_pddl_parser
  plansys2_problem_expert
  plansys2_executor
  plansys2_msgs
  geometry_msgs
  nav2_msgs
  lifecycle_msgs
  std_msgs    
  rosidl_default_runtime 
)

target_link_libraries(detect_marker_action
  ${cpp_typesupport_target}
)

target_link_libraries(sorting_marker_action
  ${cpp_typesupport_target}
)

install(TARGETS
  get_plan_and_execute
  navigate_to_waypoint
  navigate_to_marker
  detect_marker_action
  image_process_action
  sorting_marker_action
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY
  config
  launch
  rviz
  worlds
  urdf
  maps
  meshes
  src
  pddl
  msg
DESTINATION share/${PROJECT_NAME}
)


if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  # the following line skips the linter which checks for copyrights
  # comment the line when a copyright and license is added to all source files
  set(ament_cmake_copyright_FOUND TRUE)
  # the following line skips cpplint (only works in a git repo)
  # comment the line when this package is in a git repo and when
  # a copyright and license is added to all source files
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
