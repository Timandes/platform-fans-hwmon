#!/bin/sh
set -eu

PACKAGE_NAME="platform-fans-hwmon"
PACKAGE_VERSION="0.2.0"
MODULE_NAME="platform_fans_hwmon"
SUPPORTED_PLATFORM="intel-nuc-ec-v9"
PFH_PLATFORM_FILTER=""

usage() {
  echo "Usage: $0 [--platform intel-nuc-ec-v9]" >&2
}

while [ "$#" -gt 0 ]; do
  case "$1" in
    --platform)
      if [ "$#" -lt 2 ]; then
        usage
        exit 2
      fi
      PFH_PLATFORM_FILTER="$2"
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      usage
      exit 2
      ;;
  esac
done

if [ -n "$PFH_PLATFORM_FILTER" ] && [ "$PFH_PLATFORM_FILTER" != "$SUPPORTED_PLATFORM" ]; then
  echo "Unsupported platform filter: $PFH_PLATFORM_FILTER" >&2
  echo "Supported platform filter: $SUPPORTED_PLATFORM" >&2
  exit 2
fi

command -v dkms >/dev/null 2>&1 || {
  echo "dkms is required" >&2
  exit 1
}

PROJECT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
SRC_DIR="/usr/src/${PACKAGE_NAME}-${PACKAGE_VERSION}"

if /usr/sbin/modinfo "$MODULE_NAME" >/dev/null 2>&1 || modinfo "$MODULE_NAME" >/dev/null 2>&1; then
  echo "Module $MODULE_NAME already exists; continuing with DKMS reinstall" >&2
fi

if dkms status -m "$PACKAGE_NAME" -v "$PACKAGE_VERSION" >/dev/null 2>&1; then
  dkms remove -m "$PACKAGE_NAME" -v "$PACKAGE_VERSION" --all || true
fi

rm -rf "$SRC_DIR"
mkdir -p "$SRC_DIR"
mkdir -p "$SRC_DIR/core" "$SRC_DIR/backends" "$SRC_DIR/platforms/intel"

cp "$PROJECT_DIR"/Makefile "$SRC_DIR"/
cp "$PROJECT_DIR"/dkms.conf "$SRC_DIR"/
cp "$PROJECT_DIR"/core/pfh-core.c "$SRC_DIR"/core/
cp "$PROJECT_DIR"/core/pfh-core.h "$SRC_DIR"/core/
cp "$PROJECT_DIR"/backends/pfh-ec-mmio.c "$SRC_DIR"/backends/
cp "$PROJECT_DIR"/backends/pfh-ec-mmio.h "$SRC_DIR"/backends/
cp "$PROJECT_DIR"/platforms/intel/pfh-intel-nuc-ec-v9.c "$SRC_DIR"/platforms/intel/

dkms add -m "$PACKAGE_NAME" -v "$PACKAGE_VERSION"
dkms build -m "$PACKAGE_NAME" -v "$PACKAGE_VERSION"
dkms install -m "$PACKAGE_NAME" -v "$PACKAGE_VERSION"
depmod -a

if [ -n "$PFH_PLATFORM_FILTER" ]; then
  modprobe "$MODULE_NAME" platform="$PFH_PLATFORM_FILTER"
else
  modprobe "$MODULE_NAME"
fi
