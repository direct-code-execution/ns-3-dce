#/bin/bash
nodes="1 2 4 8 16 32 63"
loaders="ns3::CoojaLoaderFactory ns3::DlmLoaderFactory ns3::CopyLoaderFactory"
ARGS="--ns3::TaskManager::FiberManagerType=UcontextFiberManager --LinuxPath=./libnet-next-2.6-pic.so"
PRGM="./build/optimized/src/process-manager/example/linear-udp-perf"
for node in ${nodes}; do
    for loader in ${loaders}; do
	echo -n "${node},${loader},";
        MEM=0;
        for i in 0 1 2 3 4 5 6 7 8 9; do
	    OUT=`${PRGM} ${ARGS} --Loader=${loader} --Size=${node} |cut -d ',' -f3,4|tail -1`;
	    US=`echo $OUT|sed -e 's/,.*//g'`;
	    MEM=`echo $OUT|sed -e 's/.*,//g'`;
	    echo -n "${US},";
	done;
	echo ${MEM};
    done;
done