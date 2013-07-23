<?php
/****************************************************************************
 | Chamilo for Übercart
 |  An integration module between Drupal/Übercart and Chamilo
 | --------------------------------------------------------------------------
 | Administration panel definition
 | --------------------------------------------------------------------------
 | (c) Copyright 2013, BeezNest Belgium SPRL (info@beeznest.com)
 | (c) Copyright 2013, frenoy.net (info@frenoy.net)
 | --------------------------------------------------------------------------
 | Chamilo for Übercart is free software; you can redistribute it and/or
 | modify it under the terms of the GNU General Public License as published
 | by the Free Software Foundation; either version 2 of the License, or
 | (at your option) any later version.
 |
 | This program is distributed in the hope that it will be useful,
 | but WITHOUT ANY WARRANTY; without even the implied warranty of
 | MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 | GNU General Public License for more details.
 |
 | You should have received a copy of the GNU General Public License
 | along with Chamilo for Übercart (see LICENSE.txt).
 | If not, see <http://www.gnu.org/licenses/>.
 ****************************************************************************/

/**
 * PHP library for Chamilo
 * DevNote: should be included in Drupal "library"!
 */
include_once('lib/chamilo_ws.php');

/**
 * Some constants
 */
define('UC_CHAMILO_DEFAULT_SERVER', 'http://localhost');

/**
 * Settings of the Chamilo for Übercart integration
 */
function uc_chamilo_admin($form, &$form_state) {
  $form['info'] = array(
    '#type'        => 'fieldset',
    '#title'       => t('Chamilo server'),
    '#description' => '<div><img src="'.base_path().drupal_get_path('module', 'uc_chamilo').'/images/chamilo_logo.jpg" style="float: right;">' . t('Please provide the required correction information to connect to your Chamilo server.') . '</div>',
    '#collapsible' => true,
    '#collapsed'   => false
  );
  $form['info']['uc_chamilo_server'] = array(
    '#type'          => 'textfield',
    '#title'         => t('Chamilo server URL'),
    '#description'   => t('URL to the root path of your Chamilo server, including the protocol.  Example: http://chamilo.mycompany.com'),
    '#default_value' => variable_get('uc_chamilo_server', UC_CHAMILO_DEFAULT_SERVER)
  );
  $form['info']['update'] = array(
    '#type'     => 'submit',
    '#validate' => array('uc_chamilo_validate_settings'),
    '#submit'   => array('uc_chamilo_update_settings'),
    '#value'    => t('Save settings')
  );

  $form['products'] = array(
    '#type'        => 'fieldset',
    '#title'       => t('Course synchronization'),
    '#description' => t('The "Product" interface transfers the Master Data about your courses defined in your Chamilo application to the Übercart e-store.  All active courses will be considered as a product for Übercart.  You may later decide if you want to active them or not.'),
    '#collapsible' => true,
    '#collapsed'   => false
  );

  $last_update = variable_get('uc_chamilo_last_update', false);
  $form['products']['info'] = array(
    '#type'    => 'item',
    '#title'   => t('Last update'),
    '#markup'  => $last_update ? format_date($last_update) : t('No synchronization performed yet')
  );

  $form['products']['synchronize'] = array(
    '#type'   => 'submit',
    '#submit' => array('uc_chamilo_course_sync_submit'),
    '#value'  => t('Synchronize courses')
  );

  return $form;
}

/**
 * Process uc_chamilo_admin_settings form submissions.
 *
 * Validate Chamilo settings
 */
function uc_chamilo_validate_settings($form, &$form_state) {
  $server_url_ok = true;
  if (trim($form_state['values']['uc_chamilo_server']) == '') {
    form_set_error('uc_chamilo_server', t('Please specify the Chamilo server URL.'));
    $server_url_ok = false;
  }
  if (!valid_url($form_state['values']['uc_chamilo_server'], TRUE)) {
    form_set_error('uc_chamilo_server', t('The specified chamilo server URL is not invalid.'));
    $server_url_ok = false;
  }
  if ($server_url_ok) {
    // Try a backend connection
    try {
      $client = new chamiloWs($form_state['values']['uc_chamilo_server']);
    }
    catch (Exception $e) {
      form_set_error('uc_chamilo_server', t('Unable to connect to Chamilo server, please check your configuration.  If the problem persists, check Drupal log for further information.'));
    }
  }
}

/**
 * Process uc_chamilo_admin_settings form submissions.
 *
 * Update Chamilo settings
 */
function uc_chamilo_update_settings($form, &$form_state) {
  $ok = true;

  variable_set('uc_chamilo_server', rtrim($form_state['values']['uc_chamilo_server'], '/'));

  if ($ok) {
    drupal_set_message(t('Chamilo for Übercart settings updated successfully.'));
} else {
    drupal_set_message(t('An error occured when updating settings of Chamilo for Übercart, check the logs for more information'));
  }
}

/**
 * Process uc_chamilo_admin_settings form submissions.
 *
 * Execute the 'Synchronize courses' action
 */
function uc_chamilo_course_sync_submit($form, &$form_state) {
  $ok = true;

  // Retrieve list of courses from Chamilo
  $client = new chamiloWs(variable_get('uc_chamilo_server', UC_CHAMILO_DEFAULT_SERVER));
  $courses = $client->courseList();

  // For each course, extract additional course fields according to fields defined in Chamilo UC product
  // DevNote: TODO

  // Synchronize course defined in Chamilo with UC product
  foreach ($courses as $course) {
    if (_uc_chamilo_synchronize_course($course)) {
      drupal_set_message(t('Course [%course_name] has been correctly synchronized.', array('%course_name' => $course->code)));
    }
  }

  if ($ok) {
    drupal_set_message(t('List of courses correctly synchronized from Chamilo'));
    variable_set('uc_chamilo_last_update', time());
} else {
    drupal_set_message(t('An error occured when updating course list from Chamilo, check the logs for more information'));
  }
}

/**
 * Helper function that synchronizes one Chamilo course with corresponding Übercart product
 * @private
 */
function _uc_chamilo_synchronize_course($course) {
  // Search for an existing product with the same SKU (model)
  $result = db_select('uc_products', 'p')
    ->fields('p', array('nid'))
    ->condition('p.model', $course->code)
    ->execute()
    ->range(0, 1)
    ->fetchAssoc();

  // Check if the course is already created in Drupal
  if ($result) {
    // Product already exists, just load it
    $node = node_load($result['nid']);
  } else {
    // Product does not exist yet, prepare it
    $node = new stdClass();
    $node->type = 'chamilo_course';
    node_object_prepare($node);
  }
  // Update Drupal node according to Übercart configuration
  $node->title = $course->title;
  $node->model = $course->code;
  $node->language = LANGUAGE_NONE;
  $node->path = array('alias' => 'content/course/' . strtolower($course->code));
  
  // Apply changes
  node_save($node);

  return true;
}

?>