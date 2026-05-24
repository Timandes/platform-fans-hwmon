# Installation

[INSTALL.zh_CN.md](INSTALL.zh_CN.md)

This project can be installed either as a DKMS-managed module or loaded manually for testing.

Use DKMS for normal use. Manual loading is only intended for short validation sessions.

## Prerequisites

Install build tools and matching kernel headers on the target machine:

```bash
sudo apt update
sudo apt install -y gcc make dkms "linux-headers-$(uname -r)"
```

On minimal Debian/fnOS shells, some tools may live outside the default user `PATH`. Use full paths such as `/usr/sbin/modinfo`, `/sbin/insmod`, and `/sbin/rmmod` when needed.

## DKMS Install

From the project directory:

```bash
sudo ./scripts/install.sh
```

Verify the installed module:

```bash
/usr/sbin/modinfo intel_nuc_ec_hwmon
sensors | sed -n '/intel_nuc_ec/,/^$/p'
```

Expected `modinfo` fields include:

```text
license:        GPL
author:         Timandes White
name:           intel_nuc_ec_hwmon
```

Expected `sensors` output includes:

```text
intel_nuc_ec-isa-0000
Adapter: ISA adapter
CPU Fan:      1755 RPM
System Fan 1: 2030 RPM
System Fan 2: 2180 RPM
```

## DKMS Uninstall

From the project directory:

```bash
sudo ./scripts/uninstall.sh
```

Verify the module is no longer loaded:

```bash
lsmod | grep intel_nuc_ec_hwmon || echo "intel_nuc_ec_hwmon is not loaded"
```

## Manual Build And Test Load

Manual loading does not persist across reboots and does not rebuild automatically after kernel upgrades.

Build:

```bash
make
```

Load:

```bash
sudo insmod intel_nuc_ec_hwmon.ko
```

Verify:

```bash
sensors | sed -n '/intel_nuc_ec/,/^$/p'
```

Unload:

```bash
sudo rmmod intel_nuc_ec_hwmon
```

## Manual Cleanup

If you used `make install` instead of DKMS:

```bash
sudo make uninstall
```

If DKMS was used, prefer:

```bash
sudo ./scripts/uninstall.sh
```
