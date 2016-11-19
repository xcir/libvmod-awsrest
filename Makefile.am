ACLOCAL_AMFLAGS = -I m4 -I ${LIBVARNISHAPI_DATAROOTDIR}/aclocal

SUBDIRS = src

DISTCHECK_CONFIGURE_FLAGS = \
	VMOD_DIR='$${libdir}/varnish/vmods'

EXTRA_DIST = README.rst LICENSE vmod-awsrest.spec debian

doc_DATA = README.rst LICENSE

dist_man_MANS = vmod_awsrest.3
MAINTAINERCLEANFILES = $(dist_man_MANS)

vmod_awsrest.3: README.rst

%.1 %.2 %.3 %.4 %.5 %.6 %.7 %.8 %.9:
if HAVE_RST2MAN
	${RST2MAN} $< $@
else
	@echo "========================================"
	@echo "You need rst2man installed to make dist"
	@echo "========================================"
	@false
endif

# RPM packaging

RPMBUILD = CC=gcc PKG_CONFIG_PATH= rpmbuild -D '_rpmdir rpmbuild' \
	-D '_srcrpmdir rpmbuild/src'
MOCK = mock $(MOCK_OPTS) --resultdir mockbuild

dist: all

rpm: dist rpmbuild-clean
	$(RPMBUILD) -ts $(PACKAGE)-$(VERSION).tar.gz
	$(RPMBUILD) --rebuild rpmbuild/src/$(PACKAGE)-$(VERSION)-*.src.rpm

rpmlint: rpm
	rpmlint $(PACKAGE).spec rpmbuild/*/$(PACKAGE)-*.rpm

mockbuild: dist mockbuild-clean
	$(MOCK) --buildsrpm --sources . --spec $(PACKAGE).spec
	$(MOCK) --rebuild mockbuild/$(PACKAGE)-$(VERSION)-*.src.rpm

rpmbuild-clean:
	rm -rf rpmbuild/

mockbuild-clean:
	rm -rf mockbuild/

clean-local: rpmbuild-clean mockbuild-clean

.PHONY: rpmbuild-clean mockbuild-clean

# DEB packaging

DEB_ORIG = $(PACKAGE)_$(VERSION).orig.tar.gz
PDEBUILD = pdebuild --buildresult "$(abs_builddir)/pdebuild" $(PDEBUILD_OPTS)

$(DEB_ORIG): distdir debuild-clean
	tar -zcf $(DEB_ORIG) $(distdir)

deb: $(DEB_ORIG)
	cd $(distdir) && dpkg-buildpackage -rfakeroot -us -uc
	rm -rf $(distdir)

lintian: deb
	lintian $(PACKAGE)_$(VERSION)-1_*.deb

pdebuild: $(DEB_ORIG) pdebuild-clean
	mkdir pdebuild/
	cd $(distdir) && $(PDEBUILD)
	rm -rf $(distdir)

debuild-clean:
	rm -f $(PACKAGE)_$(VERSION)*

pdebuild-clean:
	rm -rf pdebuild/

clean-local: debuild-clean pdebuild-clean

.PHONY: debuild-clean pdebuild-clean
