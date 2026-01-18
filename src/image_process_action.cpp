#include "rclcpp/rclcpp.hpp"
#include "lifecycle_msgs/msg/state.hpp"
#include "plansys2_executor/ActionExecutorClient.hpp"
#include "sensor_msgs/msg/image.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "cv_bridge/cv_bridge.hpp"
#include <opencv2/opencv.hpp>
#include <opencv2/aruco.hpp>

using namespace std::chrono_literals;

class ImageProcessAction : public plansys2::ActionExecutorClient
{
public:
  ImageProcessAction() : plansys2::ActionExecutorClient("image_processor", 100ms)
  {
    // Initialize variables
    is_active_ = false;
    is_centered_ = false;
    progress_ = 0.0;
    center_start_time_ = rclcpp::Clock().now();
    
    // Setup ROS2
    node_ = rclcpp::Node::make_shared("image_processor_node");
    
    // Setup ROS components directly
    setup_ros_components();
    
    // Start image processing in a separate thread
    processing_thread_ = std::thread(&ImageProcessAction::processing_loop, this);
    
    printf("[PROCESS] Image processor ready\n");
  }

  rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn
  on_configure(const rclcpp_lifecycle::State & previous_state)
  {
    return ActionExecutorClient::on_configure(previous_state);
  }

  ~ImageProcessAction()
  {
    // Signal thread to stop
    is_active_ = false;
    if (processing_thread_.joinable()) {
      processing_thread_.join();
    }
  }

  void do_work() override
  {
    auto arguments = get_arguments();
    
    if (arguments.size() < 2) {
      printf("[ERROR] Need robot and waypoint arguments\n");
      finish(false, 0.0, "Invalid arguments");
      return;
    }
    
    std::string robot = arguments[0];
    std::string wp = arguments[1];
    
    // First call - start image processing
    if (!is_active_) {
      printf("[PROCESS]Starting image processing at waypoint: %s\n", wp.c_str());
      is_active_ = true;
      is_centered_ = false;
      progress_ = 0.0;
      current_waypoint_ = wp;
      center_start_time_ = rclcpp::Clock().now();
      
      send_feedback(0.0, "Starting image processing at " + wp);
      return;
    }
    
    // Update progress based on centering status
    if (is_centered_) {
      auto now = rclcpp::Clock().now();
      double elapsed = (now - center_start_time_).seconds();
      progress_ = 0.5 + 0.5 * (elapsed / 2.0); // Wait 2 seconds when centered
      
      if (elapsed >= 2.0) {
        // Complete the action
        printf("[PROCESS] Image processing complete at %s\n", wp.c_str());
        finish(true, 1.0, "Image processing complete at " + wp);
        is_active_ = false;
        stop_robot();
      }
    } else {
      progress_ = std::min(0.5f, progress_ + 0.02f);
    }
    
    send_feedback(progress_, is_centered_ ? "Centered" : "Searching/Centering");
  }

private:
  void setup_ros_components()
  {
    // Setup ROS components
    image_sub_ = node_->create_subscription<sensor_msgs::msg::Image>(
      "/camera/image", 10, [this](const sensor_msgs::msg::Image::SharedPtr msg) {
        std::lock_guard<std::mutex> lock(image_mutex_);
        latest_image_ = msg;
      });
    
    cmd_vel_pub_ = node_->create_publisher<geometry_msgs::msg::Twist>("/cmd_vel", 10);
    processed_image_pub_ = node_->create_publisher<sensor_msgs::msg::Image>("/aruco/processed_image", 10);
    
    // ArUco detector setup
    dictionary_ = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_ARUCO_ORIGINAL);
    parameters_ = cv::aruco::DetectorParameters::create();
    parameters_->adaptiveThreshWinSizeMin = 3;
    parameters_->adaptiveThreshWinSizeMax = 23;
    parameters_->adaptiveThreshConstant = 7.0;
    parameters_->minMarkerPerimeterRate = 0.03;
    
  }

  void processing_loop()
  {
    while (rclcpp::ok()) {
      // Only process if action is active
      if (is_active_) {
        process_image_once();
      } else {
        // Sleep to avoid busy waiting
        std::this_thread::sleep_for(50ms);
      }
      
      // Handle callbacks
      rclcpp::spin_some(node_);
    }
  }

  void process_image_once()
  {
    sensor_msgs::msg::Image::SharedPtr msg;
    {
      std::lock_guard<std::mutex> lock(image_mutex_);
      if (!latest_image_) {
        return;
      }
      msg = latest_image_;
    }
    
    try {
      auto cv_ptr = cv_bridge::toCvCopy(msg, "bgr8");
      cv::Mat image = cv_ptr->image;
      cv::Mat display_image = image.clone();
      
      // Detect markers
      std::vector<int> marker_ids;
      std::vector<std::vector<cv::Point2f>> marker_corners;
      cv::aruco::detectMarkers(image, dictionary_, marker_corners, marker_ids, parameters_);
      
      if (!marker_ids.empty()) {
        cv::aruco::drawDetectedMarkers(display_image, marker_corners, marker_ids);
        
        // Get first marker center
        cv::Point2f center(0, 0);
        for (const auto& corner : marker_corners[0]) {
          center.x += corner.x;
          center.y += corner.y;
        }
        center.x /= 4.0;
        center.y /= 4.0;
        
        // Draw circle
        cv::circle(display_image, center, 25, cv::Scalar(0, 255, 0), 2);
        
        // Calculate error
        cv::Point2f image_center(display_image.cols / 2.0, display_image.rows / 2.0);
        double error_x = center.x - image_center.x;
        
        // Servoing (only if action is active and not centered yet)
        if (!is_centered_) {
          if (std::abs(error_x) > 10.0) {  // center_threshold = 10 pixels
            // Move to center
            auto twist = geometry_msgs::msg::Twist();
            twist.angular.z = -0.01 * error_x;  // kp_angular = 0.01
            cmd_vel_pub_->publish(twist);
          } else {
            // Centered - stop and record time
            is_centered_ = true;
            center_start_time_ = rclcpp::Clock().now();
            stop_robot();
          }
        } else {
          stop_robot();
        }
      } else {
        // No markers - rotate to search (only if not centered yet)
        if (!is_centered_) {
          auto twist = geometry_msgs::msg::Twist();
          twist.angular.z = 0.3;
          cmd_vel_pub_->publish(twist);
          printf("No markers found - searching at waypoint: %s\n", current_waypoint_.c_str());
        }
      }
      
      // Publish processed image to topic
      auto processed_msg = cv_bridge::CvImage(msg->header, "bgr8", display_image).toImageMsg();
      processed_image_pub_->publish(*processed_msg);
      
    } catch (const std::exception& e) {
      printf("Error in image processing: %s\n", e.what());
    }
  }

  void stop_robot()
  {
    auto twist = geometry_msgs::msg::Twist();
    twist.linear.x = 0.0;
    twist.angular.z = 0.0;
    cmd_vel_pub_->publish(twist);
  }

  // ROS2
  std::shared_ptr<rclcpp::Node> node_;
  std::thread processing_thread_;
  rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr image_sub_;
  rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr cmd_vel_pub_;
  rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr processed_image_pub_;
  
  // Image
  sensor_msgs::msg::Image::SharedPtr latest_image_;
  std::mutex image_mutex_;
  
  // ArUco
  cv::Ptr<cv::aruco::Dictionary> dictionary_;
  cv::Ptr<cv::aruco::DetectorParameters> parameters_;
  
  // Action state
  bool is_active_;
  bool is_centered_;
  float progress_;
  std::string current_waypoint_;
  rclcpp::Time center_start_time_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  
  auto node = std::make_shared<ImageProcessAction>();
  
  node->set_parameter(rclcpp::Parameter("action_name", "image_processor"));
  node->trigger_transition(lifecycle_msgs::msg::Transition::TRANSITION_CONFIGURE);
  node->trigger_transition(lifecycle_msgs::msg::Transition::TRANSITION_ACTIVATE);
  
  rclcpp::spin(node->get_node_base_interface());
  
  rclcpp::shutdown();
  return 0;
}