#include <stdlib.h>
#include "vcl.h"
#include "vrt.h"
#include "bin/varnishd/cache.h"
#include <time.h>
#include <string.h>

#include <arpa/inet.h>
#include <syslog.h>
#include <poll.h>
	#include <fcntl.h>
	#include <sys/mman.h>
	#include <sys/types.h>
#include <stdio.h>

#include "vcc_if.h"
#include <mhash.h>

#include <mhash.h>
#include <curl/curl.h>
#include <json/json.h>

enum alphabets {
	BASE64 = 0,
	BASE64URL = 1,
	BASE64URLNOPAD = 2,
	N_ALPHA
};

static struct e_alphabet {
	char *b64;
	char i64[256];
	char padding;
} alphabet[N_ALPHA];


static char *aws_accessKeyId;
static char *aws_secretAccessKey;
static char *aws_securityToken;
static time_t aws_expiration = 0;


static void
vmod_digest_alpha_init(struct e_alphabet *alpha)
{
	int i;
	const char *p;

	for (i = 0; i < 256; i++)
		alpha->i64[i] = -1;
	for (p = alpha->b64, i = 0; *p; p++, i++)
		alpha->i64[(int)*p] = (char)i;
	if (alpha->padding)
		alpha->i64[alpha->padding] = 0;
}

int
init_function(struct vmod_priv *priv, const struct VCL_conf *conf)
{
    	alphabet[BASE64].b64 =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdef"
		"ghijklmnopqrstuvwxyz0123456789+/";
	alphabet[BASE64].padding = '=';
	alphabet[BASE64URL].b64 =
		 "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdef"
		 "ghijklmnopqrstuvwxyz0123456789-_";
	alphabet[BASE64URL].padding = '=';
	alphabet[BASE64URLNOPAD].b64 =
		 "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdef"
		 "ghijklmnopqrstuvwxyz0123456789-_";
	alphabet[BASE64URLNOPAD].padding = 0;
	vmod_digest_alpha_init(&alphabet[BASE64]);
	vmod_digest_alpha_init(&alphabet[BASE64URL]);
	vmod_digest_alpha_init(&alphabet[BASE64URLNOPAD]);
	return (0);
}
static size_t
base64_encode (struct e_alphabet *alpha, const char *in,
		size_t inlen, char *out, size_t outlen)
{
	size_t outlenorig = outlen;
	unsigned char tmp[3], idx;

	if (outlen<4)
		return -1;

	if (inlen == 0) {
		*out = '\0';
		return (1);
	}

	while (1) {
		assert(inlen);
		assert(outlen>3);

		tmp[0] = (unsigned char) in[0];
		tmp[1] = (unsigned char) in[1];
		tmp[2] = (unsigned char) in[2];
		*out++ = alpha->b64[(tmp[0] >> 2) & 0x3f];

		idx = (tmp[0] << 4);
		if (inlen>1)
			idx += (tmp[1] >> 4);
		idx &= 0x3f;
		*out++ = alpha->b64[idx];

		if (inlen>1) {
			idx = (tmp[1] << 2);
			if (inlen>2)
				idx += tmp[2] >> 6;
			idx &= 0x3f;

			*out++ = alpha->b64[idx];
		} else {
			if (alpha->padding)
				*out++ = alpha->padding;
		}

		if (inlen>2) {
			*out++ = alpha->b64[tmp[2] & 0x3f];
		} else {
			if (alpha->padding)
				*out++ = alpha->padding;
		}

		/*
		 * XXX: Only consume 4 bytes, but since we need a fifth for
		 * XXX: NULL later on, we might as well test here.
		 */
		if (outlen<5)
			return -1;

		outlen -= 4;

		if (inlen<4)
			break;

		inlen -= 3;
		in += 3;
	}

	assert(outlen);
	outlen--;
	*out = '\0';
	return outlenorig-outlen;
}


static const char *
vmod_base64_generic(struct sess *sp, enum alphabets a, const char *msg)
{
	char *p;
	int u;
	assert(msg);
	assert(a<N_ALPHA);
	CHECK_OBJ_NOTNULL(sp, SESS_MAGIC);
	CHECK_OBJ_NOTNULL(sp->ws, WS_MAGIC);

	u = WS_Reserve(sp->ws,0);
	p = sp->ws->f;
	u = base64_encode(&alphabet[a],msg,strlen(msg),p,u);
	if (u < 0) {
		WS_Release(sp->ws,0);
		return NULL;
	}
	WS_Release(sp->ws,u);
	return p;
}


static const char *
vmod_hmac_generic(struct sess *sp, hashid hash, const char *key, const char *msg)
{
	size_t maclen = mhash_get_hash_pblock(hash);
	size_t blocksize = mhash_get_block_size(hash);
	unsigned char mac[blocksize];
	unsigned char *hexenc;
	unsigned char *hexptr;
	int j;
	MHASH td;

	assert(msg);
	assert(key);
	CHECK_OBJ_NOTNULL(sp, SESS_MAGIC);
	CHECK_OBJ_NOTNULL(sp->ws, WS_MAGIC);

	/*
	 * XXX: From mhash(3):
	 * size_t mhash_get_hash_pblock(hashid type);
	 *     It returns the block size that the algorithm operates. This
	 *     is used in mhash_hmac_init. If the return value is 0 you
	 *     shouldn't use that algorithm in  HMAC.
	 */
	assert(mhash_get_hash_pblock(hash) > 0);

	td = mhash_hmac_init(hash, (void *) key, strlen(key),
		mhash_get_hash_pblock(hash));
	mhash(td, msg, strlen(msg));
	mhash_hmac_deinit(td,mac);

	//base64encode
	hexenc = WS_Alloc(sp->ws, 64); // 0x, '\0' + 2 per input
	base64_encode(&alphabet[0],mac,blocksize,hexenc,64);
	return hexenc;
}

void vmod_s3_generic(struct sess *sp,
	const char *accesskey,
	const char *secret,
	const char *token,
	const char *method,
	const char *contentMD5,
	const char *contentType,
	const char *CanonicalizedResource,
	double date

){
	//valid
	AN(accesskey);
	AN(secret);
	AN(method);
	AN(CanonicalizedResource);
	
	int len = 35;//Date + \n*4
	char datetxt[32];
	struct tm tm;
	time_t tt;
	char *buf;

	//calc length
	//method
	len += strlen(method);
	//content-md5
	if(contentMD5)	len += strlen(contentMD5);
	//content-type
	if(contentType)	len += strlen(contentType);
	//CanonicalizedAmzHeaders(x-amz-*)
	if(token)	len += 22 + strlen(token);
	//CanonicalizedResource
	if(CanonicalizedResource)	len += strlen(CanonicalizedResource);
	
	buf = calloc(1, len + 1);
	AN(buf);

	////////////////
	//gen date text
	datetxt[0] = 0;
	tt = (time_t) date;
	(void)gmtime_r(&tt, &tm);
	AN(strftime(datetxt, 32, "%a, %d %b %Y %T +0000", &tm));
//        AN(strftime(datetxt, 32, "%a, %d %b %Y %T GMT", &tm));


	////////////////
	//build raw signature

	//method
	strcat(buf,method);
	strcat(buf,"\n");
	
	//content-md5
	if(contentMD5)	strcat(buf,contentMD5);
	strcat(buf,"\n");
	
	//content-type
	if(contentType)	strcat(buf,contentType);
	strcat(buf,"\n");

	//date
	strcat(buf,datetxt);
	strcat(buf,"\n");

	//CanonicalizedAmzHeaders(x-amz-*)
	if(token) {
		strcat(buf,"x-amz-security-token:");
		strcat(buf,token);
		strcat(buf,"\n");
	}

	//CanonicalizedResource
	if(CanonicalizedResource)	strcat(buf,CanonicalizedResource);

	////////////////
	//build signature(HMAC-SHA1 + BASE64)
	const char* signature=vmod_hmac_generic(sp,MHASH_SHA1 , secret,buf);
	
	////////////////
	//free buffer
	free(buf);
	
	////////////////
	//set data
	VRT_SetHdr(sp, HDR_REQ, "\005Date:", datetxt,vrt_magic_string_end);
	const char* auth = VRT_WrkString(sp,"AWS ",accesskey,":",signature,vrt_magic_string_end);
	VRT_SetHdr(sp, HDR_REQ, "\016Authorization:", auth,vrt_magic_string_end);
        if (token)	VRT_SetHdr(sp, HDR_REQ, "\025x-amz-security-token:", token,vrt_magic_string_end);
}

typedef struct curl_data{
	char* buf;
	int pos;
} curl_data_t;

write_function_pt(void *ptr, size_t size, size_t nmemb, curl_data_t* data){
	memcpy(data->buf + data->pos, ptr, size*nmemb);
	data->pos += size*nmemb;

	return size*nmemb;
}

void vmod_s3_generic_iam(struct sess *sp,
	const char *iamAddress,
	const char *method,
	const char *contentMD5,
	const char *contentType,
	const char *CanonicalizedResource,
	double date

){

	time_t localtime;
	localtime = time(NULL);

        // if key expires in less that 5 min, new key is guaranteed to be available
	if(difftime(aws_expiration, localtime) > 5*60) {
		// credentials are still valid
		if(aws_accessKeyId != NULL && aws_secretAccessKey != NULL && aws_securityToken != NULL) {
			vmod_s3_generic(sp, aws_accessKeyId, aws_secretAccessKey, aws_securityToken,
					method, contentMD5, contentType, 
					CanonicalizedResource, 
					date);
			return;
		}
	} 


	CURL *curl;
	// HEAD request to get file length
	double length = -1;
        curl = curl_easy_init();
        if(curl) {            
                curl_easy_setopt(curl, CURLOPT_URL, iamAddress);
                curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);
		curl_easy_setopt(curl, CURLOPT_NOBODY, 1);                

                if(curl_easy_perform(curl) == CURLE_OK) {
			if(curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &length) != CURLE_OK){
				length = -1;
			}
		}
                curl_easy_cleanup(curl);
        }


	if(length <= 0) {
		return;
	}

	char* response = malloc(((long)length)+1);
	if(response == NULL){
		return;
	}
	
	curl_data_t data = {
		.buf = response,
		.pos = 0
	};

	// GET request to receive data
	curl = curl_easy_init();
	if(curl) {            
		curl_easy_setopt(curl, CURLOPT_URL, iamAddress);
		curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_function_pt);
		curl_easy_perform(curl);
		curl_easy_cleanup(curl);
	}

	struct json_object *resp_obj;
	struct json_object *key_id;
	struct json_object *access_key;
	struct json_object *security_token;
	struct json_object *expiration_date;

	/*  sample response
	 * {
	 * "Code" : "Success",
	 * "LastUpdated" : "2012-12-18T17:38:13Z",
	 * "Type" : "AWS-HMAC",
	 * "AccessKeyId" : "ASI...",
	 * "SecretAccessKey" : "Z8vv...",
	 * "Token" : "AQoD...",
	 * "Expiration" : "2012-12-19T00:13:24Z"
	 * }
	 */

	resp_obj = json_tokener_parse(response);
	key_id = json_object_object_get(resp_obj, "AccessKeyId");
	access_key = json_object_object_get(resp_obj, "SecretAccessKey");
	security_token = json_object_object_get(resp_obj, "Token");
	expiration_date = json_object_object_get(resp_obj, "Expiration");

	struct tm tm;
	memset(&tm, 0, sizeof(struct tm));
	strptime(json_object_get_string(expiration_date), "%Y-%m-%dT%H:%M:%SZ", &tm);
	time_t expiration = mktime(&tm);

	aws_accessKeyId = json_object_get_string(key_id);
	aws_secretAccessKey = json_object_get_string(access_key);
	aws_securityToken = json_object_get_string(security_token);
	aws_expiration = expiration;	

	free(response);

	vmod_s3_generic(sp, aws_accessKeyId, aws_secretAccessKey, aws_securityToken, method, contentMD5, contentType, CanonicalizedResource, date);
}


const char*
vmod_lf(struct sess *sp){
	return VRT_WrkString(sp,"\n",vrt_magic_string_end);
}
	
