(define (domain planner_nav_robot)
  (:requirements :strips :typing :negative-preconditions)

  (:types
    robot 
    waypoint
  )

  (:predicates
    (robot_at ?r - robot ?wp - waypoint)
    (marker_detected_at ?wp - waypoint)
  )

  (:action navigate_to_waypoint
    :parameters (?r - robot ?from ?to - waypoint)
    :precondition (robot_at ?r ?from)
    :effect (and
      (not (robot_at ?r ?from))
      (robot_at ?r ?to)
    )
  )

  (:action detect_marker
    :parameters (?r - robot ?wp - waypoint)
    :precondition (robot_at ?r ?wp)
    :effect (marker_detected_at ?wp)
  )
)