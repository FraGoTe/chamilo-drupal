<?php
// $Id$

/**
 * @file
 * Module settings UI.
 */

function chamilo_admin_settings() {
  /**
   * Space construct aditional variables, do validation etc.
   */
  return drupal_get_form('chamilo_admin_settings_form');
}

function chamilo_admin_settings_form() {
  $form['chamilo_sso_protocol'] = array(
    '#type' => 'radios',
    '#title' => t('SSO Server Protocol'),
    '#options' => array('http://', 'https://'),
    '#default_value' => variable_get('chamilo_sso_protocol', 0),
  );

  $form['chamilo_server'] = array(
    '#type' => 'textfield',
    '#title' => t('Application URL and Path'),
    '#description' => t('NOTICE: Do NOT use trailing slash nor http://. i.e: yourchamilosite.com'),
    '#default_value' => variable_get('chamilo_server', ''),
  );

  $form['chamilo_user'] = array(
    '#type' => 'textfield',
    '#title' => t('Username'),
    '#description' => t('User ID (example: user1).'),
    '#default_value' => variable_get('chamilo_user', ''),
  );
  $form['account']['chamilo_appkey'] = array(
    '#type' => 'textfield',
    '#title' => t('Security Key'),
    '#default_value' => variable_get('chamilo_appkey', ''),
    '#size' => 15,
  );
  return system_settings_form($form);
}

