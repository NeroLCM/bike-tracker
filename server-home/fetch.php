<?php
if (!isset($_SERVER['PHP_AUTH_USER'])||!($_SERVER['PHP_AUTH_PW']=='20112012hfai')) {
    header('WWW-Authenticate: Basic realm="AI Advanced Password Auth"');
    header('HTTP/1.0 401 Unauthorized');
    include("401.shtml");
    exit;
} else {
$file=$_GET['file'];
if(!$file)
{echo "Please provide a file name.";exit;}

if(!file_exists($file))
{echo "File doesn't exist.";exit;}
header("Content-Type: application/force-download");
header("Content-Disposition: attachment; filename=".basename($file));
readfile($file);
}
?>
