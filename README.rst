===================
vmod_awsrest
===================

-------------------------------
Varnish AWS REST API module
-------------------------------

:Author: Syohei Tanaka(@xcir)
:Date: 2012-07-04
:Version: 0.1
:Manual section: 3

SYNOPSIS
===========

import awsrest;

DESCRIPTION
==============


FUNCTIONS
============

s3_generic
------------------

Prototype
        ::

                s3_generic(
                    STRING accesskey,
                    STRING secret,
                    STRING method,
                    STRING contentMD5,
                    STRING contentType,
                    STRING CanonicalizedAmzHeaders,
                    STRING CanonicalizedResource,
                    TIME date)
Return value
	VOID
Description
	generate Authorization header for AWS REST API.(set to req.http.Date and req.http.Authorization)
Example
        ::

                import awsrest;
                
                sub vcl_recv{
                  awsrest.s3_generic(
                  "accessKey",            //AWSAccessKeyId
                  "secretKey",            //SecretAccessKeyID
                  req.request,            //HTTP-Verb
                  req.http.content-md5,   //Content-MD5
                  req.http.content-type,  //Content-Type
                  "",                     //canonicalizedAmzHeaders
                  "/",                    //canonicalizedResource
                  now                     //Date
                  );
                }


                //data
                15 TxHeader     b Date: Tue, 03 Jul 2012 16:21:47 +0000
                15 TxHeader     b Authorization: AWS accessKey:XUfSbQDuOWL24PTR1qavWSr6vjM=


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

Version 0.1: add s3_generic method

COPYRIGHT
=============

This document is licensed under the same license as the
libvmod-rewrite project. See LICENSE for details.

* Copyright (c) 2012 Syohei Tanaka(@xcir)

File layout and configuration based on libvmod-example

* Copyright (c) 2011 Varnish Software AS

hmac-sha1 and base64 based on libvmod-digest( https://github.com/varnish/libvmod-digest )

main logic based on  http://www.applelife100.com/2012/06/23/using-rest-api-of-amazon-s3-in-php-1/

