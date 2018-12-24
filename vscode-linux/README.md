# Installation vscode in linux debian

- Downloading code_1.30.1-1545156774_amd64.deb

- dpkg *.deb
  
  `dpkg -i code_1.30.1-1545156774_amd64.deb`

- dependency error

  dpkg: dependency problems prevent configuration of code:
 code depends on libgconf-2-4; however:
  Package libgconf-2-4 is not installed.


## resolve

  after got those error, run `$ apt-get install -f`, will be installing those dependencies automatically 
