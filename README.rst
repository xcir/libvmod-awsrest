.. image:: https://github.com/xcir/libvmod-awsrest/actions/workflows/test.yml/badge.svg?branch=master
    :target: https://github.com/xcir/libvmod-awsrest/actions/workflows/test.yml

===================
vmod_awsrest
===================

-------------------------------
Varnish AWS REST API module
-------------------------------

:Author: Shohei Tanaka(@xcir)
:Date: TBD
:Version: 70.12
:Support Varnish Version: 5.0.x ~ 7.0.x
:Manual section: 3

SYNOPSIS
========

import awsrest;

Versioning(Source)
====================
[varnish-version].[library-version]

65.1 is v1 for Varnish6.5.x

Versioning(Package)
====================
[VRT-version].[Source-version]

120.65.1 is 65.1 for VRT12.0

============ ===============
VRT Version  Varnish Version 
------------ ---------------
14.0         7.0.x
13.0         6.6.x
12.0         6.5.x
11.0         6.4.x
10.0         6.3.x
9.0          6.2.x
8.0          6.1.x
7.1          6.0.4~6.0.x
7.0          6.0.0~6.0.3
============ ===============

For Varnish3.0.x
=================

See this link.
https://github.com/xcir/libvmod-awsrest/tree/varnish30

For Varnish4.0.x
=================

See this link.
https://github.com/xcir/libvmod-awsrest/tree/varnish40

For Varnish4.1.x
=================

See this link.
https://github.com/xcir/libvmod-awsrest/tree/varnish41

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
                    BOOL   feature                // [false]                                   reserved param
                    )
Return value
	VOID
Description
	generate Authorization/x-amz-date/x-amz-content-sha256 header for AWS REST API.
Example(set to req.*)
        ::

                import awsrest;
                
                backend default {
                  .host = "example-bucket.s3.amazonaws.com";
                }
                
                sub vcl_recv{
                  set req.http.host = "example-bucket.s3.amazonaws.com";
                  awsrest.v4_generic(
                    service           = "s3",
                    region            = "ap-northeast-1",
                    access_key        = "[Your Access Key]",
                    secret_key        = "[Your Secret Key]",
                    signed_headers    = "host;",
                    canonical_headers = "host:" + req.http.host + awsrest.lf()
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
                  .host = "example-bucket.s3.amazonaws.com";
                }
                
                sub vcl_backend_fetch{
                  set bereq.http.host = "example-bucket.s3.amazonaws.com";
                  awsrest.v4_generic(
                    service           = "s3",
                    region            = "ap-northeast-1",
                    access_key        = "[Your Access Key]",
                    secret_key        = "[Your Secret Key]",
                    signed_headers    = "host;",
                    canonical_headers = "host:" + bereq.http.host + awsrest.lf()
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
                  .host = "example-bucket.s3.amazonaws.com";
                }
                
                sub vcl_backend_fetch{
                  set bereq.http.host = "example-bucket.s3.amazonaws.com";
                  awsrest.v4_generic(
                    service           = "s3",
                    region            = "ap-northeast-1",
                    access_key        = "[Your Access Key]",
                    secret_key        = "[Your Secret Key]",
                    token             = "[Your Session token]",
                    signed_headers    = "host;",
                    canonical_headers = "host:" + bereq.http.host + awsrest.lf()
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

$Function STRING formurl(STRING url)

Prototype
        ::

                formurl(url)
Return value
	STRING
Description
	Add "=" if field is not have value and delimiter.
	Strip "?","&" from the end of a string.
	AWS signature v4's query-string require sorted field(std.querysort) and field with delimiter(this function)
Example
        ::

                import awsrest;
                import std;
                
                sub vcl_recv{
                  set req.url = awsrest.formurl(std.querysort(req.url));
                }

                //log
                **** v1    0.5 vsl|       1001 ReqURL          c /?aa&cc&bb
                **** v1    0.5 vsl|       1001 ReqURL          c /?aa=&bb=&cc=

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
the appropriate path. For instance, when varnishd configure was called
with ``--prefix=$PREFIX``, use

::

 export PKG_CONFIG_PATH=${PREFIX}/lib/pkgconfig
 export ACLOCAL_PATH=${PREFIX}/share/aclocal

The module will inherit its prefix from Varnish, unless you specify a
different ``--prefix`` when running the ``configure`` script for this
module.

Make targets:

* make - builds the vmod.
* make install - installs your vmod.
* make check - runs the unit tests in ``src/tests/*.vtc``.
* make distcheck - run check and prepare a tarball of the vmod.

If you build a dist tarball, you don't need any of the autotools or
pkg-config. You can build the module simply by running::

 ./configure
 make

Installation directories
------------------------

By default, the vmod ``configure`` script installs the built vmod in the
directory relevant to the prefix. The vmod installation directory can be
overridden by passing the ``vmoddir`` variable to ``make install``.


Google Cloud Storage(GCS) sample
=================================

It can also be used in GCS.
        ::

                import awsrest;
                
                backend default {
                  .host = "example-bucket.storage.googleapis.com";
                }
                
                sub vcl_recv{
                  set req.http.host = "example-bucket.storage.googleapis.com";
                  awsrest.v4_generic(
                    service           = "storage",
                    region            = "asia-northeast1",
                    access_key        = "[Your Access Key]",
                    secret_key        = "[Your Secret Key]",
                    signed_headers    = "host;",
                    canonical_headers = "host:" + req.http.host + awsrest.lf()
                  );
                }


COMMON PROBLEMS
===============

* configure: error: Need varnish.m4 -- see README.rst

  Check if ``PKG_CONFIG_PATH`` has been set correctly before calling
  ``autogen.sh`` and ``configure``

* If you catch signature error in several request(URI-encoded)

  Please check that URI encoded.
  AWS signature v4 is require URI-encode. (ref: http://docs.aws.amazon.com/general/latest/gr/sigv4-create-canonical-request.html#d0e8062 )
  This VMOD does not automatically update update be/req.url.
  Because, can't detect URI-encoded or not.
  
  Sample(replace @ -> %40)::
  
   //////////////////////////
   //In cl-thread.

   sub vcl_recv{
     set req.url = regsuball(req.url,"@","%40");
     awsrest.v4_generic(
       service           = "s3",
       region            = "ap-northeast-1",
       access_key        = "[Your Access Key]",
       secret_key        = "[Your Secret Key]",
       signed_headers    = "host;",
       canonical_headers = "host:" + req.http.host + awsrest.lf()
     );
   }
   //////////////////////////
   //In bg-thread.

   sub vcl_backend_fetch {
     set bereq.url = regsuball(bereq.url,"@","%40");
     awsrest.v4_generic(
       service           = "s3",
       region            = "ap-northeast-1",
       access_key        = "[Your Access Key]",
       secret_key        = "[Your Secret Key]",
       signed_headers    = "host;",
       canonical_headers = "host:" + bereq.http.host + awsrest.lf()
     );
   }

* If a signature error occurs when using a query-string

  AWS signature v4's query-string require sorted field and field with delimiter.
  
  Failed url::
  
   /a?c=1&b=1
   /a?b&c
 
  Success url::

    /a?b=1&c=1
    /a?b=&c=
  
  Use std.querysort and awsrest.formurl to solve it.
  
  Sample::
  
   sub vcl_recv{
     set req.url = awsrest.formurl(std.querysort(req.url));
     awsrest.v4_generic(
       service           = "s3",
       region            = "ap-northeast-1",
       access_key        = "[Your Access Key]",
       secret_key        = "[Your Secret Key]",
       signed_headers    = "host;",
       canonical_headers = "host:" + req.http.host + awsrest.lf()
     );
   }
 

COPYRIGHT
=============

This document is licensed under the same license as the
libvmod-awsrest project. See LICENSE for details.

* Copyright (c) 2012-2021 Shohei Tanaka(@xcir)

File layout and configuration based on libvmod-example

* Copyright (c) 2011 Varnish Software AS
* https://github.com/varnishcache/libvmod-example/
* https://github.com/varnishcache/libvmod-example/blob/master/LICENSE

hmac-sha1 and base64 based on libvmod-digest

* Copyright (c) 2011-2019 Varnish Software AS
* https://github.com/varnish/libvmod-digest
* https://github.com/varnish/libvmod-digest/blob/master/src/vmod_digest.c

headersort based on libvmod-std/querysort

* Copyright (c) 2010-2014 Varnish Software AS
* https://github.com/varnishcache/varnish-cache
* https://github.com/varnishcache/varnish-cache/blob/master/vmod/vmod_std_querysort.c
