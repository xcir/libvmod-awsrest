#include <stdio.h>
#include <stdlib.h>

#include "vrt.h"
#include "cache/cache.h"

#include "vcc_if.h"


#include <time.h>
#include <string.h>
#include <syslog.h>
#include <stdio.h>
#include <mhash.h>


int
init_function(struct vmod_priv *priv, const struct VCL_conf *conf)
{
	return (0);
}

/////////////////////////////////////////////
static const char *
vmod_hmac_sha256(const struct vrt_ctx *ctx,
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
vmod_v4_getSignature(const struct vrt_ctx *ctx,
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
vmod_hash_sha256(const struct vrt_ctx *ctx, const char *msg)
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

static inline int
param_compare(char *s, char *t)
{
    /* sort up to first =,&,\0 and past when keys equal */
    /* must special-case '=' to avoid ascii sorting it */
    for (; ((*s == *t) || *s == '=' || *t == '=' ||
                ((*s == '\0' || *s == '&') &&
                 (*t == '\0' || *t == '&'))
                ); s++, t++) {
        /* s and t end together */
        if ((*s == '\0' || *s == '&') && (*t == '\0' || *t == '&'))
            return 0;
        /* s ends, t continues */
        if (*s == '\0' || *s == '&')
            return -1;
        /* s continues, t ends */
        if (*t == '\0' || *t == '&')
            return 1;
        /* s param ends, t param continues */
        if (*s == '=' && *t != '=')
            return -1;
        /* s param continues, t param ends */
        if (*s != '=' && *t == '=')
            return 1;
    }
    return *s - *t;
}

static inline int
param_copy(char *dst, char *src)
{
    int len;
    char *val, *end;

    /* param */
    end = strpbrk(src, "&");
    if (end == NULL)
        end = src + strlen(src);

    val = strpbrk(src, "=");
    if (val == NULL || val > end)
        val = end;
    len = val - src;
    memcpy(dst, src, len);
    dst += len;

    /* '=' is required by v4 sig */
    *dst++ = '=';
    ++len;

    /* value */
    val = strpbrk(src, "=");
    if (val && val < end) {
        val++;
        memcpy(dst, val, end - val);
        len += end - val;
    }

    return len;
}

/*
 * sort query parameters according to AWS SIGv4 docs
 * http://docs.aws.amazon.com/general/latest/gr/sigv4-create-canonical-request.html
 */
const char *
vmod_awsv4_sort(const struct vrt_ctx *ctx, const char *url)
{
    char *qs = NULL;
    int qs_cnt = 0;
    char **params = NULL;
    char *param = NULL;
    int p_cnt = 0;
    int p;
    char *cp, *dst = NULL;

    if (url == NULL)
        return NULL;

    /* Pass 1 - Count query params */
    qs = strchr(url, '?');
    if (qs == NULL)
        return url;

    while ((qs = strchr(qs, '&')) != NULL) {
        ++qs;
        ++qs_cnt;
    }
    params = (char **)WS_Alloc(ctx->ws, (qs_cnt + 1) * sizeof(const char *));
    WS_Assert(ctx->ws);
    bzero(params, (qs_cnt + 1) * sizeof(const char *));

    /* Pass 2 - Sort query params */
    qs = strchr(url, '?');
    if (qs == NULL)
        return url;

    /* Add first param and increment */
    params[p_cnt++] = ++qs;
    while ((qs = strchr(qs, '&')) != NULL) {
        param = ++qs;

        for (p = 0; p < p_cnt; p++) {
            if (param_compare(param, params[p]) < 0) {
                for (int i = p_cnt; i > p; i--)
                    params[i] = params[i - 1];
                break;
            }
        }
        params[p] = param;
        p_cnt++;
    }

    /* Allocate and write sorted query params, resetting qs */
    dst = WS_Alloc(ctx->ws, (strlen(url) + p_cnt + 1) * sizeof(char));
    WS_Assert(ctx->ws);
    qs = strchr(url, '?');
    cp = memcpy(dst, url, (qs - url) + 1) + (qs - url + 1);

    for (p = 0; p < p_cnt; p++) {
        /* ignore empty params which sort early */
        if (params[p][0] == '\0' || params[p][0] == '&')
            continue;
        cp += param_copy(cp, params[p]);
        if (p < p_cnt - 1)
            *cp++ = '&';
        else
            *cp++ = '\0';
    }

    return dst;
}

/*
 * Callback to process request body for payload hashing
 */
static int __match_proto__(req_body_iter_f)
vcb_processbody(struct req *req, void *priv, void *ptr, size_t len)
{
    char **last = priv;
    char *prev = "";

    if (last == NULL)
        return 0;

    if (*last)
        prev = *last;

    *last = WS_Printf(req->ws, "%s%.*s", prev, (int)len, (const char *)ptr);
    return 0;
}

/*
 * Normalize URI paths according to RFC 3986 by removing redundant and relative
 * path components. Each path segment must be URI-encoded.
 */
static char hexchars[] = "0123456789ABCDEF";
#define visalpha(c) \
    ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
#define visalnum(c) \
    ((c >= '0' && c <= '9') || visalpha(c))
#define uri_unreserved(c) \
    (c == '-' || c == '.' || c == '_' || c == '~')
#define uri_path_delims(c) \
    (c == '/')

static const char *
vmod_encode(const struct vrt_ctx *ctx, const char *str, size_t enclen)
{
    const char *start = str;
    char *b, *e;
    unsigned u;

    CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);
    CHECK_OBJ_NOTNULL(ctx->ws, WS_MAGIC);
    u = WS_Reserve(ctx->ws, 0);
    e = b = ctx->ws->f;
    e += u;
    /* encode up to enclen */
    while (b < e && str && *str && str < start + enclen) {
        if (visalnum((int) *str) || uri_unreserved((int) *str)
                || uri_path_delims(*str)) {
            /* RFC3986 2.3 */
            *b++ = *str++;
        } else if (b + 4 >= e) { /* % hex hex NULL */
            b = e; /* not enough space */
        } else {
            *b++ = '%';
            unsigned char foo = *str;
            *b++ = hexchars[foo >> 4];
            *b++ = hexchars[*str & 15];
            str++;
        }
    }
    /* copy rest of url without encoding */
    while (b < e && str && *str)
        *b++ = *str++;
    if (b < e)
        *b = '\0';
    b++;
    if (b > e) {
        WS_Release(ctx->ws, 0);
        return (NULL);
    } else {
        e = b;
        b = ctx->ws->f;
        WS_Release(ctx->ws, e - b);
        return (b);
    }
}

void vmod_v4_generic(const struct vrt_ctx *ctx,
	VCL_STRING service,               //= 's3';
	VCL_STRING region,                //= 'ap-northeast-1';
	VCL_STRING access_key,            //= 'your access key';
	VCL_STRING secret_key,            //= 'your secret key';
	VCL_STRING _signed_headers,       //= 'host;';// x-amz-content-sha256;x-amz-date is appended by default.
	VCL_STRING _canonical_headers,    //= 'host:s3-ap-northeast-1.amazonaws.com\n'
	VCL_BOOL feature                  //= reserved param(for varnish4)
){

	
	////////////////
	//get data
	const char *method;
	const char *requrl, *origurl;
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
    origurl = hp->hd[HTTP_HDR_URL].b;

    /* request url must be escaped and query sorted */
    const char *encurl;
    char *qs = strchr(origurl, '?');
    size_t url_len;
    if (qs)
        url_len = qs - origurl;
    else
        url_len = strlen(origurl);
    encurl = vmod_encode(ctx, origurl, url_len);
    requrl = vmod_awsv4_sort(ctx, encurl);
    VSLb(ctx->vsl, SLT_Debug, "v4sig url: %s", requrl);

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

    /* hash POST data if present */
    char *payload = NULL;
    if (ctx->req) {
        /* must cache req body to allow hash and send to backend */
        VRT_CacheReqBody(ctx, 10*1024*1024);
        VRB_Iterate(ctx->req, vcb_processbody, &payload);
    }
    if (payload == NULL)
        payload = "";

	////////////////
	//create payload
	const char * payload_hash = vmod_hash_sha256(ctx, payload);
	
	////////////////
	//create signed headers
	size_t len = strlen(_signed_headers) + 32;
	char *psigned_headers = WS_Alloc(ctx->ws,len);
	sprintf(psigned_headers,"%sx-amz-content-sha256;x-amz-date",_signed_headers);
	
	
	////////////////
	//create canonical headers
	len = strlen(_canonical_headers) + 115;
	char *pcanonical_headers = WS_Alloc(ctx->ws,len);
	sprintf(pcanonical_headers,"%sx-amz-content-sha256:%s\nx-amz-date:%s\n",_canonical_headers,payload_hash,amzdate);
	
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
}

VCL_STRING
vmod_lf(const struct vrt_ctx *ctx){
	char *p;
	p = WS_Alloc(ctx->ws,2);
	strcpy(p,"\n");
	return p;
}
