Name: authbind
Version: 2.1.1
Release: 0.1
Summary: Allows non-root programs to bind() to low ports

Group: Net/Utils
License: GPL
Packager: Benjamin Sherman <benjamin@jivesoftware.com>
URL: https://launchpad.net/ubuntu/+source/authbind/2.1.1
Source: https://launchpad.net/ubuntu/+archive/primary/+files/authbind_2.1.1.tar.gz
BuildRoot:	%(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)

BuildRequires: gcc
%description
Allows non-root programs to bind() to low ports
This package allows a package to be started as non-root but
still bind to low ports, without any changes to the application.

http://en.wikipedia.org/wiki/Authbind


%prep 
%setup -n authbind-2.1.1
sed -i -e "s/^prefix=.*$/prefix=$\{DESTDIR\}\/usr/" Makefile
sed -i -e "s/^etc_dir=/etc_dir=$\{DESTDIR\}/" Makefile
make

%install
#make install
rm -rf %{buildroot}
mkdir -p %{buildroot}/usr/bin
mkdir -p %{buildroot}/etc/authbind
make install DESTDIR=%{buildroot}

%clean
rm -rf %{buildroot}

%post

%files
/etc/authbind
/usr/bin/authbind
/usr/lib/authbind/helper
/usr/lib/authbind/libauthbind.so.1
/usr/lib/authbind/libauthbind.so.1.0
