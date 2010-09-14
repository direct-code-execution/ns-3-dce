
nodes="1 2 3 4 8 10 16 20"
PRGM=./build/optimized/src/process-manager/example/ring-udp-perf
CONSTANTS="--TxBandwidth=1000000 --LinkBandwidth=2MBps --ns3::TaskManager::FiberManagerType=UcontextFiberManager --Duration=200s"
args_exp1="--Ns3App=true"
args_exp2="--Ns3App=false"
args_exp3="--Ns3App=false --LinuxPath=./libnet-next-2.6.so"
for node in ${nodes}; do
    for exp in exp1 exp2 exp3; do
	echo -n "${node},${exp},";
        MEM=0;
        for i in 0 1 2 3 4 5 6 7 8 9; do
	    tmp=args_${exp}
	    ARGS=${!tmp}
	    OUT=`${PRGM} ${CONSTANTS} ${ARGS} --Size=${node} |cut -d ',' -f3,4|tail -1`;
	    US=`echo $OUT|sed -e 's/,.*//g'`;
	    MEM=`echo $OUT|sed -e 's/.*,//g'`;
	    echo -n "${US},";
	done;
	echo ${MEM};
    done;
done
