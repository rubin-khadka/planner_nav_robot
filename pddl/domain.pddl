(define (domain planner_nav)
  (:requirements :strips :typing :adl :durative-actions)

  (:types
    robot waypoint
  )

  (:predicates
    (robot_at ?r - robot ?wp - waypoint)
    (marker_detected_at ?r - robot ?wp - waypoint)
  )

 (:durative-action navigate_to_waypoint
    :parameters (?r - robot ?r1 ?r2 - waypoint)
    :duration ( = ?duration 5)
    :condition (and
        (at start(robot_at ?r ?r1))
        )
    :effect (and
        (at start(not(robot_at ?r ?r1)))
        (at end(robot_at ?r ?r2))
    )
)


  (:durative-action detect_marker_action
    :parameters (?r - robot ?wp - waypoint)
    :duration ( = ?duration 5)
    :condition (and 
    (over all (robot_at ?r ?wp))
    )
    :effect (and 
    (at end(marker_detected_at ?r ?wp))
    )
  )
)
