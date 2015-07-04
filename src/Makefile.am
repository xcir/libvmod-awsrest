AM_CPPFLAGS = @VMOD_INCLUDES@ -Wall -Werror

vmoddir = @VMOD_DIR@
vmod_LTLIBRARIES = libvmod_awsrest.la

libvmod_awsrest_la_LDFLAGS = -module -export-dynamic -avoid-version -shared  -lmhash

libvmod_awsrest_la_SOURCES = \
	vmod_awsrest.c

nodist_libvmod_awsrest_la_SOURCES = \
	vcc_if.c \
	vcc_if.h

vmod_awsrest.lo: vcc_if.c vcc_if.h

vcc_if.c: vcc_if.h

vcc_if.h: @VMODTOOL@ $(top_srcdir)/src/vmod_awsrest.vcc
	@VMODTOOL@ $(top_srcdir)/src/vmod_awsrest.vcc

VMOD_TESTS = $(top_srcdir)/src/tests/*.vtc
.PHONY: $(VMOD_TESTS)

$(top_srcdir)/src/tests/*.vtc: libvmod_awsrest.la
	@VARNISHTEST@ -Dvarnishd=@VARNISHD@ -Dvmod_topbuild=$(abs_top_builddir) $@

check: $(VMOD_TESTS)

EXTRA_DIST = \
	vmod_awsrest.vcc \
	$(VMOD_TESTS)

CLEANFILES = \
	$(builddir)/vcc_if.c \
	$(builddir)/vcc_if.h \
	$(builddir)/vmod_awsrest.rst \
	$(builddir)/vmod_awsrest.man.rst
