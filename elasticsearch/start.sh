#!/bin/bash
ESFolder=${PWD##*/}
cd ..
useradd elasticsearch
chown -R elasticsearch ./${ESFolder}
cd ./${ESFolder}

logdir=""
datadir=""
bindaddr="127.0.0.1"
bindport="9200"

read -p "log dir:" logdir
if [ "$logdir" == "" ]; then echo "Must set log dir" 1>&2; exit; fi;
if [ ! -d $logdir ]; then mkdir -p $logdir; fi;
chown elasticsearch $logdir;

read -p "data dir:" datadir
if [ "$datadir" == "" ]; then echo "Must set data dir" 1>&2; exit; fi;
if [ ! -d $datadir ]; then mkdir -p $datadir; fi;
chown elasticsearch $datadir;

read -p "bind address(default $bindaddr):" bindaddrtmp
if [ "$bindaddrtmp" != "" ]; then bindaddr=$bindaddrtmp; fi;

read -p "bind port(default ${bindport}):" bindporttmp
if [ "$bindporttmp" != "" ]; then bindport=$bindporttmp; fi;

#cp -f ./config/elasticsearch.yml ./config/elasticsearch.yml.back.`date +%s`
sed -Ei 's/\#(discovery\.seed_hosts\:.+)/\1/' ./config/elasticsearch.yml 
sed -Ei "s/(discovery\.seed_hosts\:).+/\1 [\"${bindaddr}\"]/" ./config/elasticsearch.yml

sed -Ei "s/\#(network\.host\:.+)/\1/" ./config/elasticsearch.yml
sed -Ei "s/(network\.host\:).+/\1 ${bindaddr}/" ./config/elasticsearch.yml

sed -Ei "s/\#(http\.port\:.+)/\1/" ./config/elasticsearch.yml
sed -Ei "s/(http\.port\:).+/\1 ${bindport}/" ./config/elasticsearch.yml

sed -Ei "s/\#(cluster\.initial_master_nodes\:.+)/\1/" ./config/elasticsearch.yml
sed -Ei "s/(cluster\.initial_master_nodes\:).+/\1 [\"mnode\"]/" ./config/elasticsearch.yml
sed -Ei "s/(cluster\.initial_master_nodes\:.+)/#\1/" ./config/elasticsearch.yml

for i in `ps -ef|grep -v grep|grep Elasticsearch|awk 'BEGIN{FS=" "}{print $2;}'`; do kill -9 $i; done;

ulimit -n 65535
export JAVA_HOME=""
echo "262144" > /proc/sys/vm/max_map_count

su -c "`pwd`/bin/elasticsearch -d -Enode.master=true -Enode.data=true -Ecluster.initial_master_nodes=mnode,snode1,snode2 -Enetwork.host=${bindaddr} -Ediscovery.seed_hosts=${bindaddr}:9401,127.0.0.1:9401,${bindaddr}:9402,127.0.0.1:9402 -Ehttp.port=${bindport} -Etransport.port=9400 -Epath.logs=${logdir}/log -Epath.data=${datadir}/data -Enode.name=mnode" elasticsearch 
su -c "`pwd`/bin/elasticsearch -d -Enode.master=true -Enode.data=true -Ecluster.initial_master_nodes=mnode,snode1,snode2 -Enetwork.host=${bindaddr} -Ediscovery.seed_hosts=${bindaddr}:9400,127.0.0.1:9400,${bindaddr}:9402,127.0.0.1:9402 -Ehttp.port=$((bindport+1)) -Etransport.port=9401 -Epath.logs=${logdir}/log2 -Epath.data=${datadir}/data2 -Enode.name=snode1" elasticsearch
su -c "`pwd`/bin/elasticsearch -d -Enode.master=true -Enode.data=true -Ecluster.initial_master_nodes=mnode,snode1,snode2 -Enetwork.host=${bindaddr} -Ediscovery.seed_hosts=${bindaddr}:9400,127.0.0.1:9400,${bindaddr}:9401,127.0.0.1:9401 -Ehttp.port=$((bindport+2)) -Etransport.port=9402 -Epath.logs=${logdir}/log3 -Epath.data=${datadir}/data3 -Enode.name=snode2" elasticsearch


echo "#!/bin/bash" > ./esdaemon.sh
echo "ulimit -n 65535" >> ./esdaemon.sh
echo 'export JAVA_HOME=""' >> ./esdaemon.sh
echo 'echo "262144" > /proc/sys/vm/max_map_count' >> ./esdaemon.sh
echo "StopAll() {" >> ./esdaemon.sh
echo "for i in \`ps -ef|grep -v grep|grep Elasticsearch|awk '{print \$2;}'\`; do kill \$i; done;" >> ./esdaemon.sh
echo "}" >> ./esdaemon.sh
echo "StartMNode() {" >> ./esdaemon.sh
echo "su -c \"`pwd`/bin/elasticsearch -d -Enode.master=true -Enode.data=true -Ecluster.initial_master_nodes=mnode,snode1,snode2 -Enetwork.host=${bindaddr} -Ediscovery.seed_hosts=${bindaddr}:9401,127.0.0.1:9401,${bindaddr}:9402,127.0.0.1:9402 -Etransport.port=9400 -Ehttp.port=${bindport} -Epath.logs=${logdir}/log -Epath.data=${datadir}/data -Enode.name=mnode\" elasticsearch" >> ./esdaemon.sh
echo "}" >> ./esdaemon.sh
echo "StartSNode1() {" >> ./esdaemon.sh
echo "su -c \"`pwd`/bin/elasticsearch -d -Enode.master=true -Enode.data=true -Ecluster.initial_master_nodes=mnode,snode1,snode2 -Enetwork.host=${bindaddr} -Ediscovery.seed_hosts=${bindaddr}:9400,127.0.0.1:9400,${bindaddr}:9402,127.0.0.1:9402 -Etransport.port=9401 -Ehttp.port=$((bindport+1)) -Epath.logs=${logdir}/log2 -Epath.data=${datadir}/data2 -Enode.name=snode1\" elasticsearch" >> ./esdaemon.sh
echo "}" >> ./esdaemon.sh
echo "StartSNode2() {" >> ./esdaemon.sh
echo "su -c \"`pwd`/bin/elasticsearch -d -Enode.master=true -Enode.data=true -Ecluster.initial_master_nodes=mnode,snode1,snode2 -Enetwork.host=${bindaddr} -Ediscovery.seed_hosts=${bindaddr}:9400,127.0.0.1:9400,${bindaddr}:9401,127.0.0.1:9401 -Etransport.port=9402 -Ehttp.port=$((bindport+2)) -Epath.logs=${logdir}/log3 -Epath.data=${datadir}/data3 -Enode.name=snode2\" elasticsearch" >> ./esdaemon.sh
echo "}" >> ./esdaemon.sh
echo 'SignalHandler() {' >> ./esdaemon.sh
echo 'StopAll' >> ./esdaemon.sh
echo 'trap - SIGTERM' >> ./esdaemon.sh
echo 'kill -- -$$' >> ./esdaemon.sh
echo '}' >> ./esdaemon.sh
echo 'trap SignalHandler SIGTERM' >> ./esdaemon.sh
echo "StopAll" >> ./esdaemon.sh
echo "StartMNode" >> ./esdaemon.sh
echo "StartSNode1" >> ./esdaemon.sh
echo "StartSNode2" >> ./esdaemon.sh
echo "" >> ./esdaemon.sh
echo '
while :
do
  sleep 5
  ps -ef|grep -v grep |grep Elasticsearch|grep "\-Enode.name=mnode" > /dev/null 2>&1
  mret="$?"
  ps -ef|grep -v grep |grep Elasticsearch|grep "\-Enode.name=snode1" > /dev/null 2>&1
  sret1="$?"
  ps -ef|grep -v grep |grep Elasticsearch|grep "\-Enode.name=snode2" > /dev/null 2>&1
  sret2="$?"
  if [[ "$mret" != "0" ]]; then StartMNode; fi;
  if [[ "$sret1" != "0" ]]; then StartSNode1; fi;
  if [[ "$sret2" != "0" ]]; then StartSNode2; fi;
done

exit 0
' >> ./esdaemon.sh
chmod +x ./esdaemon.sh

echo "
[Unit]
Description=Elasticsearch Service
After=network.target

[Service]
Type=simple
User=root
ExecStart=`pwd`/esdaemon.sh
Restart=always

[Install]
WantedBy=multi-user.target
" > /etc/systemd/system/elasticsearch.service
chmod +wx /etc/systemd/system/elasticsearch.service
systemctl daemon-reload
systemctl enable elasticsearch.service
systemctl start elasticsearch
