# Changelog

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
