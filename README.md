# Experimental Robotics Laboratory - Assignment 2

### Authors

**Rubin Khadka Chhetri**  
`Student ID: 6558048`  
[![GitHub](https://img.shields.io/badge/GitHub-Profile-181717?logo=github)](https://github.com/rubin-khadka)

**Sarvenaz Ashoori**  
`Student ID: 6878764`  
[![GitHub](https://img.shields.io/badge/GitHub-Profile-181717?logo=github)](https://github.com/sarvenazrobotics)

**Abdul Hayee Hafiz**  
`Student ID: 6029926`  
[![GitHub](https://img.shields.io/badge/GitHub-Profile-181717?logo=github)](https://github.com/abdulhayee181)

## Table of Contents
- [Introduction](#introduction)
- [Video Demonstrations](#video-demonstrations)
- [Getting Started](#getting-started-read-before-action)
    - [Prerequisites](#prerequisites)
    - [Setup](#setup)
- [Launching the System](#launching-the-system)

## Introduction

The Robot we created was not working properly with nav2 so we decided to take Robot Model and Navigation code from: [MOGI-ROS/Week-7-8-Gazebo-basics](https://github.com/MOGI-ROS/Week-7-8-ROS2-Navigation)

## Video Demonstrations

### Phase 1: Navigate and Detect all Marker (High Quality)

https://github.com/user-attachments/assets/64d0b4f2-a5a4-4002-950c-c4ea55feb494 

The robot explores the environment and detects all ArUco markers.

### Phase 2: Return to Starting Point and Sort the Received Markers (High Quality)

https://github.com/user-attachments/assets/40baffac-e5bf-4ac1-96e7-4bd717320511

The robot returns to the starting point and sorts detected markers.

### Phase 3: Navigate to Sorted Marker and Process Image (High Quality)

https://github.com/user-attachments/assets/d88f0765-4c75-4fda-be30-225bf08e76a2 

The robot navigates to specific markers based on the sorting results.

### Complete Working Video (Low Quality)

https://github.com/user-attachments/assets/3a609d2a-ccf2-46d2-949c-0fa9011dc9a9 

Full workflow demonstration (lower video quality).

### Demonstration of Robot Moving in Gazebo and Nav2

https://github.com/user-attachments/assets/21f5cb7f-b785-42d2-b51c-ce0586326b63

Robot navigation and path planning in Gazebo simulation.

## Getting Started (Read Before Action)

### Prerequisites
---
Before proceeding, make sure that **`ROS2 Jazzy`** is installed on your system.<br>
If you haven’t set up ROS2 yet, refer to the official installation guide for ROS2 Jazzy on Ubuntu:<br>
[Install ROS2 Jazzy](https://docs.ros.org/en/jazzy/Installation.html) <br>

**Additional Required Packages:**
- Gazebo
- OpenCV (with ArUco module)
- `ros_gz_bridge`
- `cv_bridge`
- `robot_state_publisher`
- `robot_localization`
- `PlanSys2`
- `nav2`

### Setup 
---
#### 1. Set up your ROS workspace
Create a new workspace (or use an existing one) and navigate to its `src` directory:
```bash
mkdir -p ~/planner_ws/src
cd ~/planner_ws/src
```

#### 2. Clone this repository
Clone this repository into your workspace’s `src` folder:
```bash
git clone https://github.com/rubin-khadka/planner_nav_robot.git
```

#### 3. Build the workspace
Navigate back to the root of your workspace and build the packages using `colcon build`:
```bash
cd ~/planner_ws
colcon build
```

#### 4. Source the workspace
After building, source the workspace manually for the first time in the current terminal session:
```bash
source ~/planner_ws/install/setup.bash
```

#### 5. Add the Workspace to your ROS Environment
To ensure that your workspace is sourced automatically every time you start a new terminal session, add it to your `.bashrc` file:
```bash
echo "source ~/planner_ws/install/setup.bash" >> ~/.bashrc
source ~/.bashrc
```

## Launching the System

### Launch the Robot with Gazebo Environment
---
```bash
ros2 launch planner_nav_robot spawn_robot.launch.py 
```
### Launch Navigation Stack
---
```bash
ros2 launch planner_nav_robot navigation.launch.py 
```
### Launch all necessary nodes
---
```bash
ros2 launch planner_nav_robot plansys_plan.launch.py
```

**Terminal Output:** <br>
- Terminal 1: Gazebo window with robot in environment
- Terminal 2: RViz window showing navigation data
- Terminal 3: PlanSys2 planner initialization and action execution logs

## Implementation Details

We have separated the implementation into three distinct operational phases, each with specific objectives and behaviors:

### Phase 1: Exploration and Marker Detection
---
The waypoints the robot must navigate are predefined and known. The navigation environment is also known in advance, allowing us to use `SLAM` to build a map beforehand. The pre-built map can be found in the `/maps` folder. In this phase, the goal of the plan is to detect all markers.

**Key Components in Phase 1:**
- **Pre-built Map**: A previously generated `SLAM` map stored in `/maps` containing the complete environment layout
- **Known Waypoints**: Predefined navigation points strategically placed for optimal marker detection coverage
- **Static Environment**: Consistent world configuration that remains unchanged during operation

This phase utilizes three primary nodes that work in coordination:

#### 1. `navigate_to_waypoint`
- **Function**: Responsible for executing navigation commands to reach predefined waypoints
- **Implementation**: 
  - Interfaces with the `nav2` navigation stack for path planning and execution
  - Utilizes the pre-built map for accurate localization and collision-free navigation
  - Communicates with the `BT Navigator` to execute complex navigation behaviors
  - Publishes real-time feedback on navigation progress, completion status

#### 2. `detect_marker_action`
- **Function**: Systematically scans and identifies ArUco markers at each visited waypoint
- **Implementation**:
  - Subscribes to `camera/image` topics for continuous visual input during waypoint visits
  - Employs OpenCV's ArUco module for robust marker detection and identification
  - Publishes detected markers along with their corresponding waypoint locations to the `/markers` topic
  - Implements duplicate filtering to prevent redundant marker recordings

#### 3. `get_plan_and_execute`
- **Function**: Orchestrates the sequential execution of the exploration and detection mission
- **Implementation**:
  - Initializes and maintains communication with the `PlanSys2` planning framework
  - Loads and validates PDDL domain and problem definitions specific to Phase 1 objectives
  - Requests optimized exploration plans from the planner based on current mission state
  - Coordinates synchronous execution between navigation and detection actions
  - Monitors plan progress and handles execution status reporting

**Workflow in Phase 1:**
1. The `get_plan_and_execute` node initializes and loads the exploration and detection plan from PlanSys2
2. It sequentially commands `navigate_to_waypoint` to move the robot to each predefined waypoint
3. Upon reaching each waypoint, `detect_marker_action` activates to perform a comprehensive marker scan
4. All detected markers are logged with their IDs and corresponding waypoint information
5. This iterative process continues until all designated waypoints have been visited
6. Upon completion, the complete set of detected markers and their associated waypoints are published to the `/markers` topic
7. The `all_marker_detected` problem predicate is updated to reflect successful completion
8. The system sets the `sortting_complete` goal, triggering the transition to Phase 2

### Phase 2: Return to Start and Marker Sorting
---
This phase initiates after successful completion of Phase 1. The primary objectives are to navigate the robot back to its starting position and systematically sort all detected markers in ascending order based on their IDs.

**Key Components in Phase 2:**
- **Starting Position**: Predefined origin coordinates where the robot must return to
- **Detected Marker Database**: Complete set of markers collected in Phase 1, published on the `/markers` topic

This phase employs two specialized nodes working in conjunction with the continued orchestration of the planning system:

#### 1. `navigate_to_waypoint` (Reused from Phase 1)
- This is the same navigation node used in Phase 1
- **Adaptation for Phase 2**:
  - Receives the starting position coordinates as the target waypoint
  - Utilizes the same pre-built map and `nav2` stack for efficient path planning
  - Provides confirmation upon successful arrival at the starting position

#### 2. `sorting_marker_action`
- **Function**: Processes and organizes all detected markers in ascending numerical order
- **Implementation**:
  - Subscribes to the `/markers` topic to access the complete dataset of markers detected in Phase 1
  - Parses marker information including IDs and waypoint associations
  - Implements an ascending sorting algorithm based on marker IDs

#### 3. `get_plan_and_execute` (Continued Orchestration)
- **Function**: Manages the sequential execution of Phase 2 objectives
- **Implementation**:
  - Sets new planning goals focused on return navigation and sorting
  - Coordinates the sequential execution of return navigation followed by marker sorting
  - Monitors completion of both actions before transitioning to Phase 3

**Workflow in Phase 2:**
1. Upon Phase 1 completion, `get_plan_and_execute` updates the planning system with new Phase 2 objectives
2. The node commands `navigate_to_waypoint` to navigate the robot back to the predefined starting position
3. Concurrently, `sorting_marker_action` begins subscribing to the `/markers` topic to access the collected marker data
4. Once the robot reaches the starting position, the sorting algorithm activates
5. After sorting is complete, the system dynamically updates the PDDL problem file with connected waypoint relationships that enforce sequential navigation in Phase 3:
   - Adds `connected` predicates between waypoints in sorted order (e.g., `connected wp_start wp4`, `connected wp4 wp1`, `connected wp1 wp2`, etc.)
   - These connections ensure the robot will navigate through markers in ascending ID order from lowest to highest
6. The goal for Phase 3 is established: process all markers

**Waypoint Connection Strategy:**
- The sorted marker list determines the sequence of waypoint connections
- Starting waypoint (`wp_start`) is connected to the waypoint of the lowest ID marker
- Each subsequent marker's waypoint is connected to the next in the sorted sequence
- This creates a directed path that the robot must follow in Phase 3

## Project Structure
```bash
aruco_marker_robot/
├── launch/
│   ├── mapping.launch.py
│   ├── navigation.launch.py
│   ├── plansys_plan.launch.py
│   ├── spawn_robot.launch.py
│   └── world.launch.py
├── gazebo_models/
│   └── aruco_box/
├── src/
│   ├── detect_marker_action.cpp
│   ├── get_plan_and_execute.cpp
│   ├── image_process_action.cpp
│   ├── navigate_to_marker.cpp
│   ├── navigate_to_waypoint.cpp
│   └── sorting_marker_action.cpp
├── maps/
│   ├── mogi_bot.gazebo
│   ├── mogi_bot.urdf
│   └── materials.xacro
├── meshes/
│   ├── lidar.dae
│   ├── mogi_bot.dae
│   └── wheel.dae
├── urdf/
│   ├── my_map.pgm
│   └── my_map.yaml
├── pddl/
│   ├── domain.pddl
│   └── problem.pddl
├── msg/
│   └── MarkerList.msg
├── worlds/
│   └── simple_world.sdf
├── config/
│   ├── amcl_localization.yaml
│   ├── ekf.yaml
│   ├── gz_bridge.yaml
│   └── navigation.yaml
├── rviz/
│   ├── localization.rviz
│   ├── mapping.rviz
│   ├── navigation.rviz
│   ├── rviz.rviz
│   └── urdf.rviz
├── CMakeLists.txt
├── package.xml
└── README.md
```
