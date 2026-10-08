#!/bin/bash
#
# Build everything DCE needs to run its kernel-stack tests and examples:
#   - net-next-nuse (Linux 4.4 libos) -> liblinux.so, and the kernel tree
#     used by "waf configure --enable-kernel-stack"
#   - freebsd-sim -> libfreebsd.so
#   - DCE-compatible (PIE) builds of ip, iperf, thttpd, wget, ping/ping6
#     and the quagga routing daemons
#   - the ns-3-dce-quagga module sources, patched for current ns-3
#
# Usage: ./utils/build_kernel_deps.sh [deps_dir]
#   deps_dir: output directory (default: ../dce-kernel-deps)
#   JOBS:     parallel make jobs (default: nproc)
#   LIBOS:    0 skips net-next-nuse and freebsd-sim, for builds whose Linux
#             stack is LKL (utils/build_lkl.sh) (default: 1)
#
# Outputs (sources and build trees stay in <deps_dir>/src, which can be
# deleted afterwards):
#   <deps_dir>/bin_dce/          binaries and libraries for DCE_PATH
#   <deps_dir>/kernel/           argument for --enable-kernel-stack (LIBOS=1)
#   <deps_dir>/ns-3-dce-quagga/  module to copy into myscripts/
#

set -e

DCE_DIR="$(cd "$(dirname "$0")/.." && pwd)"
DEPS_DIR="$(mkdir -p "${1:-${DCE_DIR}/../dce-kernel-deps}" && cd "${1:-${DCE_DIR}/../dce-kernel-deps}" && pwd)"
BIN_DCE="${DEPS_DIR}/bin_dce"
SRC="${DEPS_DIR}/src"
JOBS="${JOBS:-$(nproc)}"
LIBOS="${LIBOS:-1}"

NUSE_REV="libos-v4.4-fix1"
FREEBSD_REV="sim-ns3-dev-branch"
IPROUTE2_REV="v6.12.0" # matches LKL's kernel; has "ip mptcp"
IPUTILS_REV="s20101006"
QUAGGA_MODULE_REV="b57e0f3184e34107c7e452b3c41807e0aff5b5ce"
IPERF_VERSION="2.0.5"
THTTPD_VERSION="2.25b"
WGET_VERSION="1.15"
QUAGGA_VERSION="0.99.20"

# Old C code: keep building with GCC >= 10 (-fcommon) and GCC >= 14, which
# turned these warnings into errors. -U_FORTIFY_SOURCE: DCE does not provide
# the *_chk fortified libc entry points.
LEGACY_CFLAGS="-fcommon -U_FORTIFY_SOURCE \
 -Wno-error=implicit-function-declaration -Wno-error=implicit-int \
 -Wno-error=int-conversion -Wno-error=incompatible-pointer-types"

mkdir -p "${BIN_DCE}" "${SRC}"

fetch_git () {
    # fetch_git <url> <rev> <dir>
    # Shallow clone of a branch or tag; full clone only for a commit hash.
    if [ ! -d "${SRC}/$3" ]; then
        if ! git -c advice.detachedHead=false clone -q --depth 1 --branch "$2" "$1" "${SRC}/$3" 2> /dev/null; then
            git clone -q "$1" "${SRC}/$3"
            git -C "${SRC}/$3" -c advice.detachedHead=false checkout -q "$2"
        fi
    fi
}

fetch_tar () {
    # fetch_tar <url> <dir>
    if [ ! -d "${SRC}/$2" ]; then
        wget -q -O "${SRC}/$2.tar" "$1"
        tar xf "${SRC}/$2.tar" -C "${SRC}"
        rm -f "${SRC}/$2.tar"
    fi
}

if [ "${LIBOS}" = 1 ]; then
    echo "== net-next-nuse (Linux 4.4 libos)"
    fetch_git https://github.com/libos-nuse/net-next-nuse.git "${NUSE_REV}" net-next-nuse
    (
        cd "${SRC}/net-next-nuse"
        make defconfig ARCH=lib > /dev/null
        # Generate objs.mk serially: under -j, GNU Make >= 4.4 runs this rule
        # twice concurrently and the output file gets interleaved.
        make ARCH=lib arch/lib/objs.mk > /dev/null
        # The NUSE/rump tools fail to build and are not used by DCE; only the
        # libsim-linux library from arch/lib/tools is required.
        make library ARCH=lib -j"${JOBS}" -k > build.log 2>&1 || true
        test -f arch/lib/tools/libsim-linux-4.4.0.so || { tail -50 build.log; exit 1; }
    )
    cp "${SRC}/net-next-nuse/arch/lib/tools/libsim-linux-4.4.0.so" "${BIN_DCE}/"
    ln -sf libsim-linux-4.4.0.so "${BIN_DCE}/liblinux.so"
    # DCE only needs the libos API headers from the kernel tree.
    rm -rf "${DEPS_DIR}/kernel"
    mkdir -p "${DEPS_DIR}/kernel/lib"
    cp -a "${SRC}/net-next-nuse/arch/lib/include" "${DEPS_DIR}/kernel/lib/"

    echo "== freebsd-sim"
    fetch_git https://github.com/direct-code-execution/freebsd-sim.git "${FREEBSD_REV}" freebsd-sim
    (
        cd "${SRC}/freebsd-sim"
        git apply --check "${DCE_DIR}/utils/freebsd-sim-dce.patch" 2> /dev/null \
            && git apply "${DCE_DIR}/utils/freebsd-sim-dce.patch"
        cd sys/sim
        # The kernel config tool and generated headers must exist before the
        # parallel kernel build; the Makefile does not order this correctly.
        make -f Makefile.config clean > /dev/null
        make -f Makefile.config CC="gcc ${LEGACY_CFLAGS}" > /dev/null
        rm -rf compile && mkdir -p compile
        (cd conf && ../config -C DCE > /dev/null)
        (
            cd compile/DCE
            for m in device_if cpufreq_if bus_if linker_if clock_if; do
                awk -f ../../../tools/makeobjops.awk ../../../kern/$m.m -h
            done
            awk -f ../../../tools/vnode_if.awk ../../../kern/vnode_if.src -q -p -h -c
        )
        touch config
        make buildkernel -j"${JOBS}" CC="gcc ${LEGACY_CFLAGS}" > build.log 2>&1 || { tail -50 build.log; exit 1; }
    )
    cp "${SRC}/freebsd-sim/libsim-freebsd.git.so" "${BIN_DCE}/"
    ln -sf libsim-freebsd.git.so "${BIN_DCE}/libfreebsd.so"
fi

echo "== iproute2 ${IPROUTE2_REV}"
fetch_git https://git.kernel.org/pub/scm/network/iproute2/iproute2.git "${IPROUTE2_REV}" iproute2
(
    cd "${SRC}/iproute2"
    ./configure > /dev/null
    # Only ip, linked with libc only (configure finds none of the optional
    # libraries in a plain build environment).
    make -j"${JOBS}" SUBDIRS="lib ip" "CCOPTS=-fpic -D_GNU_SOURCE -O0 -g -U_FORTIFY_SOURCE" \
        "LDFLAGS=-pie -rdynamic" > build.log 2>&1 || { tail -50 build.log; exit 1; }
)
cp "${SRC}/iproute2/ip/ip" "${BIN_DCE}/"

echo "== iperf ${IPERF_VERSION}"
fetch_tar "https://sourceforge.net/projects/iperf/files/iperf-${IPERF_VERSION}.tar.gz/download" "iperf-${IPERF_VERSION}"
(
    cd "${SRC}/iperf-${IPERF_VERSION}"
    patch -p1 -N -s < "${DCE_DIR}/utils/iperf_4_dce.patch" || true
    ./configure -q CFLAGS="-g -fPIC ${LEGACY_CFLAGS}" CXXFLAGS="-g -fPIC -U_FORTIFY_SOURCE" \
        LDFLAGS="-pie -rdynamic" > /dev/null
    make -j"${JOBS}" > /dev/null
)
cp "${SRC}/iperf-${IPERF_VERSION}/src/iperf" "${BIN_DCE}/"

echo "== thttpd ${THTTPD_VERSION}"
fetch_tar "http://www.acme.com/software/thttpd/thttpd-${THTTPD_VERSION}.tar.gz" "thttpd-${THTTPD_VERSION}"
(
    cd "${SRC}/thttpd-${THTTPD_VERSION}"
    sed -i "s/rm conftest.c/rm -f conftests.c/" configure
    CC="gcc ${LEGACY_CFLAGS}" ./configure > /dev/null 2>&1
    # Same changes as utils/dce-thttpd.patch, which no longer applies to the
    # generated Makefile. No -fpie: it would make stdin/stdout/stderr copy
    # relocations, which DCE's per-process stdio cannot reach.
    sed -i -e "s/ -DHAVE_SIGSET=1//" \
        -e "s/^CFLAGS =\t\(.*\)/CFLAGS =\t\1 -fPIC -g ${LEGACY_CFLAGS}/" \
        -e "s/^LDFLAGS =.*/LDFLAGS =\t-pie -rdynamic/" Makefile
    make thttpd > /dev/null 2>&1
)
cp "${SRC}/thttpd-${THTTPD_VERSION}/thttpd" "${BIN_DCE}/"

echo "== wget ${WGET_VERSION}"
fetch_tar "https://ftp.gnu.org/gnu/wget/wget-${WGET_VERSION}.tar.gz" "wget-${WGET_VERSION}"
(
    cd "${SRC}/wget-${WGET_VERSION}"
    CFLAGS="-fPIC -g ${LEGACY_CFLAGS}" LDFLAGS="-pie -rdynamic" ./configure -q \
        --disable-opie --disable-digest --disable-ntlm --disable-largefile --disable-threads \
        --disable-nls --disable-rpath --disable-iri --without-ssl --without-zlib \
        --without-libiconv-prefix --without-libintl-prefix --without-libpth-prefix \
        --without-included-regex > /dev/null
    make -j"${JOBS}" > /dev/null 2>&1
)
cp "${SRC}/wget-${WGET_VERSION}/src/wget" "${BIN_DCE}/"

echo "== iputils ${IPUTILS_REV}"
fetch_git https://github.com/iputils/iputils.git "${IPUTILS_REV}" iputils
(
    cd "${SRC}/iputils"
    patch -p1 -N -s < "${DCE_DIR}/utils/iputils-ping6.patch" || true
    make CFLAGS="-fpic -D_GNU_SOURCE -g ${LEGACY_CFLAGS}" LDFLAGS="-pie -rdynamic" ping ping6 > /dev/null
)
cp "${SRC}/iputils/ping" "${SRC}/iputils/ping6" "${BIN_DCE}/"

echo "== quagga ${QUAGGA_VERSION}"
fetch_tar "https://src.fedoraproject.org/repo/pkgs/quagga/quagga-${QUAGGA_VERSION}.tar.gz/64cc29394eb8a4e24649d19dac868f64/quagga-${QUAGGA_VERSION}.tar.gz" "quagga-${QUAGGA_VERSION}"
(
    cd "${SRC}/quagga-${QUAGGA_VERSION}"
    CFLAGS="-fPIC -g ${LEGACY_CFLAGS}" LDFLAGS="-pie -rdynamic" ./configure -q \
        --disable-shared --enable-static --disable-user --disable-group \
        --disable-capabilities > /dev/null
    make -j"${JOBS}" > /dev/null 2>&1
)
for d in zebra ripd ripngd ospfd ospf6d bgpd; do
    cp "${SRC}/quagga-${QUAGGA_VERSION}/${d}/${d}" "${BIN_DCE}/"
done

echo "== ns-3-dce-quagga module"
fetch_git https://github.com/direct-code-execution/ns-3-dce-quagga.git "${QUAGGA_MODULE_REV}" ns-3-dce-quagga
(
    cd "${SRC}/ns-3-dce-quagga"
    git apply --check "${DCE_DIR}/utils/ns-3-dce-quagga-ns3.patch" 2> /dev/null \
        && git apply "${DCE_DIR}/utils/ns-3-dce-quagga-ns3.patch"
)
rm -rf "${DEPS_DIR}/ns-3-dce-quagga"
cp -a "${SRC}/ns-3-dce-quagga" "${DEPS_DIR}/ns-3-dce-quagga"
rm -rf "${DEPS_DIR}/ns-3-dce-quagga/.git"

echo "Done. Contents of ${BIN_DCE}:"
ls "${BIN_DCE}"
