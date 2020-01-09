# Configure elasticsearch-7.5.1-linux-x86_64


## Requirement

running **three** es nodes in **one host**, and all of these are node.master=true and node.data=true.

## Auto-configuration script

**start.sh**

## Note

1. Required system settings
```
ulimit -n 65535
export JAVA_HOME=""
echo "262144" > /proc/sys/vm/max_map_count
# elasticsearch must run on no root user
useradd elasticsearch
```
2. The needed configurations
if you loss some key configurations, you may start the es cluster normally in first time. Buf you will fail to restart the es cluster in
next time.
these key configuration are:
- node.master=true (default)
- node.data=true (default)
- **cluster.initial_master_nodes=mnode,snode1.snode2** (Must be configured, and set these three node both are master eligible. the maximum tolerance)
- network.host=127.0.0.1 (replace it with your real ip address)
- discovery.seed_hosts= (see details in **start.sh**)
- http.port=9200 (default)
- tranport.port=9400 (default is 9300)
- path.logs=/tmp/es/log/ (replace it wiht your real folder)
- path.data=/tmp/es/data/ (replace it ...)
- node.name=mnode (we have three nodes in one host, and these's names are mnode snode1 and snode2)

see start.sh to understand the detail
