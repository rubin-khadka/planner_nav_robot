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