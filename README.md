# Intel NUC EC hwmon

Read-only Linux hwmon driver for Intel NUC9 EC V9 fan tachometers.

This module exposes Intel NUC9 EC fan RPM values through the standard hwmon ABI so tools such as `sensors`, CoolerControl, and monitoring agents can read them as `fan1_input`, `fan2_input`, and `fan3_input`.

## Supported Hardware

First version support is intentionally narrow:

- Intel NUC9 Extreme Compute Element / Kit:
  - `NUC9i5QNB`
  - `NUC9i7QNB`
  - `NUC9i9QNB`
  - `NUC9i5QNX`
  - `NUC9i7QNX`
  - `NUC9i9QNX`
- Intel NUC9 Pro Compute Element / Kit:
  - `NUC9V7QNB`
  - `NUC9VXQNB`
  - `NUC9V7QNX`
  - `NUC9VXQNX`

The module also requires the Intel NUC EC identifier to be `SPG_EC` or `ELM_EC`.

## Exposed Sensors

```text
fan1_label = CPU Fan
fan1_input = EC V9 register 0x41B

fan2_label = System Fan 1
fan2_input = EC V9 register 0x41E

fan3_label = System Fan 2
fan3_input = EC V9 register 0x421
```

The module is read-only. It does not expose PWM controls and does not write EC registers.

## Installation

See [INSTALL.md](INSTALL.md) for DKMS installation, manual test loading, verification, and uninstall steps.

## Validate

Find the hwmon device:

```bash
for h in /sys/class/hwmon/hwmon*; do
  [ "$(cat "$h/name" 2>/dev/null)" = "intel_nuc_ec" ] || continue
  echo "hwmon=$h"
  cat "$h"/fan*_label
  cat "$h"/fan*_input
done
```

Expected labels:

```text
CPU Fan
System Fan 1
System Fan 2
```

Validated on Intel NUC9i7QNX / NUC9i7QNB with fnOS kernel `6.18.18-trim`:

```text
intel_nuc_ec-isa-0000
Adapter: ISA adapter
CPU Fan:      1755 RPM
System Fan 1: 2030 RPM
System Fan 2: 2180 RPM
```

Validated DKMS install:

```text
filename: /lib/modules/6.18.18-trim/updates/dkms/intel_nuc_ec_hwmon.ko
dkms: intel-nuc-ec-hwmon/0.1.0, 6.18.18-trim, x86_64: installed
lsmod: intel_nuc_ec_hwmon
```

## Safety

This module only reads EC MMIO registers. It does not write fan control registers, does not change fan curves, and does not expose `pwm*` controls.

## License

This project is licensed under the GNU General Public License version 2.
