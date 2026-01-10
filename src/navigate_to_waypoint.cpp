#include "rclcpp/rclcpp.hpp"
#include "rclcpp_action/rclcpp_action.hpp"
#include "lifecycle_msgs/msg/state.hpp"

#include "plansys2_executor/ActionExecutorClient.hpp"

using namespace std::chrono_literals;

class NavigateToWaypointAction : public plansys2::ActionExecutorClient
{
public:
  NavigateToWaypointAction()
  : plansys2::ActionExecutorClient("navigate_to_waypoint", 100ms)
  {
    progress_ = 0.0;
  }

  void do_work() override
  {
    auto arguments = get_arguments(); 
    std::string robot_name = arguments[0]; 
    std::string from_wp = arguments[1]; 
    std::string to_wp = arguments[2];
    progress_ += 0.1;
    send_feedback(progress_, "Navigating...");

    RCLCPP_INFO(get_logger(), "Navigation progress: %.0f%%", progress_ * 100);

    if (progress_ >= 1.0) {
      progress_ = 0.0;
      finish(true, 1.0, "Navigation completed");
      RCLCPP_INFO(get_logger(), "Navigation complete: %s -> %s", from_wp.c_str(), to_wp.c_str());
      return;
    }
  }

private:
  float progress_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);

  auto node = std::make_shared<NavigateToWaypointAction>();

  node->set_parameter(rclcpp::Parameter("action_name", "navigate_to_waypoint"));
  node->trigger_transition(lifecycle_msgs::msg::Transition::TRANSITION_CONFIGURE);

  rclcpp::spin(node->get_node_base_interface());

  rclcpp::shutdown();
  return 0;
}
