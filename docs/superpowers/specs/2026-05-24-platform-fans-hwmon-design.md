# Platform Fans hwmon Design

## Background

The current project is a read-only out-of-tree Linux hwmon module for Intel
NUC9 EC V9 fan tachometers. It validates NUC9 DMI data, maps the Intel NUC EC
MMIO window, verifies the EC identifier, and exposes three fan RPM values
through the standard hwmon ABI.

The next project direction is broader: turn the NUC9-specific driver into a
generic platform fan RPM framework that can accept platform-specific
contributions from hardware owners without forcing each contributor to write a
complete hwmon driver from scratch.

This project should not become an unsafe universal scanner. The core contract
is that every supported platform is explicitly matched, every hardware access
path is represented by a known backend, and every platform contribution includes
real hardware validation.

## Naming

Use these names for the generalized project:

- Repository and DKMS package: `platform-fans-hwmon`
- Kernel module: `platform_fans_hwmon`
- Internal C prefix: `pfh_`
- Kconfig prefix: `CONFIG_PFH_*`

The exported hwmon `name` should not be the generic module name. Each platform
description provides its own hwmon name so user-space output reflects the
actual sensor source. The current NUC9 backend should continue to export
`intel_nuc_ec` after migration.

## Goals

- Provide a reusable Linux hwmon core for platform-specific fan RPM sensors.
- Keep the first supported platform behavior equivalent to the current Intel
  NUC9 EC V9 driver.
- Let platform owners contribute support by adding a small platform description
  when an existing backend already covers the hardware access method.
- Let maintainers add new read backends for new access mechanisms such as EC
  MMIO, EC port I/O, ACPI, WMI, SMBus/I2C, Super I/O, or IPMI/BMC.
- Keep all default platform support read-only and expose only standard hwmon
  attributes such as `fanX_input` and `fanX_label`.
- Support both broad default builds and platform-scoped builds for debugging or
  embedded deployments.

## Non-Goals

- Do not scan unknown EC/MMIO/IO-port ranges looking for values that resemble
  fan RPM.
- Do not expose PWM or fan-control attributes in the default read-only module.
- Do not duplicate platforms that are already well supported by existing Linux
  mainline hwmon drivers unless there is a clear missing-sensor gap.
- Do not accept unverified platform entries that only compile but have not been
  tested on real hardware.
- Do not promise that one backend works across all models from a vendor without
  per-platform identity checks.

Fan control may be considered later as a separate explicit capability, for
example behind `CONFIG_PFH_CONTROL` or a separate module. It must remain
disabled by default and require per-platform write-safety review.

## Architecture

The generalized driver has three layers:

1. Core hwmon layer.
   Registers the platform hwmon device, exposes the standard hwmon ABI,
   dispatches channel reads, handles common logging and error paths, and owns
   module parameter parsing.

2. Read backends.
   Implement a hardware access method. A backend knows how to initialize,
   read, and clean up one class of sensor source, but does not know the full
   platform matrix.

3. Platform descriptions.
   Describe which machines are supported, which backend to use, and which fan
   channels exist on that platform.

The current NUC9 code becomes the first platform description using an EC MMIO
backend.

## Core Data Model

The code should distinguish backend behavior from platform data with explicit
types:

```c
struct pfh_backend_ops {
	int (*probe)(struct pfh_device *pfh);
	int (*read_fan)(struct pfh_device *pfh,
			const struct pfh_fan_channel *fan,
			long *rpm);
	void (*remove)(struct pfh_device *pfh);
};

struct pfh_platform_desc {
	const char *id;
	const char *hwmon_name;
	const struct dmi_system_id *dmi_table;
	bool (*match)(const struct pfh_platform_desc *desc);
	const struct pfh_backend_ops *backend;
	const void *backend_config;
	const struct pfh_fan_channel *fans;
	int num_fans;
};
```

The read backend is the code that answers "how is a value read?" Examples:

- Map an EC MMIO window and read a big-endian 16-bit register.
- Enter Super I/O configuration mode and read a tachometer register.
- Call an ACPI method.
- Query a WMI GUID.
- Query IPMI/BMC sensor records.

The platform description is the data that answers "which platform is this and
where are its fans?" Examples:

- DMI board/product names.
- Secondary EC, ACPI, WMI, PCI, or Super I/O identity checks.
- Fan labels.
- Per-fan register offsets, method IDs, sensor names, scaling, and byte order.

## Runtime Matching

Runtime matching starts when the module loads. The module iterates over the
compiled-in platform descriptions and selects the first one that fully matches
the current machine. If no platform matches, the module exits with `-ENODEV`
and does not access platform-private sensor registers.

Matching is layered:

1. Use low-risk identity data first, usually DMI/SMBIOS fields such as board
   vendor, board name, product name, BIOS vendor, or BIOS version.
2. If the platform description has a secondary `match` callback, run it only
   after the first identity layer matches.
3. The secondary match may check safe platform-specific identifiers such as an
   EC firmware ID, ACPI HID/path, WMI GUID, PCI ID, I2C device ID, Super I/O
   chip ID, or IPMI/BMC availability.
4. Only after both identity layers pass should the backend initialize the
   hardware access path and register hwmon.

The current NUC9 flow after migration should remain:

```text
1. Match NUC9 DMI board/product identifiers.
2. Find LPC device 0000:00:1f.0.
3. Read LGMR and confirm the EC MMIO window is enabled.
4. Map the EC MMIO window.
5. Verify the EC identifier is SPG_EC or ELM_EC.
6. Register hwmon as intel_nuc_ec.
```

The driver must not perform broad speculative scanning. In particular, it must
not probe arbitrary EC/MMIO/IO-port ranges on machines that did not first match
a known platform description.

## File Layout

Target layout:

```text
platform-fans-hwmon/
  Makefile
  dkms.conf
  README.md
  INSTALL.md
  core/
    pfh-core.c
    pfh-core.h
  backends/
    pfh-ec-mmio.c
    pfh-ec-mmio.h
    pfh-ec-port.c
    pfh-acpi.c
    pfh-wmi.c
    pfh-smbus.c
    pfh-superio.c
    pfh-ipmi.c
  platforms/
    intel/
      pfh-intel-nuc-ec-v9.c
    examples/
      pfh-example-ec-mmio.c
  docs/
    contributing-platform.md
    platform-template.md
    supported-platforms.md
  tools/
    collect-platform-info.sh
```

The initial migration does not need to implement every listed backend. It
should create only the core, the EC MMIO backend needed by NUC9, and the NUC9
platform description. The other backend files may be introduced later when
there is a real platform requiring them.

## Installation Model

Default user install:

```bash
sudo ./scripts/install.sh
sudo modprobe platform_fans_hwmon
```

The default build includes all verified platform descriptions available in the
release. At load time, only the platform matching the current machine is used.
If no platform matches, no hwmon device is registered.

For platform-scoped debugging or embedded deployment, support an install option:

```bash
sudo ./scripts/install.sh --platform intel-nuc-ec-v9
```

The scoped build should compile only the selected platform and the backend
dependencies it needs. Runtime forcing may also be supported for diagnostics:

```bash
sudo modprobe platform_fans_hwmon platform=intel-nuc-ec-v9
```

The `platform=` parameter must still require the platform's identity checks to
pass by default. Any unsafe override that bypasses identity checks must require
a separate, clearly named debug parameter and should not be documented as a
normal user path.

## Contribution Workflow

Platform owners should have a low-friction contribution path:

1. Run `tools/collect-platform-info.sh` on the target machine.
2. Confirm whether existing Linux hwmon drivers already expose the needed fan
   RPM values.
3. Choose the closest existing backend.
4. Add a platform description under `platforms/<vendor>/`.
5. Fill in the platform template with DMI data, secondary identifiers, fan
   labels, register or method mapping, and safety notes.
6. Build and load the module on real hardware.
7. Submit `sensors`, `/sys/class/hwmon`, and dmesg validation output.

When a platform needs a new access mechanism, the contributor or maintainer
adds a backend first, then adds the platform description using that backend.

## Safety Policy

Default platform support is read-only because RPM reporting and fan control
have very different failure modes. A bad RPM read can fail to load or report a
bad value. A bad write can stop fans, break EC state, alter platform thermal
policy, or behave differently across firmware versions.

Read-only platform support must satisfy:

- Explicit platform identity matching.
- No speculative hardware scanning.
- No writes to EC, Super I/O, ACPI, WMI, SMBus, or BMC state except protocol
  setup sequences required for documented read access.
- Real-hardware validation.
- Documentation that states the sensor source and limitations.

Fan control support, if added later, must satisfy a stricter per-platform
review and be opt-in at build time or module-load time.

## Migration Plan Summary

The implementation plan should be written separately after this design is
approved. At a high level, migration should happen in small steps:

1. Rename project packaging from `intel-nuc-ec-hwmon` to
   `platform-fans-hwmon`.
2. Rename module identity from `intel_nuc_ec_hwmon` to
   `platform_fans_hwmon`.
3. Extract hwmon registration into `core/pfh-core.c`.
4. Extract EC MMIO access into `backends/pfh-ec-mmio.c`.
5. Move the current NUC9 DMI, EC ID, fan label, and register mapping into
   `platforms/intel/pfh-intel-nuc-ec-v9.c`.
6. Preserve the current NUC9 exported hwmon name `intel_nuc_ec`.
7. Update DKMS scripts, README, install docs, static tests, and validation
   commands.

## Test Strategy

Static tests should verify:

- New project and module naming.
- Core/backend/platform files exist.
- NUC9 platform data still contains the same DMI entries, EC identifiers, fan
  labels, and RPM offsets.
- Default support remains read-only.
- Install scripts support default install and platform-scoped install.
- Documentation explains contribution requirements and runtime matching.

Runtime validation on NUC9 should verify:

- The module loads as `platform_fans_hwmon`.
- The hwmon device name remains `intel_nuc_ec`.
- `fan1_input`, `fan2_input`, and `fan3_input` remain present.
- RPM values remain in the same range as the current driver.
- The module unloads cleanly.

New platform contributions must include equivalent real-hardware validation
before being listed as supported.
