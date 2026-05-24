#!/bin/sh
set -eu

section() {
  printf '\n## %s\n' "$1"
}

section "System"
uname -a || true

section "DMI"
if command -v dmidecode >/dev/null 2>&1; then
  dmidecode -t system -t baseboard -t bios 2>/dev/null || true
else
  for f in /sys/class/dmi/id/sys_vendor \
           /sys/class/dmi/id/product_name \
           /sys/class/dmi/id/product_version \
           /sys/class/dmi/id/board_vendor \
           /sys/class/dmi/id/board_name \
           /sys/class/dmi/id/board_version \
           /sys/class/dmi/id/bios_vendor \
           /sys/class/dmi/id/bios_version; do
    [ -r "$f" ] && printf '%s=%s\n' "$f" "$(cat "$f")"
  done
fi

section "Existing hwmon"
for h in /sys/class/hwmon/hwmon*; do
  [ -d "$h" ] || continue
  printf 'hwmon=%s\n' "$h"
  [ -r "$h/name" ] && printf 'name=%s\n' "$(cat "$h/name")"
  for f in "$h"/fan*_label "$h"/fan*_input "$h"/temp*_label "$h"/temp*_input; do
    [ -r "$f" ] && printf '%s=%s\n' "$f" "$(cat "$f")"
  done
done

section "ACPI devices"
find /sys/bus/acpi/devices -maxdepth 2 -type f \( -name hid -o -name path -o -name status \) -print -exec cat {} \; 2>/dev/null || true

section "PCI"
if command -v lspci >/dev/null 2>&1; then
  lspci -nn || true
else
  find /sys/bus/pci/devices -maxdepth 1 -type l -print 2>/dev/null || true
fi

section "Loaded modules"
lsmod || true

section "Recent kernel messages"
dmesg | tail -200 || true
