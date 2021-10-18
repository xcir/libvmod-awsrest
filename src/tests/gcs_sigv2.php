<?php
//GCS署名v2テスト用 
//
//  php gcs_sigv4.php [access_key] [secret_key] [bucket] [path]
//  ex: php gcs_sigv2.php access_key secret_key example /test.txt
//

function getSignatureGCS($key, $dateStamp, $method, $bucket, $path)
{
	return base64_encode(hash_hmac("sha1", 
		sprintf("%s\n\n\n%s\n/%s%s", $method, $dateStamp, $bucket, $path),
		$key,
		true));
}
function main($access_key, $secret_key, $bucket, $path){


	$method="GET";
	$endpoint              = "http://storage.googleapis.com/$bucket${path}";
//	$endpoint              = "https://$bucket.storage.googleapis.com${path}";
	 
	//現在時刻の取得
	$t         = time();
	$amzdate   = date(DateTime::RFC1123);

	$sig = getSignatureGCS($secret_key, $amzdate, $method, $bucket, $path);
 
	 
	$authorization_header = sprintf('AWS %s:%s', $access_key, $sig);
	$headers = array(
		"Authorization: $authorization_header",
		"Date: $amzdate",
		);
	$request_url = $endpoint;
	 
	$context = array(
		"http" => array(
			"method"  => $method,
			"header"  => implode("\r\n", $headers)
		)
	);

	$ret = file_get_contents($request_url, false, stream_context_create($context));

	prn("Request",print_r($context,1));
	prn("Request_url",$request_url);
	prn("Response-header",print_r($http_response_header,1));
	prn("Response-body",$ret);
}
function prn($k,$v){
	echo str_repeat(">",40)."\n";
	echo ">>>>>$k\n";
	echo $v;
	echo "\n";
	echo str_repeat("<",40)."\n";
}

main($argv[1],$argv[2],$argv[3],$argv[4]);
