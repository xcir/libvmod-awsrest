<?php
//base http://docs.aws.amazon.com/general/latest/gr/sigv4-signed-request-examples.html
//GCS署名v4テスト用
//
//  php gcs_sigv4.php [access_key] [secret_key] [bucket] [url] [session_token(option)] [region(option)]
//
function hash_sha256_raw($msg, $key)
{
    return hash_hmac("sha256", $msg, $key, true);
}
 
function getSignature($key, $dateStamp, $regionName, $serviceName)
{
    $kDate    = hash_sha256_raw($dateStamp, "AWS4$key");
    $kRegion  = hash_sha256_raw($regionName, $kDate);
    $kService = hash_sha256_raw($serviceName, $kRegion);
    $kSigning = hash_sha256_raw("aws4_request", $kService);
    return $kSigning;
}
function main($access_key, $secret_key, $bucket, $canonical_uri, $session_token="", $region="asia-northeast1"){
	//パラメータ
	$service               = 'storage';
//	$canonical_uri         = 'URL指定' ;
//	$access_key            = 'アクセスキー';
//	$secret_key            = '秘密鍵';
//	$session_token         = 'セッショントークン';
	$method                = 'GET';
	$host                  = "${bucket}.${service}.googleapis.com";
	 
	$canonical_querystring = '';
	$signed_headers        = 'host;x-amz-content-sha256;x-amz-date';
	if($session_token) $signed_headers .= ";x-amz-security-token";
	 
	$endpoint              = "http://${host}${canonical_uri}";
	$payload               = '';
	 
	 
	 
	 
	$algorithm = 'AWS4-HMAC-SHA256';
	 
	 
	//現在時刻の取得
	$t         = time();
	$amzdate   = gmdate('Ymd\THis\Z', $t);
	$datestamp = gmdate('Ymd', $t);
	 
	//payload作成
	$payload_hash = hash('sha256', $payload);
	 
	//ヘッダ作成
	$canonical_headers  = "host:$host\n";
	$canonical_headers .= "x-amz-content-sha256:$payload_hash\n";
	$canonical_headers .= "x-amz-date:$amzdate\n";
	if($session_token) $canonical_headers .= "x-amz-security-token:$session_token\n";
	 
	//リクエスト生成
	$canonical_request  = "$method\n";
	$canonical_request .= "$canonical_uri\n";
	$canonical_request .= "$canonical_querystring\n";
	$canonical_request .= "$canonical_headers\n";
	$canonical_request .= "$signed_headers\n";
	$canonical_request .= "$payload_hash";
	 
	 
	$credential_scope = "$datestamp/$region/$service/aws4_request";
	$string_to_sign   = "$algorithm\n";
	$string_to_sign  .= "$amzdate\n";
	$string_to_sign  .= "$credential_scope\n";
	$string_to_sign  .= hash('sha256', $canonical_request);
	 
	$signing_key = getSignature($secret_key, $datestamp, $region, $service);
	$signature   = hash_hmac('sha256', $string_to_sign, $signing_key);
	 
	 
	$authorization_header = "$algorithm Credential=$access_key/$credential_scope, SignedHeaders=$signed_headers, Signature=$signature";
	$headers = array(
		"x-amz-content-sha256: $payload_hash",
		"Authorization: $authorization_header",
		"x-amz-date: $amzdate",
		);
	if($session_token) $headers[] = "x-amz-security-token: $session_token";
	$request_url = rtrim("$endpoint?$canonical_querystring","?");
	 
	$context = array(
		"http" => array(
			"method"  => $method,
			"header"  => implode("\r\n", $headers)
		)
	);

	$ret = file_get_contents($request_url, false, stream_context_create($context));

	prn("payload_hash",$payload_hash);
	prn("canonical_headers",$canonical_headers);
	prn("string_to_sign",$string_to_sign);
	prn("canonical_request",$canonical_request);
	prn("Request",print_r($context,1));
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
$token = '';
if(isset($argv[5]))$token = $argv[5];
$region = 'asia-northeast1';
if(isset($argv[6]))$region = $argv[6];
main($argv[1],$argv[2],$argv[3],$argv[4],$token,$region);
