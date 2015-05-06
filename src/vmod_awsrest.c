#include <stdlib.h>
#include "vcl.h"
#include "vrt.h"
#include "bin/varnishd/cache.h"
#include <time.h>
#include <string.h>
#include <syslog.h>
#include <stdio.h>
#include <mhash.h>

#include "vcc_if.h"



static const char *
vmod_hmac_sha256(struct sess *sp,
	const char *key,size_t lkey, const char *msg,size_t lmsg,bool raw)
{
	hashid hash = MHASH_SHA256;
	size_t maclen = mhash_get_hash_pblock(hash);
	size_t blocksize = mhash_get_block_size(hash);
	unsigned char mac[blocksize];
	int j;
	int i;
	MHASH td;
	char *p;
	char *ptmp;

	assert(msg);
	assert(key);

	assert(mhash_get_hash_pblock(hash) > 0);

	td = mhash_hmac_init(hash, (void *) key, lkey,
		mhash_get_hash_pblock(hash));
	mhash(td, msg, lmsg);
	mhash_hmac_deinit(td,mac);
	if(raw) return mac;
	
	p = WS_Alloc(sp->ws,mhash_get_block_size(hash)*2 + 1);
	ptmp = p;
	for (i = 0; i<mhash_get_block_size(hash);i++) {
		sprintf(ptmp,"%.2x",mac[i]);
		ptmp+=2;
	}
	return p;
	
	
}
static const char *
vmod_v4_getSignature(struct sess *sp,
	const char* secret_key, const char* dateStamp, const char* regionName, const char* serviceName,const char* string_to_sign
){
	size_t len = strlen(secret_key) + 5;
	char key[len];
	char *kp = &key;
	sprintf(kp,"AWS4%s",secret_key);
	
	char *kDate    = vmod_hmac_sha256(sp,kp,strlen(kp), dateStamp,strlen(dateStamp),true);
	char *kRegion  = vmod_hmac_sha256(sp,kDate,   32, regionName,strlen(regionName),true);
	char *kService = vmod_hmac_sha256(sp,kRegion, 32, serviceName,strlen(serviceName),true);
	char *kSigning = vmod_hmac_sha256(sp,kService,32, "aws4_request", 12,true);
	
	return vmod_hmac_sha256(sp,kSigning,32, string_to_sign,strlen(string_to_sign),false);
}


static const char *
vmod_hash_sha256(struct sess *sp, const char *msg)
{
	MHASH td;
	hashid hash = MHASH_SHA256;
	unsigned char h[mhash_get_block_size(hash)];
	int i;
	char *p;
	char *ptmp;
	td = mhash_init(hash);
	mhash(td, msg, strlen(msg));
	mhash_deinit(td, h);
	p = WS_Alloc(sp->ws,mhash_get_block_size(hash)*2 + 1);
	ptmp = p;
	for (i = 0; i<mhash_get_block_size(hash);i++) {
		sprintf(ptmp,"%.2x",h[i]);
		ptmp+=2;
	}
	return p;
}

void vmod_v4_generic(struct sess *sp,
	const char *service,               //= 's3';
	const char *region,                //= 'ap-northeast-1';
	const char *access_key,            //= 'your access key';
	const char *secret_key,            //= 'your secret key';
	const char *_signed_headers,       //= 'host;';// x-amz-content-sha256;x-amz-date is appended by default.
	const char *_canonical_headers,    //= 'host:s3-ap-northeast-1.amazonaws.com\n'
	unsigned feature                   //= reserved param(for varnish4)
){

	
	////////////////
	//get data
	char *method;
	char *requrl;
	enum gethdr_e where;
	if (sp->wrk->bereq->ws != NULL){
		//bereq
		method= sp->wrk->bereq->hd[HTTP_HDR_REQ].b;
		requrl= sp->wrk->bereq->hd[HTTP_HDR_URL].b;
		where = HDR_BEREQ;
	}else{
		//req
		method= sp->http->hd[HTTP_HDR_REQ].b;
		requrl= sp->http->hd[HTTP_HDR_URL].b;
		where = HDR_REQ;
	}

	////////////////
	//create date
	time_t tt;
	char amzdate[17];
	char datestamp[9];
	tt = time(NULL);
	struct tm * gmtm = gmtime(&tt);
	
	sprintf(amzdate,
		"%d%02d%02dT%02d%02d%02dZ",
		gmtm->tm_year +1900,
		gmtm->tm_mon  +1,
		gmtm->tm_mday,
		gmtm->tm_hour,
		gmtm->tm_min,
		gmtm->tm_sec
	);
	sprintf(datestamp,
		"%d%02d%02d",
		gmtm->tm_year +1900,
		gmtm->tm_mon  +1,
		gmtm->tm_mday
	);

	////////////////
	//create payload
	const char * payload_hash = vmod_hash_sha256(sp, "");
	
	////////////////
	//create signed headers
	size_t len = strlen(_signed_headers) + 32;
	char *psigned_headers = WS_Alloc(sp->ws,len);
	sprintf(psigned_headers,"%sx-amz-content-sha256;x-amz-date",_signed_headers);
	
	
	////////////////
	//create canonical headers
	len = strlen(_canonical_headers) + 115;
	char *pcanonical_headers = WS_Alloc(sp->ws,len);
	sprintf(pcanonical_headers,"%sx-amz-content-sha256:%s\nx-amz-date:%s\n",_canonical_headers,payload_hash,amzdate);
	
	////////////////
	//create credential scope
	len = strlen(datestamp)+ strlen(region)+ strlen(service)+ 16;
	char *pcredential_scope = WS_Alloc(sp->ws,len);
	sprintf(pcredential_scope,"%s/%s/%s/aws4_request",datestamp,region,service);
	
	////////////////
	//create canonical request
	len = strlen(method)+ strlen(requrl)+ strlen(pcanonical_headers)+ strlen(psigned_headers)+ strlen(payload_hash) + 6;
	char *pcanonical_request = WS_Alloc(sp->ws,len);
	char tmpform[32];
	tmpform[0]=0;
	char *ptmpform=&tmpform;

	char *adr = strchr(requrl, (int)'?');
	if(adr == NULL){
		sprintf(pcanonical_request,"%s\n%s\n\n%s\n%s\n%s",
			method,
			requrl,
			pcanonical_headers,
			psigned_headers,
			payload_hash
		);
	}else{
		sprintf(ptmpform,"%s.%lds\n%s","%s\n%",(adr - requrl),"%s\n%s\n%s\n%s");
		sprintf(pcanonical_request,ptmpform,
			method,
			requrl,
			adr + 1,
			pcanonical_headers,
			psigned_headers,
			payload_hash
		);
	}
	
	
	////////////////
	//create string_to_sign
	len = strlen(amzdate)+ strlen(pcredential_scope)+ 33;
	char *pstring_to_sign = WS_Alloc(sp->ws,len);
	sprintf(pstring_to_sign,"AWS4-HMAC-SHA256\n%s\n%s\n%s",amzdate,pcredential_scope,vmod_hash_sha256(sp, pcanonical_request));
	
	////////////////
	//create signature
	char *signature = vmod_v4_getSignature(sp,secret_key,datestamp,region,service,pstring_to_sign);

	////////////////
	//create authorization
	len = strlen(access_key)+ strlen(pcredential_scope)+ strlen(psigned_headers)+ strlen(signature)+ 58;
	char *pauthorization= WS_Alloc(sp->ws,len);
	
	sprintf(pauthorization,"AWS4-HMAC-SHA256 Credential=%s/%s, SignedHeaders=%s, Signature=%s",
		access_key,
		pcredential_scope,
		psigned_headers,
		signature);
	
	////////////////
	//Set to header
	VRT_SetHdr(sp, where, "\016Authorization:"        , pauthorization , vrt_magic_string_end);
	VRT_SetHdr(sp, where, "\025x-amz-content-sha256:" , payload_hash , vrt_magic_string_end);
	VRT_SetHdr(sp, where, "\013x-amz-date:"           , amzdate , vrt_magic_string_end);

	

}

const char*
vmod_lf(struct sess *sp){
	return VRT_WrkString(sp,"\n",vrt_magic_string_end);
}
	
