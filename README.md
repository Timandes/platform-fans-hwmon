# Platform Fans hwmon

`platform-fans-hwmon` is a read-only Linux hwmon framework for platform-specific fan RPM sensors that are not already exposed by existing kernel drivers.

The kernel module is `platform_fans_hwmon`. It provides a common hwmon core, reusable read backends, and platform-specific descriptors.

This project exposes fan RPM values through the standard hwmon ABI so tools such as `sensors`, CoolerControl, and monitoring agents can read `fan*_input` and `fan*_label`.

## Supported Platforms

See [docs/supported-platforms.md](docs/supported-platforms.md).

Current platforms:

- `intel-nuc-ec-v9`
  - Intel NUC9 Extreme Compute Element / Kit
  - Intel NUC9 Pro Compute Element / Kit
  - EC identifiers: `SPG_EC` or `ELM_EC`
  - hwmon name: `intel_nuc_ec`
- `ds2308-it8613e-sio`
  - DS2308 / board `Dinson`
  - Sensor chip: ITE `IT8613E`
  - Exports valid `fan1` through `fan5` tachometers
  - hwmon name: `ds2308_it8613e`

Externally supported platforms:

- `minisforum-ms01-nct6775`
  - Minisforum MS-01 / Venus Series / board `AHWSA`
  - Sensor chip: Nuvoton `NCT6798D`
  - Existing Linux hwmon driver: `nct6775`
  - hwmon name: `nct6798`

## Intel NUC9 EC V9 Sensors

```text
fan1_label = CPU Fan
fan1_input = EC V9 register 0x41B

fan2_label = System Fan 1
fan2_input = EC V9 register 0x41E

fan3_label = System Fan 2
fan3_input = EC V9 register 0x421
```

## DS2308 IT8613E Sensors

`ds2308-it8613e-sio` exposes IT8613E fan tachometers as read-only hwmon fans.
Channels whose tach register pair reads as invalid are hidden from hwmon.

## Install

See [INSTALL.md](INSTALL.md).

Default DKMS install:

```bash
sudo ./scripts/install.sh
sudo modprobe platform_fans_hwmon
```

Platform-scoped install:

```bash
sudo ./scripts/install.sh --platform intel-nuc-ec-v9
```

## Hardware Discovery

Before adding a new `platform_fans_hwmon` backend or platform descriptor, first check whether Linux already supports the board through an existing hwmon driver:

```bash
sudo sensors-detect --auto
sensors
```

If `sensors-detect` recommends an existing Linux hwmon driver such as `nct6775`, prefer loading and documenting that driver instead of duplicating chip support in this project. For example, Minisforum MS-01 exposes its Nuvoton `NCT6798D` Super I/O sensors through `nct6775`:

```bash
sudo modprobe nct6775
echo nct6775 | sudo tee /etc/modules-load.d/nct6775.conf
```

Add a `platform_fans_hwmon` implementation only when existing mainline drivers cannot expose the required fan RPM values.

## Contributing Platform Support

See [docs/contributing-platform.md](docs/contributing-platform.md) and [docs/platform-template.md](docs/platform-template.md).

Platform support must be explicitly matched and validated on real hardware. The driver does not perform speculative scanning of unknown EC, MMIO, or IO-port ranges.

## Safety

Default platform support is read-only. This module does not expose PWM controls, does not change fan curves, and does not write fan-control registers.

Fan control is a separate risk category and is not part of the default module.

## Chinese Documentation

- [README.zh_CN.md](README.zh_CN.md)
- [INSTALL.zh_CN.md](INSTALL.zh_CN.md)

## License

This project is licensed under the GNU General Public License version 2.
