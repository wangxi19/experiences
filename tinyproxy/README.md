#tinyproxy 
今天想配置tinyproxy, 作为git的http proxy, 把一些代码下载下来, 结果在:
- 下载好源代码 tinyproxy-1.10.0
- ./configure
- ./make
- ./make install
- vim /usr/local/etc/tinyproxy/tinyproxy.conf, 修改了 `1. User root; 2. Group root; 3. Port 8889; 4. #Allow`
- /usr/local/bin/tinyproxy -c /usr/local/etc/tinyproxy/tinyproxy.conf -d

运行正常, 设置浏览器代理使用 ip:8889, 访问http://www.baidu.com, 正常, https://www.baidu.com 正常. 访问 https://www.google.com, 报错 **Connection was reset**
找了半天, 然后在client和server上都抓包看`tcpdump -i eth0 -w ~/tinyproxy.cap 'tcp port 8889'`, 发现client和server直接tcp连接正常建立, 当client 发出 `CONNECT www.google.com:443`(httpproxy规定的行为)后,
server会出现两种情况(目前测试发现两种情况): 
- 1. server收到client发出的`CONNECT www.google.com:443` 然后正常ack回复, 然后就收到**伪造的client**(我在我client上有抓包, 明确知道client没有发送tcp reset) 发送的 tcp reset请求,于是 server释放了这次连接
   这种情况下, client会发生和server相同的情况, client会收到**伪造的server的tcp reset 请求**(server上有抓包, 明确知道server没有reset这次连接)
   
- 2. server没收到client发出的`CONNECT www.google.com:443`, 收到了**伪造的client发送的错误ack回复**, 导致server误以为通信过程中出错, 于是发送tcp reset, client正常收到reset

一脸懵逼的client :)

世风日下...

第一种情况的图片:
![tinyproxy_1](https://github.com/wangxi19/traps/blob/master/images/tinyproxy_1.png)
