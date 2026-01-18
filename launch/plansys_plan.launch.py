import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.actions import TimerAction
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    bringup_dir = get_package_share_directory('plansys2_bringup')
    print ("bringup_dir:", bringup_dir)
    planner_robot_dir = get_package_share_directory('planner_nav_robot')
    model_file = LaunchConfiguration('model_file')
    problem_file = LaunchConfiguration('problem_file')
    namespace = LaunchConfiguration('namespace')
    params_file = LaunchConfiguration('params_file')
    action_bt_file = LaunchConfiguration('action_bt_file')
    start_action_bt_file = LaunchConfiguration('start_action_bt_file')
    end_action_bt_file = LaunchConfiguration('end_action_bt_file')
    bt_builder_plugin = LaunchConfiguration('bt_builder_plugin')
    
    declare_model_file_cmd = DeclareLaunchArgument(
        'model_file',
        default_value=os.path.join(planner_robot_dir, "pddl", "domain.pddl"),
        description='PDDL Model file'
    )

    declare_problem_file_cmd = DeclareLaunchArgument(
        'problem_file', 
        default_value=os.path.join(planner_robot_dir, "pddl", "problem.pddl"),
        description='PDDL Problem file')
        
    declare_namespace_cmd = DeclareLaunchArgument(
        'namespace',
        default_value='',
        description='Namespace')

    declare_params_file_cmd = DeclareLaunchArgument(
        'params_file',
        default_value=os.path.join(
            bringup_dir, 'params', 'plansys2_params.yaml'),
        description='Full path to the ROS2 parameters file to use for all launched nodes')
        
    declare_action_bt_file_cmd = DeclareLaunchArgument(
        'action_bt_file',
        default_value=os.path.join(
            get_package_share_directory('plansys2_executor'),
            'behavior_trees', 'plansys2_action_bt.xml'),
        description='BT representing a PDDL action')

    declare_start_action_bt_file_cmd = DeclareLaunchArgument(
        'start_action_bt_file',
        default_value=os.path.join(
            get_package_share_directory('plansys2_executor'),
            'behavior_trees', 'plansys2_start_action_bt.xml'),
        description='BT representing a PDDL start action')

    declare_end_action_bt_file_cmd = DeclareLaunchArgument(
        'end_action_bt_file',
        default_value=os.path.join(
            get_package_share_directory('plansys2_executor'),
            'behavior_trees', 'plansys2_end_action_bt.xml'),
        description='BT representing a PDDL end action')

    declare_bt_builder_plugin_cmd = DeclareLaunchArgument(
        'bt_builder_plugin',
        default_value='SimpleBTBuilder',
        description='Behavior tree builder plugin.',
    )

    domain_expert_cmd = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(os.path.join(
            get_package_share_directory('plansys2_domain_expert'),
            'launch',
            'domain_expert_launch.py')),
        launch_arguments={
            'model_file': model_file,
            'namespace': namespace,
            'params_file': params_file
        }.items())
        
        
    problem_expert_cmd = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(os.path.join(
            get_package_share_directory('plansys2_problem_expert'),
            'launch',
            'problem_expert_launch.py')),
        launch_arguments={
            'model_file': model_file,
            'problem_file': problem_file,
            'namespace': namespace,
            'params_file': params_file
        }.items())
    
    executor_cmd = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(os.path.join(
            get_package_share_directory('plansys2_executor'),
            'launch',
            'executor_launch.py')),
        launch_arguments={
            'namespace': namespace,
            'params_file': params_file,
            'default_action_bt_xml_filename': action_bt_file,
            'default_start_action_bt_xml_filename': start_action_bt_file,
            'default_end_action_bt_xml_filename': end_action_bt_file,
            'bt_builder_plugin': bt_builder_plugin,
        }.items())

    planner_cmd = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(os.path.join(
            get_package_share_directory('plansys2_planner'),
            'launch',
            'planner_launch.py')),
        launch_arguments={
            'namespace': namespace,
            'params_file': params_file
        }.items())

   
    lifecycle_manager_cmd = Node(
        package='plansys2_lifecycle_manager',
        executable='lifecycle_manager_node',
        name='lifecycle_manager_node',
        namespace=namespace,
        output='screen',
        parameters=[])
        
    navigate_to_waypoint_cmd = Node(
        package='planner_nav_robot',
        executable='navigate_to_waypoint',
        name='navigate_to_waypoint',
        namespace=namespace,
        output='screen',
        parameters=[])

    navigate_to_marker_cmd = Node(
        package='planner_nav_robot',
        executable='navigate_to_marker',
        name='navigate_to_marker',
        namespace=namespace,
        output='screen',
        parameters=[])
    
    detect_marker_cmd = Node(
        package='planner_nav_robot',
        executable='detect_marker_action',
        name='detect_marker_action',
        namespace=namespace,
        output='screen',
        parameters=[])
    
    sorting_marker_cmd = Node(
        package='planner_nav_robot',
        executable='sorting_marker_action',
        name='sorting_marker_action',
        namespace=namespace,
        output='screen',
        parameters=[]) 
    
    image_process_cmd = Node(
        package='planner_nav_robot',
        executable='image_process_action',
        name='image_process_action',
        namespace=namespace,
        output='screen',
        parameters=[])
    
    controller_cmd = Node(
        package='planner_nav_robot',
        executable='get_plan_and_execute', 
        name='get_plan_and_execute',
        namespace=namespace,
        output='screen',
        parameters=[])
    
    delayed_controller_cmd = TimerAction(
        period= 5.0,
        actions=[controller_cmd]
    )
    
    ld = LaunchDescription()

    ld.add_action(declare_model_file_cmd)
    ld.add_action(declare_problem_file_cmd)
    ld.add_action(declare_namespace_cmd)
    ld.add_action(declare_params_file_cmd)
    ld.add_action(declare_action_bt_file_cmd)
    ld.add_action(declare_start_action_bt_file_cmd)
    ld.add_action(declare_end_action_bt_file_cmd)
    ld.add_action(declare_bt_builder_plugin_cmd)
    
    ld.add_action(domain_expert_cmd)
    ld.add_action(problem_expert_cmd)
    ld.add_action(planner_cmd)
    ld.add_action(executor_cmd)
    ld.add_action(lifecycle_manager_cmd)
    ld.add_action(navigate_to_waypoint_cmd)
    ld.add_action(navigate_to_marker_cmd)
    ld.add_action(detect_marker_cmd)
    ld.add_action(image_process_cmd)
    ld.add_action(sorting_marker_cmd)

    ld.add_action(delayed_controller_cmd)
    
    return ld