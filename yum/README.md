## yum usage

- search package
`yum search 'pattern'`

- list installed packages
`yum list installed`

- display installed all files for installed package
`rpm -ql package`

- display a package all files
`repoquery -l package`

- centos7 update gcc version
install **devtoolset-7-gcc.x86_64**, need addition yum repository
