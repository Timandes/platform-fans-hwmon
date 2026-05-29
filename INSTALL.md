# Install

Back to [README.md](README.md). Chinese version: [INSTALL.zh_CN.md](INSTALL.zh_CN.md).

## Dependencies

```bash
sudo apt update
sudo apt install -y gcc make dkms "linux-headers-$(uname -r)"
```

On minimal Debian or fnOS shells, `modinfo` may live outside the default user `PATH`. Use `/usr/sbin/modinfo` when needed.

## DKMS Install

Default install:

```bash
sudo ./scripts/install.sh
sudo modprobe platform_fans_hwmon
```

Platform-scoped install for the current first platform:

```bash
sudo ./scripts/install.sh --platform intel-nuc-ec-v9
```

Diagnostic runtime platform selection:

```bash
sudo modprobe platform_fans_hwmon platform=intel-nuc-ec-v9
```

The `platform=` parameter still requires the platform identity checks to pass.

## Validate

```bash
/usr/sbin/modinfo platform_fans_hwmon
sensors | sed -n '/intel_nuc_ec/,/^$/p'
```

Find the hwmon device:

```bash
for h in /sys/class/hwmon/hwmon*; do
  [ "$(cat "$h/name" 2>/dev/null)" = "intel_nuc_ec" ] || continue
  echo "hwmon=$h"
  cat "$h"/fan*_label
  cat "$h"/fan*_input
done
```

Expected Intel NUC9 labels:

```text
CPU Fan
System Fan 1
System Fan 2
```

## DKMS Uninstall

```bash
sudo ./scripts/uninstall.sh
```

## Manual Build And Test Load

```bash
make
sudo insmod platform_fans_hwmon.ko
sensors | sed -n '/intel_nuc_ec/,/^$/p'
sudo rmmod platform_fans_hwmon
```

## Manual Cleanup

```bash
make clean
```
