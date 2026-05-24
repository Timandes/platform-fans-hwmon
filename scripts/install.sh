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

for cmd in dkms make; do
  if ! command -v "$cmd" >/dev/null 2>&1; then
    echo "Missing required command: $cmd" >&2
    exit 1
  fi
done

if ! command -v gcc >/dev/null 2>&1 && ! command -v cc >/dev/null 2>&1; then
  echo "Missing compiler: install gcc or another C compiler" >&2
  exit 1
fi

PROJECT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)

rm -rf "$SRC_DIR"
mkdir -p "$SRC_DIR"
cp "$PROJECT_DIR"/intel_nuc_ec_hwmon.c "$SRC_DIR"/
cp "$PROJECT_DIR"/Makefile "$SRC_DIR"/
cp "$PROJECT_DIR"/dkms.conf "$SRC_DIR"/

if dkms status "$PACKAGE_NAME/$PACKAGE_VERSION" 2>/dev/null | grep -q "$PACKAGE_NAME"; then
  dkms remove -m "$PACKAGE_NAME" -v "$PACKAGE_VERSION" --all
fi

dkms add -m "$PACKAGE_NAME" -v "$PACKAGE_VERSION"
dkms build -m "$PACKAGE_NAME" -v "$PACKAGE_VERSION"
dkms install -m "$PACKAGE_NAME" -v "$PACKAGE_VERSION"
modprobe "$MODULE_NAME"
echo "Installed and loaded $MODULE_NAME"
