#!/bin/bash
set -e

# DCE Build and Run Script
# Inspired by .github/workflows/build-and-test.yml

NS3_VER="3.49"
DCE_DIR=$(pwd)
BUILD_DIR="$DCE_DIR/build"
DEPS_DIR="$DCE_DIR/deps"
LKL_SRC_DIR="$DCE_DIR/lkl-src"
NS3_DIR="$DCE_DIR/../ns-$NS3_VER"
CONTAINER_NAME="ns3-dce-dev"

# Function to show usage
usage() {
    echo "Usage: $0 [command]"
    echo "Commands:"
    echo "  setup-container - Create a Distrobox container with all dependencies"
    echo "  clean           - Remove all build artifacts"
    echo "  build           - Build everything (NS-3, deps, LKL, DCE)"
    echo "  run <app> [args] - Run a DCE application"
    echo "  demo <type> [args] - Run a demonstration (vlc, northstar or chrome)"
    echo "                     vlc: --viewer for internal X11 window"
    echo "  test            - Run DCE tests"
    echo "  env             - Print environment variables to source"
    echo ""
    echo "Options:"
    echo "  --host          - (internal) Skip Distrobox re-entry (used automatically)"
    exit 1
}

if [ $# -lt 1 ]; then
    usage
fi

USE_DISTROBOX=true
INSIDE_CONTAINER=false

# Detect if we are already inside a container (distrobox or docker)
if [ -f /run/.containerenv ] || [ -f /.dockerenv ]; then
    INSIDE_CONTAINER=true
    USE_DISTROBOX=false
fi

# --host is an internal flag added when re-entering via distrobox; treat as inside-container
if [[ "$@" == *"--host"* ]]; then
    INSIDE_CONTAINER=true
    USE_DISTROBOX=false
    # Remove --host from arguments
    NEW_ARGS=()
    for arg in "$@"; do
        if [[ "$arg" != "--host" ]]; then
            NEW_ARGS+=("$arg")
        fi
    done
    set -- "${NEW_ARGS[@]}"
fi

# If distrobox is not available, we can't use it
if ! command -v distrobox >/dev/null 2>&1; then
    USE_DISTROBOX=false
fi

COMMAND=$1
shift

# If we should use distrobox, re-run ourselves inside it
if [ "$USE_DISTROBOX" = true ] && [ "$COMMAND" != "setup-container" ]; then
    if ! distrobox list | grep -q "$CONTAINER_NAME"; then
        echo "Container $CONTAINER_NAME not found. Please run '$0 setup-container' first."
        exit 1
    fi
    
    # Check if we need to setup tap-dce on the host before entering
    if [ "$COMMAND" == "demo" ]; then
        if ! ip link show tap-dce > /dev/null 2>&1; then
            echo "--- Setting up tap-dce on host (requires sudo) ---"
            echo "Please enter your password to configure the network tap device:"
            sudo -v || { echo "ERROR: sudo authentication failed. Cannot set up tap device."; exit 1; }
            sudo ./utils/dce-tap-setup.sh up "$(whoami)" tap-dce
        fi
    fi

    echo "--- Running $COMMAND inside distrobox $CONTAINER_NAME ---"
    
    # Pass through X11 and PulseAudio environment variables
    distrobox enter "$CONTAINER_NAME" -- \
        env DISPLAY="$DISPLAY" \
            XAUTHORITY="$XAUTHORITY" \
            PULSE_SERVER="$PULSE_SERVER" \
            PULSE_COOKIE="$PULSE_COOKIE" \
            XDG_RUNTIME_DIR="$XDG_RUNTIME_DIR" \
            DBUS_SESSION_BUS_ADDRESS="$DBUS_SESSION_BUS_ADDRESS" \
            HOME="$HOME" \
        "$DCE_DIR/build-and-run.sh" "$COMMAND" "$@" --host
    exit $?
fi

setup_env() {
    export LD_LIBRARY_PATH="$BUILD_DIR/lib:$BUILD_DIR/lib64:$LD_LIBRARY_PATH"
    export DCE_PATH="$BUILD_DIR/bin_dce:$BUILD_DIR/sbin"
    # Also include deps in DCE_PATH if they are there
    if [ -d "$DEPS_DIR/bin_dce" ]; then
        export DCE_PATH="$DCE_PATH:$DEPS_DIR/bin_dce"
    fi
}

do_setup_container() {
    echo "=== Creating Distrobox Container: $CONTAINER_NAME ==="
    if distrobox list | grep -q "$CONTAINER_NAME"; then
        echo "Container $CONTAINER_NAME already exists."
    else
        distrobox create -n "$CONTAINER_NAME" -i ubuntu:24.04 --yes
    fi

    echo "=== Installing dependencies inside container ==="
    distrobox enter "$CONTAINER_NAME" -- sudo apt-get update
    distrobox enter "$CONTAINER_NAME" -- sudo DEBIAN_FRONTEND=noninteractive apt-get install -y \
        git gcc g++ python3 python3-dev cmake ninja-build pkg-config libgsl-dev \
        libsqlite3-dev libxml2-dev wget make bc flex bison byacc gawk file xz-utils \
        libssl-dev libdb-dev libsctp-dev libx11-dev libxext-dev libxv-dev libpulse-dev \
        libsdl2-dev libfltk1.3-dev libpng-dev libjpeg-dev autoconf automake libtool \
        meson libgtk-4-dev libuchardet-dev libpsl-dev libseccomp-dev libharfbuzz-dev \
        libfribidi-dev libcairo2-dev libfontconfig-dev libfreetype-dev xvfb dbus \
        libdav1d-dev libvpx-dev libopenh264-dev libopus-dev sudo vlc
}

do_build() {
    echo "=== Building NS-3 $NS3_VER ==="
    if [ ! -d "$NS3_DIR" ]; then
        git clone --branch "ns-$NS3_VER" --depth 1 \
            https://gitlab.com/nsnam/ns-3-dev.git "$NS3_DIR"
    fi

    pushd "$NS3_DIR"
    ./ns3 configure \
        --disable-werror \
        --prefix="$BUILD_DIR" \
        --build-profile=optimized
    ./ns3 build
    ./ns3 install
    popd

    echo "=== Building Kernel Stack Dependencies ==="
    mkdir -p "$DEPS_DIR"
    CFLAGS="-O3 -g -fPIC" CXXFLAGS="-O3 -g -fPIC" ./utils/build_kernel_deps.sh "$DEPS_DIR"
    # Clean up src to save space, like CI does
    rm -rf "$DEPS_DIR/src"

    echo "=== Building LKL ==="
    KCFLAGS="-O3" ./utils/build_lkl.sh "$LKL_SRC_DIR" "$DEPS_DIR/lkl"
    rm -rf "$LKL_SRC_DIR"

    echo "=== Integrating Quagga Module ==="
    if [ -d "$DEPS_DIR/ns-3-dce-quagga" ]; then
        cp -a "$DEPS_DIR/ns-3-dce-quagga" "$DCE_DIR/myscripts/"
    fi

    echo "=== Configuring and Building DCE ==="
    CFLAGS="-O3 -g" CXXFLAGS="-O3 -g" ./waf configure \
        --with-ns3="$BUILD_DIR" \
        --prefix="$BUILD_DIR" \
        --with-lkl="$DEPS_DIR/lkl" \
        --enable-opt
    ./waf build

    echo "=== Finalizing DCE Binaries ==="
    mkdir -p "$BUILD_DIR/bin_dce"
    if [ -d "$DEPS_DIR/bin_dce" ]; then
        cp -a "$DEPS_DIR/bin_dce/." "$BUILD_DIR/bin_dce/"
    fi
    if [ -f "$DEPS_DIR/lkl/liblkl.so" ]; then
        cp "$DEPS_DIR/lkl/liblkl.so" "$BUILD_DIR/bin_dce/"
    fi
    # Only build bash if we are in the distrobox or have headers
    if [ -f /usr/include/termio.h ]; then
        ./utils/build_bash.sh "$BUILD_DIR/bin_dce"
    else
        echo "Skipping build_bash.sh (termio.h missing on host). Run inside setup-container for full build."
    fi

    echo "=== Verifying Key Binaries ==="
    for bin in ip iperf wget ffmpeg northstar; do
        if [ -f "$BUILD_DIR/bin_dce/$bin" ]; then
            echo "[OK] $bin found"
        else
            echo "[WARNING] $bin NOT found in $BUILD_DIR/bin_dce"
        fi
    done

    echo "=== Build Complete ==="
}

do_run() {
    setup_env
    APP=$1
    shift
    # If the app is in bin_dce, we might want to run it via a helper or directly if it's an ns-3 script
    ARGS="$*"
    if [ -f "$BUILD_DIR/bin/$APP" ] || [ -f "$DCE_DIR/build/bin/$APP" ]; then
        echo "Running ns-3 application $APP with args: $ARGS"
        ./waf --run "$APP $ARGS"
    elif [ -f "$BUILD_DIR/bin_dce/$APP" ]; then
        echo "Running DCE binary $APP directly (not recommended for ns-3 scenarios)"
        "$BUILD_DIR/bin_dce/$APP" "$@"
    else
        echo "Application $APP not found in build/bin or build/bin_dce. Trying waf --run anyway..."
        ./waf --run "$APP $ARGS"
    fi
}

do_demo() {
    TYPE=$1
    shift
    setup_env
    case $TYPE in
        vlc)
            echo "=== Running VLC over Wi-Fi Demo ==="
            echo "This demo uses ffmpeg to stream video over a simulated Wi-Fi network."
            echo "It can either play on a host player or via an X11 window inside DCE."
            # Ensure the video file exists where dce-wifi-video expects it
            if [ ! -f "$BUILD_DIR/bin_dce/video.ts" ]; then
                echo "Warning: video.ts not found in bin_dce. Build might be incomplete."
            fi
            
            FIRST_ARG="$1"
            if [ "$FIRST_ARG" == "--viewer" ]; then
                shift
                echo "Starting with internal X11 viewer..."
                ./waf --run "dce-wifi-video --viewer=1 $@"
            elif [ "$FIRST_ARG" == "--help" ]; then
                ./waf --run "dce-wifi-video --help"
            else
                echo "Starting with host VLC player..."
                # Only launch VLC if not just asking for help
                if [[ "$@" != *"--help"* ]]; then
                     ./waf --run "dce-wifi-video --player='vlc --play-and-exit - < {}' $@"
                else
                     ./waf --run "dce-wifi-video $@"
                fi
            fi
            ;;
        northstar)
            echo "=== Running Northstar Browser Demo ==="
            echo "This demo runs the Northstar web browser inside DCE, drawing to your X server."
            if [ ! -f "$BUILD_DIR/bin_dce/northstar" ]; then
                echo "Error: northstar binary not found. Make sure it was built (requires GTK4 headers)."
                exit 1
            fi

            # Default URL
            URL="http://10.1.1.1/browser/"
            
            # Collect arguments, separating the URL if provided
            REMAINING_ARGS=()
            HAS_HELP=false
            for arg in "$@"; do
                if [[ "$arg" == "http"* ]]; then
                    URL="$arg"
                elif [[ "$arg" == "--help" ]]; then
                    HAS_HELP=true
                elif [ "$arg" != "northstar" ]; then
                    REMAINING_ARGS+=("$arg")
                fi
            done

            # Early exit for help
            if [ "$HAS_HELP" = true ]; then
                 ./waf --run "dce-browser --help"
                 return
            fi

            # Use tap if the interface exists (set up on host before entering container)
            TAP_ARGS=""
            if ip link show tap-dce > /dev/null 2>&1; then
                TAP_ARGS="--tap=tap-dce"
            fi

            if [ -n "$DISPLAY" ]; then
                ./waf --run "dce-browser --browser=northstar --url=$URL $TAP_ARGS ${REMAINING_ARGS[*]}"
            else
                echo "DISPLAY not set, falling back to xvfb-run..."
                xvfb-run -a dbus-run-session -- ./waf --run "dce-browser --browser=northstar --url=$URL $TAP_ARGS ${REMAINING_ARGS[*]}"
            fi
            ;;
        chrome)
            echo "=== Running Chromium Demo ==="
            echo "This demo runs an unmodified Chromium build inside DCE, drawing to your X server."
            # A Chromium continuous build: unlike Chrome for Testing, it keeps
            # its symbols and the messages of its CHECKs.
            CHROMIUM_REV="1725601"
            CHROME_DIR="${CHROME_DIR:-$DEPS_DIR/chromium-$CHROMIUM_REV}"
            if [ ! -x "$CHROME_DIR/chrome" ]; then
                echo "Downloading Chromium r$CHROMIUM_REV..."
                mkdir -p "$CHROME_DIR.tmp"
                wget -q -O "$CHROME_DIR.tmp/chrome-linux.zip" \
                    "https://commondatastorage.googleapis.com/chromium-browser-snapshots/Linux_x64/$CHROMIUM_REV/chrome-linux.zip"
                (cd "$CHROME_DIR.tmp" && unzip -q chrome-linux.zip)
                mv "$CHROME_DIR.tmp/chrome-linux" "$CHROME_DIR"
                rm -rf "$CHROME_DIR.tmp"
            fi
            export DCE_PATH="$DCE_PATH:$CHROME_DIR"

            URL="http://10.1.1.1/browser/"
            REMAINING_ARGS=()
            for arg in "$@"; do
                if [[ "$arg" == "http"* ]]; then
                    URL="$arg"
                else
                    REMAINING_ARGS+=("$arg")
                fi
            done
            TAP_ARGS=""
            if ip link show tap-dce > /dev/null 2>&1; then
                TAP_ARGS="--tap=tap-dce"
            fi
            if [ -n "$DISPLAY" ]; then
                ./waf --run "dce-browser --browser=chrome --url=$URL $TAP_ARGS ${REMAINING_ARGS[*]}"
            else
                echo "DISPLAY not set, falling back to xvfb-run..."
                xvfb-run -a dbus-run-session -- ./waf --run "dce-browser --browser=chrome --url=$URL $TAP_ARGS ${REMAINING_ARGS[*]}"
            fi
            ;;
        *)
            echo "Unknown demo type: $TYPE. Available: vlc, northstar, chrome"
            exit 1
            ;;
    esac
}

do_test() {
    setup_env
    # We might need xvfb-run if graphical tests are involved
    if command -v xvfb-run >/dev/null 2>&1; then
         xvfb-run -a dbus-run-session -- python3 test.py -r
    else
         python3 test.py -r
    fi
}

do_clean() {
    echo "=== Cleaning all build artifacts ==="
    echo "Cleaning DCE build..."
    ./waf distclean 2>/dev/null || true
    echo "Cleaning NS-3 build..."
    if [ -d "$NS3_DIR" ]; then
        pushd "$NS3_DIR"
        ./ns3 clean 2>/dev/null || true
        popd
    fi
    echo "Removing build directory..."
    rm -rf "$BUILD_DIR"
    echo "Removing deps directory..."
    rm -rf "$DEPS_DIR"
    echo "Removing generated files..."
    rm -rf elf-cache files-* *.pcap exitprocs testpy-output
    rm -f .lock-waf_* build-*.log configure-*.log testpy-*.log
    echo "=== Clean Complete ==="
}

case $COMMAND in
    setup-container)
        do_setup_container
        ;;
    clean)
        do_clean
        ;;
    build)
        do_build
        ;;
    run)
        do_run "$@"
        ;;
    demo)
        do_demo "$@"
        ;;
    test)
        do_test
        ;;
    env)
        setup_env
        echo "export LD_LIBRARY_PATH=\"$LD_LIBRARY_PATH\""
        echo "export DCE_PATH=\"$DCE_PATH\""
        ;;
    *)
        usage
        ;;
esac
