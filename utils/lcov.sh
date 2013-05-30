#!/bin/bash
#set -x

TESTDIR=${TESTDIR:-.}
DIRS=`find ${TESTDIR} -name "files-*" | sort`
CMD_OPT=""
KERN_DIR="ns-3-linux"
KERN_DIR="net-next-sim"
APP_SRC_DIR=$1
DCE_DIR=`pwd`
HTML_DIR=lcov-report

cd ${APP_SRC_DIR}
APP_SRC_DIR=`pwd`
cd ${DCE_DIR}


for node in ${DIRS}
do

echo "Processing "${node}
GCDA=`find ${node} -name "*.gcda"|wc -l`
if [ ${GCDA} == "0" ] ; then
  echo "skipping"
  continue
fi

#gcov *.gcda
find $1 -name "*.gcno" | grep -v files- | cpio -pud ${node}/ |&grep -v blocks > /dev/null
OUT_NODE=`echo ${node} |sed "s/CUT\//CUT\_/g"`
./utils/lcov/lcov --rc lcov_branch_coverage=1 -q -c -d ${node}${APP_SRC_DIR} -b ${APP_SRC_DIR} -o dce-run-`basename ${OUT_NODE}`.info 
CMD_OPT="$CMD_OPT"" -a dce-run-`basename ${OUT_NODE}`.info"

done

./utils/lcov/lcov --rc lcov_branch_coverage=1 -q ${CMD_OPT} -o dce-run.info

if [ -n "$HTML_REPORT" ] ; then
 ./utils/lcov/genhtml --rc lcov_branch_coverage=1 -q dce-run.info -o lcov-report
fi

rm -f dce-run-*.info
#rm -f dce-run.info
