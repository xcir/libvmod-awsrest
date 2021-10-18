Summary: Varnish AWS REST API module
Name: vmod-awsrest
Version: 70.11
Release: 1%{?dist}
License: BSD
Group: System Environment/Daemons
Source0: libvmod-awsrest.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
Requires: varnish >= 5.0.0
Requires: mhash
BuildRequires: make
BuildRequires: python-docutils
BuildRequires: varnish >= 5.0.0
BuildRequires: mhash-devel

%description
Example VMOD

%prep
%setup -n libvmod-awsrest-%{version}

%build
./autogen.sh
%configure --prefix=/usr/
%{__make} %{?_smp_mflags}
%{__make} %{?_smp_mflags} check

%install
[ %{buildroot} != "/" ] && %{__rm} -rf %{buildroot}
%{__make} install DESTDIR=%{buildroot}

%clean
[ %{buildroot} != "/" ] && %{__rm} -rf %{buildroot}

%files
%defattr(-,root,root,-)
%{_libdir}/varnis*/vmods/
%doc /usr/share/doc/lib%{name}/*
%{_mandir}/man?/*

%changelog
* Mon Oct 18 2021 Shohei Tanaka <kokoniimasu@gmail.com> - 70.11
- Support Varnish7.0.x and 6.6.x

* Fri Jun 20 2018 Shohei Tanaka <kokoniimasu@gmail.com> - 60.9
- Support Varnish6.0.x

* Thu Apr 20 2017 Shohei Tanaka <kokoniimasu@gmail.com> - 51.7
- Drop support Varnish 4.1.x
- Sync newst vmod_example

* Sun Nov 20 2016 Shohei Tanaka <kokoniimasu@gmail.com> - 50.5
- Support Varnish4.1.x, 5.0.x

* Sat Jul 04 2015 Shohei Tanaka <kokoniimasu@gmail.com> - 0.4-0.20150704
- Support Varnish4.0.x
