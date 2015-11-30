<?php


// $token = md5('ee149-fall-2015') - main board
// $token = md5('bike-tracker') - test token
$token = $_GET['token'];
if (
        !$token || 
        ($token != '598234c470cd87148cd26090f83ac4e6' &&
        $token != 'd325b8f7a0a6b1d325ab327251ade77d')
    ) {
    header('HTTP/1.0 401 Unauthorized');
    include("401.shtml");
    exit;
}
if ($token == '598234c470cd87148cd26090f83ac4e6') {
    echo 'Welcome, Arduino!';
    exit;
} else { // Test Mode
    
}

?>