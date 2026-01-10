#include "rclcpp/rclcpp.hpp"
#include "rclcpp_action/rclcpp_action.hpp"
#include "lifecycle_msgs/msg/state.hpp"

#include "plansys2_executor/ActionExecutorClient.hpp"

using namespace std::chrono_literals;

class DetectMarkerAction : public plansys2::ActionExecutorClient
{
public:
  DetectMarkerAction()
  : plansys2::ActionExecutorClient("detect_marker_action", 100ms)
  {
    progress_ = 0.0;
  }

  void do_work() override
  {
    auto arguments = get_arguments(); 
    std::string robot_name = arguments[0]; 
    std::string wp = arguments[1]; 

    progress_ += 0.02;
    send_feedback(progress_, "Detecting markers...");

    if (progress_ >= 1.0) {
      progress_ = 0.0;
      RCLCPP_INFO(get_logger(), "Detection Complete at: %s", wp.c_str());
      finish(true, 1.0, "Markers detected");
    }
  }

private:
  float progress_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<DetectMarkerAction>();
  node->set_parameter(rclcpp::Parameter("action_name", "detect_marker_action"));
  node->trigger_transition(lifecycle_msgs::msg::Transition::TRANSITION_CONFIGURE);
  rclcpp::spin(node->get_node_base_interface());
  rclcpp::shutdown();
  return 0;
}
