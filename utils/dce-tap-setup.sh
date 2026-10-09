#!/bin/bash
#
# Connect DCE simulations to the real network through a tap device
# (dce-browser --tap=<device>). Run as root, once per boot:
#
#   sudo ./utils/dce-tap-setup.sh up   [user] [device]
#   sudo ./utils/dce-tap-setup.sh down [user] [device]
#
#   user:   owner of the tap device, who runs the simulation
#           (default: $SUDO_USER)
#   device: tap device name (default: tap-dce)
#
# "up" creates a persistent tap device owned by the user, so that ns-3's
# TapBridge (UseLocal mode) can attach to it without root, gives the host
# 10.99.0.1/24 on it, routes the simulated Wi-Fi network 10.1.1.0/24
# through the simulated AP (10.99.0.2), and lets the host forward and NAT
# both networks to the Internet: through firewalld when it is running
# (runtime settings only), otherwise with an nftables table of its own.
# "down" undoes all of it.

set -e

ACTION="${1:-up}"
OWNER="${2:-${SUDO_USER}}"
DEV="${3:-tap-dce}"
HOST_ADDR="10.99.0.1/24"
ROUTER="10.99.0.2"
SIM_NETS="10.1.1.0/24 10.99.0.0/24"
NFT_TABLE="dce_tap"

if [ "$(id -u)" != 0 ]; then
    echo "run as root: sudo $0 $*" >&2
    exit 1
fi

firewalld_running () {
    command -v firewall-cmd > /dev/null && firewall-cmd --state > /dev/null 2>&1
}

case "${ACTION}" in
up)
    if [ -z "${OWNER}" ]; then
        echo "give the user who runs the simulation: sudo $0 up <user>" >&2
        exit 1
    fi
    ip link show "${DEV}" > /dev/null 2>&1 || ip tuntap add dev "${DEV}" mode tap user "${OWNER}"
    ip addr replace "${HOST_ADDR}" dev "${DEV}"
    ip link set "${DEV}" up
    ip route replace 10.1.1.0/24 via "${ROUTER}" dev "${DEV}"
    sysctl -q -w net.ipv4.ip_forward=1
    if firewalld_running; then
        firewall-cmd -q --zone=trusted --add-interface="${DEV}"
        for net in ${SIM_NETS}; do
            firewall-cmd -q --zone=trusted --add-source="${net}" 2> /dev/null || true
        done
        firewall-cmd -q --add-masquerade
    else
        nft delete table ip "${NFT_TABLE}" 2> /dev/null || true
        nft -f - <<EOF
table ip ${NFT_TABLE} {
    chain forward {
        type filter hook forward priority filter; policy accept;
    }
    chain postrouting {
        type nat hook postrouting priority srcnat; policy accept;
        ip saddr { $(echo ${SIM_NETS} | sed 's/ /, /g') } oifname != "${DEV}" masquerade
    }
}
EOF
    fi
    echo "${DEV} is up for ${OWNER}: host ${HOST_ADDR}, simulated AP ${ROUTER}, Wi-Fi 10.1.1.0/24"
    ;;
down)
    if firewalld_running; then
        firewall-cmd -q --zone=trusted --remove-interface="${DEV}" 2> /dev/null || true
        for net in ${SIM_NETS}; do
            firewall-cmd -q --zone=trusted --remove-source="${net}" 2> /dev/null || true
        done
        firewall-cmd -q --remove-masquerade 2> /dev/null || true
    else
        nft delete table ip "${NFT_TABLE}" 2> /dev/null || true
    fi
    ip route del 10.1.1.0/24 via "${ROUTER}" 2> /dev/null || true
    ip link del "${DEV}" 2> /dev/null || true
    echo "${DEV} removed (net.ipv4.ip_forward left as it is)"
    ;;
*)
    echo "usage: sudo $0 up|down [user] [device]" >&2
    exit 1
    ;;
esac
