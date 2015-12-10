<?php

// $token = md5('ee149-fall-2015') - main board
// $token = md5('bike-tracker') - test token
$token = $_GET['token'];
/*if (
        !$token || 
        ($token != '598234c470cd87148cd26090f83ac4e6' &&
        $token != 'd325b8f7a0a6b1d325ab327251ade77d')
    ) {
    header('HTTP/1.0 401 Unauthorized');
    include("401.shtml");
    exit;
}*/
if ($token == '598234c470cd87148cd26090f83ac4e6') {
    $ts = fopen('timestamp', 'w');
    fwrite($ts, time());
    fclose($ts);

    if (isset($_GET["bat"])) {
        $bat = fopen('bat', 'w');
        fwrite($bat, $_GET["bat"]);
        fclose($bat);
    }
    
    $fhandle = fopen('staterc', 'r');
    $state = fread($fhandle, 1);
    fclose($fhandle);
    if ($state == "A") {
        echo "ACK";
    } else if ($state == "T") {
        echo "TRACKING";
    } else {
        $fhandle = fopen('state', 'w');
        fwrite($fhandle, "A");
        fclose($fhandle);
        echo "ACK";
    }
    exit;
} else { // Test Mode
    // get points
    $points = array(
        // array(37.874423, -122.268020, '18:20'),
        // array(37.875210, -122.268471, '18:40'),
        // array(37.876598, -122.268814, '19:00'),
    );
    $fhandle = fopen('data.csv', 'r');

    while (!feof($fhandle)) {
        $line = fgetcsv($fhandle);
        if ($line[0] == NULL) break;
        array_push($points, $line);
    }
    $points = array_slice($points, -10, 10);
    
    //echo print_r($points);
    include("maps-head.ht");
    // convert points to JS string
    $js_str = "";
    foreach ($points as $point) {
        $pop = $point === end($points) ? "true" : "";
        $tmp = sprintf("addMarker(%f, %f, '%s', '%s');",
            $point[0], $point[1], $point[2], $pop);
        $js_str = $js_str . $tmp;
    }
    echo $js_str;
    include("maps-tail.ht");
}

?>