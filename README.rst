===================
vmod_awsrest
===================

-------------------------------
Varnish AWS REST API module
-------------------------------

:Author: Syohei Tanaka(@xcir)
:Date: 2013-01-14
:Version: 0.2
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

s3_generic
------------------

Prototype
        ::

                s3_generic(
                    STRING accesskey,
                    STRING secret,
                    STRING token,
                    STRING method,
                    STRING contentMD5,
                    STRING contentType,
                    STRING CanonicalizedResource,
                    TIME date)
Return value
	VOID
Description
	generate Authorization header for AWS REST API.(set to req.http.Date and req.http.Authorization)
Example
        ::

                import awsrest;
                
                backend default {
                  .host = "s3.amazonaws.com";
                  .port = "80";
                }
                
                sub vcl_recv{
                  awsrest.s3_generic(
                  "accessKey",            //AWSAccessKeyId
                  "secretKey",            //SecretAccessKeyID
                  "",                     //securityToken
                  req.request,            //HTTP-Verb
                  req.http.content-md5,   //Content-MD5
                  req.http.content-type,  //Content-Type
                  req.url,                //canonicalizedResource
                  now                     //Date
                  );
                }


                //data
                15 TxHeader     b Date: Tue, 03 Jul 2012 16:21:47 +0000
                15 TxHeader     b Authorization: AWS accessKey:XUfSbQDuOWL24PTR1qavWSr6vjM=

s3_generic_iam(EXPERIMENTAL)
------------------------------

Prototype
        ::

                s3_generic_iam(
                    STRING iamAddress,
                    STRING method,
                    STRING contentMD5,
                    STRING contentType,
                    STRING CanonicalizedResource,
                    TIME date)
Return value
	VOID
Description
	generate Authorization header for AWS REST API.(set to req.http.Date and req.http.Authorization)
Example
        ::

                import awsrest;
                
                backend default {
                  .host = "s3.amazonaws.com";
                  .port = "80";
                }
                
                sub vcl_recv{
                  awsrest.s3_generic_iam(
                  "http://169.254.169.254/latest/meta-data/iam/security-credentials/role-name",            //AWS-IAM URL see http://docs.amazonwebservices.com/AWSEC2/latest/UserGuide/UsingIAM.html#UsingIAMrolesWithAmazonEC2Instances
                  req.request,            //HTTP-Verb
                  req.http.content-md5,   //Content-MD5
                  req.http.content-type,  //Content-Type
                  req.url,                //canonicalizedResource
                  now                     //Date
                  );
                }


                //data
                15 TxHeader     b Date: Tue, 03 Jul 2012 16:21:47 +0000
                15 TxHeader     b Authorization: AWS accessKey:XUfSbQDuOWL24PTR1qavWSr6vjM=
                15 TxHeader     b x-amz-security-token: AQoDYXdzE...


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

COPYRIGHT
=============

This document is licensed under the same license as the
libvmod-rewrite project. See LICENSE for details.

* Copyright (c) 2012 Syohei Tanaka(@xcir)

File layout and configuration based on libvmod-example

* Copyright (c) 2011 Varnish Software AS

hmac-sha1 and base64 based on libvmod-digest( https://github.com/varnish/libvmod-digest )

main logic based on  http://www.applelife100.com/2012/06/23/using-rest-api-of-amazon-s3-in-php-1/

