Summary: Varnish AWS REST API module
Name: vmod-awsrest
Version: 0.4
Release: 1%{?dist}
License: BSD
Group: System Environment/Daemons
Source0: libvmod-awsrest.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
Requires: varnish >= 4.0.2
BuildRequires: make
BuildRequires: python-docutils
BuildRequires: varnish >= 4.0.2
BuildRequires: varnish-libs-devel >= 4.0.2
BuildRequires: mhash-devel

%description
Example VMOD

%prep
%setup -n libvmod-awsrest-%{version}

%build
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
* Sat Jul 04 2015 Shohei Tanaka <kokoniimasu@gmail.com> - 0.4-0.20150704
- Support Varnish4.0.x
