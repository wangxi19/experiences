Name:		foo
Version:	0.0.1
Release:	1%{?dist}
Summary:	test only

Group:		System
License:	GPL
URL:		www
#Source0:	

BuildRequires:	bash
BuildRequires:	make
BuildRequires:	gcc
#installation requires
Requires:	bash

%description
Test only

%prep
#%{?var_name}, if var_name doesn't exists, %{?var_name} is empty,
#if is %{var_name} and doesn't exits, then it is the %{var_name} string, rather than empty

#echo "topdir: %{_topdir}"
#echo "bindir: %{_bindir}"
#echo "buildroot: %{buildroot}"
#echo "smp: %{?_smp_mflags}"
#echo "dist: %{?dist}"
#echo "dist2: %{dist2}"

if [ -d %{_topdir}/SOURCES/hello/ ]; then rm -rf %{_topdir}/SOURCES/hello/; fi;
tar -xf %{_topdir}/SOURCES/hello.tar && mv ./hello %{_topdir}/SOURCES/
exit

%pre
#pre files
echo 'prepareing installation'
exit

%post
#post files
echo 'installation completed, enjoy it :)'
exit

#%setup -q

%build
cd %{_topdir}/SOURCES/hello/ && \
make PREFIX=/usr/local TARGET=myhello 
exit

%configure
#%{_smp_mflags} is -j4
make %{?_smp_mflags}
exit

%install
if [ ! -d $RPM_BUILD_ROOT/usr/local/bin/ ]; then mkdir -p $RPM_BUILD_ROOT/usr/local/bin/; fi;
cd %{_topdir}/SOURCES/hello/ && make install PREFIX=$RPM_BUILD_ROOT/usr/local TARGET=myhello
exit

%files
#in %{buildroot}, the entire dir structure is mirror from root path (/)
#the below list files, all the paths are relative path, in %{buildroot}
#the below files will be install to corresponding path on target computer
#example:
# /usr/local/bin/* mirror %{buildroot}/usr/local/bin/*, when pack rpm package
# /usr/local/bin/* mirror /usr/local/bin/*, when install rpm package
%attr(0777, root, root) /usr/local/bin/*

%postun
#post uninstall
echo "uninstall accomplished"

#%doc

%changelog
* Mon Apr 29 2019 feijian <baibufeijian@gmail.com>
 - comment

%clean
rm -rf %{_topdir}/SOURCES/hello/
if [ " %{_topdir}/BUILDROOT/ " != " / " ]; then rm -rf %{_topdir}/BUILDROOT/*; fi;
exit
