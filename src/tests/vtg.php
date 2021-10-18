<?php

//
//php vta.php [region] [bucket] [url]
//php vta.php asia-northeast1 examplebucket /foo/bar


//non-sts
$r   = parse_ini_file($argv[1]);
$cmd = sprintf('varnishtest -Dregion=%s -Daccesskey="%s" -Dsecretkey="%s" -Dbucket="%s" -Durl="%s" r*.vta',
         $argv[2],
         $r['aws_access_key_id'],
         $r['aws_secret_access_key'],
         $argv[3],
         $argv[4]
  );
echo shell_exec($cmd);


