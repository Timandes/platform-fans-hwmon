# Supported Platforms

| Platform ID | hwmon name | Backend | Status |
| --- | --- | --- | --- |
| `intel-nuc-ec-v9` | `intel_nuc_ec` | EC MMIO | Validated on Intel NUC9 EC V9 |

## intel-nuc-ec-v9

Supported DMI matches:

- `NUC9i5QNB`
- `NUC9i7QNB`
- `NUC9i9QNB`
- `NUC9V7QNB`
- `NUC9VXQNB`
- `NUC9i5QNX`
- `NUC9i7QNX`
- `NUC9i9QNX`
- `NUC9V7QNX`
- `NUC9VXQNX`

Required EC identifiers:

- `SPG_EC`
- `ELM_EC`

Exported fans:

| Attribute | Label | Source |
| --- | --- | --- |
| `fan1_input` | CPU Fan | EC V9 register `0x41B` |
| `fan2_input` | System Fan 1 | EC V9 register `0x41E` |
| `fan3_input` | System Fan 2 | EC V9 register `0x421` |
