#include "plansys2_executor/ActionExecutorClient.hpp"
#include "rclcpp/rclcpp.hpp"
#include "rclcpp_action/rclcpp_action.hpp"
#include "nav2_msgs/action/navigate_to_pose.hpp"
#include "lifecycle_msgs/msg/transition.hpp"
#include "nav_msgs/msg/odometry.hpp"
#include "plansys2_problem_expert/ProblemExpertClient.hpp"

#include <tf2/LinearMath/Quaternion.h>                
#include <tf2_geometry_msgs/tf2_geometry_msgs.hpp> 

#include <string>
#include <memory>
#include <mutex>
#include <unordered_set>
#include <cstdlib>
#include <cstdio> 

using namespace std::chrono_literals;

class NavigateToWaypointAction : public plansys2::ActionExecutorClient
{
public:
  NavigateToWaypointAction()
  : plansys2::ActionExecutorClient("navigate_to_waypoint", 100ms),
    goal_sent_(false),
    navigation_complete_(false),
    navigation_success_(false),
    odom_received_(false),
    nav2_client_(nullptr)
  {
    problem_expert_ = std::make_shared<plansys2::ProblemExpertClient>();

    odom_sub_ = this->create_subscription<nav_msgs::msg::Odometry>(
      "/odom", 10,
      [this](const nav_msgs::msg::Odometry::SharedPtr msg) {
        std::lock_guard<std::mutex> lock(odom_mutex_);
        current_odom_ = *msg;
        odom_received_ = true;
      });
  }

  rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn
  on_configure(const rclcpp_lifecycle::State & previous_state)
  {
    nav2_client_ =
      rclcpp_action::create_client<nav2_msgs::action::NavigateToPose>(
        shared_from_this(), "navigate_to_pose");

    return ActionExecutorClient::on_configure(previous_state);
  }

private:
  void do_work() override
  {
    if (goal_sent_ && !navigation_complete_) {
      send_feedback(0.5, "Navigating to " + target_wp_);
      return;
    }

    if (navigation_complete_) {
      if (navigation_success_) {
        printf("[OK] Process Completed in: %s\n", target_wp_.c_str());
        visited_waypoints_.insert(target_wp_);
        check_all_conditions_met();
        finish(true, 1.0, "Arrived at " + target_wp_);
      } else {
        printf("[FAIL] Navigation failed to %s\n", target_wp_.c_str());
        finish(false, 0.0, "Navigation failed");
      }
      reset_state();
      return;
    }

    auto args = get_arguments();
    if (args.size() != 3) {
      finish(false, 0.0, "Invalid arguments");
      return;
    }

    std::string robot_name = args[0];
    std::string from_wp = args[1];
    std::string to_wp = args[2];

    double goal_x, goal_y, yaw;  

    if (to_wp == "wp1") {
      goal_x = -6.0; goal_y = -6.0; yaw = -2*M_PI/3;
    } else if (to_wp == "wp2") {
      goal_x = -6.0; goal_y = 6.0; yaw = 2*M_PI/3;
    } else if (to_wp == "wp3") {
      goal_x = 6.0; goal_y = -6.0; yaw = -M_PI/3;
    } else if (to_wp == "wp4") {
      goal_x = 6.0; goal_y = 6.0; yaw = M_PI/3;
    } else if (to_wp == "wp_start") {
      goal_x = 0.0; goal_y = 1.0; yaw = M_PI;
    } else {
      printf("[ERROR] Unknown waypoint: %s\n", to_wp.c_str());
      finish(false, 0.0, "Unknown waypoint");
      return;
    }

    target_wp_ = to_wp;
    printf("[NAV] Navigating to %s\n", target_wp_.c_str());

    nav2_msgs::action::NavigateToPose::Goal goal_msg;
    goal_msg.pose.header.frame_id = "map";
    goal_msg.pose.header.stamp = now();
    goal_msg.pose.pose.position.x = goal_x;
    goal_msg.pose.pose.position.y = goal_y;

    tf2::Quaternion q;
    q.setRPY(0.0, 0.0, yaw);
    q.normalize();
    goal_msg.pose.pose.orientation = tf2::toMsg(q);

    auto options =
      rclcpp_action::Client<nav2_msgs::action::NavigateToPose>::SendGoalOptions();

    options.result_callback =
      [this](const rclcpp_action::ClientGoalHandle<
             nav2_msgs::action::NavigateToPose>::WrappedResult & result) {
        navigation_complete_ = true;
        navigation_success_ =
          (result.code == rclcpp_action::ResultCode::SUCCEEDED);
      };

    nav2_client_->async_send_goal(goal_msg, options);

    goal_sent_ = true;
    send_feedback(0.1, "Started navigation to " + target_wp_);
  }

  void check_all_conditions_met()
  {
    bool all_wps =
      visited_waypoints_.count("wp1") &&
      visited_waypoints_.count("wp2") &&
      visited_waypoints_.count("wp3") &&
      visited_waypoints_.count("wp4") &&
      visited_waypoints_.count("wp_start");

    if (all_wps && !all_markers_reported_) {
      printf("\n[PLAN] ========================================\n");
      printf("[PLAN] All waypoints visited! \n");
      printf("[PLAN] Setting Predicated: all_markers_detected \n");
      printf("[PLAN] Setting Goal: (and (sorting_complete planner_robot))\n");
      printf("[PLAN] Starting new planning cycle...\n");
      printf("[PLAN] ========================================\n");
      
      problem_expert_->addPredicate(
        plansys2::Predicate("(all_markers_detected"));
      problem_expert_->setGoal(
        plansys2::Goal("(and (sorting_complete planner_robot))"));
      
      all_markers_reported_ = true;
      run_controller();
    }
  }

  void run_controller()
  {
    std::system("ros2 run planner_nav_robot get_plan_and_execute &");
  }

  void reset_state()
  {
    goal_sent_ = false;
    navigation_complete_ = false;
    navigation_success_ = false;
    target_wp_.clear();
  }

  bool goal_sent_;
  bool navigation_complete_;
  bool navigation_success_;
  std::string target_wp_;

  std::unordered_set<std::string> visited_waypoints_;
  bool all_markers_reported_ = false;

  nav_msgs::msg::Odometry current_odom_;
  rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr odom_sub_;
  std::mutex odom_mutex_;
  bool odom_received_;

  rclcpp_action::Client<nav2_msgs::action::NavigateToPose>::SharedPtr nav2_client_;
  std::shared_ptr<plansys2::ProblemExpertClient> problem_expert_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);

  auto node = std::make_shared<NavigateToWaypointAction>();

  node->set_parameter(rclcpp::Parameter("action_name", "navigate_to_waypoint"));

  node->trigger_transition(lifecycle_msgs::msg::Transition::TRANSITION_CONFIGURE);
  node->trigger_transition(lifecycle_msgs::msg::Transition::TRANSITION_ACTIVATE);

  
  rclcpp::spin(node->get_node_base_interface());
  rclcpp::shutdown();
  
  return 0;
}