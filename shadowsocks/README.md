# 配置shadowsocks debian

## server debian
服务端使用python完成的版本, 试用过golang的版本, client连接的时候报错说不支持udp转发
```
  $ python --version
    Python 2.6.8
  $ pip install shadowsocks
```  
安装后, executable binary 文件 位于/usr/local/bin/ssserver下, 然后创建一个配置文件
```
{
    "server":"172.31.104.163",
    "server_port":18888,
    "local_port":11080,
    "local_address":"127.0.0.1",
    "password":"wangxi",
    "method": "aes-128-cfb",
    "timeout":600
}
```
位于/etc/shadowsocks/sss_conf, 然后 /usr/local/bin/ssserver -c /etc/shadowsocks/sss_conf, 启动server


## client windows debian
debian上推荐使用 shadowsocks-qt5
```
  $ apt-get install shadowsocks-qt5
```
安装后, 位于 /usr/bin/ss-qt5, 输入 `$ /usr/bin/ss-qt5` 启动client, 出现图形界面, 之后根据界面提示添加一个连接,
qt的client和许多命令行client一样有点奇怪, 它会在本地监听一个端口, 用于开启socks5 或 https的代理服务, 你用client连接到server, 成功后,
把你的浏览器proxy改为 你client在本地监听的地址,如(127.0.0.1:1080), 然后完成.
原理: 你浏览器访问网址, 把你的client(也就是ss-qt5)作为proxy, 你的client收到的任何incoming数据包都会转发往server, server再给你转发一次

**Note** go的client不支持httpsproxy, 只支持socks5, 也就是你如果用go的client, 也就是你用go的client你本地开启的proxy只支持socks5的代理(简而言之, 浏览器要使用又还需额外配置)
大多数命令行client也如此, 所以选择`shadowsocks-qt5`

windows 和 android上推荐使用outline, https://shadowsocks.org/en/download/clients.html, android 在google play store中, 免费的, windows的版本直接从前面连接去下载，
然后运行, 根据服务端的配置 去 https://shadowsocks.org/en/config/quick-guide.html 网址下生成链接地址(Try it yourself)

**Note** 完成后无需额外配置
