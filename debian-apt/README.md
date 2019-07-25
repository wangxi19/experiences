# Add a new repository to apt's repositories

## Package entries website

https://packages.debian.org/

Searching package on the website firstlly, will see the searching result, then click the download link that suite your processor architecture on the page bottom, you will see the download page.
example, to search libboost-all-dev package, will get the following result:
   https://packages.debian.org/buster/amd64/libboost-all-dev/download


on the page, you will see the available repository url like below:

```
如果您正在运行 Debian，请尽量使用像 aptitude 或者 synaptic 一样的软件包管理器，代替人工手动操作的方式从这个网页下载并安装软件包。

您可以使用以下列表中的任何一个源镜像只要往您的 /etc/apt/sources.list 文件中像下面这样添加一行：

deb http://ftp.de.debian.org/debian buster main 
请使用最终确定的源镜像替换 ftp.de.debian.org/debian。
```
