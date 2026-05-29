#!/bin/sh
set -eu

PACKAGE_NAME="platform-fans-hwmon"
PACKAGE_VERSION="0.2.0"
MODULE_NAME="platform_fans_hwmon"
SRC_DIR="/usr/src/${PACKAGE_NAME}-${PACKAGE_VERSION}"

if lsmod | awk '{print $1}' | grep -qx "$MODULE_NAME"; then
  modprobe -r "$MODULE_NAME"
fi

if command -v dkms >/dev/null 2>&1; then
  if dkms status -m "$PACKAGE_NAME" -v "$PACKAGE_VERSION" >/dev/null 2>&1; then
    dkms remove -m "$PACKAGE_NAME" -v "$PACKAGE_VERSION" --all
  fi
fi

rm -rf "$SRC_DIR"
depmod -a
