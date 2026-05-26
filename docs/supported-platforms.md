# Supported Platforms

| Platform ID | hwmon name | Backend | Status |
| --- | --- | --- | --- |
| `intel-nuc-ec-v9` | `intel_nuc_ec` | EC MMIO | Validated on Intel NUC9 EC V9 |
| `minisforum-ms01-nct6775` | `nct6798` | Linux mainline `nct6775` | Validated on Minisforum MS-01 |

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

## minisforum-ms01-nct6775

This platform is supported by the existing Linux mainline `nct6775` hwmon
driver. It does not require `platform_fans_hwmon`.

Observed DMI data:

- System vendor: `Micro Computer (HK) Tech Limited`
- Product name: `Venus Series`
- Board vendor: `Shenzhen Meigao Electronic Equipment Co.,Ltd`
- Board name: `AHWSA`
- BIOS version: `AHWSA.1.22`

Detected sensor chip:

```text
Nuvoton NCT6798D Super IO Sensors
ISA address 0xa20
driver nct6775
```

Enable manually:

```bash
sudo modprobe nct6775
```

Enable persistently:

```bash
echo nct6775 | sudo tee /etc/modules-load.d/nct6775.conf
```

Validated hwmon:

```text
nct6798-isa-0a20
fan1_input = 2149
fan2_input = 1781
fan3_input = 0
fan4_input = 0
fan5_input = 0
fan7_input = 0
```
