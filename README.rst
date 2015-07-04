===================
vmod_awsrest
===================

-------------------------------
Varnish AWS REST API module
-------------------------------

:Author: Shohei Tanaka(@xcir)
:Date: 2015-07-04
:Version: 0.3-varnish40
:Support Varnish Version: 4.0.x
:Manual section: 3

SYNOPSIS
========

import awsrest;

For Varnish3.0.x
=================

See this link.
https://github.com/xcir/libvmod-awsrest/tree/varnish30

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
                      "[Your Access Key]",
                      "[Your Secret Key]",
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
                  awsrest.v4_generic(
                      "s3",
                      "ap-northeast-1",
                      "[Your Access Key]",
                      "[Your Secret Key]",
                      "host;",
                      "host:" + bereq.http.host + awsrest.lf(),
                      false
                  );
                }
                //data
                //25 BereqHeader    b Authorization: AWS4-HMAC-SHA256 Credential=****************/20150704/ap-northeast-1/s3/aws4_request, SignedHeaders=host;x-amz-content-sha256;x-amz-date, Signature=****************
                //25 BereqHeader    b x-amz-content-sha256: e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855
                //25 BereqHeader    b x-amz-date: 20150704T103159Z



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

In your VCL you could then use this vmod along the following lines::

        import awsrest;

        sub vcl_deliver {
                # This sets resp.http.hello to "Hello, World"
                set resp.http.hello = awsrest.hello("World");
        }

COMMON PROBLEMS
===============

* configure: error: Need varnish.m4 -- see README.rst

  Check if ``PKG_CONFIG_PATH`` has been set correctly before calling
  ``autogen.sh`` and ``configure``
