<?php
/****************************************************************************
 | PHP library for Chamilo
 |  A PHP library to manage Chamilo information through the SOAP API
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

class chamiloWs extends SoapClient {
  /**
   * Set this to true if you need to troubleshoot this module,
   * Messages will be logged to though Drupal watchdog
   */
  private $_debug = false;

  /**
   * The Chamilo server URL
   */
  private $_chamiloServerUrl = false;

  /**
   * The client IP address (as seen by the Chamilo server)
   */
  private $_clientIP = false;

  /**
   * Class constructor
   * @param {string} The Chamilo server URL (with protocol, example: http://chamilo.mycompany.com)
   */
  function __construct($chamiloServer) {
    // Remember Chamilo server URL
    $this->_chamiloServerUrl = rtrim($chamiloServer, '/');

    // Create SOAP client
    parent::__construct($this->wsEndPoint());

    // Get the client IP address (to be used for signing SOAP requests)
    $url = $this->_chamiloServerUrl . '/main/webservices/testip.php';
    if (ini_get('allow_url_fopen')) {
      $this->_clientIP = file_get_contents($url);
    } else {
      if (!($ch = curl_init())) {
        throw new Exception('Your hosting must support cURL or allow http wrapper for file_get_contents.');
      } else {
        curl_setopt($ch, CURLOPT_URL, $url);
        curl_setopt($ch, CURLOPT_RETURNTRANSFER, 1);
        $this->_clientIP = curl_exec($ch);
        curl_close($ch); 
        unset($ch);
      }
    }
    if ($this->_debug) {
      error_log('Found client IP [' . $this->_clientIP . '] for signing SAOP requests.');
    }
  }

  /**
   * Generic call to backend webservice
   * @private
   */
  private function call() {
    $args = func_get_args();

    // Get method to call
    $method = array_shift($args);

    // Calculate the secret key
    $secret_key = sha1($this->_clientIP.variable_get('chamilo_appkey', ''));

    // Any other argument?
    if (count($args) == 0) {
      // If secret key is the only argument, do not use an array
      $args = array($secret_key);
    } elseif (count($args) == 1 && is_array($args[0])) {
      // Add secret key to given argument
      $args[0]['secret_key'] = $secret_key;
    } else {
      watchdog('chamilo', 'Invalid arguments for webservice method [%method]', array('%method' => $method), WATCHDOG_ERROR); 
    }
  
    // Call API
    return call_user_func_array(array($this, $method), $args);
  }

  /**
   * Returns the Chamilo SOAP webservice endpoint (according to user configuration)
   * @private
   */
  private function wsEndPoint() {
    return $this->_chamiloServerUrl . '/main/webservices/registration.soap.php?wsdl';
  }

  /**
   * List of Chamilo courses
   */
  function courseList() {
    return $this->call('WSListCourses');
  }

}
 
?>