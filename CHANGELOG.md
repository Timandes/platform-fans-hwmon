# Changelog

## [0.2.0]

Generalize the project into Platform Fans hwmon.

### Added

- Add `platform_fans_hwmon` as the generalized module name.
- Add a reusable hwmon core, EC MMIO backend, and platform descriptor layout.
- Add `intel-nuc-ec-v9` as the first platform descriptor while preserving the `intel_nuc_ec` hwmon name.
- Add platform-scoped install support with `--platform intel-nuc-ec-v9`.
- Add platform contribution documentation, supported platform matrix, and a read-only platform information collection tool.

### Changed

- Rename the DKMS package to `platform-fans-hwmon`.
- Bump DKMS and install script package version to `0.2.0`.
- Replace the monolithic Intel NUC9 source file with `core/`, `backends/`, and `platforms/` source layout.
- Update English and zh_CN installation and usage documentation for the generalized framework.

### Verified

- Static tests pass with `python3 -m unittest discover -s tests -q`.
- Shell syntax checks pass for install, uninstall, and collection scripts.
- Built `platform_fans_hwmon.ko` on fnOS / Debian 12 with kernel `6.18.18-trim`.
- Runtime validated on NAS-11: `platform_fans_hwmon` loads, exports `intel_nuc_ec`, and reports CPU/System fan RPM through `sensors`.

## [0.1.0]

Initial release.

### Added

- Add read-only Linux hwmon driver for Intel NUC9 EC V9 fan tachometers.
- Expose CPU and system fan RPM values through standard hwmon attributes:
  - `fan1_input` / `fan1_label`
  - `fan2_input` / `fan2_label`
  - `fan3_input` / `fan3_label`
- Support Intel NUC9 Extreme and NUC9 Pro DMI matches for `QNB` and `QNX` models.
- Validate EC identifier before registering hwmon device; supported identifiers are `SPG_EC` and `ELM_EC`.
- Add DKMS packaging and install/uninstall scripts.
- Add English and zh_CN documentation.
- License project under GPL-2.0.

### Verified

- Built and loaded on fnOS / Debian 12 with kernel `6.18.18-trim`.
- Verified `sensors` output for `intel_nuc_ec-isa-0000`.
