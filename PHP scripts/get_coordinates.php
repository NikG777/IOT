 <?
require_once 'connect.php';
$link = mysqli_connect($host, $user, $password, $database) 
    or die("Не подключился" . mysqli_error($link));
   
   $str = file_get_contents("coordinates.txt",FILE_USE_INCLUDE_PATH);
   print_r($str);