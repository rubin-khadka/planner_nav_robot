(define (domain planner_nav)
  (:requirements :strips :typing :adl :durative-actions)

  (:types
    robot waypoint
  )

  (:predicates
    (robot_at ?r - robot ?wp - waypoint)
    (marker_detected_at ?r - robot ?wp - waypoint)
    (connected ?wp1 - waypoint ?wp2 - waypoint)
    (all_markers_detected)
    (sorting_complete ?r - robot)
    (image_not_processed ?r - robot ?wp - waypoint)
    (image_processed ?r - robot ?wp - waypoint)
    (navigation_enabled ?r - robot)
  )

  (:durative-action navigate_to_waypoint
    :parameters (?r - robot ?wp1 ?wp2 - waypoint)
    :duration ( = ?duration 5)
    :condition (and
        (at start (robot_at ?r ?wp1))
        (over all (navigation_enabled ?r))
    )
    :effect (and
        (at start (not (robot_at ?r ?wp1)))
        (at end (robot_at ?r ?wp2))
    )
  )

  (:durative-action detect_marker_action
    :parameters (?r - robot ?wp - waypoint)
    :duration ( = ?duration 5)
    :condition (and
      (over all (robot_at ?r ?wp))
    )
    :effect (and 
      (at end (marker_detected_at ?r ?wp))
    )
  )

  (:durative-action sort_markers
    :parameters (?r - robot)
    :duration ( = ?duration 3)
    :condition (and
      (over all (all_markers_detected))
    )
    :effect (and 
      (at end (sorting_complete ?r))
    )
  )

  (:durative-action navigate_to_marker
    :parameters (?r - robot ?wp1 ?wp2 - waypoint)
    :duration ( = ?duration 5)
    :condition (and
        (at start (robot_at ?r ?wp1))
        (over all (sorting_complete ?r))
        (over all (connected ?wp1 ?wp2))
        (over all (image_not_processed ?r ?wp2))
    )
    :effect (and
        (at start (not (robot_at ?r ?wp1)))
        (at end (robot_at ?r ?wp2))
    )
  )

  (:durative-action image_processor
    :parameters (?r - robot ?wp - waypoint)
    :duration ( = ?duration 5)
    :condition (and
      (at start (image_not_processed ?r ?wp))
      (over all (robot_at ?r ?wp))
    )
    :effect (and 
      (at end (image_processed ?r ?wp))
      (at end (not (image_not_processed ?r ?wp)))
    )
  )
)