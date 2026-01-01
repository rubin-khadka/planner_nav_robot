(define (problem planner_problem)
  (:domain planner_nav_robot)

  (:objects
    planner_robot - robot
    wp_start wp1 wp2 wp3 wp4 - waypoint
  )

  (:init
    (robot_at planner_robot wp_start)
  )

  (:goal
    (and
      (marker_detected_at wp1)
      (marker_detected_at wp2)
      (marker_detected_at wp3)
      (marker_detected_at wp4)
    )
  )
)