 <?
require_once 'connect.php';
$link = mysqli_connect($host, $user, $password, $database) 
    or die("Не подключился" . mysqli_error($link));
  //  $str = array();
if (isset($_POST['info'])) 
	{
        $info = $_POST['info'];
        file_put_contents("coordinates.txt",$info);
        
   }
if(!empty($info)){
ini_set( 'default_charset', 'UTF-8' );
printf($info);
}
?>