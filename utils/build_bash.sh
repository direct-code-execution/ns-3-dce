#!/bin/bash
#
# Build Bash as a PIE binary for DCE.
# The resulting binary is placed in the build/bin_dce/ directory.
#
# Usage: ./utils/build_bash.sh [install_dir]
#   install_dir: directory to install the Bash binary (default: build/bin_dce)
#

set -e

ORIG_DIR="$(pwd)"
INSTALL_DIR="$(cd "$(dirname "${1:-build/bin_dce}")" && pwd)/$(basename "${1:-build/bin_dce}")"
BASH_VERSION="5.2.37"
BUILD_DIR="/tmp/build_bash_dce_$$"

echo "Building Bash ${BASH_VERSION} for DCE..."
echo "Install directory: ${INSTALL_DIR}"

mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

# Download Bash source
echo "Downloading Bash ${BASH_VERSION}..."
wget -q "https://ftp.gnu.org/gnu/bash/bash-${BASH_VERSION}.tar.gz"

# Extract
echo "Extracting Bash source..."
tar xf "bash-${BASH_VERSION}.tar.gz"
cd "bash-${BASH_VERSION}/"

# Configure and build with DCE-compatible flags
# -fPIC and -pie are required for DCE's dlopen-based loader
# -mcmodel=large is needed on x86_64 for DCE
echo "Configuring..."
if [ "$(uname -m)" = "x86_64" ]; then
    MCMODEL="-mcmodel=large"
else
    MCMODEL=""
fi

CFLAGS="-g -fPIC -fpie ${MCMODEL} -Wno-error=implicit-function-declaration" \
LDFLAGS="-pie -rdynamic" \
CFLAGS_FOR_BUILD="-g -fPIC -Wno-error=implicit-function-declaration" \
    ./configure --without-bash-malloc --quiet

echo "Building..."
make -j"$(nproc)" --quiet

# Install
mkdir -p "${INSTALL_DIR}"
cp "${BUILD_DIR}/bash-${BASH_VERSION}/bash" "${INSTALL_DIR}/bash"
echo "Installed Bash to ${INSTALL_DIR}/bash"

# Cleanup
rm -rf "${BUILD_DIR}"
echo "Done."
