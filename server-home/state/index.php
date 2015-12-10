<?php
$fhandle = fopen('../staterc', 'r');
$ts = fopen('../timestamp', 'r');
$bat = fopen('../bat', 'r');
$bat_lv = fread($bat, 10);
fclose($bat);

$state = fread($fhandle, 1);
fclose($fhandle);
$time_res = intval(fread($ts, 100));
fclose($ts);
if ($state == "T") {
    echo "The current state is Tracking. Response will be TRACKING. <br/><hr/>";
}
else {
    echo "The current state is Normal. Response will be ACK. <br/><hr/>";
}
$dt1 = new DateTime("@$time_res");
$dt1->setTimezone(new DateTimeZone('America/Los_Angeles'));
$time_res += 1800;
$dt2 = new DateTime("@$time_res");
$dt2->setTimezone(new DateTimeZone('America/Los_Angeles'));
$d1s = $dt1->format('m-d-Y H:i T');
$d2s = $dt2->format('m-d-Y H:i T');
if ($state == "T") {
    $aux = sprintf(
    "Last request recieved at %s. The next request will come around %s.<br/>",
    $d1s,
    $d2s
    );
    $aux .= "After the next request the board will enter Tracking mode, where";
    $aux .= " it sends a request every 5 seconds.<br/><hr/>";
} else {
    $aux = sprintf(
        "Last request recieved at %s. The next request will come around %s.<br/><hr/>",
        $d1s,
        $d2s
    );
}
echo $aux;

$aux = sprintf("The current battery level is %s.<br/><hr/>", $bat_lv . "%");
echo $aux;

if (isset($_GET['switch'])) {
    $fhandle = fopen('../staterc', 'w');
    if ($state == "A") {
        fwrite($fhandle, "T");
        echo "Changing state to TRACKING";
    } else {
        fwrite($fhandle, "A");
        echo "Changing state to NORMAL";
    }
    fclose($fhandle);
}
?>