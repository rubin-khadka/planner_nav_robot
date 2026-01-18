#include "rclcpp/rclcpp.hpp"
#include "lifecycle_msgs/msg/state.hpp"
#include "plansys2_executor/ActionExecutorClient.hpp"
#include "plansys2_problem_expert/ProblemExpertClient.hpp"
#include "sensor_msgs/msg/image.hpp"
#include "cv_bridge/cv_bridge.hpp"
#include "planner_nav_robot/msg/marker_list.hpp"
#include <opencv2/opencv.hpp>
#include <opencv2/aruco.hpp>
#include <unordered_set>
#include <vector>
#include <memory>
#include <cstdlib>
#include <cstdio>

using namespace std::chrono_literals;

class DetectMarkerAction : public plansys2::ActionExecutorClient
{
public:
  DetectMarkerAction()
  : plansys2::ActionExecutorClient("detect_marker_action", 100ms)
  {
    progress_ = 0.0;
    all_reported_ = false;
    marker_detected_ = false;
    
    problem_expert_ = std::make_shared<plansys2::ProblemExpertClient>();
    
    // Initialize ArUco detector
    dictionary_ = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_ARUCO_ORIGINAL);
    parameters_ = cv::aruco::DetectorParameters::create();
    
    // Create image subscriber
    image_sub_ = create_subscription<sensor_msgs::msg::Image>(
      "/camera/image", 10,
      std::bind(&DetectMarkerAction::image_callback, this, std::placeholders::_1));
      
    // Create publisher for raw marker data
    marker_pub_ = create_publisher<planner_nav_robot::msg::MarkerList>(
      "/markers", 10); 
    
    printf("[DETECT] Detection node initialized\n");
  }

  void do_work() override
  {
    auto arguments = get_arguments(); 
    
    if (arguments.size() < 2) {
      finish(false, 0.0, "Invalid arguments");
      return;
    }
    
    current_waypoint_ = arguments[1];  // Store current waypoint

    printf("[DETECT] Detected at: %s\n", current_waypoint_.c_str());

    // Check if already detected at this waypoint
    if (detected_waypoints_.find(current_waypoint_) != detected_waypoints_.end()) {
      printf("[SKIP] Already visited %s\n", current_waypoint_.c_str());
      finish(true, 1.0, "Already visited " + current_waypoint_);
      return;
    }

    // Try to detect marker
    if (!marker_detected_) {
      send_feedback(progress_, "Detecting at " + current_waypoint_ + "...");
      
      // Check if any markers detected
      std::lock_guard<std::mutex> lock(mutex_);
      if (!current_marker_ids_.empty()) {
        marker_detected_ = true;
         
        // Store each detected marker IN ORDER OF DETECTION
        for (int id : current_marker_ids_) {
          printf("[DETECT]   ID: %d\n", id);
          
          // Add to our arrays in the order they come
          marker_ids_.push_back(id);
          waypoint_names_.push_back(current_waypoint_);
        }
        
        // Mark waypoint as visited
        detected_waypoints_.insert(current_waypoint_);
        
        // Publish all the data
        if (detected_waypoints_.size() >= 4 && !all_reported_) {
          publish_markers();  // Changed function name
          report_all_detected();
          all_reported_ = true;
        }
      } else {
        // Still trying
        progress_ = std::min(progress_ + 0.05, 0.95);
        printf("[DETECT] Scanning... (%.0f%%)\n", progress_ * 100);
      }
    }
    
    if (marker_detected_) {
      // Reset for next detection
      marker_detected_ = false;
      progress_ = 0.0;
      {
        std::lock_guard<std::mutex> lock(mutex_);
        current_marker_ids_.clear();
      }
      
      finish(true, 1.0, "Marker detected at " + current_waypoint_);
    }
  }

private:
  void publish_markers()  // NO SORTING HERE!
  {
    if (marker_ids_.empty()) {
      printf("[ERROR] No markers to publish!\n");
      return;
    }
    
    auto msg = planner_nav_robot::msg::MarkerList();
    msg.marker_ids = marker_ids_;        
    msg.waypoint_names = waypoint_names_;  
    
    // Publish raw data
    marker_pub_->publish(msg);
    
    printf("\n[PUBLISH] ========================================\n");
    
    for (size_t i = 0; i < marker_ids_.size(); i++) {
      printf("[PUBLISH] Detection %zu: ID %d at %s\n", 
             i + 1, marker_ids_[i], waypoint_names_[i].c_str());
    }
    
    printf("[PUBLISH] ========================================\n\n");
    
  }

  void image_callback(const sensor_msgs::msg::Image::SharedPtr msg)
  {
    try {
      cv_bridge::CvImagePtr cv_ptr = cv_bridge::toCvCopy(msg, "bgr8");
      cv::Mat image = cv_ptr->image;
      
      // Detect markers
      std::vector<int> marker_ids;
      std::vector<std::vector<cv::Point2f>> marker_corners;
      cv::aruco::detectMarkers(image, dictionary_, marker_corners, marker_ids, parameters_);
      
      // Store detected markers
      {
        std::lock_guard<std::mutex> lock(mutex_);
        current_marker_ids_ = marker_ids;
      }
      
      if (!marker_ids.empty()) {
        // Draw simple rectangles around markers
        for (size_t i = 0; i < marker_corners.size(); i++) {
          // Draw rectangle
          cv::rectangle(image, marker_corners[i][0], marker_corners[i][2], 
                       cv::Scalar(0, 255, 0), 2);
          
          // Put ID text at top-right of rectangle
          std::string id_text = "ID: " + std::to_string(marker_ids[i]);
          cv::putText(image, id_text, 
                     cv::Point(marker_corners[i][1].x + 10, marker_corners[i][1].y - 10),
                     cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 0), 2);
        }
      }
      
      cv::imshow("Marker Detection", image);
      cv::waitKey(1);
      
    } catch (const std::exception& e) {
      printf("[ERROR] Image error: %s\n", e.what());
    }
  }

  void report_all_detected()
  { 
    std::string robot_name = "planner_robot";
    std::string goal_str = "(and (robot_at " + robot_name + " wp_start))";
    
    try {
      problem_expert_->setGoal(plansys2::Goal(goal_str));
      printf("[PLAN] New goal set: return to wp_start\n");
      
      std::this_thread::sleep_for(500ms);
      run_controller();
      
    } catch (const std::exception& e) {
      printf("[ERROR] %s\n", e.what());
    }
  }

  void run_controller()
  {
    std::string command = "ros2 run planner_nav_robot get_plan_and_execute &";
    std::system(command.c_str());
  }

  // Variables
  float progress_;
  bool all_reported_;
  bool marker_detected_;
  std::mutex mutex_;
  std::vector<int> current_marker_ids_;
  std::string current_waypoint_;
  
  std::unordered_set<std::string> detected_waypoints_;
  std::vector<int> marker_ids_;           
  std::vector<std::string> waypoint_names_;
  
  // ArUco
  cv::Ptr<cv::aruco::Dictionary> dictionary_;
  cv::Ptr<cv::aruco::DetectorParameters> parameters_;
  
  // ROS2
  std::shared_ptr<plansys2::ProblemExpertClient> problem_expert_;
  rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr image_sub_;
  rclcpp::Publisher<planner_nav_robot::msg::MarkerList>::SharedPtr marker_pub_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  
  auto node = std::make_shared<DetectMarkerAction>();
  
  node->set_parameter(rclcpp::Parameter("action_name", "detect_marker_action"));
  node->trigger_transition(lifecycle_msgs::msg::Transition::TRANSITION_CONFIGURE);
  node->trigger_transition(lifecycle_msgs::msg::Transition::TRANSITION_ACTIVATE);
  
  rclcpp::spin(node->get_node_base_interface());
  
  rclcpp::shutdown();

  return 0;
}