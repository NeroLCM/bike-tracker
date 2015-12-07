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
    echo 'ACK';
    exit;
} else { // Test Mode
    // get points
    $points = array(
        array(37.874423, -122.268020, '18:20'),
        array(37.875210, -122.268471, '18:40'),
        array(37.876598, -122.268814, '19:00'),
    );
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