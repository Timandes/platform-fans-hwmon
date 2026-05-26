# DS2308 IT8613E Platform Backend Design

## Context

NAS-9 is a DS2308 / Dinson AMD Ryzen 7 5800H system. `sensors-detect` finds an
ITE IT8613E Super I/O hardware monitor at ISA address `0x0a30`, but the current
mainline `it87` driver does not support this chip in the running kernel.

An earlier attempt to validate the upstream v4 IT8613E `it87` patch series was
stopped because that series was withdrawn after a NULL pointer issue was
reported. The external-driver path remains preferred long term, but the user
now wants this project to add a DS2308-specific read-only platform backend.

## Probe Evidence

The NAS-9 read-only probe reported:

```text
devid=0x8613
devrev=0x0c
active=0x01
base=0x0a30
fan16_0c=0x00
fan_main_ctrl_13=0x72
fan_ctl_14=0xc4
chipid_58=0x90
fan1 raw=0xffff rpm=0
fan2 raw=0x0244 rpm=1163
fan3 raw=0xffff rpm=0
fan4 raw=0x0000 rpm=-1
fan5 raw=0x0000 rpm=-1
```

The tachometer register pairs follow the Linux `it87` 16-bit fan register
layout:

| Logical fan | Low register | High register |
| --- | --- | --- |
| fan1 | `0x0d` | `0x18` |
| fan2 | `0x0e` | `0x19` |
| fan3 | `0x0f` | `0x1a` |
| fan4 | `0x80` | `0x81` |
| fan5 | `0x82` | `0x83` |

RPM conversion uses the `it87` 16-bit formula:

```text
raw == 0x0000 -> invalid reading
raw == 0xffff -> 0 RPM
otherwise     -> 1350000 / (raw * 2)
```

## Goal

Expose DS2308 IT8613E fan RPM readings through the existing
`platform_fans_hwmon` framework, while keeping the implementation
board-specific, read-only, and limited to fan tachometer reads.

## Non-Goals

- Do not implement a generic IT8613E or `it87` replacement driver.
- Do not expose PWM controls.
- Do not write fan curves, fan control mode, temperature limits, voltage
  limits, or alarm registers.
- Do not support non-DS2308 boards through this platform descriptor.
- Do not hide raw `0xffff`; it represents 0 RPM and remains visible.

## Architecture

Add a new Super I/O backend specialized for IT8613E tachometer reads:

- `backends/pfh-it8613e-sio.h`
- `backends/pfh-it8613e-sio.c`

Add a new DS2308 platform descriptor:

- `platforms/ite/pfh-ds2308-it8613e.c`

Register the descriptor in the core platform registry.

The backend will:

- Enter Super I/O config mode at `0x2e/0x2f`.
- Verify device ID `0x8613`.
- Select LDN 4, the environmental monitor logical device.
- Verify LDN 4 is active.
- Verify HWM base address is exactly `0x0a30`.
- Exit Super I/O config mode after identity and base discovery.
- Read tachometer registers through HWM address/data ports `base + 5` and
  `base + 6`.

The backend must only write values required by the Super I/O access protocol:

- Super I/O enter sequence.
- Logical device select.
- Register index select for reads.
- Super I/O exit sequence.
- HWM register index select for reads.

It must not write HWM data registers or fan-control registers.

## Channel Visibility

The current core exposes every fan channel listed by the platform descriptor.
DS2308 needs runtime channel visibility because fan4/fan5 can report raw
`0x0000`, which is invalid and should not be exposed.

Extend `struct pfh_backend_ops` with an optional channel visibility callback:

```c
bool (*fan_visible)(struct pfh_device *pfh,
		    const struct pfh_fan_channel *fan);
```

Core behavior:

- If `fan_visible` is absent, keep the existing behavior and expose all
  described fan channels.
- If `fan_visible` is present, call it from `pfh_hwmon_is_visible()`.
- Return `0` for hidden channels.
- Return `0444` for visible `fan*_input` and `fan*_label`.

DS2308 visibility behavior:

- Read the channel raw tachometer value.
- Hide channel when raw value is `0x0000`.
- Expose channel when raw value is `0xffff`; read returns `0`.
- Expose channel when raw value is any other value; read returns converted RPM.

This means the current NAS-9 observation exposes fan1, fan2, and fan3, and
hides fan4 and fan5.

## Platform Descriptor

Platform ID:

```text
ds2308-it8613e-sio
```

hwmon name:

```text
ds2308_it8613e
```

DMI match:

- `DMI_PRODUCT_NAME` contains `DS2308`
- `DMI_BOARD_NAME` contains `Dinson`

The backend identity checks provide the secondary guard:

- Super I/O device ID `0x8613`
- LDN 4 active
- HWM base `0x0a30`

Fan channel labels:

| Exported hwmon channel | Physical IT8613E tach source | Label |
| --- | --- | --- |
| fan1 | IT8613E fan1 | `Fan 1` |
| fan2 | IT8613E fan2 | `Fan 2` |
| fan3 | IT8613E fan3 | `Fan 3` |
| fan4 | IT8613E fan4 | `Fan 4` |
| fan5 | IT8613E fan5 | `Fan 5` |

Hidden invalid channels do not create sysfs attributes.

## Error Handling

Platform matching skips DS2308 support when:

- DMI does not match.

Backend probe fails with `-ENODEV` when:

- Super I/O device ID is not `0x8613`.
- LDN 4 is inactive.
- HWM base is not `0x0a30`.
- I/O region reservation fails because another driver owns the ports.

Read fails with:

- `-ENODEV` if backend data is unavailable.
- `-EIO` if a raw value is `0x0000` after visibility previously exposed the
  channel.

The `0x0000` read-failure case is acceptable because fan visibility can change
when hardware state changes; hwmon users should treat the read as temporarily
unavailable.

## Testing

Static tests should verify:

- The new backend and platform files exist.
- The Makefile builds the new objects.
- The core supports optional `fan_visible`.
- The backend contains the required identity checks: `0x8613`, LDN `0x04`,
  base `0x0a30`.
- The backend contains the 16-bit fan formula and invalid/zero handling:
  `0x0000`, `0xffff`, `1350000`.
- The backend does not contain PWM-related writes or `hwmon_pwm`.
- The platform descriptor registers `ds2308-it8613e-sio`,
  `ds2308_it8613e`, `DS2308`, and `Dinson`.

Runtime validation on NAS-9 should confirm:

```bash
sudo insmod platform_fans_hwmon.ko platform=ds2308-it8613e-sio
for h in /sys/class/hwmon/hwmon*; do
  [ "$(cat "$h/name" 2>/dev/null)" = "ds2308_it8613e" ] || continue
  echo "hwmon=$h"
  for f in "$h"/fan*_label "$h"/fan*_input; do
    [ -e "$f" ] && printf '%s=' "$f" && cat "$f"
  done
done
sensors | sed -n '/ds2308_it8613e/,/^$/p'
sudo rmmod platform_fans_hwmon
```

Expected from the current probe:

- `fan1_input` may be `0`.
- `fan2_input` should be around `1163 RPM`, with normal runtime variation.
- `fan3_input` may be `0`.
- `fan4_*` and `fan5_*` should be absent while raw tach values remain
  `0x0000`.

## Documentation

Update:

- `README.md`
- `README.zh_CN.md`
- `docs/supported-platforms.md`
- `CHANGELOG.md` only if this is released in a later version.

The docs must state that DS2308 support is board-specific and read-only, and
that it is not a generic IT8613E driver.

## References

- NAS-9 `sensors-detect --auto` output captured on 2026-05-26.
- NAS-9 read-only IT8613E register probe captured on 2026-05-26.
- Linux `it87` source for register layout and 16-bit fan conversion:
  <https://codebrowser.dev/linux/linux/drivers/hwmon/it87.c.html>
- Linux `it87` documentation:
  <https://dri.freedesktop.org/docs/drm/hwmon/it87.html>
- Withdrawn IT8613E v4 patch series:
  <https://www.spinics.net/lists/kernel/msg6003724.html>
- Follow-up describing the NULL pointer issue:
  <https://www.spinics.net/lists/kernel/msg6009013.html>
