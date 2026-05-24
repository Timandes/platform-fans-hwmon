# Platform Fans hwmon

`platform-fans-hwmon` is a read-only Linux hwmon framework for platform-specific fan RPM sensors that are not already exposed by existing kernel drivers.

The kernel module is `platform_fans_hwmon`. It provides a common hwmon core and platform-specific descriptors. The first supported platform is Intel NUC9 EC V9, exported as hwmon name `intel_nuc_ec`.

This project exposes fan RPM values through the standard hwmon ABI so tools such as `sensors`, CoolerControl, and monitoring agents can read `fan*_input` and `fan*_label`.

## Supported Platforms

See [docs/supported-platforms.md](docs/supported-platforms.md).

Current platform:

- `intel-nuc-ec-v9`
  - Intel NUC9 Extreme Compute Element / Kit
  - Intel NUC9 Pro Compute Element / Kit
  - EC identifiers: `SPG_EC` or `ELM_EC`
  - hwmon name: `intel_nuc_ec`

## Intel NUC9 EC V9 Sensors

```text
fan1_label = CPU Fan
fan1_input = EC V9 register 0x41B

fan2_label = System Fan 1
fan2_input = EC V9 register 0x41E

fan3_label = System Fan 2
fan3_input = EC V9 register 0x421
```

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
