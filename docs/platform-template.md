# Platform Support Template

## Platform ID

`vendor-model-backend-version`

## Hardware

- Vendor:
- Product name:
- Board name:
- BIOS vendor:
- BIOS version:
- Kernel version:

## Backend

- Backend name:
- Access method:
- Secondary identifier:
- Reason this backend is safe for read-only use:

## Fan channels

| hwmon label | Source | Encoding | Expected RPM range |
| --- | --- | --- | --- |
| CPU Fan | register or method | 16-bit RPM | 0-10000 |

## Match rules

- Primary match:
- Secondary match:
- Rejected similar platforms:

## Validation evidence

Paste these command outputs:

```bash
sudo tools/collect-platform-info.sh
for h in /sys/class/hwmon/hwmon*; do
  [ -f "$h/name" ] || continue
  echo "hwmon=$h name=$(cat "$h/name")"
  cat "$h"/fan*_label "$h"/fan*_input 2>/dev/null || true
done
sensors
dmesg | tail -100
```

## Limitations

- Read-only support:
- Unsupported firmware versions:
- Known missing sensors:
