===================
vmod_awsrest
===================

-------------------------------
Varnish AWS REST API module
-------------------------------

:Author: Shohei Tanaka(@xcir)
:Date: 2015-05-06
:Version: 0.3
:Manual section: 3

SYNOPSIS
===========

import awsrest;

DESCRIPTION
==============

ATTENTION
==============
Attention to 307 Temporary Redirect.

ref: http://docs.amazonwebservices.com/AmazonS3/latest/dev/Redirects.html

FUNCTIONS
============

v4_generic
------------------

Prototype
        ::

                v4_generic(
                    STRING service,               // [s3]
                    STRING region,                // [ap-northeast-1]
                    STRING access_key,            // [your access key]
                    STRING secret_key,            // [your secret key]
                    STRING signed_headers,        // [host;]                                   x-amz-content-sha256;x-amz-date is appended by default.
                    STRING canonical_headers,     // [host:s3-ap-northeast-1.amazonaws.com\n]
                    BOOL   feature                // [false]                                   reserved param(for varnish4)
                    )
Return value
	VOID
Description
	generate Authorization/x-amz-date/x-amz-content-sha256 header for AWS REST API.
Example(set to req.*)
        ::

                import awsrest;
                
                backend default {
                  .host = "s3-ap-northeast-1.amazonaws.com";
                }
                
                sub vcl_recv{
                  awsrest.v4_generic(
                      "s3",
                      "ap-northeast-1",
                      "accessKey",
                      "secretKey",
                      "host;",
                      "host:" + req.http.host + awsrest.lf(),
                      false
                  );
                }
                
                //data
                //13 TxHeader     b Authorization: AWS4-HMAC-SHA256 Credential=******/20150506/ap-northeast-1/s3/aws4_request, SignedHeaders=host;x-amz-content-sha256;x-amz-date, Signature=******
                //13 TxHeader     b x-amz-content-sha256: e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855
                //13 TxHeader     b x-amz-date: 20150506T092703Z
                
Example(set to bereq.*)
        ::

                import awsrest;
                
                backend default {
                  .host = "s3-ap-northeast-1.amazonaws.com";
                }
                
                sub vcl_miss{
                  awsrest.v4_generic(
                      "s3",
                      "ap-northeast-1",
                      "accessKey",
                      "secretKey",
                      "host;",
                      "host:" + bereq.http.host + awsrest.lf(),
                      false
                  );
                }
                //data
                //13 TxHeader     b Authorization: AWS4-HMAC-SHA256 Credential=******/20150506/ap-northeast-1/s3/aws4_request, SignedHeaders=host;x-amz-content-sha256;x-amz-date, Signature=******
                //13 TxHeader     b x-amz-content-sha256: e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855
                //13 TxHeader     b x-amz-date: 20150506T093008Z



lf
------------------

Prototype
        ::

                lf()
Return value
	STRING
Description
	return LF
Example
        ::

                "x-amz-hoge1:hoge" + awsrest.lf() + "x-amz-hoge2:hoge" + awsrest.lf()


                //data
                x-amz-hoge1:hoge
                x-amz-hoge2:hoge


INSTALLATION
==================

Installation requires Varnish source tree.

Usage::

 ./autogen.sh
 ./configure VARNISHSRC=DIR [VMODDIR=DIR]

`VARNISHSRC` is the directory of the Varnish source tree for which to
compile your vmod. Both the `VARNISHSRC` and `VARNISHSRC/include`
will be added to the include search paths for your module.

Optionally you can also set the vmod install directory by adding
`VMODDIR=DIR` (defaults to the pkg-config discovered directory from your
Varnish installation).

Make targets:

* make - builds the vmod
* make install - installs your vmod in `VMODDIR`
* make check - runs the unit tests in ``src/tests/*.vtc``


HISTORY
===========

Version 0.1: add s3_generic() , lf() method

Version 0.2: add s3_generic_iam() [pullreq #1 Thanks RevaxZnarf]

Version 0.3: Support V4 Signature.
             Delete method for v1 signature.

COPYRIGHT
=============

This document is licensed under the same license as the
libvmod-awsrest project. See LICENSE for details.

* Copyright (c) 2015 Shohei Tanaka(@xcir)

File layout and configuration based on libvmod-example

* Copyright (c) 2011 Varnish Software AS

hmac-sha1 and base64 based on libvmod-digest( https://github.com/varnish/libvmod-digest )

main logic based on  http://www.applelife100.com/2012/06/23/using-rest-api-of-amazon-s3-in-php-1/

