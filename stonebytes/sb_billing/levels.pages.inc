<?php

function sb_billing_levels () {

  $output = "";

  $output .= "<h2>Billing Levels</h2>";

  $output .= l("+ Add a new level", billing_level_url("", "add"));

  $th = tutor_level_row_header();

  $result = db_query("SELECT level_id FROM {sb_billing_level} AS sbl" . tablesort_sql($th));

  $rows = array();
  while ($level = db_fetch_object($result)) {
    $level = sb_billing_level_load($level->level_id);
    $rows[] = theme('tutor_level_row', $level);
  }

  $output .= theme_table($th, $rows);

  $output .= l("+ Add a new level", billing_level_url("", "add"));

  return $output;
} 

function sb_billing_level_view($id = "") {
  $node = sb_billing_level_load($id);
  if (!$node) {
    drupal_not_found();
    exit;
  }

  $output = "";

  drupal_set_title(check_plain($node->title));

  $output .= "<h2>" . check_plain($node->title) . "</h2>";

  $output .= theme_table(tutor_level_row_header(0),
  			 array( theme('tutor_level_row', $node)) );

  if ($node->tutors) {
    $output .= "<h2>Tutors at this level:</h2>\n";
    $output .= "<p>" . l("+Add a tutor", billing_level_url("", $level->level_id, "add-tutor")) . "</p>";
   
    $th = array (
		 array('data' => 'Tutor', 'field' => 'u.name', 'sort' => 'asc'),
		 array('data' => 'Split', 'field' => 'sbtl.split' ),
		 '',
		 );

    $result = db_query("SELECT sbtl.*, u.name
                        FROM {sb_billing_tutor_level} AS sbtl
                           INNER JOIN {users} AS u ON sbtl.uid=u.uid
                        WHERE level_id = %d" . tablesort_sql($th),
		       $node->level_id);
    $rows = array();
    while ($tutor = db_fetch_object($result)) {
      $rows[] = array (
		       l($tutor->name, "user/" . $tutor->uid),
		       $tutor->split,
		       l('[edit]', billing_level_url('', $node->level_id, 'edit', $tutor->uid))
		       );
    }

    // we don't want an infinitely-stretched table here
    $output .= "<div class='skinny-table'>\n";
    $output .= theme('table', $th, $rows);
    $output .= "</div>\n";
    if ($rows) {
      $output .= "<p>" . l("+Add a tutor", billing_level_url("", $level->level_id, "add-tutor")) . "</p>";
    }
  }

  return $output;
}

function sb_billing_level_edit($id = "") {
  $output .= "<h2>Add a new Billing Level</h2>";

  if ($id) {
    $node = sb_billing_level_load($id);
  } else {
    $node = array();
  }

  $output .= drupal_get_form('sb_billing_level_form', $node);

  return $output;
}

function sb_billing_level_form (&$form_state, $node) {
  $form = array();

  $form['level_id'] = array (
				   '#type' => 'hidden',
				   '#value' => ($node->level_id ? $node->level_id : ""),
				   );
  
  $form['title'] = array (
			  '#type' => 'textfield',
			  '#title' => 'Title',
			  '#description' => 'This name is visible to tutors.',
			  '#default_value' => $node->title,
			  );

  $form['status_flag'] = array (
				'#type' => 'radios',
				'#title' => 'Active Level',
				'#description' => 'If this is set to yes, new tutors can be added to this level.  If set to no, old tutors will not be changed but new tutors cannot be added.',
				'#options' => array('0' => 'No', '1' => 'Yes'),
				'#default_value' => (isset($node->status_flag) ? $node->status_flag : '1'),
				);

  $form['school'] = array (
			   '#type'  => 'fieldset',
			   '#title' => 'Schoolyear Rates',
			   );

  $form['school']['school_day_hours_rate'] = array (
				      '#type' => 'textfield',
				      '#size' => '4',
				      '#title' => 'School Hours Rate',
				      '#description' =>  "9am - 3pm (tutor's timezone)",
				      '#default_value' => theme('sb_rate', ($node->school_day_hours_rate ? $node->school_day_hours_rate : '000')),
				      '#field_prefix' => '$',
				      '#field_suffix' => '/hour',
				      '#element_validate' => array ('_rate_validate'),
				      );
  $form['school']['school_day_hours_label'] = array (
				      '#type' => 'textfield',
				      '#size' => '32',
				      '#title' => 'School Hours Label',
				      '#description' =>  "e.g. 'Homeschool'",
				      '#default_value' => ($node->school_day_hours_label ? $node->school_day_hours_label : 'Homeschool'),
				      );

  $form['school']['school_after_hours_rate'] = array (
				      '#type' => 'textfield',
				      '#size' => '4',
				      '#title' => 'After Hours Rate',
				      '#description' =>  "3pm - 9am (tutor's timezone)",
				      '#default_value' => theme('sb_rate', ($node->school_after_hours_rate ? $node->school_after_hours_rate : '000')),
				      '#field_prefix' => '$',
				      '#field_suffix' => '/hour',
				      '#element_validate' => array ('_rate_validate'),
				      );
  $form['school']['school_after_hours_label'] = array (
				      '#type' => 'textfield',
				      '#size' => '32',
				      '#title' => 'After Hours Label',
				      '#description' =>  "e.g. 'Standard'",
				      '#default_value' => ($node->school_after_hours_label ? $node->school_after_hours_label : 'Standard'),
				      );


  $form['summer'] = array (
			   '#type'  => 'fieldset',
			   '#title' => 'Summer Rates',
			   );

  $form['summer']['summer_day_hours_rate'] = array (
				      '#type' => 'textfield',
				      '#size' => '4',
				      '#title' => 'School Hours Rate',
				      '#description' =>  "9am - 3pm (tutor's timezone)",
				      '#default_value' => theme('sb_rate', ($node->summer_day_hours_rate ? $node->summer_day_hours_rate : '000')),
				      '#field_prefix' => '$',
				      '#field_suffix' => '/hour',
				      '#element_validate' => array ('_rate_validate'),
				      );
  $form['summer']['summer_day_hours_label'] = array (
				      '#type' => 'textfield',
				      '#size' => '32',
				      '#title' => 'School Hours Label',
				      '#description' =>  "e.g. 'Standard'",
				      '#default_value' => ($node->summer_day_hours_label ? $node->summer_day_hours_label : 'Standard'),
				      );

  $form['summer']['summer_after_hours_rate'] = array (
				      '#type' => 'textfield',
				      '#size' => '4',
				      '#title' => 'After Hours Rate',
				      '#description' =>  "3pm - 9am (tutor's timezone)",
				      '#default_value' => theme('sb_rate', ($node->summer_after_hours_rate ? $node->summer_after_hours_rate : '000')),
				      '#field_prefix' => '$',
				      '#field_suffix' => '/hour',
				      '#element_validate' => array ('_rate_validate'),
				      );
  $form['summer']['summer_after_hours_label'] = array (
				      '#type' => 'textfield',
				      '#size' => '32',
				      '#title' => 'School Hours Label',
				      '#description' =>  "e.g. 'Standard'",
				      '#default_value' => ($node->summer_after_hours_label ? $node->summer_after_hours_label : 'Standard'),
				      );


  $form['submit'] = array (
			   '#type' => 'submit',
			   '#value' => ($node->level_id ? "Save changes" : "Add New Billing Level"),
			   );

  $form['#submit'][] = 'sb_billing_level_form_submit_handler';

  return $form;
}

function _rate_validate ($element, &$form_state) {
  if ($element['#value']) {
    // needs to be of the form: 00.00 or 00, with an optional initial dollar sign
    $value = sb_rate_validate($element['#value']);
    if (!preg_match('/^\d+$/', $value)) {
      form_error($element, "You must enter a dollar value for the hourly rate ($value)", "error");
    }
  }
}

function sb_billing_level_form_submit_handler ($form, &$form_state) {

  // need to rewrite our billing rates if appropriate
  foreach (array('school_day_hours_rate', 'school_after_hours_rate', 
		 'summer_day_hours_rate', 'summer_after_hours_rate') as $element) {
    $form_state['values'][$element] = sb_rate_validate($form_state['values'][$element]);
  }

  if ($form_state['values']['level_id'] > 0) {
    sb_billing_level_update( (object) $form_state['values']);
    $verb = "updated";
  } else {
    $form_state['values']['level_id'] = sb_billing_level_insert( (object) $form_state['values']);
    $verb = "created";
  }
  drupal_set_message("Your billing level has been $verb successfully.", "success");
  $form_state['redirect'] = billing_level_url();
}

function sb_billing_level_insert ($node) {
  db_query("INSERT INTO {sb_billing_level}
              (title, status_flag, school_day_hours_rate, school_day_hours_label,
               school_after_hours_rate, school_after_hours_label,
               summer_day_hours_rate, summer_day_hours_label,
               summer_after_hours_rate, summer_after_hours_label)
            VALUES
              ('%s', %d, %d, '%s', %d, '%s', %d, '%s', %d, '%s')",
	   $node->title, $node->status_flag, 
	   $node->school_day_hours_rate, $node->school_day_hours_label,
	   $node->school_after_hours_rate, $node->school_after_hours_label,
	   $node->summer_day_hours_rate, $node->summer_day_hours_label,
	   $node->summer_after_hours_rate, $node->summer_after_hours_label);
  $level_id = db_last_insert_id("sb_billing_level", "level_id");

  return $level_id;
}

function sb_billing_level_update ($node) {
  db_query("UPDATE {sb_billing_level} SET
               title = '%s',
               status_flag = %d,
               school_day_hours_rate = %d,
               school_day_hours_label = '%s',
               school_after_hours_rate = %d,
               school_after_hours_label = '%s',
               summer_day_hours_rate = %d,
               summer_day_hours_label = '%s',
               summer_after_hours_rate = %d,
               summer_after_hours_label = '%s'
            WHERE level_id = %d",
	   $node->title, $node->status_flag, 
	   $node->school_day_hours_rate, $node->school_day_hours_label,
	   $node->school_after_hours_rate, $node->school_after_hours_label, 
	   $node->summer_day_hours_rate, $node->summer_day_hours_label,
	   $node->summer_after_hours_rate, $node->summer_after_hours_label, 
	   $node->level_id);

}

/**
 * All three of these lead to the same form just with different presets filled in.
 */

function sb_billing_level_edit_tutor ($name) {
  // editing an existing one, so fix the uid
  $user = eto_user_load(array('name' => $name));
  $relationship = sb_billing_tutor_level_load($user->uid);
  $relationship->tutor_fixed = 1;
  if (!$relationship->uid) {
    // new relationship, so add in us
    $relationship->uid = $user->uid;
  }
  $output .= drupal_get_form('sb_billing_tutor_level_form', $relationship);
    
  return $output;
}

function sb_billing_level_add_tutor ($level) {

  $relationship = (object) array ( 'level_id' => $level );
  $relationship->level_fixed = 1;

  $output .= drupal_get_form('sb_billing_tutor_level_form', $relationship);
    
  return $output;
}

function sb_billing_level_add_level ($uid) {

  $relationship = (object) array ( 'uid' => $uid );
  $relationship->tutor_fixed = 1;

  $output .= drupal_get_form('sb_billing_tutor_level_form', $relationship);
    
  return $output;
}

/*
 * This expects something to be defined as fixed: level or UID, and we edit the other
 * if both are fixed, edit just the split
 */

function sb_billing_tutor_level_form (&$form_state, $node) {
  $form = array();
  $form['id'] = array ('#type' => 'hidden',
		       '#value' => $node->id);

  if ($node->level_fixed == 1) {
    $form['level_fixed'] = array ('#type' => 'hidden', '#value' => 1);

    if ($node->level_id) {
      $level = sb_billing_level_load($node->level_id);
    }

    $form['level_title'] = array ('#type' => 'textfield',
				  '#disabled' => TRUE,
				  '#title' => 'Level',
				  '#value' => $level->title,
				  );
    $form['level_id'] = array ('#type' => 'hidden',
			       '#value' => $node->level_id);
  } else {
    $form['level_id'] = array ('#type' => 'select',
			       '#title' => 'Tutor',
			       '#options' => sb_billing_level_array('--Select a Level--'),
			       '#default_value' => ($node->level_id ? $node->level_id : 0)
			       );
  }
  
  if ($node->tutor_fixed == 1) {
    $form['tutor_fixed'] = array ('#type' => 'hidden', '#value' => 1);

    if ($node->uid) {
      $tutor = eto_user_load($node->uid);
    }
    $form['user_name'] = array ('#type' => 'textfield',
				'#disabled' => TRUE,
				'#title' => 'Tutor',
				'#value' => $tutor->name
				);
    $form['uid'] = array ('#type' => 'hidden',
			  '#value' => $node->uid);
    
  } else {
    $form['uid'] = array ('#type' => 'select',
			  '#title' => 'Tutor',
			  '#options' => sb_tutor_array('--Select a Tutor--'),
			  '#default_value' => ($node->uid ? $node->uid : 0)
			  );
  }

  $form['split'] = array ('#type' => 'textfield',
			  '#size' => 3,
			  '#title' => 'Tutor net percent',
			  '#default_value' => ($node->split ? $node->split : 65),
			  '#element_validate' => array ('sb_split_validate'),
			  );

  $form['submit'] = array (
			   '#type' => 'submit',
			   '#value' => ($node->id ? "Save changes" : "Add Billing Level to this Tutor"),
			   );

  //  $form['#validate'][] = 'sb_billing_tutor_level_form_validate';

  $form['#submit'][] = 'sb_billing_tutor_level_form_submit_handler';

  return $form;
}

function sb_split_validate($element) {
  $v = $element_value;
  if (! ($v >= 0 & $v <= 100) ) {
    form_error($element, "You must pick a split beteen 0% and 100%");
  } else if ($v = "") {
    form_error($element, "You must pick a split beteen 0% and 100%");
  }
}


function sb_billing_tutor_level_form_validate($form, &$form_state) {
  // if we're adding to a level (i.e. level_fixed = 1) then make sure this isn't going
  // to overwrite an existing uid record
  if ($form_state['values']['level_fixed'] == 1) {
    $obj = sb_billing_tutor_level_load($form_state['values']['uid']);
    if ($obj) {
      form_set_error('uid', "You already have an entry for this tutor.  " . l("Click here if you want to edit it.", 
									      billing_level_url('', '', 'edit',
												$form_state['values']['uid'])));
    }
  }
}

function sb_billing_tutor_level_form_submit_handler ($form, &$form_state) {
  // FIXME -- need to search to see if we should update or not
  if (! is_numeric($form_state['values']['uid'])) {
    $u = eto_user_load($form_state['values']['uid']);
    $form_state['values']['uid'] = $u->uid;
  }
    
  if (sb_billing_tutor_level_load($form_state['values']['uid'])) {
    sb_billing_tutor_level_update( (object) $form_state['values']);
  } else {
    sb_billing_tutor_level_insert( (object) $form_state['values']);
  }
  $form_state['redirect'] = billing_level_url();
}


function sb_billing_tutor_level_insert ($node) {
  db_query("INSERT INTO {sb_billing_tutor_level}
              (uid, level_id, split)
            VALUES
              (%d, %d, %d)",
	   $node->uid, $node->level_id, $node->split);
}

function sb_billing_tutor_level_update ($node) {
  db_query("UPDATE {sb_billing_tutor_level} SET
               level_id = %d,
               split = %d
            WHERE uid = %d",
	   $node->level_id, $node->split, $node->uid);
}

