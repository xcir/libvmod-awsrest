===================
vmod_awsrest
===================

-------------------------------
Varnish AWS REST API module
-------------------------------

:Author: Shohei Tanaka(@xcir)
:Date: 2016-11-19
:Version: 50.5
:Support Varnish Version: 4.1.x, 5.0.x
:Manual section: 3

SYNOPSIS
========

import awsrest;

For Varnish3.0.x
=================

See this link.
https://github.com/xcir/libvmod-awsrest/tree/varnish30

For Varnish4.0.x
=================

See this link.
https://github.com/xcir/libvmod-awsrest/tree/varnish40

DESCRIPTION
===========

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
                    STRING session_token,         // [your session token (optional)]
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
                  set req.http.host = "s3-ap-northeast-1.amazonaws.com";
                  awsrest.v4_generic(
                      "s3",
                      "ap-northeast-1",
                      "[Your Access Key]",
                      "[Your Secret Key]",
                      "",
                      "host;",
                      "host:" + req.http.host + awsrest.lf(),
                      false
                  );
                }
                
                //data

                //2 ReqHeader      c Authorization: AWS4-HMAC-SHA256 Credential=****************/20150704/ap-northeast-1/s3/aws4_request, SignedHeaders=host;x-amz-content-sha256;x-amz-date, Signature=****************
                //2 ReqHeader      c x-amz-content-sha256: e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855
                //2 ReqHeader      c x-amz-date: 20150704T103402Z
                
Example(set to bereq.*)
        ::

                import awsrest;
                
                backend default {
                  .host = "s3-ap-northeast-1.amazonaws.com";
                }
                
                sub vcl_backend_fetch{
                  set bereq.http.host = "s3-ap-northeast-1.amazonaws.com";
                  awsrest.v4_generic(
                      "s3",
                      "ap-northeast-1",
                      "[Your Access Key]",
                      "[Your Secret Key]",
                      "",
                      "host;",
                      "host:" + bereq.http.host + awsrest.lf(),
                      false
                  );
                }
                //data
                //25 BereqHeader    b Authorization: AWS4-HMAC-SHA256 Credential=****************/20150704/ap-northeast-1/s3/aws4_request, SignedHeaders=host;x-amz-content-sha256;x-amz-date, Signature=****************
                //25 BereqHeader    b x-amz-content-sha256: e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855
                //25 BereqHeader    b x-amz-date: 20150704T103159Z

Example(using session token)
        ::

                import awsrest;
                
                backend default {
                  .host = "s3-ap-northeast-1.amazonaws.com";
                }
                
                sub vcl_backend_fetch{
                  set bereq.http.host = "s3-ap-northeast-1.amazonaws.com";
                  awsrest.v4_generic(
                      "s3",
                      "ap-northeast-1",
                      "[Your Access Key]",
                      "[Your Secret Key]",
                      "[Your Session Token]",
                      "host;",
                      "host:" + bereq.http.host + awsrest.lf(),
                      false
                  );
                }
                //data
                //25 BereqHeader    b Authorization: AWS4-HMAC-SHA256 Credential=****************/20150704/ap-northeast-1/s3/aws4_request, SignedHeaders=host;x-amz-content-sha256;x-amz-date, Signature=****************
                //25 BereqHeader    b x-amz-content-sha256: e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855
                //25 BereqHeader    b x-amz-date: 20150704T103159Z
                //25 BereqHeader    b x-amz-security-token: [Your Session Token]


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
============

The source tree is based on autotools to configure the building, and
does also have the necessary bits in place to do functional unit tests
using the ``varnishtest`` tool.

Building requires the Varnish header files and uses pkg-config to find
the necessary paths.

Usage::

 ./autogen.sh
 ./configure

If you have installed Varnish to a non-standard directory, call
``autogen.sh`` and ``configure`` with ``PKG_CONFIG_PATH`` pointing to
the appropriate path. For awsrest, when varnishd configure was called
with ``--prefix=$PREFIX``, use

 PKG_CONFIG_PATH=${PREFIX}/lib/pkgconfig
 export PKG_CONFIG_PATH

Make targets:

* make - builds the vmod.
* make install - installs your vmod.
* make check - runs the unit tests in ``src/tests/*.vtc``
* make distcheck - run check and prepare a tarball of the vmod.


COMMON PROBLEMS
===============

* configure: error: Need varnish.m4 -- see README.rst

  Check if ``PKG_CONFIG_PATH`` has been set correctly before calling
  ``autogen.sh`` and ``configure``

* If you catch signature error in several request

  Please check that URI encoded.
  AWS signature v4 is require URI-encode. (ref: http://docs.aws.amazon.com/general/latest/gr/sigv4-create-canonical-request.html#d0e8062 )
  This VMOD does not update be/req.url.
  Because, can't detect URI-encoded or not.
  
  Sample(replace @ -> %40)::
  
   //////////////////////////
   //In cl-thread.

   sub vcl_recv{
     set req.url = regsuball(req.url,"@","%40");
     awsrest.v4_generic(
       "s3",
       "ap-northeast-1",
       "[Your Access Key]",
       "[Your Secret Key]",
       "",
       "host;",
       "host:" + req.http.host + awsrest.lf(),
       false
     );
   }
   //////////////////////////
   //In bg-thread.

   sub vcl_backend_fetch {
     set bereq.url = regsuball(bereq.url,"@","%40");
     awsrest.v4_generic(
       "s3",
       "ap-northeast-1",
       "[Your Access Key]",
       "[Your Secret Key]",
       "",
       "host;",
       "host:" + bereq.http.host + awsrest.lf(),
       false
     );
   }



HISTORY
===========

Version 50.5: Support Varnish4.1.x / Varnish5.0.x [Thanks pullreq #12 poblahblahblah, issue #11 huayra]

Version 0.4-varnish40: Support Varnish4.0.x

Version 0.3-varnish30: Support V4 Signature. Delete method for v1 signature.

Version 0.2-varnish30: add s3_generic_iam() [pullreq #1 Thanks RevaxZnarf]

Version 0.1-varnish30: add s3_generic() , lf() method

COPYRIGHT
=============

This document is licensed under the same license as the
libvmod-awsrest project. See LICENSE for details.

* Copyright (c) 2016 Shohei Tanaka(@xcir)

File layout and configuration based on libvmod-example

* Copyright (c) 2011 Varnish Software AS

hmac-sha1 and base64 based on libvmod-digest( https://github.com/varnish/libvmod-digest )
