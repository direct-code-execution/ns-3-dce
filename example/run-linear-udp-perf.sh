
nodes="1 2 3 4 8 10 16 20"
DCE_RUNNER="./build/optimized/src/dce/utils/dce-runner"
PRGM="./build/optimized/src/dce/example/linear-udp-perf"
CONSTANTS="--TxBandwidth=1000000 --LinkBandwidth=2MBps --ns3::TaskManager::FiberManagerType=UcontextFiberManager --Duration=200s"
args_a="--Ns3App=true"
args_b="--Ns3App=false"
args_c="--Ns3App=false --LinuxPath=./libnet-next-2.6.so"
name_a="dce-none"
name_b="dce-user"
name_c="dce-user+kernel"
for node in ${nodes}; do
    for exp in a b c; do
	tmp=name_${exp}
	NAME=${!tmp}
	echo -n "${node},${NAME},";
        MEM=0;
        for i in 0 1 2 3 4 5 6 7 8 9; do
	    tmp=args_${exp}
	    ARGS=${!tmp}
	    OUT=`${DCE_RUNNER} ${PRGM} ${CONSTANTS} ${ARGS} --Size=${node} |cut -d ',' -f3,4|tail -1`;
	    US=`echo $OUT|sed -e 's/,.*//g'`;
	    MEM=`echo $OUT|sed -e 's/.*,//g'`;
	    echo -n "${US},";
	done;
	echo ${MEM};
    done;
done
