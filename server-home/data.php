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

$fhandle = fopen('data.csv', 'a');



?>