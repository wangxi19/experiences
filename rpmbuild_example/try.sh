#!/bin/bash
#echo -e "%_topdir  $(pwd)/rpmbuild/\r\n%buildroot  $(pwd)/rpmbuild/BUILDROOT/\r\n%prefix  myhello" > rpmmacro

if [ -d ~/rpmbuild ]; then rm -rf ~/rpmbuild/; fi;
mkdir -p ~/rpmbuild/{BUILDROOT,SOURCES,RPMS,SRPMS,SPECS,BUILD}

tar -cf ~/rpmbuild/SOURCES/hello.tar ./hello/

cp ./foo.spec ~/rpmbuild/SPECS/
cd ~/rpmbuild/SPECS/ && rpmbuild -bb ./foo.spec


myhello
