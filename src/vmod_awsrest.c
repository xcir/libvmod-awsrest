#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* need vcl.h before vrt.h for vmod_evet_f typedef */
#include "cache/cache.h"
#include "vcl.h"

#ifndef VRT_H_INCLUDED
#include <vrt.h>
#endif

#ifndef VDEF_H_INCLUDED
#include <vdef.h>
#endif

#include "vtim.h"
#include "vcc_awsrest_if.h"

#include <time.h>
#include <string.h>
#include <stdio.h>
#include <mhash.h>

int
#if VRT_MAJOR_VERSION > 8U
  vmod_event_function(VRT_CTX, struct vmod_priv *priv, enum vcl_event_e e)
#else
  event_function(VRT_CTX, struct vmod_priv *priv, enum vcl_event_e e)
#endif
{
	return (0);
}

static int
compa(const void *a, const void *b)
{
	const char * const *pa = a;
	const char * const *pb = b;
	const char *a1, *b1;

	for (a1 = pa[0], b1 = pb[0]; a1 < pa[1] && b1 < pb[1]; a1++, b1++)
		if (*a1 != *b1)
			return (*a1 - *b1);
	return (0);
}

char * 
headersort(VRT_CTX, char *txt, char sep, char sfx)
{
	const char *cq, *cu;
	char *p, *r;
	const char **pp;
	const char **pe;
	unsigned u;
	int np;
	int i;

	CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);

	if (txt == NULL)
		return (NULL);

	/* Split :query from :url */
	cu = txt;

	/* Spot single-param queries */
	cq = strchr(cu, sep);
	if (cq == NULL)
		return (txt);

	r = WS_Copy(ctx->ws, txt, -1);
	if (r == NULL)
		return (txt);

	u = WS_ReserveLumps(ctx->ws, sizeof(const char **));
#if VRT_MAJOR_VERSION >= 12U
	pp = WS_Reservation(ctx->ws);
#else
	pp = (const char**)(void*)(ctx->ws->f);
#endif
	if (u < 4) {
		WS_Release(ctx->ws, 0);
		WS_MarkOverflow(ctx->ws);
		return (txt);
	}
	pe = pp + u;

	/* Collect params as pointer pairs */
	np = 0;
	pp[np++] =  cu;
	for (cq =  cu; *cq != '\0'; cq++) {
		if (*cq == sep) {
			if (pp + np + 3 > pe) {
				WS_Release(ctx->ws, 0);
				WS_MarkOverflow(ctx->ws);
				return (txt);
			}
			pp[np++] = cq;
			/* Skip trivially empty params */
			while (cq[1] == sep)
				cq++;
			pp[np++] = cq + 1;
		}
	}
	pp[np++] = cq;
	assert(!(np & 1));

	qsort(pp, np / 2, sizeof(*pp) * 2, compa);

	/* Emit sorted params */
	p =  r + (cu - txt);
	cq = "";
	for (i = 0; i < np; i += 2) {
		/* Ignore any edge-case zero length params */
		if (pp[i + 1] == pp[i])
			continue;
		assert(pp[i + 1] > pp[i]);
		if (*cq)
			*p++ = *cq;
		memcpy(p, pp[i], pp[i + 1] - pp[i]);
		p += pp[i + 1] - pp[i];
		cq = &sep;
	}
	if(sfx){
		*p = sfx;
		p++;
	}
	*p = '\0';

	WS_Release(ctx->ws, 0);
	return (r);
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
	#if VRT_MAJOR_VERSION > 9
	u = WS_ReserveAll(ctx->ws);
	#else
	u = WS_Reserve(ctx->ws, 0);
	#endif
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
	char amzdate[33];
	char datestamp[25];
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
	char *psh = WS_Alloc(ctx->ws,len);
	char *psigned_headers = WS_Alloc(ctx->ws,len);
	if(tokenlen > 0) {
		sprintf(psh,"%sx-amz-content-sha256;x-amz-date;x-amz-security-token",signed_headers);
	} else {
		sprintf(psh,"%sx-amz-content-sha256;x-amz-date",signed_headers);
	}
	psigned_headers = headersort(ctx, psh, ';', 0);
	////////////////
	//create canonical headers
	len = strlen(canonical_headers) + 115;
	// Account for addition of "x-amz-security-token:[token]\n"
	if(tokenlen > 0) len += 22 + tokenlen;
	char *pch = WS_Alloc(ctx->ws,len);
	char *pcanonical_headers = WS_Alloc(ctx->ws,len);

	if(tokenlen > 0) {
		sprintf(pch,"%sx-amz-content-sha256:%s\nx-amz-date:%s\nx-amz-security-token:%s\n",canonical_headers,payload_hash,amzdate,token);
	} else {
		sprintf(pch,"%sx-amz-content-sha256:%s\nx-amz-date:%s\n",canonical_headers,payload_hash,amzdate);
	}
	pcanonical_headers = headersort(ctx, pch, '\n', '\n');

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
	
#if VRT_MAJOR_VERSION >= 14U
        ////////////////
        //Set to header
        gs.what = "\016Authorization:";
        VRT_SetHdr(ctx, &gs        , pauthorization , NULL);
        gs.what = "\025x-amz-content-sha256:";
        VRT_SetHdr(ctx, &gs , payload_hash , NULL);
        gs.what = "\013x-amz-date:";
        VRT_SetHdr(ctx, &gs           , amzdate , NULL);
        if(tokenlen > 0){
          gs.what="\025x-amz-security-token:";
          VRT_SetHdr(ctx, &gs, token, NULL);
        }
#else
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
#endif
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
	const char *lst= url + strlen(url) -1;

	adr = strchr(url, (int)'?');
	
	if(adr == NULL){
		return url;
	}

	#if VRT_MAJOR_VERSION > 9
	u = WS_ReserveAll(ctx->ws);
	#else
	u = WS_Reserve(ctx->ws, 0);
	#endif
	pp = p = ctx->ws->f;

	len = adr - url;
	if(len + 1 > u){ // 1=(null)
		WS_Release(ctx->ws, 0);
		WS_MarkOverflow(ctx->ws);
		return url;
	}
	memcpy(p, url, len);
	p+=len;
	for(;lst >url;lst--){
		if(*lst != '?' && *lst != '&'){
			lst++;
			break;
		}
	}
	if(lst <= adr){
		// url: /xxxx? /?
		*p = 0;
		p++;
		WS_Release(ctx->ws, p - pp);
		return(pp);
	}
	
	while(1){
		ampadr = memchr(adr +1, (int)'&', lst - adr -1);
		if(ampadr == NULL){
			len = lst - adr;
			if(p - pp + len + 2 > u){ // 2= strlen("=")+1(null)
				WS_Release(ctx->ws, 0);
				WS_MarkOverflow(ctx->ws);
				return url;
			}
			memcpy(p, adr, len);
			p+=len;
			
			eqadr = memchr(adr +1, (int)'=', lst - adr -1);
			if(eqadr == NULL){
				cnt++;
				*p = '=';
				p++;
			}
			break;
		}else{
			eqadr = memchr(adr +1, (int)'=', ampadr - adr -1);
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
	*p = 0;
	p++;
	WS_Release(ctx->ws, p - pp);
	
	return(pp);
	
}
