#!/bin/sh -ex
# See dce-user-kernel.rst for more information
# You should run this from the mptcp repository on tag 0.89
git remote add dce git://github.com/thehajime/net-next-sim.git || echo "ignore"
git fetch dce
git merge dce/sim-ns3-3.14.0-branch --no-commit

cat >> arch/sim/defconfig <<END
CONFIG_MPTCP=y
CONFIG_MPTCP_PM_ADVANCED=y
CONFIG_MPTCP_FULLMESH=y
CONFIG_MPTCP_NDIFFPORTS=y
CONFIG_DEFAULT_FULLMESH=y
CONFIG_DEFAULT_MPTCP_PM="fullmesh"

CONFIG_TCP_CONG_COUPLED=y
CONFIG_TCP_CONG_OLIA=y

END

make clean ARCH=sim
make defconfig ARCH=sim
make library ARCH=sim COV=yes

# this will download dce & run tests
# make testbin -C arch/sim/test
