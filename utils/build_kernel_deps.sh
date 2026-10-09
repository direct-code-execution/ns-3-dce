#!/bin/bash
#
# Build the programs DCE's kernel-stack tests and examples run (the kernel
# itself is LKL, built by utils/build_lkl.sh):
#   - DCE-compatible (PIE) builds of ip, iperf, thttpd, wget, ping/ping6,
#     the quagga routing daemons, a minimal ffmpeg (dce-wifi-video) and,
#     when their development packages are installed, the dillo (FLTK) and
#     Northstar (GTK4, JavaScript) web browsers (dce-browser)
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
# 5.1 is the last ffmpeg whose command line tool works without threads.
FFMPEG_REV="n5.1.6"
DILLO_REV="v3.1.1"
CURL_VERSION="8.5.0"
NORTHSTAR_REPO="https://github.com/nordstjernen-web/northstar-browser.git"
# main, with Media Source Extensions video and audio streaming (PR #18)
NORTHSTAR_REV="488121e30687857eaa6ee22af0e8b14fc2030b7a"

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
    # A source tree from an earlier run must be at the same revision.
    if [ "$(git -C "${SRC}/$3" rev-parse HEAD)" != \
         "$(git -C "${SRC}/$3" rev-parse -q --verify "$2^{commit}")" ]; then
        echo "${SRC}/$3 is not at $2: delete it to fetch it again." >&2
        exit 1
    fi
}

fetch_tar () {
    # fetch_tar <url> <dir> <sha256>
    if [ ! -d "${SRC}/$2" ]; then
        wget -q -O "${SRC}/$2.tar" "$1"
        if ! echo "$3  ${SRC}/$2.tar" | sha256sum -c --quiet - > /dev/null 2>&1; then
            echo "Wrong checksum for $1" >&2
            rm -f "${SRC}/$2.tar"
            exit 1
        fi
        tar xf "${SRC}/$2.tar" -C "${SRC}"
        rm -f "${SRC}/$2.tar"
    fi
}

apply_patch () {
    # apply_patch <patch>, in the current directory: once.
    if patch -p1 -R -s -f --dry-run < "$1" > /dev/null 2>&1; then
        return # already applied
    fi
    patch -p1 -s < "$1"
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
fetch_tar "https://sourceforge.net/projects/iperf/files/iperf-${IPERF_VERSION}.tar.gz/download" "iperf-${IPERF_VERSION}" \
    636b4eff0431cea80667ea85a67ce4c68698760a9837e1e9d13096d20362265b
(
    cd "${SRC}/iperf-${IPERF_VERSION}"
    apply_patch "${DCE_DIR}/utils/iperf_4_dce.patch"
    ./configure -q CFLAGS="-g -fPIC ${LEGACY_CFLAGS}" CXXFLAGS="-g -fPIC -U_FORTIFY_SOURCE" \
        LDFLAGS="-pie -rdynamic" > /dev/null
    make -j"${JOBS}" > /dev/null
)
cp "${SRC}/iperf-${IPERF_VERSION}/src/iperf" "${BIN_DCE}/"

echo "== thttpd ${THTTPD_VERSION}"
fetch_tar "https://www.acme.com/software/thttpd/thttpd-${THTTPD_VERSION}.tar.gz" "thttpd-${THTTPD_VERSION}" \
    07719b08b1cff6a21c08697a7bcb4395425b07ee753106262fb62a03a7d32360
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
fetch_tar "https://ftp.gnu.org/gnu/wget/wget-${WGET_VERSION}.tar.gz" "wget-${WGET_VERSION}" \
    52126be8cf1bddd7536886e74c053ad7d0ed2aa89b4b630f76785bac21695fcd
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
    apply_patch "${DCE_DIR}/utils/iputils-ping6.patch"
    make CFLAGS="-fpic -D_GNU_SOURCE -g ${LEGACY_CFLAGS}" LDFLAGS="-pie -rdynamic" ping ping6 > /dev/null
)
cp "${SRC}/iputils/ping" "${SRC}/iputils/ping6" "${BIN_DCE}/"

echo "== ffmpeg ${FFMPEG_REV}"
fetch_git https://github.com/FFmpeg/FFmpeg.git "${FFMPEG_REV}" ffmpeg
(
    cd "${SRC}/ffmpeg"
    # Single threaded (DCE schedules one task at a time), no assembly, and
    # only what the dce-wifi-video example needs: file/UDP/RTP I/O, MPEG-TS
    # in and out, the parsers and decoders needed to probe the streams being
    # copied, and the xv (XVideo) and pulse output devices of --viewer=1 when
    # the X11 and PulseAudio headers are there. -fno-stack-protector/-U_FORTIFY_SOURCE: DCE
    # provides neither __stack_chk_fail nor the fortified libc entry points.
    XV_OPTIONS="--disable-avdevice"
    if [ -f /usr/include/X11/extensions/Xvlib.h ]; then
        XV_OPTIONS="--enable-avdevice --enable-xlib --enable-outdev=xv --enable-encoder=wrapped_avframe"
        # sound of --viewer=1, played on the host PulseAudio server through DCE
        if [ -f /usr/include/pulse/pulseaudio.h ]; then
            XV_OPTIONS="${XV_OPTIONS} --enable-libpulse --enable-outdev=pulse --enable-encoder=pcm_s16le --enable-filter=aresample,aformat"
        fi
    fi
    ./configure \
        --disable-everything --disable-autodetect --disable-doc \
        --disable-pthreads --disable-w32threads --disable-os2threads \
        --disable-asm --disable-stripping --disable-iconv \
        --disable-ffplay --disable-ffprobe --enable-ffmpeg \
        --disable-postproc ${XV_OPTIONS} \
        --enable-protocol=file,udp,rtp,tcp,pipe \
        --enable-demuxer=mpegts,rtp \
        --enable-muxer=mpegts,rtp,rtp_mpegts,null,mpeg1system \
        --enable-parser=h264,aac,mpegaudio,mpegvideo \
        --enable-decoder=h264,aac,mpeg2video,mp2,mp3 \
        --enable-encoder=mpeg1video,mp2 \
        --enable-filter=null,anull,scale,format,aresample,aformat \
        --enable-pic \
        --extra-cflags="-fPIC -g -U_FORTIFY_SOURCE -fno-stack-protector" \
        --extra-ldflags="-pie -rdynamic" > /dev/null
    # DCE has no aligned allocators and no sched_getaffinity: use plain
    # malloc() and sysconf() instead.
    sed -i -e 's/^#define HAVE_POSIX_MEMALIGN 1/#define HAVE_POSIX_MEMALIGN 0/' \
           -e 's/^#define HAVE_MEMALIGN 1/#define HAVE_MEMALIGN 0/' \
           -e 's/^#define HAVE_ALIGNED_MALLOC 1/#define HAVE_ALIGNED_MALLOC 0/' \
           -e 's/^#define HAVE_SCHED_GETAFFINITY 1/#define HAVE_SCHED_GETAFFINITY 0/' config.h
    make -j"${JOBS}" ffmpeg > /dev/null 2>&1
)
cp "${SRC}/ffmpeg/ffmpeg" "${BIN_DCE}/"
# The same binary runs natively: make the MPEG-1 program stream version of
# the video sample that Northstar's player decodes (dce-browser's video page).
"${BIN_DCE}/ffmpeg" -nostdin -hide_banner -loglevel error -y \
    -i "${DCE_DIR}/example/dce-wifi-video-sample.ts" -vf scale=320:180 -r 24 \
    -c:v mpeg1video -q:v 3 -g 24 -c:a mp2 -b:a 128k -ac 2 -ar 44100 \
    -f mpeg "${BIN_DCE}/video.mpg"

if [ -f /usr/include/FL/Fl.H ]; then
    echo "== dillo ${DILLO_REV}"
    fetch_git https://github.com/dillo-browser/dillo.git "${DILLO_REV}" dillo
    (
        cd "${SRC}/dillo"
        # Single process, single thread (no threaded DNS), no TLS: the browser
        # of the dce-dillo example, drawing on the host X display through
        # DCE's host socket passthrough.
        [ -f configure ] || ./autogen.sh > /dev/null 2>&1
        CFLAGS="-fPIC -g -O1 -U_FORTIFY_SOURCE -fno-stack-protector" \
        CXXFLAGS="-fPIC -g -O1 -U_FORTIFY_SOURCE -fno-stack-protector" \
        LDFLAGS="-pie -rdynamic" ./configure --disable-tls --disable-threaded-dns > /dev/null
        make -j"${JOBS}" > /dev/null 2>&1
    )
    cp "${SRC}/dillo/src/dillo" "${BIN_DCE}/"
fi

if [ -d /usr/include/gtk-4.0 ] && command -v meson > /dev/null && command -v ninja > /dev/null; then
    echo "== curl ${CURL_VERSION} (minimal static library for northstar)"
    fetch_tar "https://curl.se/download/curl-${CURL_VERSION}.tar.gz" "curl-${CURL_VERSION}" \
        05fc17ff25b793a437a0906e0484b82172a9f4de02be5ed447e0cab8c3475add
    (
        cd "${SRC}/curl-${CURL_VERSION}"
        # HTTP only, no TLS and none of the libraries Ubuntu's libcurl drags
        # in (krb5, ldap, ssh, gnutls...), which would each need DCE shims.
        CFLAGS="-fPIC -g -O1 -U_FORTIFY_SOURCE -fno-stack-protector" ./configure \
            --prefix="${SRC}/curl-install" --disable-shared --enable-static \
            --without-ssl --without-libpsl --without-zstd --without-brotli \
            --without-nghttp2 --without-libidn2 --without-librtmp --without-libssh \
            --without-libssh2 --disable-ldap --disable-ldaps --disable-rtsp --disable-dict \
            --disable-telnet --disable-tftp --disable-pop3 --disable-imap --disable-smb \
            --disable-smtp --disable-gopher --disable-mqtt --disable-ares \
            --disable-threaded-resolver --disable-unix-sockets --disable-ntlm \
            --disable-docs --disable-manual > /dev/null
        make -j"${JOBS}" > /dev/null 2>&1
        make install > /dev/null 2>&1
    )

    echo "== northstar (${NORTHSTAR_REV})"
    fetch_git "${NORTHSTAR_REPO}" "${NORTHSTAR_REV}" northstar
    (
        cd "${SRC}/northstar"
        # DCE finds the program's main() with dlsym(): export it (the project
        # localises every symbol of its executables).
        printf '{\n  global: main;\n  local: *;\n};\n' > src/exe-local.map
        # Link the browser as a shared object: DCE loads it like one, and the
        # linker would otherwise turn its thread local variables into fixed
        # %fs offsets (local-exec TLS) that point into the simulator's TLS.
        # No PIE (copy relocations of stdio variables), no LTO.
        sed -i "s/'b_pie=true'/'b_pie=false'/; s/'b_lto=true'/'b_lto=false'/" meson.build
        sed -i "s/pie: host_machine.system() != 'windows',/pie: false,/" src/gtk/meson.build
        # Keep symbols (gdb backtraces of the simulation name the browser's
        # functions). Sound: DCE threads are cooperative, so the SDL2 mixer
        # thread only runs between the browser's decode and paint bursts; a
        # 4096 frame (93 ms) device buffer outlasts them, 1024 underruns.
        sed -i "s/cc.get_supported_link_arguments(\['-Wl,-s'\]),/cc.get_supported_link_arguments([]),/" meson.build
        sed -i "s/want.samples = 1024;/want.samples = 4096;/" src/audio/audio.c
        grep -q "DCE: loaded like a shared object" src/meson.build || sed -i \
            "s|\(\['-Wl,--version-script=' + meson.current_source_dir() / 'exe-local.map'\]\))|\1) + ['-shared']  # DCE: loaded like a shared object|" src/meson.build
        # -fPIC and no LTO/-fPIE: a PIE compiled executable gets copy
        # relocations of stdin/stdout/stderr that DCE's per-process stdio
        # cannot reach. -ftls-model=global-dynamic: an executable's own
        # thread local variables use fixed %fs offsets, which point into the
        # simulator's TLS once DCE has loaded the program like a shared
        # object. Audio (SDL2 mixer, played on the host PulseAudio
        # server through DCE) when the SDL2 headers are there, no AVIF. The
        # browser is run with NS_NO_SANDBOX=1 --no-watchdog by dce-browser.
        AUDIO=disabled
        [ -f /usr/include/SDL2/SDL.h ] && AUDIO=enabled
        PKG_CONFIG_PATH="${SRC}/curl-install/lib/pkgconfig" meson setup build \
            -Daudio=${AUDIO} -Davif=disabled \
            -Dc_args="-fPIC -ftls-model=global-dynamic -mno-direct-extern-access -g -O1 -U_FORTIFY_SOURCE -fno-stack-protector" \
            -Dcpp_args="-fPIC -ftls-model=global-dynamic -mno-direct-extern-access -g -O1 -U_FORTIFY_SOURCE -fno-stack-protector" \
            -Dc_link_args="-rdynamic" -Dcpp_link_args="-rdynamic" > /dev/null
        ninja -C build > /dev/null 2>&1
    )
    cp "${SRC}/northstar/build/src/gtk/northstar" "${BIN_DCE}/"
fi

echo "== quagga ${QUAGGA_VERSION}"
fetch_tar "https://src.fedoraproject.org/repo/pkgs/quagga/quagga-${QUAGGA_VERSION}.tar.gz/64cc29394eb8a4e24649d19dac868f64/quagga-${QUAGGA_VERSION}.tar.gz" "quagga-${QUAGGA_VERSION}" \
    b7a98cc6b022bb0cb405557b3d920cf513150f64384dbd0a2248b5bd248df58b
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
    git apply --reverse --check "${DCE_DIR}/utils/ns-3-dce-quagga-ns3.patch" 2> /dev/null \
        || git apply "${DCE_DIR}/utils/ns-3-dce-quagga-ns3.patch"
)
rm -rf "${DEPS_DIR}/ns-3-dce-quagga"
cp -a "${SRC}/ns-3-dce-quagga" "${DEPS_DIR}/ns-3-dce-quagga"
rm -rf "${DEPS_DIR}/ns-3-dce-quagga/.git"

echo "Done. Contents of ${BIN_DCE}:"
ls "${BIN_DCE}"
