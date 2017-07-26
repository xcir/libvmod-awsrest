#include "config.h"

#include <stdio.h>
#include <stdlib.h>

/* need vcl.h before vrt.h for vmod_evet_f typedef */
#include "vcl.h"
#include "vrt.h"
#include "cache/cache.h"

#include "vtim.h"
#include "vcc_awsrest_if.h"

#include <time.h>
#include <string.h>
#include <stdio.h>
#include <mhash.h>


int __match_proto__(vmod_event_f)
event_function(VRT_CTX, struct vmod_priv *priv, enum vcl_event_e e)
{
	return (0);
}

/////////////////////////////////////////////
static const char *
vmod_hmac_sha256(VRT_CTX,
	const char *key,size_t lkey, const char *msg,size_t lmsg,bool raw)
{
	hashid hash = MHASH_SHA256;
	size_t blocksize = mhash_get_block_size(hash);

	char *p;
	char *ptmp;
	p    = WS_Alloc(ctx->ws, blocksize * 2 + 1);
	ptmp = p;

	
	unsigned char *mac;
	unsigned u;
	u = WS_Reserve(ctx->ws, 0);
	assert(u > blocksize);
	mac = (unsigned char*)ctx->ws->f;
	
	int i;
	MHASH td;

	assert(msg);
	assert(key);

	assert(mhash_get_hash_pblock(hash) > 0);

	td = mhash_hmac_init(hash, (void *) key, lkey,
		mhash_get_hash_pblock(hash));
	mhash(td, msg, lmsg);
	mhash_hmac_deinit(td,mac);
	if(raw){
		WS_Release(ctx->ws, blocksize);
		return (char *)mac;
	}
	WS_Release(ctx->ws, 0);
	
	for (i = 0; i<blocksize;i++) {
		sprintf(ptmp,"%.2x",mac[i]);
		ptmp+=2;
	}
	return p;
}

static const char *
vmod_v4_getSignature(VRT_CTX,
	const char* secret_key, const char* dateStamp, const char* regionName, const char* serviceName,const char* string_to_sign
){
	size_t len = strlen(secret_key) + 5;
	char key[len];
	char *kp = key;
	sprintf(kp,"AWS4%s",secret_key);
	
	const char *kDate    = vmod_hmac_sha256(ctx,kp,strlen(kp), dateStamp,strlen(dateStamp),true);
	const char *kRegion  = vmod_hmac_sha256(ctx,kDate,   32, regionName,strlen(regionName),true);
	const char *kService = vmod_hmac_sha256(ctx,kRegion, 32, serviceName,strlen(serviceName),true);
	const char *kSigning = vmod_hmac_sha256(ctx,kService,32, "aws4_request", 12,true);
	
	return vmod_hmac_sha256(ctx,kSigning,32, string_to_sign,strlen(string_to_sign),false);
}


static const char *
vmod_hash_sha256(VRT_CTX, const char *msg)
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
	p = WS_Alloc(ctx->ws,mhash_get_block_size(hash)*2 + 1);
	ptmp = p;
	for (i = 0; i<mhash_get_block_size(hash);i++) {
		sprintf(ptmp,"%.2x",h[i]);
		ptmp+=2;
	}
	return p;
}
void vmod_v4_generic(VRT_CTX,
	VCL_STRING service,               //= 's3';
	VCL_STRING region,                //= 'ap-northeast-1';
	VCL_STRING access_key,            //= 'your access key';
	VCL_STRING secret_key,            //= 'your secret key';
	VCL_STRING token,                 //= 'optional session token';
	VCL_STRING signed_headers,       //= 'host;';// x-amz-content-sha256;x-amz-date is appended by default.
	VCL_STRING canonical_headers,    //= 'host:s3-ap-northeast-1.amazonaws.com\n'
	VCL_BOOL feature                  //= reserved param(for varnish4)
){
	////////////////
	//get data
	const char *method;
	const char *requrl;
	struct http *hp;
	struct gethdr_s gs;
	
	if (ctx->http_bereq !=NULL && ctx->http_bereq->magic== HTTP_MAGIC){
		//bg-thread
		hp = ctx->http_bereq;
		gs.where = HDR_BEREQ;
	}else{
		//cl-thread
		hp = ctx->http_req;
		gs.where = HDR_REQ;
	}
	method= hp->hd[HTTP_HDR_METHOD].b;
	requrl= hp->hd[HTTP_HDR_URL].b;

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
	const char * payload_hash = vmod_hash_sha256(ctx, "");
	
	////////////////
	//create signed headers
	size_t tokenlen = 0;
	if(token != NULL) tokenlen = strlen(token);

	size_t len = strlen(signed_headers) + 32;
	if(tokenlen > 0) len += 21; // ;x-amz-security-token
	char *psigned_headers = WS_Alloc(ctx->ws,len);
	if(tokenlen > 0) {
		sprintf(psigned_headers,"%sx-amz-content-sha256;x-amz-date;x-amz-security-token",signed_headers);
	} else {
		sprintf(psigned_headers,"%sx-amz-content-sha256;x-amz-date",signed_headers);
	}
	
	////////////////
	//create canonical headers
	len = strlen(canonical_headers) + 115;
	// Account for addition of "x-amz-security-token:[token]\n"
	if(tokenlen > 0) len += 22 + tokenlen;
	char *pcanonical_headers = WS_Alloc(ctx->ws,len);
	if(tokenlen > 0) {
		sprintf(pcanonical_headers,"%sx-amz-content-sha256:%s\nx-amz-date:%s\nx-amz-security-token:%s\n",canonical_headers,payload_hash,amzdate,token);
	} else {
		sprintf(pcanonical_headers,"%sx-amz-content-sha256:%s\nx-amz-date:%s\n",canonical_headers,payload_hash,amzdate);
	}
	
	////////////////
	//create credential scope
	len = strlen(datestamp)+ strlen(region)+ strlen(service)+ 16;
	char *pcredential_scope = WS_Alloc(ctx->ws,len);
	sprintf(pcredential_scope,"%s/%s/%s/aws4_request",datestamp,region,service);
	
	////////////////
	//create canonical request
	len = strlen(method)+ strlen(requrl)+ strlen(pcanonical_headers)+ strlen(psigned_headers)+ strlen(payload_hash) + 6;
	char *pcanonical_request = WS_Alloc(ctx->ws,len);
	char tmpform[32];
	tmpform[0]=0;
	char *ptmpform = &tmpform[0];

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
	char *pstring_to_sign = WS_Alloc(ctx->ws,len);
	sprintf(pstring_to_sign,"AWS4-HMAC-SHA256\n%s\n%s\n%s",amzdate,pcredential_scope,vmod_hash_sha256(ctx, pcanonical_request));
	
	////////////////
	//create signature
	const char *signature = vmod_v4_getSignature(ctx,secret_key,datestamp,region,service,pstring_to_sign);

	////////////////
	//create authorization
	len = strlen(access_key)+ strlen(pcredential_scope)+ strlen(psigned_headers)+ strlen(signature)+ 58;
	char *pauthorization= WS_Alloc(ctx->ws,len);
	
	sprintf(pauthorization,"AWS4-HMAC-SHA256 Credential=%s/%s, SignedHeaders=%s, Signature=%s",
		access_key,
		pcredential_scope,
		psigned_headers,
		signature);
	
	////////////////
	//Set to header
	gs.what = "\016Authorization:";
	VRT_SetHdr(ctx, &gs        , pauthorization , vrt_magic_string_end);
	gs.what = "\025x-amz-content-sha256:";
	VRT_SetHdr(ctx, &gs , payload_hash , vrt_magic_string_end);
	gs.what = "\013x-amz-date:";
	VRT_SetHdr(ctx, &gs           , amzdate , vrt_magic_string_end);
	if(tokenlen > 0){
	  gs.what="\025x-amz-security-token:";
	  VRT_SetHdr(ctx, &gs, token, vrt_magic_string_end);
	}
}

VCL_STRING
vmod_lf(VRT_CTX){
	char *p;
	p = WS_Alloc(ctx->ws,2);
	strcpy(p,"\n");
	return p;
}


VCL_STRING
vmod_formurl(VRT_CTX, VCL_STRING url){
	char *adr, *ampadr, *eqadr;
	char *pp, *p;
	unsigned u;
	int len = 0;
	int cnt = 0;

	adr = strchr(url, (int)'?');
	
	if(adr == NULL){
		return url;
	}

	u = WS_Reserve(ctx->ws, 0);
	pp = p = ctx->ws->f;
	
	len = adr - url;
	if(len > u){
		WS_Release(ctx->ws, 0);
		WS_MarkOverflow(ctx->ws);
		return url;
	}
	memcpy(p, url, len);
	p+=len;

	while(1){
		ampadr = strchr(adr +1, (int)'&');
		if(ampadr == NULL){
			len = strlen(adr);
			if(p - pp + len + 2 > u){ // 2= strlen("=")+1(null)
				WS_Release(ctx->ws, 0);
				WS_MarkOverflow(ctx->ws);
				return url;
			}
			memcpy(p, adr, len);
			p+=len;
			eqadr = strchr(adr +1, (int)'=');
			if(eqadr == NULL){
				cnt++;
				*p = '=';
				p++;
			}
			break;
		}else{
			eqadr = memchr(adr +1, (int)'=', ampadr - adr);
			len = ampadr - adr;
			if(p - pp + len + 2 > u){
				WS_Release(ctx->ws, 0);
				WS_MarkOverflow(ctx->ws);
				return url;
			}
			memcpy(p, adr, len);
			p+=len;
			if(eqadr == NULL){
				cnt++;
				*p = '=';
				p++;
			}
			adr = ampadr;
		}
	}
	
	if(cnt == 0){
		WS_Release(ctx->ws, 0);
		return url;
	}

	*p = 0;
	p++;
	WS_Release(ctx->ws, p - pp);
	
	return(pp);
	
}
