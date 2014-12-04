# set overwrite_vconf by default
%bcond_without overwrite_vconf

%if %{with overwrite_vconf}
%define libname vconf
%define toolname vconftool
%else
%define libname vconf-buxton
%define toolname vconf-buxton-tool
%endif

Name:       vconf-buxton
Summary:    Configuration system library
Version:    0.1
Release:    1
Group:      System/Libraries
License:    Apache-2.0
Source0:    %{name}-%{version}.tar.gz
Source1001: 	vconf-buxton.manifest
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig
BuildRequires:  cmake
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(libbuxton)
BuildRequires:  pkgconfig(vconf-internal-keys)
Obsoletes: vconf

%description 
Configuration system library having vconf API and buxton backend

%package devel
Summary:    Vconf-buxton (devel)
Requires:   %{name} = %{version}-%{release}
Requires:   vconf-buxton = %{version}-%{release}
Requires:   vconf-buxton-keys-devel = %{version}-%{release}
Obsoletes:  vconf-devel

%description devel
Vconf library (devel)

%package keys-devel
Summary:    Vconf-buxton (devel)
Requires:   %{name} = %{version}-%{release}
Requires:   vconf-buxton = %{version}-%{release}
Requires:   vconf-internal-keys-devel
Obsoletes:  vconf-keys-devel

%description keys-devel
Vconf key management header files

%prep
%setup -q -n %{name}-%{version}
cp %{SOURCE1001} .

%build
%cmake -DLIBNAME:STRING=%{libname} -DTOOLNAME:STRING=%{toolname} .
make %{?jobs:-j%jobs}

%install
%make_install
mv %{buildroot}%{_unitdir}/vconf-buxton-setup.service %{buildroot}%{_unitdir}/%{libname}-setup.service
mkdir -p %{buildroot}%{_unitdir}/basic.target.wants
ln -sf ../%{libname}-setup.service %{buildroot}%{_unitdir}/basic.target.wants/

%post
/sbin/ldconfig

%postun
/sbin/ldconfig

%files
%manifest %{name}.manifest
%license LICENSE.APLv2
%{_bindir}/%{toolname}
%{_bindir}/vconf-buxton-init-from-vconf.sh
%{_bindir}/vconf-buxton-restore-mem-layer.sh
%{_bindir}/vconf-buxton-backup-mem-layer.sh
%{_libdir}/lib%{libname}.so.*
%{_unitdir}/basic.target.wants/%{libname}-setup.service
%{_unitdir}/%{libname}-setup.service

%files devel
%manifest %{name}.manifest
%{_includedir}/vconf/vconf-buxton.h
%{_libdir}/pkgconfig/%{libname}.pc
%{_libdir}/lib%{libname}.so

%files keys-devel
%manifest %{name}.manifest
%{_includedir}/vconf/vconf-buxton-keys.h

