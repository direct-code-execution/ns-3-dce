#!/bin/bash
#
# Build the programs DCE's kernel-stack tests and examples run (the kernel
# itself is LKL, built by utils/build_lkl.sh):
#   - DCE-compatible (PIE) builds of ip, iperf, thttpd, wget, ping/ping6
#     and the quagga routing daemons
#   - the ns-3-dce-quagga module sources, patched for current ns-3
#
# Usage: ./utils/build_kernel_deps.sh [deps_dir]
#   deps_dir: output directory (default: ../dce-kernel-deps)
#   JOBS:     parallel make jobs (default: nproc)
#
# Outputs (sources and build trees stay in <deps_dir>/src, which can be
# deleted afterwards):
#   <deps_dir>/bin_dce/          binaries and libraries for DCE_PATH
#   <deps_dir>/ns-3-dce-quagga/  module to copy into myscripts/
#

set -e

DCE_DIR="$(cd "$(dirname "$0")/.." && pwd)"
DEPS_DIR="$(mkdir -p "${1:-${DCE_DIR}/../dce-kernel-deps}" && cd "${1:-${DCE_DIR}/../dce-kernel-deps}" && pwd)"
BIN_DCE="${DEPS_DIR}/bin_dce"
SRC="${DEPS_DIR}/src"
JOBS="${JOBS:-$(nproc)}"

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
