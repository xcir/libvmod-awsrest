<?php

//
//php vta.php [aws config] [region] [bucket] [url]
//php vta.php ~/.aws/config ap-northeast-1 examplebucket /foo/bar


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

//sts
$r   = json_decode(shell_exec('aws sts get-session-token'), 1);
$i   = ceil(strlen($r['Credentials']['SessionToken'])/2);
$cmd = sprintf('varnishtest -Dregion=%s -Daccesskey="%s" -Dsecretkey="%s" -Dtoken1="%s" -Dtoken2="%s" -Dbucket="%s" -Durl="%s" t*.vta',
         $argv[2],
         $r['Credentials']['AccessKeyId'],
         $r['Credentials']['SecretAccessKey'],
         substr($r['Credentials']['SessionToken'],0,$i),
         substr($r['Credentials']['SessionToken'],$i),
         $argv[3],
         $argv[4]
  );
echo shell_exec($cmd);


