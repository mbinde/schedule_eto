<?php

/*****************************************************************************
 *****************************************************************************
 *
 * SECTION:
 *
 * Calendar functionality
 *
 *****************************************************************************
 *****************************************************************************/

function sb_view_schedule_block($target_user, $arg) {
  $array = sb_parse_arg($arg);

  if ($array[1]) {
    $instance = $array[1];
  }
  $id = $array[0];

  if (!$id) {
    drupal_not_found();
    exit;
  } else {
    $schedule = sb_schedule_load($id);
    if (!$schedule) {
      drupal_not_found();
      exit;
    }
  }

  // we need to be an admin or own it
  if (! user_access('view others schedule')) {
    if ($schedule->uid != $user->uid) {
      drupal_access_denied();
      exit;
    }
  }

  if ($instance) {
    $event = sb_schedule_event_load(array('schedule_event_id' => $instance));
    if (! $event) {
      drupal_not_found();
      exit;
    }
    $noun = "block";
    $existing_appointments = sb_appointment_by_schedule_id_load($id, $event);
  } else {
    $noun = "series";
    $existing_appointments = sb_appointment_by_schedule_id_load($id);
  }


  $output .= _schedule_tabs($target_user, $id, $instance, "view");

  $title = $schedule->title;
  drupal_set_title($title);

  if ($instance) {
    $date = date_make_date($event->start_datetime_utc, "UTC");
    $date = date_timezone_set($date, timezone_open(user_tz($target_user)));
    $title = $title . "<br>" . date_format($date, DATE_FORMAT_ETO_LONG);
  }
  $output .= "<h2>$title</h2>";

  if (!$instance) {
    $output .= theme('child_event_list', $schedule, $target_user, "");
  } else {
    if ($existing_appointments) {
      $output .= "<ul class='session-event-list'>\n";
      foreach ($existing_appointments as $app) {
	$output .= "<li>" . theme('appointment_event_item', $app) . "</li>";
      }
      $output .= "</ul>\n";
    } else {
      $output .= "<em>No appointments scheduled yet.</em>\n";
    }
  }

  return $output;
}

function _schedule_tabs($user, $id, $active) {
  $base = "/" . sb_user_base($user) . "/";
  if ($active == 'view') {
    $view_active = "class='active'";
  } else if ($active == 'edit') {
    $edit_active = "class='active'";
  } else if ($active == 'cancel') {
    $cancel_active = "class='active'";
  }
  return "
<ul class='tabs primary'>
<li $view_active><a href='" . schedule_url($base, $id) . "' $view_active><span class='link-span'>View</span></a></li>
<li $edit_active><a href='" . schedule_url($base, $id, "", "edit") . "' $edit_active><span class='link-span'>Edit</span></a></li>
<li $cancel_active><a href='" . schedule_url($base, $id, "", "cancel") . "' $edit_active><span class='link-span'>Cancel</span></a></li>
</ul>";
}



function user_calendar_display($target_user = "") {
  global $user;

  $r = sb_check_permissions($target_user, "view others schedule");

  if ($target_user) {
    $target_user = $r;
    $base_path = 'users/' . $target_user->name . '/calendar';
  } else {
    $target_user = $user;
    $base_path = 'user/calendar';
  }

  drupal_set_title("Calendar for " . $target_user->name);

  if (user_access('create schedule')) {
    if ($target_user->uid != $user->uid) {
      $appendix = " for " . $target_user->name;
    }
  }

  return user_calendar_display_block($target_user, FALSE);
}


function user_calendar_display_block ($target_user, $perm_check = TRUE) {
  $output = "";
  global $user;

  if ($perm_check) {
    if (($user->uid != $target_user->uid) && (! user_access("view others schedule"))) {
      return "";
    } else if (! user_access("create appointments")) {
      return "";
    }
  }

  if ($target_user) {
    $base_path = 'users/' . $target_user->name . '/calendar';
  } else {
    $target_user = $user;
    $base_path = 'user/calendar';
  }

  if (user_access('create schedule')) {
    if ($target_user->uid != $user->uid) {
      $appendix = " for " . $target_user->name;
    }
  }

  if (user_access('create schedule')) {
    $output .= "<div id='calendar-controls'>\n";
    $output .= "<h2>Modify your calendar:</h2>\n";
    $output .= tutor_schedule_options (sb_user_base($target_user), "calendar", $appendix);
    $output .= "</div>\n";
  }
  $events = sb_schedule_uid_load($target_user->uid, TRUE);

  jquery_ui_add('ui.datepicker');
  drupal_add_js(drupal_get_path('module', 'sb_scheduler') . '/js/choose-date.js');

  $today = date('m/d/Y');
  $output .= "<h2>Appointment Calendar for " . $target_user->name . ":</h2>";
  $output .= '<div id="show-calendar" class="caller-' . $user->name . '"></div><div id="date-details">' . callback_user_calendar_fetch($today, $target_user->name, 1) . '</div>';

  return $output;
}


/* small stub for vacations that redirects to sb_edit_schedule_block */

function sb_add_vacation($target_user = "") {
  return sb_edit_schedule_block($target_user, "", SCHEDULE_VACATION);
}

/* adds a block of time */

function sb_edit_schedule_block($target_user = "", $id = "", $time_type = "") {
  global $user;

  // we might be adding vacation, so check our args;
  if ($id && ($id == "vacation")) {
    $id = "";
    $time_type = SCHEDULE_VACATION;
  } else if (! $time_type) {
    $time_type = SCHEDULE_AVAILABILITY;
  }
  if ($id) {
    $array = sb_parse_arg($id);
    $id = $array[0];
  }
  $r = sb_check_permissions($target_user, "view others schedule");

  if ($target_user) {
    $target_user = $r;
    $base_path = 'users/' . $target_user->name . '/calendar/add';
  } else {
    $target_user = $user;
    $base_path = 'user/calendar/add';
  }

  if ($id) {
    $node = sb_schedule_load($id);
    // can only edit schedules for the user it's associated with
    if ( (! user_access('edit others schedule')) && ($node->uid != $target_user->uid)) {
      drupal_not_found();
      exit;
    }
  } else {
    $node = (object) array('uid' => $target_user->uid );
    if ($time_type) {
      $node->schedule_type = $time_type;
    }
    drupal_set_title('Add a new block of scheduled time');
  }

  if ($node->schedule_type == SCHEDULE_VACATION) {
    $noun = "Vacation";
  } else {
    $noun = "Available Tutor";
  }

  $output .= _schedule_tabs($target_user,
			    $node->schedule_id,
			    "",
			    "edit");

  $output .= "<h2>Schedule " . $noun . " Time</h2>";
  if ($node->schedule_id) {
    $output .= "<div class='message warning'>Warning: If you change the number of weeks or the date/time of this session, you will lose any cancellations you made.</div>\n";
  }
  $output .= drupal_get_form('sb_schedule_form', $node);

  return $output;
}

/*
 * Schedule/Calendar:
 *
 * Cancel or reinstate a block or an entire series
 *
 * Can call either with user, ids, action
 * or ids, action
 */

function sb_schedule_action ($arg1, $arg2, $action = "") {

  global $user;
  if (!$action) {
    $action = $arg2;
    $arg = $arg1;
    $target_user = $user;
  }
  else if (!$arg1) {
    $arg = $arg2;
    $target_user = $user;
  }
  else {
    $target_user = eto_user_load(array('name' => $arg1));
    $arg = $arg2;
  }
  $array = sb_parse_arg($arg);

  if ($array[1]) {
    $instance = $array[1];
  }
  $id = $array[0];
  $action = strtolower(check_plain($action));
  $action_string = ucwords($action);

  if (!$id) {
    drupal_not_found();
    exit;
  } else {
    $schedule = sb_schedule_load($id);
    if (!$schedule) {
      drupal_not_found();
      exit;
    }
  }

  // we need to be an admin or own it
  if (! user_access('view others schedule')) {
    if ($schedule->uid != $user->uid) {
      drupal_access_denied();
      exit;
    }
  }

  // load our instance and any existing appointments if needed
  if ($instance) {
    $event = sb_schedule_event_load(array('schedule_event_id' => $instance));
    if (! $event) {
      drupal_not_found();
      exit;
    }
    $noun = "block";
    $existing_appointments = sb_appointment_by_schedule_id_load($id, $event);
  } else {
    $noun = "series";
    $existing_appointments = sb_appointment_by_schedule_id_load($id);
  }

  // cannot cancel a canceled schedule block; cannot reinstate a non-canceled schedule block
  // since I'm always tempted to combine this: I can't, because a block can be canceled but the instance not
  if ($action == "reinstate") {
    if ($instance) {
      if ($event->status_flag == 1) {
	$reinstate_message = 1;
      }
    } else if ($schedule->status_flag == 1) {
      $reinstate_message = 1;
    }
  } else if ($action == "cancel") {
    if ($instance) {
      if ($event->status_flag == 0) {
	$cancel_message = 1;
      }
    } else if ($schedule->status_flag == 0) {
      $cancel_message = 1;
    }
  }

  $output .= _schedule_tabs($target_user,
			    $event->schedule_id,
			    "",
			    "cancel");


  if ($reinstate_message) {
    $cancel_link = schedule_url(sb_user_base($target_user) . "/", $id, $instance, "cancel");
    $output .= "<div class='status'>This is already active.  Do you want " . l("to cancel it", $cancel_link) . " instead?</div>";
    $replacement_form_message = l("Click here to cancel.", $cancel_link);
  } else if ($cancel_message) {
    $reinstate_link = schedule_url(sb_user_base($target_user) . "/", $id, $instance, "reinstate");
    $output .= "<div class='status'>This is already canceled.  Do you want " . l("to reinstate it", $reinstate_link) . " instead?</div>";
    $replacement_form_message = l("Click here to reinstate.", $reinstate_link);
  }

  // we're either canceling, or reinstating
  $title = ucwords($action) . " Schedule " . ucwords($noun);
  drupal_set_title($title);
  $output .= "<h2>$title</h2>\n";

  if (($instance != "") && ($action == 'cancel')) {
    $output .= "<p><em>Did you want to $action all future sessions in this schedule series?<br>If so, click here instead: <strong>" .
      l("$action_string all future schedule blocks in this series",
	schedule_url(sb_user_base($target_user) . "/", $id, "", $action)) . "</strong></em></p>";
  } else {
    $event = $schedule;
  }

  $date = date_make_date($event->start_datetime_utc, "UTC");
  date_timezone_set($date, timezone_open(user_tz()));
  $date_string = "on " . date_format($date, DATE_FORMAT_ETO_LONG) . " at " . date_format($date, TIME_FORMAT_ETO_STANDARD);

  if ($replacement_form_message) {
    $output .= "<h2>Schedule " . $noun. " $date_string</h2>";
    $output .= "<p></p>";
    $output .= $replacement_form_message;
  } else {

    if ($noun == "series") {
      $output .= "<h2>$action_string future occurences of schedule " . $noun. " $date_string</h2>";
      if ($action == 'cancel') {
        $now = date_make_date(time(), "UTC");
        $from_date = date_format($now, DATE_FORMAT_DATETIME);
        $cancel_count = count(sb_schedule_event_load(array(
                            'schedule_id' => $event->schedule_id,
                            'from_date' => $from_date)));
        $total_count = count(sb_schedule_event_load(array(
                            'schedule_id' => $event->schedule_id
                         )));
        $output .= "This will cancel $cancel_count of the $total_count events in the series.";
      }

    }
    else {
      $output .= "<h2>$action_string schedule " . $noun. " $date_string</h2>";
    }

    if ($action == "reinstate") {
      $output .= "<p>Are you sure you want to reinstate this schedule " . $noun . "?</p>\n";
      $output .= "<p>Please note that this will not unreinstate any appointments that were canceled earlier.</p>\n";
    }

    if ($existing_appointments) {
      $output .= "<div class='status warning'>Warning: You have " . count($existing_appointments) . " appointments scheduled at this time.  If you cancel this availability, these appointments will be automatically canceled.  This action cannot be undone.</div>";

      drupal_add_js('misc/collapse.js');

      $output .= "<fieldset class='collapsible collapsed' id='list-appointments'>\n";
      $output .= "<legend><a href='#'>" . t("View the appointments that will be canceled") . "</a></legend>\n";
      $output .= "<div class='fieldset-wrapper'>\n";
      $output .= "<ul class='session-event-list'>\n";
      foreach ($existing_appointments as $app) {
	$output .= "<li>" . theme('appointment_event_item', $app) . "</li>";
      }
      $output .= "</ul>\n";
      $output .= "</div>\n";
      $output .= "</fieldset>\n";
    }

    $output .= drupal_get_form('sb_' . $action . '_schedule_form', $event);
    if (! $instance) {
      // see if they want to instead cancel a specific instance
      $output .= "<h2>Did you want to just cancel one instance?</h2>";
      $output .= "If so, select one below:";

      $output .= theme('child_event_list', $event, $target_user, "cancel");

    }
  }

  return $output;
}

/**
 ** Cancellation functionality
 **/

function sb_cancel_schedule_form (&$form_state, $node) {
  $form = array();

  global $user;

  $app = sb_schedule_load($node->schedule_id);

  if ($node->schedule_event_id) {
    // we're an schedule event
    $form['schedule_event_id'] = array(
					  '#type' => 'hidden',
					  '#value' => $node->schedule_event_id
					  );
    $noun = "block";
  } else {
    $noun = "series";
  }

  $form['schedule_id'] = array(
					'#type' => 'hidden',
					'#value' => $node->schedule_id
					);
  $form['uid'] = array(
					'#type' => 'hidden',
					'#value' => $node->uid,
					);


  if ($noun == 'series') {
    $form['submit'] = array (
      '#type' => 'submit',
      '#value' => 'Yes, cancel future occurences of this schedule ' . $noun,
    );
  }
  else {
    $form['submit'] = array (
      '#type' => 'submit',
      '#value' => 'Yes, cancel this schedule ' . $noun,
    );
  }

  $form['#submit'] = array('sb_cancel_schedule_submit_handler');

  return $form;
}

function sb_cancel_schedule_submit_handler($form, &$form_state) {

  if ($form_state['values']['schedule_event_id']) {
    $event = sb_schedule_event_load(array('schedule_event_id' => $form_state['values']['schedule_event_id']));
    $existing_appointments = sb_appointment_by_schedule_id_load($form_state['values']['schedule_id'],
								$event);
  } else {
    $existing_appointments = sb_appointment_by_schedule_id_load($form_state['values']['schedule_id']);
  }

  // first cancel each affected appointment, then cancel the time block
  if ($existing_appointments) {
    foreach ($existing_appointments as $app) {
      sb_appointment_event_cancel($app->appointment_event_id);
    }
  }

  if ($form_state['values']['schedule_event_id']) {
    sb_schedule_event_cancel($form_state['values']['schedule_event_id']);
  } else {
    sb_schedule_cancel($form_state['values']['schedule_id']);
  }

  $return = l("Return to your calendar",
	      sb_user_base($form_state['values']['uid']) . "/calendar");
  drupal_set_message('Your schedule has been successfully canceled.  ' . $return, 'success');
  $form_state['redirect'] = schedule_url(sb_user_base($form_state['values']['uid']) . "/",
					 $form_state['values']['schedule_id'],
					 $form_state['values']['schedule_event_id'],
					 "cancel");
}

function sb_schedule_cancel ($schedule_id) {
  // we used to mark the schedule as cancelled. now just cancel the individual 'future events'
  /*
  db_query('UPDATE {sb_schedule} SET status_flag = 0 WHERE schedule_id = %d',
	   $schedule_id);
  */
  $now = date_make_date(time(), "UTC");
  $from_date = date_format($now, DATE_FORMAT_DATETIME);

  $events = sb_schedule_event_load(array(
              'schedule_id' => $schedule_id,
              'from_date' => $from_date));
  foreach ($events as $e) {
    sb_schedule_event_cancel($e->schedule_event_id);
  }
}

function sb_schedule_event_cancel ($event_id) {
   db_query('UPDATE {sb_schedule_event} SET status_flag = 0 WHERE schedule_event_id = %d',
	    $event_id);
}

/**
 ** Reinstatement functionality
 **/

function sb_reinstate_schedule_form (&$form_state, $node) {
  $form = array();

  global $user;

  $app = sb_schedule_load($node->schedule_id);

  if ($node->schedule_event_id) {
    // we're an schedule event
    $form['schedule_event_id'] = array(
					  '#type' => 'hidden',
					  '#value' => $node->schedule_event_id
					  );
    $noun = "block";
  } else {
    $noun = "series";
  }

  $form['schedule_id'] = array(
					'#type' => 'hidden',
					'#value' => $node->schedule_id
					);
  $form['uid'] = array(
					'#type' => 'hidden',
					'#value' => $node->uid,
					);


  $form['submit'] = array (
			   '#type' => 'submit',
			   '#value' => 'Yes, reinstate this schedule ' . $noun,
			   );

  $form['#submit'] = array('sb_reinstate_schedule_submit_handler');

  return $form;
}

function sb_reinstate_schedule_submit_handler($form, &$form_state) {

  if ($form_state['values']['schedule_event_id']) {
    sb_schedule_event_reinstate($form_state['values']['schedule_event_id']);
  } else {
    sb_schedule_reinstate($form_state['values']['schedule_event']);
  }

  $return = l("Return to your calendar",
	      sb_user_base($form_state['values']['uid']) . "/calendar");
  drupal_set_message('Your schedule has been successfully reinstated.  ' . $return, 'success');
  $form_state['redirect'] = schedule_url(sb_user_base($form_state['values']['uid']) . "/",
					 $form_state['values']['schedule_id'],
					 $form_state['values']['schedule_event_id'],
					 "reinstate");
}

function sb_schedule_reinstate ($schedule_id) {
  db_query('UPDATE {sb_schedule} SET status_flag = 1 WHERE schedule_id = %d',
	   $schedule_id);
}

function sb_schedule_event_reinstate ($event_id) {
   db_query('UPDATE {sb_schedule_event} SET status_flag = 1 WHERE schedule_event_id = %d',
	    $event_id);
}

function tutor_schedule_options ($base_path, $page, $appendix = "") {
  $output = "<ul>";
  if ($page == "schedule") {
    $output .= "<li>" . l("Schedule availability" . $appendix, $base_path . '/calendar/add') . "</li>";
    $output .= "<li>" . l("Add vacation days" . $appendix, $base_path . '/calendar/add/vacation') . "</li>";
    $output .= "<li>" . "Select a date and time to either:";
    $output .= "<ul><li>block off personal time</li><li>or schedule a tutor session</li></ul>";
  } else {
    $output .= "<li>" . l("Schedule availability" . $appendix, $base_path . '/calendar/add') . "</li>";
    $output .= "<li>" . l("Add vacation days" . $appendix, $base_path . '/calendar/add/vacation') . "</li>";
    $output .= "<li>" . l("Schedule personal time or a tutor session with a student" . $appendix, $base_path . "/schedule") . "</li>\n";
  }
  $output .= "</ul>";
  return $output;
}


function user_schedule_display($target_user = "") {
  jquery_ui_add('ui.datepicker');
  drupal_add_js(drupal_get_path('module', 'sb_scheduler') . '/js/choose-date.js');
  global $user;

  if ($target_user) {
    $target_user = eto_user_load(array('name' => $target_user));
    $message = "<h2>Select a date and time to schedule an tutor session with " . theme('eto_user', $target_user) . ":</h2>";
    if (user_access("administer the site", $user)) {
      $header = tutor_schedule_options(sb_user_base($target_user), "schedule", " for " . $target_user->name);
    }
  } else {
    $target_user = $user;
    $header = tutor_schedule_options(sb_user_base($target_user), "schedule");
    $message = "<h2>Select a date and time to schedule some personal time or a tutor session with a student:</h2>";
  }
  if (!$target_user) {
    drupal_not_found();
    exit;
  }


  $output .= $header;
  $output .= $message;

  $today = date('m/d/Y');
  $output .= '<div id="choose-date"></div><div id="date-details">' . callback_user_schedule_fetch($today, $target_user->name, 1) . '</div>';

  return $output;
}

/*
 * callback for displaying a user's calendar
 */

function callback_user_calendar_fetch ($target_date = "", $target_user = "", $return_data = 0) {

  $output = "";

  if (! ($target_date || $target_user) ) {
    // js called, rather than php
    $target_date = $_POST['date']; // MM/DD/YYYY
    $target_user = $_POST['user'];
    $third_arg = $_POST['path1'];
    // otherwise, use the current logged in user
    if ( ($third_arg == 'user')
	 && ( ($target_user == 'calendar') || ($target_user == 'undefined' ) ) ) {
      global $user;
      $target_user = $user;
    }
  }

  if (! is_object($target_user)) {
    $target_user = eto_user_load(array('name' => $target_user));
  }

  if (!$target_user) {
    print "User not found.";
    exit;
  }

  if ($target_user->uid == $user->uid) {
    $base_user_url = "user/";
  } else {
    $base_user_url = "users/" . $target_user->name;
  }

  if ($target_date == date('m/d/Y')) {
    $today = " (today)";
  } else {
    $today = "";
  }

  // rewrite our target date
  $d = split('/', $target_date);
  $db_date_start = $d[2] . "-" . $d[0] . "-" . $d[1] . " 00:00:00";
  $db_date_start = date_make_date($db_date_start, user_tz());
  date_timezone_set($db_date_start, timezone_open('UTC'));
  $db_date_end = $d[2] . "-" . $d[0] . "-" . $d[1] . " 23:59:59";
  $db_date_end = date_make_date($db_date_end, user_tz());
  date_timezone_set($db_date_end, timezone_open('UTC'));

  $target_date = $d[2] . "-" . $d[0] . "-" . $d[1] . " 00:00:00";
  $target_date = date_make_date($target_date, user_tz());
  $tz_string = date_format($target_date, "T");

  $events = sb_appointment_uid_load($target_user->uid, $db_date_start, $db_date_end);
  $blocks = sb_schedule_uid_load($target_user->uid, TRUE, $db_date_start, $db_date_end);
  $vacation = sb_schedule_uid_load($target_user->uid, TRUE, $db_date_start, $db_date_end, SCHEDULE_VACATION);

  $output .= "<p>" . date_format($target_date, "l M jS, Y") . "$today:</p>\n";

  if ($vacation) {
    $v = sb_schedule_load($vacation[0]->schedule_id);
    $start = date_make_date($v->start_datetime_utc, "UTC");
    date_timezone_set($start, timezone_open(user_tz()));
    $interval = "P" . ($v->repeat_count - 1) . "D";
    $interval = new DateInterval($interval);
    $end = date_make_date($v->start_datetime_utc, "UTC"); // works off the start date to reduce TZ issues
    date_timezone_set($end, timezone_open(user_tz()));
    $end = date_add($end, $interval);

    $output .= "<p><em>"
      . ( ($target_user->uid == $user->uid) ? "You are " : $target_user->name . " is ")
      . " on vacation from "
      . date_format($start, DATE_FORMAT_ETO_LONG_NO_YEAR)
      . " to "
      . date_format($end, DATE_FORMAT_ETO_LONG_NO_YEAR)
      . "</em> "
      . l("[edit]", schedule_url($base_user_url, $v->schedule_id, "", "edit"))
      . "</p>";

    if ($target_user->uid == $user->uid) {
      $output .= "<p>No new appointments will be allowed to be scheduled today due to your vacation.</p>\n";
    } else {
      $output .= "<p>No new appointments can be scheduled during a tutor's vacation.</p>\n";
    }

  }

  if (!$blocks) {

    if (!$events) {

      $output .= "<em>Nothing is scheduled for this day.</em>";
    } else {

      foreach ($events as $event) {
	$start = date_make_date($event->start_datetime_utc, "UTC");
	date_timezone_set($start, timezone_open($tz_string));
	$end = date_make_date($event->end_datetime_utc, "UTC");
	date_timezone_set($end, timezone_open($tz_string));
	$event_details = sb_appointment_load(array('appointment_id' => $event->appointment_id));
	$participants = sb_appointment_participants_load_pretty($event->appointment_id);
	$tutor = theme('eto_user', eto_user_load($event->tutor_uid), TRUE);
	$p = array_merge(array($tutor), $participants);
	$title = l($event_details->title,
		   appointment_url("", $event->appointment_id, $event->appointment_event_id),
		   array('attributes' => array('target' => '_blank')));
	$rows[] = format_tutor_appointment(date_format($start, TIME_FORMAT_ETO_STANDARD),
					   date_format($end, TIME_FORMAT_ETO_STANDARD),
					   $title,
					   $p,
					   $event->appointment_type);
      }

      $output .= "<table>";
      foreach ($rows as $r) {
	$output .= "<tr>";
	$output .= "<td>" . $r[0] . "</td><td>" . $r[1] . "</td><td>" . $r[2] . "</td>\n";
	$output .= "</tr>\n";
      }
      $output .= "</table>";

    }

  } else {

    $output .= "<table>";

    // check if we have any block that is a vacation block
    $vacation_recorded = 0;
    foreach ($blocks as $b) {
      if (!$vacation_recorded && (($b->schedule_type == SCHEDULE_VACATION))) {
	$vacation_recorded = 1;
	$rows[] = array('', '**Vacation Day**', '');
      }
    }

    // both blocks and events are ordered in time order, so we can just step through them
    $block_index = 0;
    $current_block = $blocks[$block_index];
    while ($current_block && ($current_block->schedule_type == SCHEDULE_VACATION)) {
      $block_index++;
      $current_block = $blocks[$block_index];
    }

    // if we get here and we're still vacation, we have no real blocks scheduled today

    if ($current_block && ($current_block->schedule_type != SCHEDULE_VACATION)) {

      $current_block_start = date_make_date($current_block->start_datetime_utc, 'UTC');
      date_timezone_set($current_block_start, timezone_open($tz_string));
      $current_block_end = date_make_date($current_block->end_datetime_utc, 'UTC');
      date_timezone_set($current_block_end, timezone_open($tz_string));

      if ($current_block && ($current_block->schedule_type != SCHEDULE_VACATION)) {
	$rows[] = format_tutor_session_begin(date_format($current_block_start, TIME_FORMAT_ETO_STANDARD), $current_block);
      }

      if ($events) {
	foreach ($events as $event) {
	  $start = date_make_date($event->start_datetime_utc, "UTC");
	  date_timezone_set($start, timezone_open($tz_string));
	  $end = date_make_date($event->end_datetime_utc, "UTC");
	  date_timezone_set($end, timezone_open($tz_string));

	  // if our event begins after our block end, advance our block
	  if ($start >= $current_block_end) {
	    if ($current_block && ($current_block->schedule_type != SCHEDULE_VACATION)) {
	      $rows[] = array(date_format($current_block_end, TIME_FORMAT_ETO_STANDARD), "Tutoring ends");
	    }

	    $block_index++;
	    $current_block = $blocks[$block_index];
	    while ($current_block && ($current_block->schedule_type == SCHEDULE_VACATION)) {
	      $block_index++;
	      $current_block = $blocks[$block_index];
	    }
	    // if we have a valid one
	    if ($current_block) {
	      $current_block_start = date_make_date($current_block->start_datetime_utc, 'UTC');
	      date_timezone_set($current_block_start, timezone_open($tz_string));
	      $current_block_end = date_make_date($current_block->end_datetime_utc, 'UTC');
	      date_timezone_set($current_block_end, timezone_open($tz_string));
	    }

	  }
	  $event_details = sb_appointment_load(array('appointment_id' => $event->appointment_id));
	  $participants = sb_appointment_participants_load_pretty($event->appointment_id);
	  //	  $p = format_participant_array($participants);
	  $title = l($event_details->title,
		     appointment_url("", $event->appointment_id, $event->appointment_event_id),
		     array('attributes' => array('target' => '_blank')));
	  $rows[] = format_tutor_appointment(date_format($start, TIME_FORMAT_ETO_STANDARD),
					     date_format($end, TIME_FORMAT_ETO_STANDARD),
					     $title,
					     $participants,
					     $event->appointment_type);
	}
      }
    }
    if ($current_block && ($current_block->schedule_type != SCHEDULE_VACATION)) {
      $rows[] = format_tutor_session_end(date_format($current_block_end, TIME_FORMAT_ETO_STANDARD));
    }


    // continue if we have more blocks
    for ($i = ($block_index + 1); $i< count($blocks); $i++) {

      $current_block = $blocks[$i];
      $current_block_start = date_make_date($current_block->start_datetime_utc, 'UTC');
      date_timezone_set($current_block_start, timezone_open($tz_string));
      if ($current_block && ($current_block->schedule_type != SCHEDULE_VACATION)) {
	$rows[] = format_tutor_session_begin(date_format($current_block_start, TIME_FORMAT_ETO_STANDARD), $current_block);
      }
      $current_block_end = date_make_date($current_block->end_datetime_utc, 'UTC');
      date_timezone_set($current_block_end, timezone_open($tz_string));
      if ($current_block && ($current_block->schedule_type != SCHEDULE_VACATION)) {
	$rows[] = format_tutor_session_end(date_format($current_block_end, TIME_FORMAT_ETO_STANDARD));
      }

    }

    // iterate through the rows and add them
    foreach ($rows as $r) {
      $output .= "<tr>";
      $output .= "<td>" . $r[0] . "</td><td>" . $r[1] . "</td><td>" . $r[2] . "</td>\n";
      $output .= "</tr>\n";
    }
    $output .= "</table>";
  }

  if ($return_data) {
    return $output;
  } else {
    print $output;
  }
}

/* minor helper functions */

function format_tutor_session_begin($date, $b) {
  return array($date, "Tutoring begins", l("[cancel]",
					   schedule_url(sb_user_base($b->uid) . "/", $b->schedule_id, $b->schedule_event_id, "cancel"))
	       . " "
	       . l("[edit]",
		   schedule_url(sb_user_base($b->uid) . "/", $b->schedule_id, $b->schedule_event_id, "edit")));
}

function format_tutor_session_end($date) {
  return array($date, "Tutoring ends");
}

function format_tutor_appointment($start, $end, $title, $p, $type = TUTOR_SESSION) {
  if ($type == PERSONAL_SESSION) {
    return array($start, "$title<br>Until: $end");
  } else {
    return array($start,
		 "$title<br>Until: $end<br>With: " . join(", ", $p));
  }
}


/*
 * callback for displaying a user's schedule
 */

function callback_user_schedule_fetch ($target_date = "", $target_user = "", $return_data = 0) {
  global $user;
  $output = "";
  if (! ($target_date || $target_user) ) {
    $target_date = $_POST['date']; // MM/DD/YYYY
    $target_user = $_POST['user'];
    $third_arg = $_POST['path1'];
    if ( ($third_arg == 'user')
	 && ( ($target_user == 'schedule') || ($target_user == 'undefined' ) ) ) {
      $target_user = $user;
    }
  }

  if (! is_object($target_user)) {
    $target_user = eto_user_load(array('name' => $target_user));
  }

  if (!$target_user) {
    print "User not found.";
    exit;
  }

  if ($target_date == date('m/d/Y')) {
    $today = " (today)";
  } else {
    $today = "";
  }

  // first establish some levels.  don't show dates in the past, and grey things we can't schedule
  // we work off user's timezone for everything, so just work with that rather than UTC
  $unavailable = date_make_date(time(), user_tz());
  if (! ( (user_access("is a tutor") || (user_access("administer the site") ) ) ) ) {
    $grey = date_make_date(time(), user_tz());
    $interval = new DateInterval('PT' . variable_get('eto_minimum_appointment_lead_time', 6) . 'H');
    date_add($grey, $interval);
  }

  // rewrite our target date
  $d = split('/', $target_date);
  $db_date_start = $d[2] . "-" . $d[0] . "-" . $d[1] . " 00:00:00";
  $db_date_start = date_make_date($db_date_start, user_tz());
  date_timezone_set($db_date_start, timezone_open('UTC'));
  $db_date_end = $d[2] . "-" . $d[0] . "-" . $d[1] . " 23:59:59";
  $db_date_end = date_make_date($db_date_end, user_tz());
  date_timezone_set($db_date_end, timezone_open('UTC'));

  $target_date = $d[2] . "-" . $d[0] . "-" . $d[1] . " 00:00:00";
  $target_date = date_make_date($target_date, user_tz());
  $tz_string = date_format($target_date, "T");

  $blocks = sb_schedule_uid_load($target_user->uid, TRUE, $db_date_start, $db_date_end);
  $vacation = sb_schedule_uid_load($target_user->uid, TRUE, $db_date_start, $db_date_end, SCHEDULE_VACATION);

  $output .= "<h2>Available times on " . date_format($target_date, "M jS, Y") . "$today:</h2>";

  // add a link to manually schedule an appointment
  if ( ($target_user->uid == $user->uid) || user_access("administer the site") ) {
    $url = 'users/' . $target_user->name . '/schedule/appointment/' . sb_encode(date_format($target_date, DATE_FORMAT_DATETIME));
    $output .= '<p>' . l('Click here to schedule an appointment with a student.', $url) . '</p>';
  }

  if ($vacation) {
    $v = sb_schedule_load($vacation[0]->schedule_id);
    $start = date_make_date($v->start_datetime_utc, "UTC");
    date_timezone_set($start, timezone_open(user_tz()));
    $interval = "P" . ($v->repeat_count - 1) . "D";
    $interval = new DateInterval($interval);
    $end = date_make_date($v->start_datetime_utc, "UTC"); // works off the start date to reduce TZ issues
    date_timezone_set($end, timezone_open(user_tz()));
    $end = date_add($end, $interval);

    $output .= "<em>Sorry, "
      . $target_user->name
      . " is on vacation from "
      . date_format($start, DATE_FORMAT_ETO_LONG_NO_YEAR)
      . " to "
      . date_format($end, DATE_FORMAT_ETO_LONG_NO_YEAR)
      . ", so new appointments cannot be scheduled.  </em>";
  } else if ($blocks) {

    $event = "";
    $events = array();

    foreach ($blocks as $block) {
      $start = date_make_date($block->start_datetime_utc, "UTC");
      date_timezone_set($start, timezone_open($tz_string));
      $end = date_make_date($block->end_datetime_utc, "UTC");
      date_timezone_set($end, timezone_open($tz_string));

      $interval = new DateInterval("PT15M");
      $continue = 1;
      $count = 0;

      while ($continue == 1) {

	// load the next event if we don't have one
	if (!$event) {
	  date_timezone_set($start, timezone_open("UTC"));
	  $events = sb_appointment_load_by_time($target_user->uid, date_format($start, DATE_FORMAT_DATETIME), TRUE, 20);
	  if (isset($events[0])) {
	    $event = $events[0];
	  } else {
	    $event = "";
	  }
	  if (isset($events[1])) {
	    $next_event = $events[1];
	  } else {
	    $next_event = "";
	  }
	  global $user;
	  date_timezone_set($start, timezone_open(user_tz()));
	  if ($event) {
	    $event_start = date_make_date($event->start_datetime_utc, "UTC");
	    date_timezone_set($event_start, timezone_open($tz_string));
	    $event_end = date_make_date($event->end_datetime_utc, "UTC");
	    date_timezone_set($event_end, timezone_open($tz_string));
	  }
	  if ($next_event) {
	    $next_event_start = date_make_date($next_event->start_datetime_utc, "UTC");
	    date_timezone_set($next_event_start, timezone_open($tz_string));
	    $next_event_end = date_make_date($next_event->end_datetime_utc, "UTC");
	    date_timezone_set($next_event_end, timezone_open($tz_string));
	  }
	}

	foreach ($events as $e) {
	  $e_start = date_make_date($e->start_datetime_utc, "UTC");
	  date_timezone_set($e_start, timezone_open($tz_string));
	  $e_end = date_make_date($e->end_datetime_utc, "UTC");
	  date_timezone_set($e_end, timezone_open($tz_string));

	  if ($e && ($start >= $e_start) && ($start < $e_end) ) {
	    $skipped++;
	    $skipped_this_time++;
	  }
	}

	/*
	// if this time doesn't overlap with a scheduled event, display it as available
	if ( $event && ($start >= $event_start) && ($start < $event_end) ) {
	  // register that we've skipped a time
	  $skipped++;
	} else if ( $next_event && ($start >= $next_event_start) && ($start < $next_event_end) ) {
	  // the next event was back to back with this one, so keep skipping
	  $skipped++;
	} else {
	*/
	if (! $skipped_this_time) {
	  // so the problem is, we can reach this point where our current event has ended,
	  // but the next event starts now -- and we display the time as available even
	  // if it isn't.
	  if ($skipped > 0) {
	    // if we've skipped a time, we need a blank line to visually indicate this
	    $output .= "<p></p>";
	  }
	  date_timezone_set($start, timezone_open("UTC"));
	  $rate_label = sb_get_hourly_rate($target_user->uid, date_format($start, DATE_FORMAT_DATETIME), 15, TRUE);
	  date_timezone_set($start, timezone_open(user_tz()));
	  $arguments = $block->schedule_id . '/' . $block->schedule_event_id . '/' . $count;
	  $url = 'users/' . $target_user->name . '/schedule/appointment/' . sb_encode($arguments);

	  if ($start >= $unavailable) {
	    // only show if it's after our unavailable time
	    if ($grey && ($start < $grey)) {
	      // show greyed out
	      $output .= "<span class='grey'>" . date_format($start, 'g:ia') . " ($rate_label) <em>Contact the tutor to schedule at this time</em><br></span>\n";
	    } else {
	      $output .= l(date_format($start, 'g:ia'), $url) . " ($rate_label)" . "<br>";
	    }
	  }
	  // if we skipped, get a new event next time through
	  if ($skipped > 0) {
	    $skipped = 0;
	    $skipped_this_time = 0;
	    $event = "";
	  }
	}
	else {
	  $skipped_this_time = 0;
	}
	$start = date_add($start, $interval);
	// stop incrementing when we've reached our end time, since appointments cannot be scheduled right when our
	// timeblock ends
	if ($start >= $end) {
	  $continue = 0;
	}
	$count ++;
      }
      $output .= "<p></p>";
    }


  } else {
    $output .= "<em>No available times found, please try another day.</em>";
  }

  if ($return_data) {
    return $output;
  } else {
    print $output;
  }
}

function sb_schedule_form($form_state, $node) {
  $form = array();
  global $user;

  if ($node->schedule_type == SCHEDULE_VACATION) {
    $unit = "day(s)";
    $unit_singular = "day";
    $unit_plural = "days";
  } else {
    $unit = "week(s)";
    $unit_singular = "week";
    $unit_plural = "weeks";
  }

  // show the title but don't allow it to be edited
  $form['title'] = array (
			  '#type' => 'textfield',
			  '#disabled' => TRUE,
			  '#title' => 'Title',
			  '#value' => ($node->title ? $node->title : "(tbd)"),
			  '#description' => 'The title will be automatically updated when you save.',
			  );

  /* Hidden fields */

  $form['schedule_id'] = array (
				   '#type' => 'hidden',
				   '#value' => ($node->schedule_id ? $node->schedule_id : ""),
				   );

  $form['uid'] = array (
			      '#type' => 'hidden',
			      '#value' => ($node->uid ? $node->uid : $user->uid),
			      );

  $form['schedule_type'] = array (
				  '#type' => 'hidden',
				  '#value' => ($node->schedule_type ? $node->schedule_type : 1),
				  );
  $form['status_flag'] = array (
				  '#type' => 'hidden',
				  '#value' => ($node->status_flag ? $node->status_flag : 1),
				  );
  $form['repeat_interval'] = array (
				    '#type' => 'hidden',
				    '#value' => ($node->repeat_interval ? $node->repeat_interval : $unit_singular),
				    );

  /* hide this for now */
  $form['hourly_rate'] = array (
				'#type' => 'hidden',
				'#size' => '4',
				'#title' => 'Hourly rate',
				'#default_value' => ($node->hourly_rate ? $node->hourly_rate : '0.00'),
				'#field_prefix' => '$',
				'#field_suffix' => '/hour',
				);


  /* Regular fields */
  if ($node->uid && ($user->uid != $node->uid)) {
    $timezone_suffix = ($node->tz_owner ? " (timezone: " . $node->tz_owner . ")" : "");
  }

  $now_date = date("m/d/Y", time());

  // don't include this in the tab index because it doesn't behave well
  $form['date_field'] = array (
			       '#type' => 'date_popup',
			       '#date_format' => 'm/d/Y',
			       '#date_year_range' => '-0+1',
			       '#title' => 'Date',
			       '#default_value' => ($node->start_datetime_owner ? $node->start_datetime_owner : ""),
			       '#required' => TRUE,
			       );

  /* these are working around a bug I don't understand where the form element
   above tries to shove its date value into the 2nd field below it */
  $form['foobar_1'] = array ( '#type' => 'hidden');
  $form['foobar_2'] = array ( '#type' => 'hidden');

  if ($node->schedule_type == SCHEDULE_AVAILABILITY) {
    // time isn't an option with vacations

    $form['start_time'] = array (
      '#type' => 'textfield',
      '#field_suffix' => $timezone_suffix,
      '#title' => 'Start Time' . $timezone_suffix,
      '#default_value' => ($node->start_datetime_owner ? $node->start_datetime_owner : ""),
      '#description' => "Format: 9:30am",
      '#required' => TRUE,
      '#size' => 8,
      '#attributes' => array ('tabindex' => 1),
    );
    $form['end_time'] = array (
      '#type' => 'textfield',
      '#title' => 'End Time' . $timezone_suffix,
      '#default_value' => ($node->end_datetime_owner ? $node->end_datetime_owner : ""),
      '#description' => "Format: 9:30am",
      '#size' => 8,
      '#required' => TRUE,
      '#attributes' => array ('tabindex' => 2),
    );
  }

  $form['repeat_count'] = array (
				 '#type' => 'textfield',
				 '#size' => '3',
				 '#title' => 'How many ' . $unit_plural . ' would you like this repeated for?',
				 '#default_value' => ($node->repeat_count ? $node->repeat_count : '1'),
				 '#field_suffix' => ' ' . $unit,
				 '#description' => 'If you want this time to be scheduled for more than one ' . $unit . ', enter the number of ' . $unit_plural . ' here.  Maximum allowed is 16.',
				 '#attributes' => array ('tabindex' => 3),
				 );
  $form['body'] = array (
			  '#type' => 'textarea',
			  '#title' => "Notes",
			  '#description' => 'Any notes you want to attach to this block of time',
			  '#default_value' => $node->body,
			  '#attributes' => array ('tabindex' => 4),
			 );

  $form['submit'] = array (
			   '#type' => 'submit',
			   '#value' => 'Schedule Now',
			   );

  if ($node->schedule_id) {
    $form['cancel'] = array (
			     '#type' => 'submit',
			     '#value' => 'Cancel this',
			     );
  }

  $form['#validate'][] = 'sb_schedule_form_validate';
  $form['#submit'] = array('sb_schedule_submit_handler');

  return $form;
}

function sb_schedule_form_validate($node, &$form_state) {
  // max number of weeks for anything is 16
  if (isset($form_state['values']['repeat_count']) && ($form_state['values']['repeat_count'] > 16)) {
    form_set_error('repeat_count', 'Repeat cannot be more than 16.');
  }

  // check that our times are valid, if they're entered
  if (isset($form_state['values']['start_time'])) {
    $err = validate_time($form_state['values']['start_time']);
    if ($err) {
      form_set_error('start_time', $err);
    }
  }
  if (isset($form_state['values']['end_time'])) {
    $err = validate_time($form_state['values']['end_time']);
    if ($err) {
      form_set_error('end_time', $err);
    }
  }
}

function validate_time($time) {
  $r = preg_match('/^(\d+)(:\d+)?(\s*)(am|pm)$/i', $time);
  if ($r) {
    return ""; // success
  }
  else {
    return "Time must be formatted like 9:00am";
  }
}

/*
 * our submit handler; constructs the correct start/end times and our title
 */

function sb_schedule_submit_handler($form, &$form_state) {
  global $base_path;
  $base_user_url = sb_user_base($form_state['values']['uid']);

  // check if we selected "cancel"

  if ($form_state['clicked_button']['#parents'][0] == "cancel") {

    $form_state['redirect'] = schedule_url($base_user_url, $form_state['values']['schedule_id'], "", "cancel");
  } else {

    $user = eto_user_load(check_plain($form_state['values']['uid']));

    $timezone = user_tz($user);

    $date = date_extract($form_state['values']['date_field'], "date");
    if ($form_state['values']['schedule_type'] == SCHEDULE_AVAILABILITY) {
      // need this fancier since we're not using date module anymore.
      //      $start_date = $date . ' ' . date_extract($form_state['values']['start_time'], "time");
      //      $end_date = $date . ' ' . date_extract($form_state['values']['end_time'], "time");
      $start_date = $date . ' ' . sb_date_extract($form_state['values']['start_time'], 'time');
      $end_date = $date . ' ' . sb_date_extract($form_state['values']['end_time'], 'time');
    } else {
      $start_date = $date . ' 00:00:00';
      $end_date = $date . ' 23:59:59';
    }
    $start = date_make_date($start_date, $timezone);
    $end = date_make_date($end_date, $timezone);

    if (date_format($start, "U") > date_format($end, "U") ) {
      // crossed a day boundary, so end our interval on the next day
      $interval = date_interval_create_from_date_string('1 day');
      $end = date_add($end, $interval);
    }

    // get our title before we change timezones
    $form_state['values']['title'] = check_plain($user->name) . ": " . date_format($start, "l g:ia") . " - " . date_format($end, "l g:ia") . " (" . date_format($start, "T") . ")";

    // convert date and times from our TZ to UTC
    date_timezone_set($start, timezone_open('UTC'));
    date_timezone_set($end, timezone_open('UTC'));

    // save them for the database
    $form_state['values']['start_datetime_utc'] = date_format($start, DATE_FORMAT_DATETIME);
    $form_state['values']['end_datetime_utc'] = date_format($end, DATE_FORMAT_DATETIME);

    $form_state['values']['rate_override'] = sb_rate_validate($form_state['values']['hourly_rate']);

    // insert if needed, otherwise update
    if ($form_state['values']['schedule_id'] > 0) {
      // update
      sb_schedule_update( (object) $form_state['values']);
      $id = $form_state['values']['schedule_id'];
      $verb = "updated";
    } else {
      // insert
      $id = sb_schedule_insert( (object) $form_state['values']);
      $verb = "created";
    }

    if ($form_state['values']['schedule_type'] == SCHEDULE_AVAILABILITY) {
      $noun = "session";
    } else {
      $noun = "vacation";
    }

    drupal_set_message("Your $noun has been $verb successfully.", "success");
    $form_state['redirect'] = $base_user_url . "/calendar";
  }
}


function sb_date_extract($date, $part) {
  // only handles time right now, as that's all we need
  if ($part == 'time') {
    $r = preg_match('/^(\d+)(:\d+)?(\s*)(am|pm)$/i', $date, $matches);

    $hour = ''; $minute = '00'; $ampm = '';
    array_shift($matches); // skip the full string
    foreach ($matches as $part) {
      if (!$hour) {
        $hour = $part; // always the first
      }
      else if (preg_match('/:(\d+)/', $part, $m)) {
        $minute = $m[1];
      }
      else if (preg_match('/pm/i', $part) && ($hour < 12)) {
        $hour += 12;
      }
    }

    return "$hour:$minute:00";
  }
  else {
    return '';
  }
}

/**
 * Implementation of hook_insert()
 */

function sb_schedule_insert($node) {
  global $user;

  db_query("INSERT INTO {sb_schedule}
              (title, body, uid,
               status_flag, schedule_type, start_datetime_utc,
               end_datetime_utc, repeat_interval, repeat_count)
            VALUES
              ('%s', '%s', %d, %d, %d, '%s', '%s', '%s', %d)",
	   $node->title, $node->body, $node->uid,
	   $node->status_flag, $node->schedule_type, $node->start_datetime_utc,
	   $node->end_datetime_utc, $node->repeat_interval, $node->repeat_count);
  $schedule_id = db_last_insert_id("sb_schedule", "schedule_id");

  // need to insert nodes for as many repeats as we have
  sb_schedule_insert_repeat_event($node, $schedule_id);

  return $schedule_id;
}


/**
 * implementation of hook_update()
 */

function sb_schedule_update($node) {
  $original_node = node_load($node->nid);
  $str_fields = array('repeat_interval', 'start_datetime_utc', 'end_datetime_utc');
  foreach ($str_fields as $field) {
    if (! strcmp($original_node->{$field}, $node->{$field})) {
      $significant_change ++;
    }
  }
  if ($original_node->repeat_count != $node->repeat_count) {
    $significant_change ++;
  }

  db_query("UPDATE {sb_schedule} SET
                title = '%s',
                body = '%s',
                uid = %d,
                status_flag = %d,
                schedule_type = %d,
                start_datetime_utc = '%s',
                end_datetime_utc = '%s',
                repeat_interval = %d,
                repeat_count = %d
              WHERE schedule_id = %d",
	   $node->title, $node->body, $node->uid,
	   $node->status_flag, $node->schedule_type, $node->start_datetime_utc,
	   $node->end_datetime_utc, $node->repeat_interval, $node->repeat_count,
	   $node->schedule_id);

  // if the date,time, or repeat count changed we need to drop our blocks and recreate them
  if ($significant_change) {
    sb_schedule_delete_repeat_event($node->schedule_id);
    sb_schedule_insert_repeat_event($node, $node->schedule_id);
  }
}

function sb_schedule_delete_repeat_event($id) {
  db_query("DELETE FROM {sb_schedule_event} WHERE schedule_id = %d", $id);
}

function sb_schedule_insert_repeat_event ($node, $parent_id) {
  $interval = $node->repeat_interval;

  if ($interval == "week") {
    $interval = "P1W";
  } else if ($interval == "day") {
    $interval = "P1D";
  } else {
    // same for now
    $interval = "P1D";
  }
  $interval = new DateInterval($interval);
  $date_start = new DateTime($node->start_datetime_utc);
  $date_end = new DateTime($node->end_datetime_utc);

  for ($i = 1; $i<= $node->repeat_count; $i++) {
    db_query("INSERT INTO {sb_schedule_event}
                (schedule_id, status_flag, start_datetime_utc, end_datetime_utc)
              VALUES
                (%d, %d, '%s', '%s')",
	     $parent_id, "1", date_format($date_start, DATE_FORMAT_DATETIME), date_format($date_end, DATE_FORMAT_DATETIME));
    $date_start->add($interval);
    $date_end->add($interval);
  }
}



/*****************************************************************************
 *****************************************************************************
 *
 * SECTION:
 *
 * Appointment functionality
 *
 *****************************************************************************
 *****************************************************************************/

function sb_appointment_view($arg) {
  $array = sb_parse_arg($arg);

  if ($array[1]) {
    $requested_event_id = $array[1];
  }
  $id = $array[0];

  //
  // 2 ways we can view an appointment:
  // (1) able to view all appointments or
  // (2) a member of the appointment
  //

  if ($requested_event_id && (! is_numeric($requested_event_id))) {
    drupal_not_found();
    exit;
  }

  global $user;
  if (user_access('view all appointments') || isa_participant($id, $user->uid)) {

    $output .= _appointment_tabs($id, "view");
    $output .= "<div id='tutor-session-view'>\n";

    $appointment = sb_appointment_load($id);
    drupal_set_title($appointment->title);

    $tutor = eto_user_load($appointment->tutor_uid);

    if ($appointment->student_uid_0) {
      $student = eto_user_load($appointment->student_uid_0);
    }

    $output .= "<div class='tutor-session'>";

    $noun = "Appointment";
    $output .= "<span class='appointment-title'><strong>$noun:</strong> " . $appointment->title . "</span><br>";

    if ($requested_event_id) {
      // if we asked for a particular event, show that no matter what
      $selected_event = sb_appointment_event_load(array(
                          'appointment_event_id' => $requested_event_id));
      if ((!$selected_event) || ($selected_event->appointment_id != $id)){
	// if we couldn't load it, not found
	drupal_not_found();
	exit;
      }
    }

    if ($appointment->repeat_count > 1) {
      $events = sb_appointment_load_events($appointment->appointment_id);
      $first = $events[0];
      $last = $events[count($events) - 1];

      $output .= theme_appointment_details($appointment, $first, $last, $selected_event);

      if ($selected_event) {
	$event = $selected_event;
      } else {
	// change our start/end to the next instance if we didn't ask for a particular one
        $now = date_make_date(time(), "UTC");
        $from_date = date_format($now, DATE_FORMAT_DATETIME);
	$next = sb_appointment_next_event_load($appointment->appointment_id, $from_date); // FIXME?

	if ($next) {
	  $event = $next;
	} else {
	  $event = $last;
	}
      }
    } else {
      $next = sb_appointment_next_event_load($appointment->appointment_id, 0); // Next and, well, only!
      $event = $next;
      $output .= theme_appointment_details($appointment, $next, $next, $event);
    }

    $start_datetime_utc = $event->start_datetime_utc;
    $end_datetime_utc = $event->end_datetime_utc;
    $cancelled = $event->status_flag;

    $output .= "</div>";

    if ($appointment->repeat_count > 1) {
      $output .= theme('tutor_session_time', $start_datetime_utc, $end_datetime_utc, $event);
    }

    $output .= theme('tutor_session_view_all', $appointment, $requested_event_id);


    if ($appointment->appointment_type != PERSONAL_SESSION) {
      // use the teacher's room for group sessions, the student's room otherwise
      if ($appointment->appointment_type == TUTOR_SESSION) {
	$output .= theme('vyew_room', $student->name, $event->appointment_event_id);
      } else {
	$output .= theme('vyew_room', $tutor->name, $event->appointment_event_id);
      }

      $tutor_string = _construct_participant_string($appointment, "tutor");
      $student_string = _construct_participant_string($appointment, "student");
      $output .= theme('meeting_participants', $tutor_string, $student_string);
    }

    if ($appointment->body) {
      $output .= theme('meeting_notes', check_markup($appointment->body, 1));
    }

  } else {
    drupal_access_denied();
    exit;
  }

  $output .= "</div>\n";
  return $output;
}

function theme_appointment_details ($appointment, $first, $last, $event) {
  global $user;

  $start = date_make_date($first->start_datetime_utc, "UTC");
  date_timezone_set($start, timezone_open(user_tz()));

  if ($first != $last) {
    $plural = "s";
  } else {
    $plural = ", " . date_format($start, "F jS");
  }

  $output .= "<span>";
  $output .= "Scheduled on " . date_format($start, "l") . $plural . " at " . date_format($start, TIME_FORMAT_ETO_STANDARD) . " for " . $appointment->duration . " minutes";
  if ($first != $last) {
    $output .= " from " . date_format($start, "M d, Y");

    $end = date_make_date($last->start_datetime_utc, "UTC");
    date_timezone_set($end, timezone_open(user_tz()));
    $output .= " until " . date_format($end, "M d, Y");
  }

  $output .= "</span>";

  if ($appointment->status_flag == 0) {
    $output .= "<br><span class='cancelled'>This series of appointments is cancelled.</span>";
    if (user_access('view all appointments') || ($appointment->tutor_uid == $user->uid)) {
      $output .= "<br><span>Canceled in error? " . l('Reinstate this appointment series.', "appointments/" . sb_encode($appointment->appointment_id) . "/reinstate") . "</span>";
    }
  } else if (user_access('view all appointments') || ($appointment->tutor_uid == $user->uid)) {
    $output .= "<br><span class='canceled'>" . l('Cancel all future appointments in this series.', "appointments/" . sb_encode($appointment->appointment_id) . "/cancel") . "</span>";
  }

  if ($event) {
    $output .= theme_cancellations_summary($event);
  }

  return $output;
}

function _appointment_tabs($id, $active) {
  if ($active == 'view') {
    $view_active = "class='active'";
  } else if ($active == 'edit') {
    $edit_active = "class='active'";
  } else if ($active == 'cancel') {
    $cancel_active = "class='active'";
  }
  return "
<ul class='tabs primary'>
<li $view_active><a href='" . appointment_url("/", $id) . "' $view_active><span class='link-span'>View</span></a></li>
<li $edit_active><a href='" . appointment_url("/", $id, "", "edit") . "' $edit_active><span class='link-span'>Edit</span></a></li>
<li $cancel_active><a href='" . appointment_url("/", $id, "", "cancel") . "' $cancel_active><span class='link-span'>Cancel</span></a></li>
</ul>";
}

function user_schedule_appointment_display ($name, $arg) {

  if ($name) {
    $calendar_user = eto_user_load($name);
  }
  if (!$calendar_user) {
    return drupal_not_found();
  }

  $app = _schedule_decode_time_args($arg);

  $title = "Schedule session with " . $name . " on " . date_format($app['target_date_user'], 'M jS, Y');
  if ($app['block_exists']) {
    $title .= " at " . date_format($app['target_date_user'], 'g:ia');
  }
  drupal_set_title($title);

  $output .= "<h2>" . $title . "</h2>";

  // if we're a student, we can't schedule appointments in the past; if we're a tutor, warn

  $now = date_make_date(time(), 'UTC');
  if (($now >= $app['target_date_utc']) && $app['block_exists']) {
    if (user_access("is a tutor") || user_access("administer the site")) {
      drupal_set_message("Warning: you are scheduling an appointment in the past.", "warning");
    } else {
      $output .= "<p>Sorry, you cannot schedule appointments in the past.  Please pick another time or day!</p>";
      return $output;
      // exits
    }
  } else {
    $interval = new DateInterval('PT' . variable_get('eto_minimum_appointment_lead_time', 6) . 'H');
    date_add($now, $interval);
    if (($now >= $app['target_date_utc']) && $app['block_exists']) {
      if (user_access("is a tutor") || user_access("administer the site")) {
	drupal_set_message("Warning: you are scheduling an appointment within " . variable_get('eto_minimum_appointment_lead_time', 6) . " hours.", "warning");
      } else {
	$output .= "<p>Sorry, you cannot schedule appointments without at least " . variable_get('eto_minimum_appointment_lead_time', 6) . " hours lead time.  Please pick another time or day!</p>";
	return $output;
	// exits
      }
    }
  }

  global $user;
  $type = "sb_appointment";
  $node = array('uid' => $user->uid,
		'name' => (isset($user->name) ? $user->name : ''),
		'type' => $type,
		'language' => '',
		'schedule_id' => $app['schedule_id']);
  $node['tutor_uid'] = $calendar_user->uid;
  $node['start_datetime_utc'] = date_format($app['target_date_utc'], DATE_FORMAT_DATETIME);
  $node['block_exists'] = $app['block_exists'];

  $output .= drupal_get_form('sb_appointment_form', (object) $node);

  return $output;
}

function user_schedule_appointment_edit($arg) {
  $array = sb_parse_arg($arg);
  $id = $array[0]; // appointment id
  $app = sb_appointment_load($id);
  if (! $app) {
    return drupal_not_found();
  }

  $date = date_make_date($app->start_datetime_utc, 'UTC');
  date_timezone_set($date, timezone_open(user_tz()));

  // else
  $title = "Edit Session with " . _sb_format_user($app->tutor_uid) . " on " . date_format($date, 'M jS, Y') . " at " . date_format($date, 'g:ia');
  drupal_set_title($title);

  $output .= _appointment_tabs($id, "edit");
  $output .= "<h2>$title</h2>";

  if ($app->appointment_id && user_access('schedule repeating appointments')) {
    $output .= "<div class='message warning'>Warning: if you change the number of weeks or the date/time of this session, you will lose any cancellations you made.</div>\n";
  }

  $output .= drupal_get_form('sb_appointment_form', $app);

  return $output;
}


function sb_appointment_form(&$form_state, $node) {
  $form = array();
  ahah_helper_register($form, $form_state);
  $type = node_get_types('type', $node);
  global $user;

  $_base_defaults = array ('duration' => 15,
			   'appointment_type' => TUTOR_SESSION,
			   'participant_count' => 1,
			   );

  foreach ( array('duration') as $key ) {
    if (!isset($form_state['storage']['participants']['rates'][$key])) {
      $default[$key] = ($node->$key ? $node->$key : $_base_defaults[$key]);
    } else {
      $default[$key] = $form_state['storage']['participants']['rates'][$key];
    }
  }
  // hourly rate default is handled differently, needs to be done after duration, and either way we want to store the set price
  $default['hourly_rate_calculated'] = sb_get_hourly_rate($node->tutor_uid, $node->start_datetime_utc, $default['duration']);
  if (! isset($form_state['storage']['participants']['rates']['hourly_rate_override'])) {
    $default['hourly_rate'] = $default['hourly_rate_calculated'];
  } else {
    $new_rate = sb_rate_validate($form_state['storage']['participants']['rates']['hourly_rate_override']);
    $default['hourly_rate'] = $new_rate;
  }
  foreach ( array('appointment_type', 'participant_count') as $key ) {
    if (!isset($form_state['storage']['participants'][$key])) {
      $default[$key] = ($node->$key ? $node->$key : $_base_defaults[$key]);
    } else {
      $default[$key] = $form_state['storage']['participants'][$key];
    }
  }
  // special case
  if (isset($form_state['storage']['participants']['add_more'])) {
    $default['participant_count'] = $default['participant_count'] + 1;
  }
  // students
  if ($default['appointment_type'] == TUTOR_SESSION) {
    $i = 0;
    if (isset($form_state['storage']['participants']['student_' . $i])) {
      // assign our default
      $default['student_' . $i] = $form_state['storage']['participants']['student_' . $i];
      // make sure student exists
      $s = eto_user_load($default['student_' . $i]);
      if (!$s) {
	// student doesn't exist
	$default['student_message_' . $i] = "<div class='error message'>" . $default['student_' . $i] . " doesn't have an account on this website.  Perhaps you misspelled it?</div>";
      } else {
	// check if the student is available at this time
	$result = sb_check_student_availability($default['student_' . $i],
						$form_state['storage']['start_datetime_utc'],
						$form_state['storage']['participants']['rates']['duration'],
						$form_state['storage']['appointment_id']);
	if ($result > 0) {
	  $default['student_message_' . $i] = "<div class='error message'>" . $s->name . " is busy at that time.</div>";
	} else {
	  $default['student_message_' . $i] = "";
	}
      }
    } else if ($node->{"student_" . $i}) {
      $default['student_' . $i] = $node->{"student_" . $i};
    }
  } else if ($default['appointment_type'] == GROUP_SESSION) {
    for ($i = 1; $i<= 100; $i++) {
      // if we have a formstate use that; otherwise if we have a student available, use that; otherwise, nothing
      if (isset($form_state['storage']['participants']['student_' . $i])) {
	$default['student_' . $i] = $form_state['storage']['participants']['student_' . $i];
	// make sure student exists
	$s = eto_user_load($default['student_' . $i]);
	if (!$s) {
	  // student doesn't exist
	  $default['student_message_' . $i] = "<div class='error message'>" . $default['student_' . $i] . " doesn't have an account on this website.  Perhaps you misspelled it?</div>";
	} else {
	  // check if the student is available at this time
	  $result = sb_check_student_availability($default['student_' . $i],
						  $form_state['storage']['start_datetime_utc'],
						  $form_state['storage']['participants']['rates']['duration'],
						  $form_state['storage']['appointment_id']);
	  if ($result > 0) {
	    $default['student_message_' . $i] = "<div class='error message'>" . $s->name . " is busy at that time.</div>";
	  } else {
	    $default['student_message_' . $i] = "";
	  }
	}
      } else if ($node->{"student_" . $i}) {
	$default['student_' . $i] = $node->{"student_" . $i};
      }
    }
  }



  /****************************************
   * Hidden
   ****************************************/

  $form['status_flag'] = array (
				  '#type' => 'hidden',
				  '#value' => ($node->status_flag ? $node->status_flag : 1),
				  );
  $form['start_datetime_utc'] = array (
				       '#type' => 'hidden',
				       '#value' => $node->start_datetime_utc
				       );
  $form['appointment_id'] = array (
				   '#type' => 'hidden',
				   '#value' => ($node->appointment_id ? $node->appointment_id : ""),
				   );

  $form['schedule_id'] = array (
				   '#type' => 'hidden',
				   '#value' => ($node->schedule_id ? $node->schedule_id : ""),
				   );

  $form['repeat_interval'] = array (
				    '#type' => 'hidden',
				    '#value' => ($node->repeat_interval ? $node->repeat_interval : 'week'),
				    );

  /****************************************
   * Basic Fields
   ****************************************/

  $form['title'] = array (
			  '#type' => 'textfield',
			  '#title' => "Session Title",
			  '#description' => 'If you do not provide a title, we will generate one for you',
			  '#default_value' => ($node->title ? $node->title : $form_state['storage']['title']),
			  );

  if (user_access('is a tutor')) {

    $form['repeat_count'] = array (
				     '#type' => 'textfield',
				     '#size' => '3',
				     '#title' => 'How many weeks would you like this repeated for?',
				     '#default_value' => ($node->repeat_count ? $node->repeat_count : 1),
				     '#field_suffix' => ' week(s)',
				     '#description' => 'If you want this time to be scheduled for more than one week, enter the number of weeks here. Maximum allowed is 16.',
				   );
  }

  if (user_access('is a tutor') && (! $node->block_exists)) {
    $form['start_time'] = array (
      '#type' => 'textfield',
      '#field_suffix' => $timezone_suffix,
      '#title' => 'Start Time' . $timezone_suffix,
      '#default_value' => ($node->start_datetime_owner ? $node->start_datetime_owner : ""),
      '#description' => "Format: 9:30am",
      '#required' => TRUE,
      '#size' => 8,
      '#attributes' => array ('tabindex' => 1),
    );
  }


  /****************************************
   * Participants Fieldset
   ****************************************/

  // this is only visible if we're a tutor

  if (user_access('is a tutor')) {

    $form['participants'] = array(
				  '#type' => 'fieldset',
				  '#title' => 'Session Participants',
				  '#prefix' => '<div id="participants-wrapper">',
				  '#suffix' => '</div>',
				  '#tree' => TRUE,
				  );
    $form['participants']['appointment_type'] = array (
						   '#type' => 'radios',
						   '#options' => array (TUTOR_SESSION => 'Tutor Session',
									GROUP_SESSION => 'Group Session',
									PERSONAL_SESSION => 'Personal Time'),
						   '#title' => 'Type of Session',
						   '#default_value' => $default['appointment_type'],
						   '#ahah' => array(
								    'event' => 'change',
								    'path' => ahah_helper_path(array('participants')),
								    'wrapper' => 'participants-wrapper',
								    ),
						   );

    if ($default['appointment_type'] == PERSONAL_SESSION) {

      $form['participants']['tutor_uid'] = array (
						  '#type' => 'hidden',
						  '#value' => _sb_format_user($node->tutor_uid ? $node->tutor_uid : $form_state['storage']['participants']['tutor_uid']),
						  );
    } else {
      $form['participants']['tutor_uid'] = array (
						  '#type' => 'textfield',
						  '#title' => 'Tutor',
						  '#disabled' => TRUE,
						  '#value' => _sb_format_user($node->tutor_uid ? $node->tutor_uid : $form_state['storage']['participants']['tutor_uid']),
						  );
    }

    if ($default['appointment_type'] == TUTOR_SESSION) {
      $form['participants']['student_0'] = array (
						  '#type' => 'select',
						  '#title' => 'Student ',
						  '#options' => sb_tutors_students_array($node->tutor_uid, "--Select a Student--"),
						  '#default_value' => $default['student_0'],
						  '#description' => $default['student_message_0'],
						  '#ahah' => array(
								   'event' => 'change',
								   'path' => ahah_helper_path(array('participants')),
								   'wrapper' => 'participants-wrapper',
								   ),
						  );
    } else if ($default['appointment_type'] == GROUP_SESSION) {
      // this is a group session, so we want to be able to add more participants

      $form['participants']['participant_count'] = array (
						      '#type' => 'hidden',
						      '#value' => $default['participant_count'],
						      );

      for ($i = 1; $i <= $default['participant_count']; $i++) {
	$form['participants']['student_' . $i] = array (
							'#type' => 'select',
							'#title' => 'Student #' . $i,
							'#options' => sb_tutors_students_array($node->tutor_uid, "--Select a Student--"),
							'#default_value' => $default['student_' . $i],
							'#description' => $default['student_message_' . $i],
							'#ahah' => array(
									 'event' => 'change',
									 'path' => ahah_helper_path(array('participants')),
									 'wrapper' => 'participants-wrapper',
									 ),
							);
      }
      $form['participants']['add_more'] = array (
						 '#type' => 'submit',
						 '#value' => 'Add More Students',
						 '#submit' => array('ahah_helper_generic_submit'),
						 '#attributes' => array('class' => 'no-js'),
						 );

    }
  } else {
    // still need our formset, just called something else
    $form['participants'] = array(
				  '#type' => 'fieldset',
				  '#title' => '',
				  '#prefix' => '<div id="participants-wrapper">',
				  '#suffix' => '</div>',
				  '#tree' => TRUE,
				  );
    $form['participants']['tutor_uid'] = array (
						'#type' => 'hidden',
						'#value' => _sb_format_user($node->tutor_uid ? $node->tutor_uid : $form_state['storage']['participants']['tutor_uid']),
						);

    $form['participants']['appointment_type'] = array (
						       '#type' => 'hidden',
						       '#value' =>  $default['appointment_type'],
						       );
    // student doing this, so it's always ourselves as the student
    $form['participants']['student_0'] = array (
						  '#type' => 'hidden',
						  '#value' => $user->name,
						);
    // students cannot do repeating appointments
    $form['repeat_count'] = array (
				   '#type' => 'hidden',
				   '#value' => 1,
				   );


  }


  /****************************************
   * Rates Fieldset
   ****************************************/

  $form['participants']['para'] = array( '#value' => "<p></p>" );

  if ($default['appointment_type'] != PERSONAL_SESSION) {

    $form['participants']['rates'] = array(
					   '#type' => 'fieldset',
					   '#title' => 'Rate and Duration',
					   '#description' => '<strong>Cost: $' . sprintf('%.2f', ($default['hourly_rate'] / 100) * ($default['duration'] / 60)) . "</strong>",
					   '#prefix' => '<div id="rates-wrapper">',
					   '#suffix' => "</div>",
					   '#tree' => TRUE,
					   );

    // if we're working with a block, we can calculate availble duration times. otherwise have to wing it
    if ($block_exists) {
      $avail = _available_durations($node->start_datetime_utc, $node->tutor_uid);
      $avail_text = ( (count($avail) < 4) ? "If you want a longer session, please pick a time when the tutor has a longer block of time available." : "");
    }
    else {
      $avail = _available_durations();
    }
    $form['participants']['rates']['duration'] = array (
							'#type' => 'radios',
							'#options' => $avail,
							'#default_value' => $default['duration'],
							'#title' => 'Select your session duration',
							'#description' => $avail_text,
							'#ahah' => array(
									 'event' => 'change',
									 'path' => ahah_helper_path(array('participants')),
									 'wrapper' => 'participants-wrapper',
									 ),
							);
    $form['participants']['rates']['hourly_rate_markup'] = array (
								  '#type' => 'markup',
								  '#value' => '<strong>Rate:</strong> ' . theme('sb_rate', $default['hourly_rate_calculated'], '$') . '/hour' . " (" . sb_get_hourly_rate($node->tutor_uid, $node->start_datetime_utc, $default['duration'], TRUE) . ")",
								  );
    $form['participants']['rates']['hourly_rate'] = array (
							   '#type' => 'hidden',
							   '#value' => $default['hourly_rate_calculated']
							   );

    if (user_access('is a tutor')) {
      $form['participants']['rates']['hourly_rate_override'] = array (
								      '#type' => 'textfield',
								      '#size' => '4',
								      '#title' => 'New hourly rate',
								      '#description' => 'If you want to use a different hourly rate, enter it here',
								      '#default_value' => theme('sb_rate', $default['hourly_rate']),
								      '#field_prefix' => '$',
								      '#field_suffix' => '/hour',
								      '#ahah' => array(
										       'event' => 'change',
										       'path' => ahah_helper_path(array('participants')),
										       'wrapper' => 'participants-wrapper',
										       ),
								      );
    }
    $form['participants']['rates']['update_rate'] = array (
							   '#type' => 'submit',
							   '#value' => 'Update Duration' . (user_access('is a tutor') ? ' and Rate' : ""),
							   '#submit' => array('ahah_helper_generic_submit'),
							   '#attributes' => array('class' => 'no-js'),
							   );

  } else {
    // personal session just has a duration, and a bigger set of options

        $form['participants']['rates'] = array(
					   '#type' => 'fieldset',
					   '#title' => 'Duration',
					   '#prefix' => '<div id="rates-wrapper">',
					   '#suffix' => "</div>",
					   '#tree' => TRUE,
					   );
	$avail = _available_durations($node->start_datetime_utc, $node->tutor_uid, $default['appointment_type']);
    $avail_text = ( (count($avail) < 4) ? "If you want a longer session, please pick a time when you have a longer block of time available or cancel some tutoring sessions." : "");
    $form['participants']['rates']['duration'] = array (
							'#type' => 'radios',
							'#options' => $avail,
							'#default_value' => $default['duration'],
							'#title' => 'Select how much personal time you need reserved',
							'#description' => $avail_text,
							'#ahah' => array(
									 'event' => 'change',
									 'path' => ahah_helper_path(array('participants')),
									 'wrapper' => 'participants-wrapper',
									 ),
							);
  }

  $form['body'] = array (
			 '#type' => 'textarea',
			 '#title' => 'Notes',
			 '#description' => 'Notes will be visible to all session participants',
			 '#default_value' => $node->body,
			 );

  $form['submit'] = array (
			   '#type' => 'submit',
			   '#value' => 'Schedule Now',
			   );

  $form['#validate'][] = 'sb_appointment_validate';
  $form['#submit'] = array('sb_appointment_submit_handler');

  return $form;
}


function sb_appointment_validate($form, &$form_state) {
  // if we have a start_time set, it means we're setting a free-form appointment.
  // the start_time will be in the users timezone. we also have a UTC date in start_datetime_utc
  if (isset($form_state['values']['start_time'])) {
    $date = explode(' ', $form_state['values']['start_datetime_utc']);
    $date = $date[0] . ' ' . sb_date_extract($form_state['values']['start_time'], 'time');
    $date = date_create($date);
    date_timezone_set($date, timezone_open('UTC'));

    $form_state['values']['start_datetime_utc'] = date_format($date, DATE_FORMAT_DATETIME);
    $verify_block = FALSE;
  }
  else {
    $verify_block = TRUE;
  }

  if ($form_state['values']['hourly_rate']) {
    // needs to be of the form: 00.00 or 00, with an optional initial dollar sign
    $value = sb_rate_validate($form_state['values']['hourly_rate']);
    if (!preg_match('/^\d+$/', $value)) {
      form_set_error("hourly_rate", "You must enter a dollar value for the hourly rate ($value)", "error");
    }
  }
  $tutor = eto_user_load($form_state['values']['participants']['tutor_uid']);

  $avail = _available_durations($form_state['values']['start_datetime_utc'], $tutor->uid, $form_state['values']['participants']['appointment_type'], $verify_block);

  if ($form_state['values']['participants']['appointment_type'] == PERSONAL_SESSION) {
    $max = 180;
  } else {
    $max = 60;
  }
  if ( ($form_state['values']['participants']['rates']['duration'] <= 0) || ($form_state['values']['participants']['rates']['duration'] > $max) || (! in_array($form_state['values']['participants']['rates']['duration'], $avail))) {
    form_set_error("duration", "'" . $form_state['values']['participants']['rates']['duration'] . "' is not a valid session duration.");
  }

  if ($form_state['values']['participants']['appointment_type'] == TUTOR_SESSION) {
    $i = 0;
    $s = eto_user_load($form_state['values']['participants']['student_' . $i]);
    if (!$s) {
      form_set_error("student_" . $i, $form_state['values']['participants']['student_' . $i] . " doesn't have an account on this website.  Perhaps you misspelled it?", "error");
    } else {
      // check their availability
      $result = sb_check_student_availability($form_state['values']['participants']['student_' . $i],
					      $form_state['values']['start_datetime_utc'],
					      $form_state['values']['participants']['rates']['duration'],
					      $form_state['values']['appointment_id']
					      );
      if ($result > 0) {
	form_set_error("student_" . $i, $s->name . " is busy at that time.", "error");
      }
    }
  } else {
    for ($i = 1; $i<= 100; $i++) {
      if (isset($form_state['values']['participants']['student_' . $i])
	  && ($form_state['values']['participants']['student_' . $i] > 0)) {
	$s = eto_user_load($form_state['values']['participants']['student_' . $i]);
	if (!$s) {
	  form_set_error("student_" . $i, $form_state['values']['participants']['student_' . $i] . " doesn't have an account on this website.  Perhaps you misspelled it?", "error");
	} else {
	  // check their availability
	  $result = sb_check_student_availability($form_state['values']['participants']['student_' . $i],
						  $form_state['values']['start_datetime_utc'],
						  $form_state['values']['participants']['rates']['duration'],
						  $form_state['values']['appointment_id']
						  );
	  if ($result > 0) {
	    form_set_error("student_" . $i, $s->name . " is busy at that time.", "error");
	  }
	}
      }
    }
  }

  // max number of weeks for anything is 16
  if (isset($form_state['repeat_count']) && ($form_state['repeat_count'] > 16)) {
    form_set_error('repeat_count', 'Repeat cannot be more than 16.');
  }

  // make sure at least one student was selected if it's not personal time

  if ( ($form_state['values']['participants']['appointment_type'] != PERSONAL_SESSION)
       && (! $form_state['values']['participants']['student_0']) ) {
    form_set_error("student_0", "You must add at least one student to the appointment.");
  }

}


/*
 * our submit handler; constructs the correct start/end times and our title and sends notifications
 */

function sb_appointment_submit_handler($form, &$form_state) {
  $start_date = date_make_date($form_state['values']['start_datetime_utc'], 'UTC');
  $form_state['values']['start_datetime_utc'] = date_format($start_date, DATE_FORMAT_DATETIME);

  $interval = new DateInterval('PT' . $form_state['values']['participants']['rates']['duration'] . 'M');
  $end_date = date_add($start_date, $interval);
  $form_state['values']['end_datetime_utc'] = date_format($end_date, DATE_FORMAT_DATETIME);

  $form_state['values']['duration'] = $form_state['values']['participants']['rates']['duration'];
  $form_state['values']['appointment_type'] = $form_state['values']['participants']['appointment_type'];
  $form_state['values']['participant_count'] = $form_state['values']['participants']['participant_count'];

  $form_state['values']['rate_override'] = sb_rate_validate($form_state['values']['participants']['rates']['hourly_rate_override']);
  $form_state['values']['hourly_rate'] = $form_state['values']['participants']['rates']['hourly_rate'];

  // record the tutor properly
  $tutor = eto_user_load($form_state['values']['participants']['tutor_uid']);
  $form_state['values']['tutor_uid'] = $tutor->uid;

  if (!$form_state['values']['title']) {
    // generate a title
    if ($form_state['values']['appointment_type'] == TUTOR_SESSION) {
      $s = eto_user_load($form_state['values']['participants']['student_0']);
      $form_state['values']['title'] = "Tutor Session: " . $tutor->name . "/" . $s->name;
    } else if ($form_state['values']['appointment_type'] == GROUP_SESSION) {
      $form_state['values']['title'] = "Group Session with " . $tutor->name;
    } else { // PERSONAL_SESSION
      $form_state['values']['title'] = "Personal Time";
    }
  }

  // create an array of our non-tutor participants
  if ($form_state['values']['appointment_type'] == TUTOR_SESSION) {
    // just one participant
    $u = eto_user_load($form_state['values']['participants']['student_0']);
    if ($u) {
      $form_state['values']['student_uids'][] = $u->uid;
    }
  } else if ($form_state['values']['appointment_type'] == GROUP_SESSION) {
    for ($i = 1; $i <= $form_state['values']['participants']['participant_count']; $i++) {
      $u = eto_user_load($form_state['values']['participants']['student_' . $i]);
      if ($u && ($u->uid > 0)) {
	$form_state['values']['student_uids'][] = $u->uid;
      }
    }
    $form_state['values']['student_uids'] = array_unique($form_state['values']['student_uids']);
  }

  // send out mail if it's a tutor session or group session and there's only one instance
  if ( ($form_state['values']['repeat_count'] == 1)
       && ( ($form_state['values']['appointment_type'] == TUTOR_SESSION)
	    || $form_state['values']['appointment_type'] == GROUP_SESSION) ) {

    $messages = array();

    if ($tutor->mail) {
      $date = date_make_date($form_state['values']['start_datetime_utc'], "UTC");
      date_timezone_set($date, timezone_open(user_tz($tutor)));
      $app_date = date_format($date, DATE_FORMAT_ETO_LONG_NO_YEAR);
      $short_app_date = date_format($date, DATE_FORMAT_ETO_ABBREVIATED);
      $start_time = date_format($date, 'g:ia');
      $end_date = date_make_date($form_state['values']['end_datetime_utc'], "UTC");
      date_timezone_set($end_date, timezone_open(user_tz($tutor)));
      $end_time = date_format($end_date, 'g:ia (T)');

      $s = array();
      if ($form_state['values']['appointment_type'] == TUTOR_SESSION) {
	$u = eto_user_load($form_state['values']['participants']['student_0']);
	$s[] = $u->eto_profile->first_name . " " . $u->eto_profile->last_name;
      } else if ($form_state['values']['appointment_type'] == GROUP_SESSION) {
	for ($i = 1; $i <= $form_state['values']['participants']['participant_count']; $i++) {
	  $u = eto_user_load($form_state['values']['participants']['student_' . $i]);
	  if ($u && ($u->uid > 0)) {
	    $s[] = $u->eto_profile->first_name . " " . $u->eto_profile->last_name;
	  }
	}
      }
      // special for tutor
      $tutor_name = ($tutor->eto_profile->first_name ? $tutor->eto_profile->first_name . " " . $tutor->eto_profile->last_name : $tutor->name);


      // always email
      if (preg_match(EMAIL_REGEX, $tutor->mail) === 1) {
	$params = array('tutor_full_name' =>  $tutor_name,
			'student_full_name' => implode(", ", $s),
			'date' => $app_date,
			'start_time' => $start_time,
			'end_time' => $end_time);
	drupal_mail("eto_admin",
		    "session_confirmation_tutor",
		    $tutor->mail,
		    language_default(),
		    $params,
		    variable_get('site_mail', 'admin@elitetutorsonline.com'),
		    TRUE);
	watchdog('eto_notifications',
		 'Emailing contirmation to @tutor_name (@tutor_id)',
		 array('@tutor_name' => $tutor->name,
		       '@tutor_id' => $tutor->uid));
      }


      // then SMS if we asked for it and can
      $method = notifications_user_setting('send_method', $tutor);
      if (($method == 'sms') && ($tutor->sms_user[0]['number'] > 0) && ($tutor->sms_user[0]['status'] == 2)) {
	watchdog('eto_notifications',
		 'Texting confirmation to @tutor_name (@tutor_id)',
		 array('@tutor_name' => $tutor->name,
		       '@tutor_id' => $tutor->uid));

	$texts[] = array ('uid' => $tutor->uid,
			  'message' => "ETO session booked $short_app_date $start_time - $end_time - " . $u->eto_profile->first_name . " " . $u->eto_profile->last_name,
			  );
      }

    }

    if ($form_state['values']['appointment_type'] == TUTOR_SESSION) {

      $u = eto_user_load($form_state['values']['participants']['student_0']);
      profile_load_profile($u);

      $date = date_make_date($form_state['values']['start_datetime_utc'], "UTC");
      date_timezone_set($date, timezone_open(user_tz($u)));
      $app_date = date_format($date, DATE_FORMAT_ETO_LONG_NO_YEAR);
      $start_time = date_format($date, 'g:ia');
      $end_date = date_make_date($form_state['values']['end_datetime_utc'], "UTC");
      date_timezone_set($end_date, timezone_open(user_tz($u)));
      $end_time = date_format($end_date, 'g:ia (T)');

      // see if we want to be notified via text
      /*
      if ($u->profile_notify_with == "Text Message") {
	if ( ($u->sms_user[0]['number'] > 0) && ($u->sms_user[0]['status'] == 2) ) {
	  $texts[] = array ('uid' => $u->uid,
			    'message' => "Tutoring $app_date $start_time - $end_time " . $tutor_name,
			    );
	}

      } else if ($u->mail) {
      */

      if ($u->mail) {
	$messages[] = array('email' => $u->mail,
			    'name' => $u->eto_profile->first_name . " " . $u->eto_profile->last_name,
			    'date' => $app_date,
			    'start_time' => $start_time,
			    'end_time' => $end_time);
      }
      if ( ($u->eto_profile->email_parent) && ($u->mail != $u->eto_profile->email_parent)) {
	$messages[] = array('email' => $u->eto_profile->email_parent,
			    'name' => $u->eto_profile->first_name . " " . $u->eto_profile->last_name,
			    'date' => $app_date,
			    'start_time' => $start_time,
			    'end_time' => $end_time);
      }


    } else if ($form_state['values']['appointment_type'] == GROUP_SESSION) {

      for ($i = 1; $i <= $form_state['values']['participants']['participant_count']; $i++) {

	$u = eto_user_load($form_state['values']['participants']['student_' . $i]);
	profile_load_profile($u);

	if ($u && ($u->uid > 0)) {
	  $date = date_make_date($form_state['values']['start_datetime_utc'], "UTC");
	  date_timezone_set($date, timezone_open(user_tz($u)));
	  $app_date = date_format($date, DATE_FORMAT_ETO_LONG_NO_YEAR);
	  $start_time = date_format($date, 'g:ia');
	  $end_date = date_make_date($form_state['values']['end_datetime_utc'], "UTC");
	  date_timezone_set($end_date, timezone_open(user_tz($u)));
	  $end_time = date_format($end_date, 'g:ia (T)');

	  // see if we want to be notified via text
	  /*
	  if ($u->profile_notify_with == "Text Message") {
	    if ( ($u->sms_user[0]['number'] > 0) && ($u->sms_user[0]['status'] == 2) ) {
	      $texts[] = array ('uid' => $u->uid,
				'message' => "Tutoring $app_date $start_time - $end_time " . $tutor_name,
				);
	    }

	  } else if ($u->mail) {
*/
	  if ($u->mail) {
	    $messages[] = array('email' => $u->mail,
				'name' => $u->eto_profile->first_name . " " . $u->eto_profile->last_name,
				'date' => $app_date,
				'start_time' => $start_time,
				'end_time' => $end_time);
	  }
	  if ( ($u->eto_profile->email_parent) && ($u->mail != $u->eto_profile->email_parent)) {
	    $messages[] = array('email' => $u->eto_profile->email_parent,
				'name' => $u->eto_profile->first_name . " " . $u->eto_profile->last_name,
				'date' => $app_date,
				'start_time' => $start_time,
				'end_time' => $end_time);
	  }
	}
      }
    }

    if (count($messages) > 0) {
      $tutor_name = ($tutor->eto_profile->first_name ? $tutor->eto_profile->first_name . " " . $tutor->eto_profile->last_name : $tutor->name);
      foreach ($messages as $m) {
	$params = array('tutor_full_name' => $tutor_name,
			'student_full_name' => $m['name'],
			'date' => $m['date'],
			'start_time' => $m['start_time'],
			'end_time' => $m['end_time']);
	if (preg_match(EMAIL_REGEX, $m['email']) === 1) {
	  drupal_mail("eto_admin",
		      "session_confirmation",
		      $m['email'],
		      language_default(),
		      $params,
		      variable_get('site_mail', 'admin@elitetutorsonline.com'),
		      TRUE);
	}
      }
    }

    if (count($texts) > 0) {
      foreach ($texts as $t) {
	sms_user_send($t['uid'], $t['message']);
      }
    }

  }

  // insert if needed, otherwise update
  if ($form_state['values']['appointment_id'] > 0) {
    // update
    sb_appointment_update( (object) $form_state['values']);
    $id = $form_state['values']['appointment_id'];
    $verb = "updated";
  } else {
    // insert
    $id = sb_appointment_insert( (object) $form_state['values']);
    $verb = "created";
  }

  $calendar = 'users/' . $tutor->name . '/schedule';

  drupal_set_message("Your appointment has been $verb successfully. " . l("Click here to return to " . $tutor->name . "'s calendar.", $calendar), "success");
  // this is annoying, why is ahah_helper not clearing storage?
  $form_state['storage'] = array();

  $form_state['redirect'] = appointment_url("", $id);
}


/**
 * Implementation of hook_insert()
 */

function sb_appointment_insert($node) {
  global $user;

  db_query("INSERT INTO {sb_appointment}
              (schedule_id, title, body, tutor_uid, status_flag, appointment_type, start_datetime_utc,
               end_datetime_utc, duration, repeat_interval, repeat_count, hourly_rate, rate_override)
            VALUES
              (%d, '%s', '%s', %d, %d, %d, '%s', '%s', '%d', '%s', %d, %d, %d)",
	   $node->schedule_id, $node->title, $node->body, $node->tutor_uid, $node->status_flag,
	   $node->appointment_type, $node->start_datetime_utc, $node->end_datetime_utc, $node->duration,
	   $node->repeat_interval, $node->repeat_count, $node->hourly_rate, $node->rate_override);
  $appointment_id = db_last_insert_id("sb_appointment", "appointment_id");

  // need to insert nodes for as many repeats as we have
  sb_appointment_insert_repeat_event($node, $appointment_id);
  if ($node->student_uids) {
    sb_appointment_insert_participants($node, $appointment_id);
  }

  return $appointment_id;
}


/**
 * implementation of hook_update()
 */

function sb_appointment_update($node) {
  $original_node = sb_appointment_load($node->appointment_id);
  $str_fields = array('repeat_interval', 'start_datetime_utc', 'end_datetime_utc');
  foreach ($str_fields as $field) {
    if (! strcmp($original_node->{$field}, $node->{$field})) {
      $significant_change ++;
    }
  }
  if ($original_node->repeat_count != $node->repeat_count) {
    $significant_change ++;
  }

  db_query("UPDATE {sb_appointment} SET
                schedule_id = %d,
                title = '%s',
                body = '%s',
                tutor_uid = %d,
                status_flag = %d,
                appointment_type = %d,
                start_datetime_utc = '%s',
                end_datetime_utc = '%s',
                duration = %d,
                repeat_interval = %d,
                repeat_count = %d,
                hourly_rate = %d,
                rate_override = %d
              WHERE appointment_id = %d",
	   $node->schedule_id, $node->title, $node->body, $node->tutor_uid, $node->status_flag,
	   $node->appointment_type, $node->start_datetime_utc, $node->end_datetime_utc, $node->duration,
	   $node->repeat_interval, $node->repeat_count, $node->hourly_rate, $node->rate_override,
	   $node->appointment_id);

  // if the date,time, or repeat count changed we need to drop our blocks and recreate them
  if ($significant_change) {
    sb_appointment_delete_repeat_event($node->appointment_id);
    sb_appointment_insert_repeat_event($node, $node->appointment_id);
  }
  sb_appointment_delete_participants($node->appointment_id);
  if ($node->student_uids) {
    sb_appointment_insert_participants($node, $node->appointment_id);
  }
}


function sb_appointment_delete_repeat_event($id) {
  db_query("DELETE FROM {sb_appointment_event} WHERE appointment_id = %d", $id);
}

function sb_appointment_insert_repeat_event ($node, $parent_id) {
  $interval = $node->repeat_interval;

  if ($interval == "week") {
    $interval = "P1W";
  } else {
    // same for now
    $interval = "P1W";
  }
  $interval = new DateInterval($interval);
  $date_start = new DateTime($node->start_datetime_utc);
  $date_end = new DateTime($node->end_datetime_utc);

  for ($i = 1; $i<= $node->repeat_count; $i++) {
    db_query("INSERT INTO {sb_appointment_event}
                (appointment_id, status_flag, start_datetime_utc, end_datetime_utc)
              VALUES
                (%d, %d, '%s', '%s')",
	     $parent_id, "1", date_format($date_start, DATE_FORMAT_DATETIME), date_format($date_end, DATE_FORMAT_DATETIME));
    $date_start->add($interval);
    $date_end->add($interval);
  }
}

function sb_appointment_delete_participants($id) {
  db_query("DELETE FROM {sb_appointment_participants} WHERE appointment_id = %d", $id);
}

function sb_appointment_insert_participants($node, $appointment_id) {
  $ids = $node->student_uids;
  foreach ($ids as $id) {
    if ($id > 0) {
      db_query("INSERT INTO {sb_appointment_participants}
                   (appointment_id, participant_uid)
                VALUES
                   (%d, %d)",
	       $appointment_id, $id);
    }
  }
}


function user_schedule_appointment_action($arg, $action) {
  $output = "";

  $array = sb_parse_arg($arg);

  if ($array[1]) {
    $instance = $array[1];
  }
  $id = $array[0];
  $action = strtolower(check_plain($action));
  $action_string = ucwords($action);

  if (! $id) {
    drupal_not_found();
    exit;
  }
  $appointment = sb_appointment_load($id);
  if (! $appointment) {
    drupal_not_found();
    exit;
  }

  if ($instance) {
    $event = sb_appointment_event_load(array(
               'appointment_event_id' => $instance));
    if (! $event) {
      drupal_not_found();
      exit;
    }
  }

  // cannot cancel a canceled appointment; cannot reinstate a non-canceled appointment
  if ($action == "reinstate") {
    if ($instance) {
      if ($event->status_flag == 1) {
	return "You cannot reinstate an appointment that isn't canceled.";
      }
    } else if ($appointment->status_flag == 1) {
      return "You cannot reinstate an appointment that isn't canceled.";
    }
  } else if ($action == "cancel") {
    if ($instance) {
      if ($event->status_flag == 0) {
	return "You cannot cancel an appointment that is already canceled.";
      }
    } else if ($appointment->status_flag == 0) {
      return "You cannot cancel an appointment that is already canceled.";
    }
  }


  // if we're trying to reinstate or log an appointment, we need to be an admin or the tutor
  if ($action == "reinstate") {
    if (! user_access('view all appointments')) {
      if ($appointment->tutor_uid != $user->uid) {
	drupal_access_denied();
      }
    }
  }

  $output .= _appointment_tabs($id, "cancel");

  // we're either canceling, or reinstating

  //
  // we can cancel an individual appointment if:
  // (1) we're an admin, or
  // (2) we're a participant
  //
  global $user;

  if (user_access('view all appointments') || isa_participant($id, $user->uid))  {
    $title = $action_string . " appointment";
    drupal_set_title($title);
    $output .= "<h2>$title</h2>\n";

    //
    // we can cancel the series if this is an instance and:
    // (1) we're an admin, or
    // (2) we're a participant AND it's a single-person session, or
    // (3) we're the tutor and it's a group session
    //
    if (user_access('view all appointments')
	|| ( ($appointment->appointment_type == TUTOR_SESSION) && (isa_participant($id, $user->uid)) )
	|| ( ($appointment->appointment_type == GROUP_SESSION) && ($appointment->tutor_uid == $user->uid) ) ) {
      if ($instance != "") {
	$output .= "<p><em>Did you want to $action all future sessions in this appointment series?<br>If so, click here instead: <strong>" .
	  l("$action_string all future sessions in this series",
	    appointment_url("", $id, "", $action)) . "</strong></em></p>";
      } else {
	$event = $appointment;
	$noun = " series";
        $action_string = "Cancel all future appointments in the";
      }
    }

    $date = date_make_date($event->start_datetime_utc, "UTC");
    date_timezone_set($date, timezone_open(user_tz()));
    $date_string = "on " . date_format($date, DATE_FORMAT_ETO_LONG) . " at " . date_format($date, TIME_FORMAT_ETO_STANDARD);

    $output .= "<h2>$action_string appointment " . $noun. " <em>'" . $appointment->title . "'</em> $date_string</h2>";

    if (($action == "cancel") && ($instance == "")) {
      $now = date_make_date(time(), "UTC");
      $from_date = date_format($now, DATE_FORMAT_DATETIME);
      $cancel_count = count(sb_appointment_event_load(array(
                          'appointment_id' => $event->appointment_id,
                          'from_date' => $from_date)));
      $total_count = count(sb_appointment_event_load(array(
                         'appointment_id' => $event->appointment_id
                       )));
      $output .= "This will cancel $cancel_count of the $total_count events in the series.";
    }


    $tutor_string = _construct_participant_string($appointment, "tutor", 0);
    $student_string = _construct_participant_string($appointment, "student", 0);
    $output .= theme('meeting_participants', $tutor_string, $student_string);

    if ($action == "reinstate") {
      $output .= "<p>Are you sure you want to reinstate this appointment " . $noun . "?";
    }

    $output .= drupal_get_form('sb_' . $action . '_appointment_form', $event);

    return $output;
  } else {
    drupal_access_denied();
    exit;
  }
}


/**
 ** Cancellation functionality
 **/

function sb_cancel_appointment_form (&$form_state, $node) {
  $form = array();

  global $user;

  $app = sb_appointment_load($node->appointment_id);

  if ((isa_participant($node->appointment_id, $user->uid) && (! user_access('view all appointments')) )) {
    // if we're a participant and not an admin, just silently encode our uid.
    $form['canceling_user'] = array (
			  '#type' => 'hidden',
			  '#value' => $user->name,
			  );
  } else {
    // otherwise, we need to be able to select who is doing the canceling
    $tutor = eto_user_load($app->tutor_uid);
    $options[$tutor->name] = $tutor->name . " (tutor)";
    if ($app->appointment_type == TUTOR_SESSION) {
      $options[$app->student_0] = $app->student_0 . " (student)";
    } else {
      $participants = sb_appointment_participants_load($app->appointment_id, "names");
      foreach ($participants as $p) {
	$options[$p] = $p . " (student)";
      }
    }
    $form['canceling_user'] = array (
			  '#type' => 'radios',
			  '#title' => 'Select who is canceling',
			  '#options' => $options,
			  );
  }

  if ( ($node->appointment_event_id) || ($node->repeat_count == 1) ) {
    // we're an appointment event or a series of 1
    $noun = "";
    $form['appointment_event_id'] = array(
					  '#type' => 'hidden',
					  '#value' => $node->appointment_event_id
					  );
  } else {
    $noun = " series";
  }
  $form['appointment_id'] = array(
					'#type' => 'hidden',
					'#value' => $node->appointment_id
					);

  $form['body'] = array(
			'#type' => 'textfield',
			'#default_value' => $node->body,
			'#title' => 'Reason for canceling (optional)',
			'#size' => 120,
			'#maxlength' => 255,
			);

  if ($noun == 'series') {
    $form['submit'] = array (
      '#type' => 'submit',
      '#value' => 'Yes, cancel future sessions of this appointment' . $noun,
    );
  }
  else {
    $form['submit'] = array (
      '#type' => 'submit',
      '#value' => 'Yes, cancel this appointment' . $noun,
    );
  }

  $form['#submit'] = array('sb_cancel_appointment_submit_handler');

  return $form;
}



function sb_cancel_appointment_submit_handler($form, &$form_state) {
  //
  // Log the cancel
  //
  $u = eto_user_load(array('name' => $form_state['values']['canceling_user']));
  $form_state['values']['canceling_uid'] = $u->uid;

  sb_cancel_log_insert( (object) $form_state['values'] );

  //
  // for single-student sessions, either party canceling it cancels.
  // for group sessions, only the tutor canceling will cancel it.
  //

  $appointment = sb_appointment_load($form_state['values']['appointment_id']);

  if ( ($appointment->appointment_type == TUTOR_SESSION)
       || ( ($appointment->appointment_type == GROUP_SESSION)
	    && ($appointment->tutor_uid == $form_state['values']['canceling_uid'] ) ) ) {
    if ($form_state['values']['appointment_event_id']) {

      $isa_series = 0;

      $event = sb_appointment_event_load(array(
                 'appointment_event_id' => $form_state['values']['appointment_event_id']));
      sb_appointment_event_cancel($form_state['values']['appointment_event_id']);

      $date = date_make_date($event->start_datetime_utc, "UTC");
      date_timezone_set($date, timezone_open(user_tz()));
      $date_string = "on " . date_format($date, DATE_FORMAT_ETO_LONG) . " at " . date_format($date, TIME_FORMAT_ETO_STANDARD);
    } else {
      $isa_series = 1;

      // for our "event", use the first cancelled one in a series
      $now = date_make_date(time(), "UTC");
      $from_date = date_format($now, DATE_FORMAT_DATETIME);
      $events = sb_appointment_event_load(array(
                  'appointment_id' => $form_state['values']['appointment_id'],
                  'from_date' => $from_date));
      $event = $events[0];

      // if they only had one appointment remaining, let's pretend they
      // only cancelled one appointment instead of the series
      if (count($events) == 1) {
        $isa_series = 0;
      }

      sb_appointment_cancel($form_state['values']['appointment_id']);

      $date = date_make_date($event->start_datetime_utc, "UTC");
      date_timezone_set($date, timezone_open(user_tz()));
      $date_string = "on " . date_format($date, "l") . "s at " . date_format($date, TIME_FORMAT_ETO_STANDARD);
    }

    // this is an event cancellation by either party for a tutor session or the tutor for
    // a group session, so send out emails

    $tutor = eto_user_load($appointment->tutor_uid);
    $u = eto_user_load($appointment->student_uid_0);

    if ($tutor->mail) {
      $date = date_make_date($event->start_datetime_utc, "UTC");
      date_timezone_set($date, timezone_open(user_tz($tutor)));
      $app_date = date_format($date, DATE_FORMAT_ETO_LONG_NO_YEAR);
      $short_app_date = date_format($date, DATE_FORMAT_ETO_ABBREVIATED);
      $start_time = date_format($date, 'g:ia');
      $end_date = date_make_date($event->end_datetime_utc, "UTC");
      date_timezone_set($end_date, timezone_open(user_tz($tutor)));
      $end_time = date_format($end_date, 'g:ia (T)');

      // always email even if they asked for a text
      $messages[] = array('email' => $tutor->mail,
			  'date' => $app_date,
			  'start_time' => $start_time,
			  'end_time' => $end_time);
      watchdog('eto_notifications',
	       'Emailing cancellation to @tutor_name (@tutor_id)',
	       array('@tutor_name' => $tutor->name,
		     '@tutor_id' => $tutor->uid));


      // then SMS if we asked for it and can
      $method = notifications_user_setting('send_method', $tutor);
      if (($method == 'sms') && ($tutor->sms_user[0]['number'] > 0) && ($tutor->sms_user[0]['status'] == 2)) {
	watchdog('eto_notifications',
		 'Texting cancellation to @tutor_name (@tutor_id)',
		 array('@tutor_name' => $tutor->name,
		       '@tutor_id' => $tutor->uid));

        if ($isa_series) {
          $message = "ETO standing appointment cancelled $short_app_date $start_time - $end_time - " . $u->eto_profile->first_name . " " . $u->eto_profile->last_name;
        }
        else {
          $message = "ETO appointment cancelled $short_app_date $start_time - $end_time - " . $u->eto_profile->first_name . " " . $u->eto_profile->last_name;
        }
	$texts[] = array ('uid' => $tutor->uid,
			  'message' => $message,
			  );
      }
    }

    if ($appointment->appointment_type == TUTOR_SESSION) {
      $u = eto_user_load($appointment->student_uid_0);
      $date = date_make_date($event->start_datetime_utc, "UTC");
      date_timezone_set($date, timezone_open(user_tz($u)));
      $app_date = date_format($date, DATE_FORMAT_ETO_LONG_NO_YEAR);
      $start_time = date_format($date, 'g:ia');
      $end_date = date_make_date($event->end_datetime_utc, "UTC");
      date_timezone_set($end_date, timezone_open(user_tz($u)));
      $end_time = date_format($end_date, 'g:ia (T)');

      if ($u->mail) {
	$messages[] = array('email' => $u->mail,
			    'date' => $app_date,
			    'start_time' => $start_time,
			    'end_time' => $end_time);
      }
      if ( ($u->eto_profile->email_parent) && ($u->mail != $u->eto_profile->email_parent)) {
	$messages[] = array('email' => $u->eto_profile->email_parent,
			    'date' => $app_date,
			    'start_time' => $start_time,
			    'end_time' => $end_time);
      }


    } else if ($appointment->appointment_type == GROUP_SESSION) {

      for ($i = 1; $i <= 100; $i++) {
	if ($appointment->{"student_uid_" . $i} > 0) {
	  $u = eto_user_load($appointment->{"student_uid_" . $i});
	  $date = date_make_date($event->start_datetime_utc, "UTC");
	  date_timezone_set($date, timezone_open(user_tz($u)));
	  $app_date = date_format($date, DATE_FORMAT_ETO_LONG_NO_YEAR);
	  $start_time = date_format($date, 'g:ia');
	  $end_date = date_make_date($event->end_datetime_utc, "UTC");
	  date_timezone_set($end_date, timezone_open(user_tz($u)));
	  $end_time = date_format($end_date, 'g:ia (T)');

	  if ($u->mail) {
	    $messages[] = array('email' => $u->mail,
				'date' => $app_date,
				'start_time' => $start_time,
				'end_time' => $end_time);
	  }
	  if ( ($u->eto_profile->email_parent) && ($u->mail != $u->eto_profile->email_parent)) {
	    $messages[] = array('email' => $u->eto_profile->email_parent,
				'date' => $app_date,
				'start_time' => $start_time,
				'end_time' => $end_time);
	  }
	}
      }
    }
    if (count($messages) > 0) {
      $tutor_name = ($tutor->eto_profile->first_name ? $tutor->eto_profile->first_name . " " . $tutor->eto_profile->last_name : $tutor->name);
      foreach ($messages as $m) {
	$params = array('tutor_full_name' => $tutor_name,
			'date' => $m['date'],
			'start_time' => $m['start_time'],
			'end_time' => $m['end_time'],
			'notes' => check_plain($form_state['values']['body']));
	if (preg_match(EMAIL_REGEX, $m['email']) === 1) {
          if ($isa_series) {
            drupal_mail("eto_admin",
              "session_cancellation_future",
              $m['email'],
              language_default(),
              $params,
              variable_get('site_mail', 'admin@elitetutorsonline.com'),
              TRUE);
          }
          else {
            drupal_mail("eto_admin",
              "session_cancellation",
              $m['email'],
              language_default(),
              $params,
              variable_get('site_mail', 'admin@elitetutorsonline.com'),
              TRUE);
          }
	}
      }
    }

    if (count($texts) > 0) {
      foreach ($texts as $t) {
	sms_user_send($t['uid'], $t['message']);
      }
    }

  } else {

    if ($form_state['values']['appointment_event_id']) {
      $event = sb_appointment_event_load(array(
                 'appointment_event_id' => $form_state['values']['appointment_event_id']));
      sb_appointment_event_cancel($form_state['values']['appointment_event_id']);

      $date = date_make_date($event->start_datetime_utc, "UTC");
      date_timezone_set($date, timezone_open(user_tz()));
      $date_string = "on " . date_format($date, DATE_FORMAT_ETO_LONG) . " at " . date_format($date, TIME_FORMAT_ETO_STANDARD);
    } else {
      $event = $appointment;
      sb_appointment_cancel($form_state['values']['appointment_id']);
      $date = date_make_date($event->start_datetime_utc, "UTC");
      date_timezone_set($date, timezone_open(user_tz()));
      $date_string = "on " . date_format($date, "l") . "s at " . date_format($date, TIME_FORMAT_ETO_STANDARD);
    }
  }

  drupal_set_message("Appointment $date_string is cancelled.", 'success');
  $form_state['redirect'] = appointment_url("", $form_state['values']['appointment_id'], $form_state['values']['appointment_event_id']);
}

function sb_appointment_cancel ($appointment_id) {
  // we used to mark the schedule as cancelled. now just cancel the individual 'future events'
  /*
  db_query('UPDATE {sb_appointment} SET status_flag = 0 WHERE appointment_id = %d',
	   $appointment_id);
  */
  $now = date_make_date(time(), "UTC");
  $from_date = date_format($now, DATE_FORMAT_DATETIME);

  $events = sb_appointment_event_load(array(
              'appointment_id' => $appointment_id,
              'from_date' => $from_date));

  foreach ($events as $e) {
    sb_appointment_event_cancel($e->appointment_event_id);
  }
}

function sb_appointment_event_cancel ($event_id) {
   db_query('UPDATE {sb_appointment_event} SET status_flag = 0 WHERE appointment_event_id = %d',
	    $event_id);
}


function sb_cancel_log_insert ($node) {
  $now = date_make_date(time(), "UTC");

  db_query("INSERT INTO {sb_cancel_log}
              (appointment_id, appointment_event_id, uid, body, datetime_utc)
            VALUES
              (%d, %d, %d, '%s', '%s')",
	   $node->appointment_id, $node->appointment_event_id,
	   $node->canceling_uid, $node->body,
	   date_format($now, DATE_FORMAT_DATETIME));
}

function sb_cancel_log_delete($args) {
  if ($args['appointment_event_id']) {
    $field = 'appointment_event_id';
    $value = $args['appointment_event_id'];
  } else {
    $field = 'appointment_id';
    $value = $args['appointment_id'];
  }
  db_query("DELETE FROM {sb_cancel_log} WHERE %s = %d",
	   $field, $value);
}

function sb_cancel_log_delete_uid($args) {
  if ($args['appointment_event_id']) {
    $field = 'appointment_event_id';
    $value = $args['appointment_event_id'];
  } else {
    $field = 'appointment_id';
    $value = $args['appointment_id'];
  }
  $uid = $args['uid'];
  if ($uid) {
    db_query("DELETE FROM {sb_cancel_log} WHERE %s = %d AND uid = %d",
	     $field, $value, $uid);
  }
}

/**
 ** Reinstating functionality
 **/

function sb_reinstate_appointment_form (&$form_state, $node) {
  $form = array();

  global $user;

  if ( ($node->appointment_event_id) || ($node->repeat_count == 1) ) {
    // we're an appointment event or a series of 1
    $form['appointment_event_id'] = array(
					  '#type' => 'hidden',
					  '#value' => $node->appointment_event_id
					  );
  } else {
    $noun = " series";
  }
  $form['appointment_id'] = array(
					'#type' => 'hidden',
					'#value' => $node->appointment_id
					);


  $form['submit'] = array (
			   '#type' => 'submit',
			   '#value' => 'Yes, reinstate this appointment' . $noun,
			   );

  $form['#submit'] = array('sb_reinstate_appointment_submit_handler');

  return $form;
}

function sb_reinstate_appointment_submit_handler($form, &$form_state) {
  $appointment = sb_appointment_load($form_state['values']['appointment_id']);

  // delete our log events if it's a tutor session; otherwise just delete a tutor's log
  if ($appointment->appointment_type == TUTOR_SESSION) {
    if ($form_state['values']['appointment_event_id']) {
      sb_cancel_log_delete(array('appointment_event_id' => $form_state['values']['appointment_event_id']));
    } else {
      sb_cancel_log_delete(array('appointment_id' => $form_state['values']['appointment_id']));
    }
  } else {
    if ($form_state['values']['appointment_event_id']) {
      sb_cancel_log_delete_uid(array('appointment_event_id' => $form_state['values']['appointment_event_id'],
				     'uid' => $appointment->tutor_uid));
    } else {
      sb_cancel_log_delete_uid(array('appointment_id' => $form_state['values']['appointment_id'],
				     'uid' => $appointment->tutor_uid));
    }
  }


  if ($form_state['values']['appointment_event_id']) {
    $event = sb_appointment_event_load(array(
               'appointment_event_id' => $form_state['values']['appointment_event_id']));
    db_query('UPDATE {sb_appointment_event} SET status_flag = 1 WHERE appointment_event_id = %d',
	     $form_state['values']['appointment_event_id']);

    $date = date_make_date($event->start_datetime_utc, "UTC");
    date_timezone_set($date, timezone_open(user_tz()));
    $date_string = "on " . date_format($date, DATE_FORMAT_ETO_LONG) . " at " . date_format($date, TIME_FORMAT_ETO_STANDARD);

  } else {
    $event = $appointment;
    db_query('UPDATE {sb_appointment} SET status_flag = 1 WHERE appointment_id = %d',
	     $form_state['values']['appointment_id']);

    $date = date_make_date($event->start_datetime_utc, "UTC");
    date_timezone_set($date, timezone_open(user_tz()));
    $date_string = "on " . date_format($date, "l") . "s at " . date_format($date, TIME_FORMAT_ETO_STANDARD);
  }

  drupal_set_message("Appointment $date_string is reinstated.", 'success');
  $form_state['redirect'] = appointment_url("", $form_state['values']['appointment_id'], $form_state['values']['appointment_event_id']);
}

/*
 * This function finds out how long we can schedule an appointment for starting at this time.
 * The end-time cannot exceed the end of our current block of tutoring time, and it
 * cannot overlap with any other appointments the tutor has scheduled
 *
 * 'date' is a DATE_FORMAT_DATETIME in UTC
 */

function _available_durations($start = '', $uid = '', $type = "", $verify_block = TRUE) {

  if (!$start) {
    // return everything
    return array(15 => 15, 30 => 30, 45 => 45, 60 => 60);
  }
  // get the next appointment scheduled after this one
  $appointment = sb_appointment_next_appointment_load($uid, $start, TRUE);
  // we also need our block end if relevant
  if ($verify_block) {
    $block = sb_schedule_event_time_load($uid, $start, TRUE);
    $block_date = date_make_date($block->end_datetime_utc, "UTC");
  }
  $start_date = date_make_date($start, "UTC");
  $app_date = date_make_date($appointment->start_datetime_utc, "UTC");

  // our comparison date is whichever happens first: next appointment or end of our block of time
  if (! $app_date) {
      if ($verify_block) {
        $limit_date = $block_date;
      }
      else {
        $limit_date = -1;
      }
  } else if ( ( $app_date > $block_date) && $verify_block) {
    $limit_date = $block_date;
  } else {
    $limit_date = $app_date;
  }

  // get the number of seconds between them
  $seconds = date_format($limit_date, "U") - date_format($start_date, "U");
  $minutes = $seconds / 60;

  // round up to the nearest multiple of 15
  $minutes = 15 * ceil($minutes/15);

  // we can go up to $minutes in increments of 15, to a max of 4 (for 60 minutes) unless it's a personal session
  if ($type == PERSONAL_SESSION) {
    $max = 16;
  } else {
    $max = 4;
  }
  $intervals = ( ($minutes / 15) > $max ? $max : ($minutes / 15) );

  $available = array_slice ( array(15 => 15, 30 => 30, 45 => 45, 60 => 60, 75 => 75, 90 => 90, 105 => 105, 120 => 120, 135 => 135, 150 => 150, 165 => 165, 180 => 180),
			     0,
			     $intervals,
			     TRUE);

  return $available;
}

function _utc($date) {
  return date_timezone_set($date, timezone_open('UTC'));
}



function user_upcoming_appointment_display_block($account) {

  $now = date_make_date(time(), "UTC");
  $date = date_format($now, DATE_FORMAT_DATETIME);

  $appointments = sb_appointment_load_by_time($account->uid, $date, TRUE, 5);

  $output .= "<h2>Upcoming Appointments</h2>";
  if (!$appointments) {
    $output .= "<em>You have no upcoming appointments.</em>";
  } else {
    $output .= "<ul>";
    foreach ($appointments as $a) {
      $start = date_make_date($a->start_datetime_utc, "UTC");
      date_timezone_set($start, timezone_open(user_tz($account)));
      $end = date_make_date($a->end_datetime_utc, "UTC");
      date_timezone_set($end, timezone_open(user_tz($account)));

      $start_string = date_format($start, DATE_FORMAT_ETO_LONG_NO_YEAR) . " at " . date_format($start, TIME_FORMAT_ETO_STANDARD);
      $end_string = date_format($end, TIME_FORMAT_ETO_STANDARD);
      $tutor = eto_user_load($a->tutor_uid);

      $output .= "<li>" . $start_string . ": tutor session with " . theme('eto_user', $tutor, TRUE) . " until " . $end_string . "</li>";

    }
    $output .= "</ul>";
  }
  return $output;
}