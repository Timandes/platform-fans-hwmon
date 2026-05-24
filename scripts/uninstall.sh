#!/bin/sh
set -eu

PACKAGE_NAME="intel-nuc-ec-hwmon"
PACKAGE_VERSION="0.1.0"
SRC_DIR="/usr/src/${PACKAGE_NAME}-${PACKAGE_VERSION}"
MODULE_NAME="intel_nuc_ec_hwmon"

if [ "$(id -u)" -ne 0 ]; then
  echo "Please run as root: sudo $0" >&2
  exit 1
fi

if lsmod | awk '{print $1}' | grep -qx "$MODULE_NAME"; then
  modprobe -r "$MODULE_NAME"
fi

if command -v dkms >/dev/null 2>&1; then
  dkms remove -m "$PACKAGE_NAME" -v "$PACKAGE_VERSION" --all || true
fi

rm -rf "$SRC_DIR"
echo "Removed $MODULE_NAME"
