# Contributing Platform Support

`platform-fans-hwmon` accepts platform-specific fan RPM support when the platform owner can provide real hardware validation.

## Requirements

- The platform must be explicitly matched by DMI, ACPI, WMI, PCI, I2C, Super I/O chip ID, IPMI/BMC availability, or another stable identity source.
- The contribution must not perform speculative scanning of unknown EC, MMIO, or IO-port ranges.
- Default support must be read-only and must not expose `pwm*` controls.
- The platform must provide `fanX_input` through the standard Linux hwmon ABI.
- The submission must include validation output from real hardware.

## Workflow

1. Run `tools/collect-platform-info.sh` on the target machine.
2. Check whether existing Linux hwmon drivers already expose the needed fan RPM values.
3. Choose an existing backend if one matches the hardware access method.
4. Add a platform descriptor under `platforms/<vendor>/`.
5. Fill in `docs/platform-template.md` with platform identity, backend, fan channels, safety notes, and validation evidence.
6. Build and load the module on the target machine.
7. Submit `sensors`, `/sys/class/hwmon`, and dmesg output with the patch.

## Backend And Platform Boundary

A backend implements how to read a value. A platform descriptor defines which machine is supported and where its fans are located.

Examples of backend responsibilities:

- Map an EC MMIO window and read a big-endian RPM register.
- Call an ACPI method.
- Query a WMI GUID.
- Read a Super I/O tachometer register.
- Query an IPMI/BMC sensor.

Examples of platform descriptor responsibilities:

- DMI board and product names.
- Secondary identifier checks.
- Fan labels.
- Per-fan register offsets or method names.
- Real-hardware validation notes.

## Safety

No speculative scanning is accepted. Match the platform first, then perform the minimum platform-specific reads needed to verify the backend source. Fan control writes require a separate design and are not part of default read-only support.
