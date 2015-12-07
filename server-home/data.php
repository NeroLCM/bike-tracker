<?php
// $token = md5('ee149-fall-2015') - main board - 598234c470cd87148cd26090f83ac4e6
// $token = md5('bike-tracker') - test token - d325b8f7a0a6b1d325ab327251ade77d
$file_name = 'data.csv'; // 'test-data.csv'
$token = $_GET['token'];
// Auth
// Authed
if (!isset($_GET['lat']) || !isset($_GET['lon']) || !isset($_GET['utc'])) {
    echo "Bad Request";
    exit;
}

// Resolve timing
$result = "";
$utc_hms = $_GET['utc'];
$utc_hh = (int) (substr($utc_hms, 0, 2));
$curr_hh = (int) (date('H'));

$diff = $curr_hh - $utc_hh;
if ($diff >= 0 && $diff <= 12) {
    $result = $result . date('m-d-Y');
    $result = $result . ' ' . substr($utc_hms, 0, 2) . ':' .
              substr($utc_hms, 2, 2) . ':' . substr($utc_hms, 4, 2);
    $result = $result . ' ' . date('T');
} else {
    $result = $result . date('m') . '-';
    $result = $result . (string)(((int) date('d')) - 1) . '-';
    $result = $result . date('Y');
    $result = $result . ' ' . substr($utc_hms, 0, 2) . ':' .
              substr($utc_hms, 2, 2) . ':' . substr($utc_hms, 4, 2);
    $result = $result . ' ' . date('T');
}

$fhandle = fopen('data.csv', 'a');
fwrite(
    $fhandle, 
    $_GET['lat'] . ',' .
    $_GET['lon'] . ',' .
    $result . "\n"
);
fclose($fhandle);
echo "OK &nbsp;";
echo $_GET['lat'], $_GET['lon'], $result
?>