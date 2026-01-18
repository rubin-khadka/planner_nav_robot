#include "rclcpp/rclcpp.hpp"
#include "lifecycle_msgs/msg/state.hpp"
#include "plansys2_executor/ActionExecutorClient.hpp"
#include "planner_nav_robot/msg/marker_list.hpp"
#include <vector>
#include <algorithm>
#include <memory>
#include <cstdlib>  
#include <cstdio>   
#include <thread>  
#include <mutex>

using namespace std::chrono_literals;

class SortMarkersAction : public plansys2::ActionExecutorClient
{
public:
  SortMarkersAction()
  : plansys2::ActionExecutorClient("sort_markers", 100ms)
  {
    markers_received_ = false;
    
    // Create subscriber for raw markers
    raw_markers_sub_ = this->create_subscription<planner_nav_robot::msg::MarkerList>(
      "/markers", 10,
      std::bind(&SortMarkersAction::raw_markers_callback, this, std::placeholders::_1));
    
    printf("[SORT] Sorting node initialized\n");
    printf("[SORT] Waiting for raw markers on /raw_markers topic\n");
  }

  rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn
  on_configure(const rclcpp_lifecycle::State & previous_state)
  {
    // Initialize problem_expert_ in on_configure
    problem_expert_ = std::make_shared<plansys2::ProblemExpertClient>();
    
    return ActionExecutorClient::on_configure(previous_state);
  }

  void do_work() override
  {
    // Check if problem_expert_ is initialized
    if (!problem_expert_) {
      finish(false, 0.0, "ProblemExpert not initialized");
      return;
    }
    
    auto arguments = get_arguments(); 
    
    if (arguments.size() < 1) {
      finish(false, 0.0, "Invalid arguments");
      return;
    }
    
    std::string robot_name = arguments[0]; 

    // Check if we've received markers yet
    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (!markers_received_) {
        send_feedback(0.0, "Waiting for marker data...");
        printf("[SORT] Waiting for detection node to publish markers...\n");
        return;  // Don't finish, keep waiting
      }
    }

    // Send feedback that we're starting
    send_feedback(0.5, "Sorting markers by ID...");
    
    std::vector<std::pair<int, std::string>> markers_to_sort;
    
    // Get the raw markers data
    {
      std::lock_guard<std::mutex> lock(mutex_);
      printf("[SORT] ========================================\n");
      printf("[SORT] Received raw data from detection node:\n");
      printf("[SORT] Total markers: %zu\n", raw_marker_ids_.size());
      
      for (size_t i = 0; i < raw_marker_ids_.size(); i++) {
        printf("[SORT]   Raw: ID %d at %s\n", 
               raw_marker_ids_[i], raw_waypoint_names_[i].c_str());
        markers_to_sort.push_back({raw_marker_ids_[i], raw_waypoint_names_[i]});
      }
    }
    
    // Sort by ID (lowest first)
    std::sort(markers_to_sort.begin(), markers_to_sort.end(),
      [](const auto& a, const auto& b) { return a.first < b.first; });
    
    printf("[SORT] ========================================\n");
    printf("[SORT] Sorted markers (lowest ID first):\n");
    
    for (size_t i = 0; i < markers_to_sort.size(); i++) {
      printf("[SORT]   %zu. ID %d at %s\n", 
             i + 1, markers_to_sort[i].first, markers_to_sort[i].second.c_str());
    }
    printf("[SORT] ========================================\n");
    
    // Connect them in sorted order
    connect_markers_in_order(markers_to_sort);
    
    // Send final feedback
    send_feedback(1.0, "Markers sorted and connected");
    
    // SHUTDOWN THE NAVIGATION AND DETECTION NODES
    shutdown_navigation_and_detection_nodes();
    
    finish(true, 1.0, "Markers connected in sorted order");

    run_controller();
  }

private:
  void raw_markers_callback(const planner_nav_robot::msg::MarkerList::SharedPtr msg)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Store the received data
    raw_marker_ids_ = msg->marker_ids;
    raw_waypoint_names_ = msg->waypoint_names;
    markers_received_ = true;
    
    printf("\n[SORT] ========================================\n");
    printf("[SORT] RECEIVED RAW MARKER DATA!\n");
    printf("[SORT] Topic: /raw_markers\n");
    printf("[SORT] Total markers: %zu\n", raw_marker_ids_.size());
    
    for (size_t i = 0; i < raw_marker_ids_.size(); i++) {
      printf("[SORT]   Marker %d at waypoint %s\n", 
             raw_marker_ids_[i], raw_waypoint_names_[i].c_str());
    }
    
    printf("[SORT] Ready to sort when action is triggered\n");
    printf("[SORT] ========================================\n\n");
  }

  void connect_markers_in_order(const std::vector<std::pair<int, std::string>>& markers)
  {
    try {
      
      // Assuming robot is at wp_start after exploration
      std::string current = "wp_start";
      
      printf("[CONNECT] Creating connections...\n");
      
      for (size_t i = 0; i < markers.size(); i++) {
        std::string next_wp = markers[i].second;
        
        std::string connection = "(connected " + current + " " + next_wp + ")";
        bool success = problem_expert_->addPredicate(plansys2::Predicate(connection));
        
        if (success) {
          printf("[CONNECT]  %s → %s\n", current.c_str(), next_wp.c_str());
        } else {
          printf("[CONNECT]  Failed: %s → %s\n", current.c_str(), next_wp.c_str());
        }
        
        current = next_wp; 
      }

      printf("[REMOVE] Removing navigation_enabled predicate...\n");
      bool success = problem_expert_->removePredicate(plansys2::Predicate("(navigation_enabled planner_robot)"));

      if (success) {
        printf("[REMOVE] Successfully removed predicate\n");
      } else {
        printf("[REMOVE] Failed to remove predicate\n");
      }
      
      // Set goal to process all images
      std::string goal = "(and (image_processed planner_robot wp1) (image_processed planner_robot wp2) (image_processed planner_robot wp3) (image_processed planner_robot wp4))";
      
      try {
        problem_expert_->setGoal(plansys2::Goal(goal));
        printf("[GOAL]   Goal set: process all images\n");
      } catch (const std::exception& e) {
        printf("[GOAL]   Failed to set goal: %s\n", e.what());
      }
      
    } catch (const std::exception& e) {
      printf("[ERROR] Exception in connect_markers_in_order: %s\n", e.what());
    }
  }

  void shutdown_navigation_and_detection_nodes()
  {
    std::system("pkill -f 'navigate_to_waypoint'");
    std::system("pkill -f 'detect_marker_action'");
    
    std::this_thread::sleep_for(100ms);
    
    int verify_nav = std::system("pgrep -f 'navigate_to_waypoint'");
    int verify_detect = std::system("pgrep -f 'detect_marker_action'");
    
    if (verify_nav != 0 && verify_detect != 0) {
    } else {
      std::system("pkill -9 -f 'navigate_to_waypoint'");
      std::system("pkill -9 -f 'detect_marker_action'");
    }
    
  }

  void run_controller()
  {
    std::system("ros2 run planner_nav_robot get_plan_and_execute &");
  }

  bool markers_received_;
  std::mutex mutex_;
  
  // Store raw marker data
  std::vector<int> raw_marker_ids_;
  std::vector<std::string> raw_waypoint_names_;
  
  std::shared_ptr<plansys2::ProblemExpertClient> problem_expert_;
  rclcpp::Subscription<planner_nav_robot::msg::MarkerList>::SharedPtr raw_markers_sub_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  
  auto node = std::make_shared<SortMarkersAction>();
  
  node->set_parameter(rclcpp::Parameter("action_name", "sort_markers"));
  node->trigger_transition(lifecycle_msgs::msg::Transition::TRANSITION_CONFIGURE);
  node->trigger_transition(lifecycle_msgs::msg::Transition::TRANSITION_ACTIVATE);
  
  
  rclcpp::spin(node->get_node_base_interface());
  
  rclcpp::shutdown();
  return 0;
}