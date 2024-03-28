<?php
  $address = '127.0.0.1';
  $port = 5000;
  try{
    // 소켓 오브젝트를 만듭니다.
    $socket = socket_create(AF_INET, SOCK_STREAM, SOL_TCP);
    if ($socket === false) {
      echo "socket_create() failed: reason: " . socket_strerror(socket_last_error()) . "\n";
    }
    // 접속을 합니다.
    $result = socket_connect($socket, $address, $port);
    if ($result === false) {
      echo "socket_connect() failed.\nReason: ($result) " . socket_strerror(socket_last_error($socket)) . "\n";
    } 
    // 소켓 서버로부터 메시지를 받는다.
    // $out = socket_read($socket, 1024);
    // 소켓 서버로 메시지를 보낸다.
    $in = "[11:PASSWD]";
    socket_write($socket, $in, strlen($in));
  }finally{
    socket_close($socket);
  }
?>
<!DOCTYPE html>
<html>
<head>
  <title>title</title>
</head>
<body>
  <div><button onclick=LED_ON()>LED ON</button></div>
  <?=$out?>
</body>
<script>function LED_ON = () => {
  $address = '127.0.0.1';
  $port = 5000;
  try{
    // 소켓 오브젝트를 만듭니다.
    $socket = socket_create(AF_INET, SOCK_STREAM, SOL_TCP);
    if ($socket === false) {
      echo "socket_create() failed: reason: " . socket_strerror(socket_last_error()) . "\n";
    }
    // 접속을 합니다.
    $result = socket_connect($socket, $address, $port);
    if ($result === false) {
      echo "socket_connect() failed.\nReason: ($result) " . socket_strerror(socket_last_error($socket)) . "\n";
    } 
    // 소켓 서버로부터 메시지를 받는다.
    // $out = socket_read($socket, 1024);
    // 소켓 서버로 메시지를 보낸다.
    $in = "[11:PASSWD]";
    socket_write($socket, $in, strlen($in));
  }finally{
    socket_close($socket);
  }
}</script>
</html>
