#/bin/bash
nodes="1 2 3 4 8 10 16 20 32 50 63"
loaders="Cooja Dlm Copy"
ARGS="--TxBandwidth=1000000 --LinkBandwidth=2MBps --ns3::TaskManager::FiberManagerType=UcontextFiberManager --LinuxPath=./libnet-next-2.6-pic.so --Duration=200s"
PRGM="./build/optimized/src/dce/example/linear-udp-perf"
DCE_RUNNER="./build/optimized/src/dce/utils/dce-runner"
for node in ${nodes}; do
    for loader in ${loaders}; do
	echo -n "${node},${loader},";
        MEM=0;
        for i in 0 1 2 3 4 5 6 7 8 9; do
	    if test ${loader} == "Dlm"; then CMD="${DCE_RUNNER} ${PRGM}"; else CMD=${PRGM}; fi
	    OUT=`${CMD} ${ARGS} --ns3::DceManagerHelper::LoaderFactory=ns3::${loader}LoaderFactory[] --Size=${node} |cut -d ',' -f3,4|tail -1`;
	    US=`echo $OUT|sed -e 's/,.*//g'`;
	    MEM=`echo $OUT|sed -e 's/.*,//g'`;
	    echo -n "${US},";
	done;
	echo ${MEM};
    done;
done