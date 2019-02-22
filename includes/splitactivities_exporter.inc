<?php
/**
 *  @author Klaas Eikelboom (CiviCooP) <klaas.eikelboom@civicoop.org>
 *  @date 2/21/19 7:21 PM
 *  @license AGPL-3.0
 */

class SplitActivitiesExporter {


  var $data;
  var $cf;
  var $wachtpostDienstTypeId;
  var $activityTargetTypeId;
  var $activityAssigneeTypeId;
  var $params;

  /**
   * SplitActivitiesExporter constructor.
   */
  public function __construct($params) {
    $this->params = $params;
    $this->cf = $this->customFields(
      ['einddatum_tijd']
    );
    $this->wachtpostDienstTypeId = civicrm_api3('OptionValue', 'getvalue', [
      'return' => "value",
      'option_group_id' => "activity_type",
      'name' => "domusmedica_wachtpostdienst",
    ]);
    $this->activityAssigneeTypeId = civicrm_api3('OptionValue', 'getvalue', [
      'return' => "value",
      'option_group_id' => "activity_contacts",
      'name' => "Activity Assignees",
    ]);
    $this->activityTargetTypeId = civicrm_api3('OptionValue', 'getvalue', [
      'return' => "value",
      'option_group_id' => "activity_contacts",
      'name' => "Activity Targets",
    ]);
  }

  /**
   * @param $field
   *
   * @return string
   * @throws \Exception
   */
  private function day($field){
    $date = new DateTime($field);
    return $date->format('d/m/Y');
  }

  /**
   * @param $field
   *
   * @return string
   * @throws \Exception
   */
  private function time($field){
    $date = new DateTime($field);
    return $date->format('H:i');
  }

  private function queryDay($field) {
    if (empty($field)) {
      return FALSE;
    }
    $date = DateTime::createFromFormat('d-m-Y', $field);
    if($date){
      return $date->format('Y-m-d');
    } else {
      return false;
    }
  }

  /**
   * @param $fields
   *
   * @return array
   * @throws \CiviCRM_API3_Exception
   */
  private function customFields($fields){
    $result = [];
    foreach($fields as $field){
      $cf = civicrm_api3('CustomField', 'getsingle', [
        'name' => $field,
      ]);
      $cf['table_name'] = civicrm_api3('CustomGroup', 'getvalue', [
        'return' => "table_name",
        'id' => $cf['custom_group_id'],
      ]);
      $result[$field]=$cf;
    }
    return $result;
  }

  /**
   * Read the activities to be exported from civicrm
   */
  public function readFromCivi(){
    $sql = <<< SQL
SELECT DISTINCT act.id AS id
,               act.activity_date_time AS start_time
,               c1.last_name AS last_name
,               c1.first_name AS first_name
,               c1.external_identifier AS external_identifier
,               cf.{$this->cf['einddatum_tijd']['column_name']} AS end_time
,               c2.display_name AS wachtdienst
FROM civicrm_activity act
  LEFT JOIN civicrm_activity_contact ac ON act.id = ac.activity_id AND (ac.record_type_id = {$this->activityAssigneeTypeId})
  LEFT JOIN civicrm_contact c1 ON ac.contact_id = c1.id
  LEFT JOIN civicrm_activity_contact ac2 ON act.id = ac2.activity_id AND (ac2.record_type_id = {$this->activityTargetTypeId})
  LEFT JOIN civicrm_contact c2 ON ac2.contact_id = c2.id
  LEFT JOIN {$this->cf['einddatum_tijd']['table_name']} cf ON act.id = cf.entity_id
  WHERE act.activity_type_id = {$this->wachtpostDienstTypeId}
SQL;
    $result[] = ["Datum","Achternaam","Voornaam","Riziv","Starttijd","Eindtijd","Wachtdienst","Aangepaste tekst"];
    $queryParams = [];

    if(isset($this->params['display_name'])&&!empty($this->params['display_name'])){
      $sql .= " AND c2.display_name like %1";
      $queryParams[1] = [
        '%'.$this->params['display_name'].'%',
        'String'
      ];
    }

    if(isset($this->params['activity_date_time'])){
      $min = $this->queryDay($this->params['activity_date_time']['min']);
      if($min){
        $sql .= " AND  act.activity_date_time > %2";
        $queryParams[2] = [$min,'String'];
      }
      $max = $this->queryDay($this->params['activity_date_time']['max']);
      if($max){
        $sql .= " AND  act.activity_date_time < %3";
        $queryParams[3] = [$max,'String'];
      }
    }


    $dao = CRM_Core_DAO::executeQuery($sql,$queryParams);
    while($dao->fetch()){
      
      $dayStart = $this->day($dao-> start_time);
      $dayEnd   = $this->day($dao->end_time);
      if($dayStart==$dayEnd){
        // just one line
        $result [] =   [
        'datum' => $dayStart,
        'achternaam'=> $dao->last_name,
        'voornaam' => $dao -> first_name,
        'riziv' =>  $dao -> external_identifier,
        'startijd'=> $this->time($dao-> start_time),
        'eindtijd'=> $this->time($dao-> end_time),
        'wachtdienst' => $dao->wachtdienst
        ];
      } else {

      $result[]  = [
        'id'=> $dao->id,
        'datum' => $dayStart,
        'achternaam'=> $dao->last_name,
        'voornaam' => $dao -> first_name,
        'riziv' =>  $dao -> external_identifier,
        'startijd'=> $this->time($dao-> start_time),
        'eindtijd'=> '00:00',
        'wachtdienst' => $dao->wachtdienst
      ];
      $result[]  = [
        'id'=> $dao->id,
        'datum' => $dayEnd,
        'achternaam'=> $dao->last_name,
        'voornaam' => $dao -> first_name,
        'riziv' =>  $dao -> external_identifier,
        'startijd'=> '00:00',
        'eindtijd'=> $this->time($dao->end_time),
        'wachtdienst' => $dao->wachtdienst
      ];
    }}
    $this->data=$result;
  }

  /**
   * @return string
   */
  public function format() {
    $output = "<pre>";
    foreach ($this->data as $row) {
      $first = TRUE;
      foreach ($row as $field) {
        if ($first) {
          $first = FALSE;
        }
        else {
          $output .= ',';
        }
        $output .= '"' . $field . '"';

      }
      $output .= "\n";
    }
    return $output.'</pre>';
  }

}