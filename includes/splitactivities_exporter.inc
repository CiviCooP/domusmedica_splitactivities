<?php
/**
 *  @author Klaas Eikelboom (CiviCooP) <klaas.eikelboom@civicoop.org>
 *  @date 2/21/19 7:21 PM
 *  @license AGPL-3.0
 */

class SplitActivitiesExporter {


  var $data;

  private function day($field){
    $date = new DateTime($field);
    return $date->format('d/m/Y');
  }

  private function time($field){
    $date = new DateTime($field);
    return $date->format('H:i');
  }

  /**
   *
   */
  public function readFromCivi(){
    $sql = <<< SQL
SELECT DISTINCT act.id AS id
,               act.activity_date_time AS start_time
,               c1.last_name AS last_name
,               c1.first_name AS first_name
,               c1.external_identifier AS external_identifier
,               cf.einddatum_tijd_33 AS end_time
,               c2.display_name AS wachtdienst
FROM civicrm_activity act
  LEFT JOIN civicrm_activity_contact ac ON act.id = ac.activity_id AND (ac.record_type_id = '1')
  LEFT JOIN civicrm_contact c1 ON ac.contact_id = c1.id
 
  LEFT JOIN civicrm_activity_contact ac2 ON act.id = ac2.activity_id AND (ac2.record_type_id = '3')
  LEFT JOIN civicrm_contact c2 ON ac2.contact_id = c2.id
  LEFT JOIN civicrm_value_wachtpost_vel_6 cf ON act.id = cf.entity_id
  WHERE (( (act.activity_type_id IN  ('200')) ))
SQL;
    $result = [];
    $dao = CRM_Core_DAO::executeQuery($sql,[]);
    while($dao->fetch()){
      
      $dayStart = $this->day($dao-> start_time);
      $dayEnd   = $this->day($dao->end_time);
      if($dayStart==$dayEnd){
        // just one line
        $result [] =   ['id'=> $dao->id,
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