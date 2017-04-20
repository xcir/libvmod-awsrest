AM_CFLAGS = $(VARNISHAPI_CFLAGS) -Wall -Werror
AM_LDFLAGS = $(VARNISHAPI_LIBS) $(VMOD_LDFLAGS) -lmhash

vmod_LTLIBRARIES = libvmod_awsrest.la

libvmod_awsrest_la_SOURCES = \
	vmod_awsrest.c

nodist_libvmod_awsrest_la_SOURCES = \
	vcc_awsrest_if.c \
	vcc_awsrest_if.h

dist_man_MANS = vmod_awsrest.3

@BUILD_VMOD_AWSREST@

AM_TESTS_ENVIRONMENT = \
	PATH="$(VMOD_TEST_PATH)" \
	LD_LIBRARY_PATH="$(VARNISH_LIBRARY_PATH)"
TEST_EXTENSIONS = .vtc
VTC_LOG_COMPILER = varnishtest -v
AM_VTC_LOG_FLAGS = -Dvmod_awsrest="$(VMOD_AWSREST)"

TESTS = @VMOD_TESTS@

EXTRA_DIST = \
	vmod_awsrest.vcc \
	$(VMOD_TESTS)
