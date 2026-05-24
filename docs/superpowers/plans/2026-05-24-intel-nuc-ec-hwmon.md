# Intel NUC EC hwmon Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a read-only out-of-tree Linux hwmon module that exposes Intel NUC9 EC V9 fan RPM values as standard `fan1_input` through `fan3_input`.

**Architecture:** The module registers a single platform driver that validates NUC9 DMI data, reads the LPC `LGMR` register from PCI device `0000:00:1f.0`, maps the EC MMIO window, verifies `SPG_EC` or `ELM_EC`, and registers a hwmon device. Build and deployment are handled by a normal kernel-module `Makefile`, DKMS metadata, and small install/uninstall scripts.

**Tech Stack:** Linux kernel module C, Linux hwmon API, PCI config access, DMI matching, `ioremap`, DKMS, POSIX shell.

---

## File Structure

- Create `intel_nuc_ec_hwmon.c`: module source; owns DMI matching, LPC/LGMR read, EC MMIO mapping, hwmon callbacks, init/exit cleanup.
- Create `Makefile`: out-of-tree kernel module build, clean, install, uninstall targets.
- Create `dkms.conf`: DKMS metadata for `intel_nuc_ec_hwmon`.
- Create `scripts/install.sh`: copies project into `/usr/src/intel-nuc-ec-hwmon-<version>`, registers DKMS, builds, installs, and loads module.
- Create `scripts/uninstall.sh`: unloads module if present, removes DKMS package, and removes `/usr/src` copy.
- Create `README.md`: purpose, supported hardware, build requirements, install paths, validation commands, and safety notes.
- Existing `docs/superpowers/specs/2026-05-24-intel-nuc-ec-hwmon-design.md`: design source of truth; do not modify unless implementation reveals a design change.

## Constants To Use

Use these exact constants in implementation:

```c
#define DRIVER_NAME "intel_nuc_ec_hwmon"
#define HWMON_NAME "intel_nuc_ec"
#define NUC_EC_LPC_BUS 0
#define NUC_EC_LPC_DEV 31
#define NUC_EC_LPC_FUNC 0
#define NUC_EC_LGMR_OFFSET 0x98
#define NUC_EC_LGMR_ENABLE BIT(0)
#define NUC_EC_LGMR_BASE_MASK 0xFFFF0000U
#define NUC_EC_MMIO_SIZE 0x10000
#define NUC_EC_IDENTIFIER_OFFSET 0x400
#define NUC_EC_FAN1_TYPE_OFFSET 0x41A
#define NUC_EC_FAN1_RPM_OFFSET 0x41B
#define NUC_EC_FAN2_TYPE_OFFSET 0x41D
#define NUC_EC_FAN2_RPM_OFFSET 0x41E
#define NUC_EC_FAN3_TYPE_OFFSET 0x420
#define NUC_EC_FAN3_RPM_OFFSET 0x421
```

---

### Task 1: Create Module Skeleton And Build File

**Files:**
- Create: `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/intel_nuc_ec_hwmon.c`
- Create: `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/Makefile`

- [ ] **Step 1: Write the initial module skeleton**

Create `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/intel_nuc_ec_hwmon.c` with:

```c
// SPDX-License-Identifier: GPL-2.0
/*
 * Read-only hwmon driver for Intel NUC EC V9 fan tachometers.
 */

#include <linux/dmi.h>
#include <linux/err.h>
#include <linux/hwmon.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

#define DRIVER_NAME "intel_nuc_ec_hwmon"
#define HWMON_NAME "intel_nuc_ec"

static int __init intel_nuc_ec_hwmon_init(void)
{
	pr_info(DRIVER_NAME ": init\n");
	return 0;
}

static void __exit intel_nuc_ec_hwmon_exit(void)
{
	pr_info(DRIVER_NAME ": exit\n");
}

module_init(intel_nuc_ec_hwmon_init);
module_exit(intel_nuc_ec_hwmon_exit);

MODULE_AUTHOR("Timandes White");
MODULE_DESCRIPTION("Read-only Intel NUC EC hwmon driver");
MODULE_LICENSE("GPL");
```

- [ ] **Step 2: Write the kernel module Makefile**

Create `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/Makefile` with:

```make
KERNEL_VERSION ?= $(shell uname -r)
KDIR ?= /lib/modules/$(KERNEL_VERSION)/build
PWD := $(shell pwd)

obj-m += intel_nuc_ec_hwmon.o

.PHONY: all clean install uninstall

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

install: all
	install -D -m 0644 intel_nuc_ec_hwmon.ko /lib/modules/$(KERNEL_VERSION)/extra/intel_nuc_ec_hwmon.ko
	depmod -a $(KERNEL_VERSION)

uninstall:
	rm -f /lib/modules/$(KERNEL_VERSION)/extra/intel_nuc_ec_hwmon.ko
	depmod -a $(KERNEL_VERSION)
```

- [ ] **Step 3: Verify the local files parse structurally**

Run:

```bash
sed -n '1,220p' /Users/timandes/Projects/fnos/nuc9-ec-hwmon/intel_nuc_ec_hwmon.c
sed -n '1,120p' /Users/timandes/Projects/fnos/nuc9-ec-hwmon/Makefile
```

Expected:

- Source includes `MODULE_AUTHOR("Timandes White")`.
- Makefile builds `intel_nuc_ec_hwmon.o`.

- [ ] **Step 4: Commit if repository exists**

Run:

```bash
cd /Users/timandes/Projects/fnos/nuc9-ec-hwmon
if git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
  git add intel_nuc_ec_hwmon.c Makefile
  git commit -m "feat: add intel nuc ec hwmon module skeleton"
else
  echo "not a git repository; skipping commit"
fi
```

Expected: commit succeeds if this directory has been initialized as git; otherwise it prints `not a git repository; skipping commit`.

---

### Task 2: Add DMI Guard And Platform Driver Lifecycle

**Files:**
- Modify: `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/intel_nuc_ec_hwmon.c`

- [ ] **Step 1: Replace the skeleton with platform driver scaffolding and DMI matching**

Update `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/intel_nuc_ec_hwmon.c` to:

```c
// SPDX-License-Identifier: GPL-2.0
/*
 * Read-only hwmon driver for Intel NUC EC V9 fan tachometers.
 */

#include <linux/dmi.h>
#include <linux/err.h>
#include <linux/hwmon.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

#define DRIVER_NAME "intel_nuc_ec_hwmon"
#define HWMON_NAME "intel_nuc_ec"

struct nuc_ec_data {
	struct device *hwmon_dev;
	void __iomem *ec_base;
	struct pci_dev *lpc;
};

static const struct dmi_system_id nuc_ec_dmi_table[] = {
	{ .matches = { DMI_MATCH(DMI_BOARD_NAME, "NUC9i5QNB") } },
	{ .matches = { DMI_MATCH(DMI_BOARD_NAME, "NUC9i7QNB") } },
	{ .matches = { DMI_MATCH(DMI_BOARD_NAME, "NUC9i9QNB") } },
	{ .matches = { DMI_MATCH(DMI_BOARD_NAME, "NUC9V7QNB") } },
	{ .matches = { DMI_MATCH(DMI_BOARD_NAME, "NUC9VXQNB") } },
	{ .matches = { DMI_MATCH(DMI_PRODUCT_NAME, "NUC9i5QNX") } },
	{ .matches = { DMI_MATCH(DMI_PRODUCT_NAME, "NUC9i7QNX") } },
	{ .matches = { DMI_MATCH(DMI_PRODUCT_NAME, "NUC9i9QNX") } },
	{ .matches = { DMI_MATCH(DMI_PRODUCT_NAME, "NUC9V7QNX") } },
	{ .matches = { DMI_MATCH(DMI_PRODUCT_NAME, "NUC9VXQNX") } },
	{}
};
MODULE_DEVICE_TABLE(dmi, nuc_ec_dmi_table);

static int nuc_ec_probe(struct platform_device *pdev)
{
	if (!dmi_check_system(nuc_ec_dmi_table))
		return -ENODEV;

	dev_info(&pdev->dev, "matched Intel NUC9 DMI guard\n");
	return 0;
}

static void nuc_ec_remove(struct platform_device *pdev)
{
	dev_info(&pdev->dev, "removed\n");
}

static struct platform_driver nuc_ec_driver = {
	.driver = {
		.name = DRIVER_NAME,
	},
	.probe = nuc_ec_probe,
	.remove = nuc_ec_remove,
};

static struct platform_device *nuc_ec_pdev;

static int __init intel_nuc_ec_hwmon_init(void)
{
	int ret;

	ret = platform_driver_register(&nuc_ec_driver);
	if (ret)
		return ret;

	nuc_ec_pdev = platform_device_register_simple(DRIVER_NAME, PLATFORM_DEVID_NONE, NULL, 0);
	if (IS_ERR(nuc_ec_pdev)) {
		ret = PTR_ERR(nuc_ec_pdev);
		platform_driver_unregister(&nuc_ec_driver);
		return ret;
	}

	return 0;
}

static void __exit intel_nuc_ec_hwmon_exit(void)
{
	platform_device_unregister(nuc_ec_pdev);
	platform_driver_unregister(&nuc_ec_driver);
}

module_init(intel_nuc_ec_hwmon_init);
module_exit(intel_nuc_ec_hwmon_exit);

MODULE_AUTHOR("Timandes White");
MODULE_DESCRIPTION("Read-only Intel NUC EC hwmon driver");
MODULE_LICENSE("GPL");
```

- [ ] **Step 2: Check source for DMI entries**

Run:

```bash
rg -n "NUC9i7QNB|NUC9i7QNX|DMI_BOARD_NAME|DMI_PRODUCT_NAME|platform_driver" /Users/timandes/Projects/fnos/nuc9-ec-hwmon/intel_nuc_ec_hwmon.c
```

Expected: output includes both board and product DMI matches plus platform driver registration.

- [ ] **Step 3: Build-test on NAS-11 or equivalent kernel header environment**

Run on a machine with matching Linux headers:

```bash
cd /tmp/nuc9-ec-hwmon-build
make KDIR=/lib/modules/$(uname -r)/build
```

If building directly from project path on NAS-11, use:

```bash
cd /tmp/nuc9-ec-hwmon
make
```

Expected: `intel_nuc_ec_hwmon.ko` is produced. If `gcc` is missing, install build prerequisites before continuing:

```bash
sudo apt update
sudo apt install -y gcc make linux-headers-$(uname -r)
```

- [ ] **Step 4: Commit if repository exists**

Run:

```bash
cd /Users/timandes/Projects/fnos/nuc9-ec-hwmon
if git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
  git add intel_nuc_ec_hwmon.c
  git commit -m "feat: add nuc9 dmi guard"
else
  echo "not a git repository; skipping commit"
fi
```

---

### Task 3: Implement EC MMIO Detection And Cleanup

**Files:**
- Modify: `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/intel_nuc_ec_hwmon.c`

- [ ] **Step 1: Add constants and EC helper functions**

In `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/intel_nuc_ec_hwmon.c`, add these constants after `#define HWMON_NAME "intel_nuc_ec"`:

```c
#define NUC_EC_LPC_BUS 0
#define NUC_EC_LPC_DEV 31
#define NUC_EC_LPC_FUNC 0
#define NUC_EC_LGMR_OFFSET 0x98
#define NUC_EC_LGMR_ENABLE BIT(0)
#define NUC_EC_LGMR_BASE_MASK 0xFFFF0000U
#define NUC_EC_MMIO_SIZE 0x10000
#define NUC_EC_IDENTIFIER_OFFSET 0x400
```

Add these helper functions before `nuc_ec_probe`:

```c
static bool nuc_ec_identifier_valid(void __iomem *ec_base)
{
	char id[7];
	int i;

	for (i = 0; i < 6; i++)
		id[i] = readb(ec_base + NUC_EC_IDENTIFIER_OFFSET + i);
	id[6] = '\0';

	return !strcmp(id, "SPG_EC") || !strcmp(id, "ELM_EC");
}

static int nuc_ec_map_mmio(struct device *dev, struct nuc_ec_data *data)
{
	u32 lgmr;
	resource_size_t base;
	int ret;

	data->lpc = pci_get_domain_bus_and_slot(0, NUC_EC_LPC_BUS,
						PCI_DEVFN(NUC_EC_LPC_DEV, NUC_EC_LPC_FUNC));
	if (!data->lpc) {
		dev_err(dev, "LPC device 0000:00:1f.0 not found\n");
		return -ENODEV;
	}

	ret = pci_read_config_dword(data->lpc, NUC_EC_LGMR_OFFSET, &lgmr);
	if (ret) {
		dev_err(dev, "failed to read LGMR config register: %d\n", ret);
		return ret;
	}

	if (!(lgmr & NUC_EC_LGMR_ENABLE)) {
		dev_err(dev, "EC MMIO window is disabled: LGMR=0x%08x\n", lgmr);
		return -ENODEV;
	}

	base = lgmr & NUC_EC_LGMR_BASE_MASK;
	if (!base) {
		dev_err(dev, "EC MMIO base is zero: LGMR=0x%08x\n", lgmr);
		return -ENODEV;
	}

	data->ec_base = ioremap(base, NUC_EC_MMIO_SIZE);
	if (!data->ec_base)
		return -ENOMEM;

	if (!nuc_ec_identifier_valid(data->ec_base)) {
		dev_err(dev, "unsupported Intel NUC EC identifier\n");
		iounmap(data->ec_base);
		data->ec_base = NULL;
		return -ENODEV;
	}

	dev_info(dev, "mapped EC MMIO at 0x%pa\n", &base);
	return 0;
}

static void nuc_ec_unmap_mmio(struct nuc_ec_data *data)
{
	if (data->ec_base) {
		iounmap(data->ec_base);
		data->ec_base = NULL;
	}

	if (data->lpc) {
		pci_dev_put(data->lpc);
		data->lpc = NULL;
	}
}
```

- [ ] **Step 2: Update probe/remove to allocate data and map MMIO**

Replace `nuc_ec_probe` and `nuc_ec_remove` with:

```c
static int nuc_ec_probe(struct platform_device *pdev)
{
	struct nuc_ec_data *data;
	int ret;

	if (!dmi_check_system(nuc_ec_dmi_table))
		return -ENODEV;

	data = devm_kzalloc(&pdev->dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	ret = nuc_ec_map_mmio(&pdev->dev, data);
	if (ret)
		return ret;

	platform_set_drvdata(pdev, data);
	dev_info(&pdev->dev, "Intel NUC EC V9 detected\n");
	return 0;
}

static void nuc_ec_remove(struct platform_device *pdev)
{
	struct nuc_ec_data *data = platform_get_drvdata(pdev);

	nuc_ec_unmap_mmio(data);
}
```

- [ ] **Step 3: Add missing include if the build requires it**

Ensure the source includes string helpers. Add after other includes:

```c
#include <linux/string.h>
```

- [ ] **Step 4: Build-test**

Run:

```bash
make -C /Users/timandes/Projects/fnos/nuc9-ec-hwmon clean
make -C /Users/timandes/Projects/fnos/nuc9-ec-hwmon
```

Expected on a local non-Linux/macOS host: this may fail because `/lib/modules/.../build` is unavailable. Expected on NAS-11 or a Linux build host with headers: `.ko` builds without errors.

- [ ] **Step 5: Runtime smoke test on NAS-11**

Copy the project to NAS-11 and run:

```bash
cd /tmp/nuc9-ec-hwmon
make
sudo insmod intel_nuc_ec_hwmon.ko
dmesg | tail -30
sudo rmmod intel_nuc_ec_hwmon
```

Expected dmesg includes `Intel NUC EC V9 detected` and `mapped EC MMIO`.

- [ ] **Step 6: Commit if repository exists**

Run:

```bash
cd /Users/timandes/Projects/fnos/nuc9-ec-hwmon
if git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
  git add intel_nuc_ec_hwmon.c
  git commit -m "feat: detect intel nuc ec mmio"
else
  echo "not a git repository; skipping commit"
fi
```

---

### Task 4: Implement hwmon Channel Reads

**Files:**
- Modify: `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/intel_nuc_ec_hwmon.c`

- [ ] **Step 1: Add fan register constants**

Add after `#define NUC_EC_IDENTIFIER_OFFSET 0x400`:

```c
#define NUC_EC_FAN1_RPM_OFFSET 0x41B
#define NUC_EC_FAN2_RPM_OFFSET 0x41E
#define NUC_EC_FAN3_RPM_OFFSET 0x421
```

- [ ] **Step 2: Add read helpers and labels**

Add before `nuc_ec_identifier_valid`:

```c
static const char * const nuc_ec_fan_labels[] = {
	"CPU Fan",
	"System Fan 1",
	"System Fan 2",
};

static const u16 nuc_ec_fan_offsets[] = {
	NUC_EC_FAN1_RPM_OFFSET,
	NUC_EC_FAN2_RPM_OFFSET,
	NUC_EC_FAN3_RPM_OFFSET,
};

static u16 nuc_ec_read_be16(void __iomem *base, u16 offset)
{
	u16 high = readb(base + offset);
	u16 low = readb(base + offset + 1);

	return (high << 8) | low;
}
```

- [ ] **Step 3: Add hwmon callbacks and chip info**

Add before `nuc_ec_probe`:

```c
static umode_t nuc_ec_hwmon_is_visible(const void *drvdata,
				       enum hwmon_sensor_types type,
				       u32 attr, int channel)
{
	if (type != hwmon_fan)
		return 0;

	switch (attr) {
	case hwmon_fan_input:
	case hwmon_fan_label:
		return 0444;
	default:
		return 0;
	}
}

static int nuc_ec_hwmon_read(struct device *dev, enum hwmon_sensor_types type,
			     u32 attr, int channel, long *val)
{
	struct nuc_ec_data *data = dev_get_drvdata(dev);

	if (type != hwmon_fan || attr != hwmon_fan_input)
		return -EOPNOTSUPP;

	if (channel < 0 || channel >= ARRAY_SIZE(nuc_ec_fan_offsets))
		return -EINVAL;

	*val = nuc_ec_read_be16(data->ec_base, nuc_ec_fan_offsets[channel]);
	return 0;
}

static int nuc_ec_hwmon_read_string(struct device *dev,
				    enum hwmon_sensor_types type,
				    u32 attr, int channel,
				    const char **str)
{
	if (type != hwmon_fan || attr != hwmon_fan_label)
		return -EOPNOTSUPP;

	if (channel < 0 || channel >= ARRAY_SIZE(nuc_ec_fan_labels))
		return -EINVAL;

	*str = nuc_ec_fan_labels[channel];
	return 0;
}

static const struct hwmon_ops nuc_ec_hwmon_ops = {
	.is_visible = nuc_ec_hwmon_is_visible,
	.read = nuc_ec_hwmon_read,
	.read_string = nuc_ec_hwmon_read_string,
};

static const struct hwmon_channel_info * const nuc_ec_hwmon_info[] = {
	HWMON_CHANNEL_INFO(fan,
			   HWMON_F_INPUT | HWMON_F_LABEL,
			   HWMON_F_INPUT | HWMON_F_LABEL,
			   HWMON_F_INPUT | HWMON_F_LABEL),
	NULL
};

static const struct hwmon_chip_info nuc_ec_hwmon_chip_info = {
	.ops = &nuc_ec_hwmon_ops,
	.info = nuc_ec_hwmon_info,
};
```

- [ ] **Step 4: Register hwmon from probe**

In `nuc_ec_probe`, after `ret = nuc_ec_map_mmio(&pdev->dev, data);`, replace the final success block with:

```c
	platform_set_drvdata(pdev, data);

	data->hwmon_dev = devm_hwmon_device_register_with_info(&pdev->dev,
							       HWMON_NAME,
							       data,
							       &nuc_ec_hwmon_chip_info,
							       NULL);
	if (IS_ERR(data->hwmon_dev)) {
		ret = PTR_ERR(data->hwmon_dev);
		nuc_ec_unmap_mmio(data);
		return ret;
	}

	dev_info(&pdev->dev, "Intel NUC EC V9 hwmon registered\n");
	return 0;
```

- [ ] **Step 5: Build-test**

Run on NAS-11 or equivalent Linux build host:

```bash
cd /tmp/nuc9-ec-hwmon
make clean
make
```

Expected: `.ko` builds without compiler errors. If `hwmon_fan_label` is not available for this kernel, replace label support with explicit sysfs attributes only after confirming the compiler error; do not preemptively diverge from standard hwmon API.

- [ ] **Step 6: Runtime hwmon validation on NAS-11**

Run:

```bash
cd /tmp/nuc9-ec-hwmon
sudo insmod intel_nuc_ec_hwmon.ko
for h in /sys/class/hwmon/hwmon*; do
  [ "$(cat "$h/name" 2>/dev/null)" = "intel_nuc_ec" ] || continue
  echo "hwmon=$h"
  cat "$h"/fan*_label
  cat "$h"/fan*_input
done
sensors
sudo rmmod intel_nuc_ec_hwmon
```

Expected:

- One hwmon device has `name=intel_nuc_ec`.
- Labels are `CPU Fan`, `System Fan 1`, `System Fan 2`.
- RPM values are non-zero and close to the validated EC script output.
- `sensors` displays three fans.

- [ ] **Step 7: Commit if repository exists**

Run:

```bash
cd /Users/timandes/Projects/fnos/nuc9-ec-hwmon
if git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
  git add intel_nuc_ec_hwmon.c
  git commit -m "feat: expose nuc ec fans via hwmon"
else
  echo "not a git repository; skipping commit"
fi
```

---

### Task 5: Add DKMS Metadata And Install Scripts

**Files:**
- Create: `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/dkms.conf`
- Create: `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/scripts/install.sh`
- Create: `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/scripts/uninstall.sh`

- [ ] **Step 1: Create DKMS metadata**

Create `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/dkms.conf`:

```bash
PACKAGE_NAME="intel-nuc-ec-hwmon"
PACKAGE_VERSION="0.1.0"
BUILT_MODULE_NAME[0]="intel_nuc_ec_hwmon"
DEST_MODULE_LOCATION[0]="/extra"
AUTOINSTALL="yes"
MAKE[0]="make KERNEL_VERSION=${kernelver}"
CLEAN="make KERNEL_VERSION=${kernelver} clean"
```

- [ ] **Step 2: Create install script**

Create `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/scripts/install.sh`:

```bash
#!/bin/sh
set -eu

PACKAGE_NAME="intel-nuc-ec-hwmon"
PACKAGE_VERSION="0.1.0"
SRC_DIR="/usr/src/${PACKAGE_NAME}-${PACKAGE_VERSION}"
MODULE_NAME="intel_nuc_ec_hwmon"

if [ "$(id -u)" -ne 0 ]; then
  echo "Please run as root: sudo $0" >&2
  exit 1
fi

for cmd in dkms make; do
  if ! command -v "$cmd" >/dev/null 2>&1; then
    echo "Missing required command: $cmd" >&2
    exit 1
  fi
done

if ! command -v gcc >/dev/null 2>&1 && ! command -v cc >/dev/null 2>&1; then
  echo "Missing compiler: install gcc or another C compiler" >&2
  exit 1
fi

PROJECT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)

rm -rf "$SRC_DIR"
mkdir -p "$SRC_DIR"
cp "$PROJECT_DIR"/intel_nuc_ec_hwmon.c "$SRC_DIR"/
cp "$PROJECT_DIR"/Makefile "$SRC_DIR"/
cp "$PROJECT_DIR"/dkms.conf "$SRC_DIR"/

if dkms status "$PACKAGE_NAME/$PACKAGE_VERSION" 2>/dev/null | grep -q "$PACKAGE_NAME"; then
  dkms remove -m "$PACKAGE_NAME" -v "$PACKAGE_VERSION" --all
fi

dkms add -m "$PACKAGE_NAME" -v "$PACKAGE_VERSION"
dkms build -m "$PACKAGE_NAME" -v "$PACKAGE_VERSION"
dkms install -m "$PACKAGE_NAME" -v "$PACKAGE_VERSION"
modprobe "$MODULE_NAME"
echo "Installed and loaded $MODULE_NAME"
```

- [ ] **Step 3: Create uninstall script**

Create `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/scripts/uninstall.sh`:

```bash
#!/bin/sh
set -eu

PACKAGE_NAME="intel-nuc-ec-hwmon"
PACKAGE_VERSION="0.1.0"
SRC_DIR="/usr/src/${PACKAGE_NAME}-${PACKAGE_VERSION}"
MODULE_NAME="intel_nuc_ec_hwmon"

if [ "$(id -u)" -ne 0 ]; then
  echo "Please run as root: sudo $0" >&2
  exit 1
fi

if lsmod | awk '{print $1}' | grep -qx "$MODULE_NAME"; then
  modprobe -r "$MODULE_NAME"
fi

if command -v dkms >/dev/null 2>&1; then
  dkms remove -m "$PACKAGE_NAME" -v "$PACKAGE_VERSION" --all || true
fi

rm -rf "$SRC_DIR"
echo "Removed $MODULE_NAME"
```

- [ ] **Step 4: Make scripts executable**

Run:

```bash
chmod +x /Users/timandes/Projects/fnos/nuc9-ec-hwmon/scripts/install.sh
chmod +x /Users/timandes/Projects/fnos/nuc9-ec-hwmon/scripts/uninstall.sh
```

Expected: both scripts are executable.

- [ ] **Step 5: Syntax-check scripts**

Run:

```bash
sh -n /Users/timandes/Projects/fnos/nuc9-ec-hwmon/scripts/install.sh
sh -n /Users/timandes/Projects/fnos/nuc9-ec-hwmon/scripts/uninstall.sh
```

Expected: no output and exit code 0.

- [ ] **Step 6: Commit if repository exists**

Run:

```bash
cd /Users/timandes/Projects/fnos/nuc9-ec-hwmon
if git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
  git add dkms.conf scripts/install.sh scripts/uninstall.sh
  git commit -m "feat: add dkms packaging"
else
  echo "not a git repository; skipping commit"
fi
```

---

### Task 6: Add README And Usage Documentation

**Files:**
- Create: `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/README.md`

- [ ] **Step 1: Write README**

Create `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/README.md`:

```markdown
# Intel NUC EC hwmon

Read-only Linux hwmon driver for Intel NUC9 EC V9 fan tachometers.

This module exposes Intel NUC9 EC fan RPM values through the standard hwmon ABI so tools such as `sensors`, CoolerControl, and monitoring agents can read them as `fan1_input`, `fan2_input`, and `fan3_input`.

## Supported Hardware

First version support is intentionally narrow:

- Intel NUC9 Extreme Compute Element / Kit:
  - `NUC9i5QNB`
  - `NUC9i7QNB`
  - `NUC9i9QNB`
  - `NUC9i5QNX`
  - `NUC9i7QNX`
  - `NUC9i9QNX`
- Intel NUC9 Pro Compute Element / Kit:
  - `NUC9V7QNB`
  - `NUC9VXQNB`
  - `NUC9V7QNX`
  - `NUC9VXQNX`

The module also requires the Intel NUC EC identifier to be `SPG_EC` or `ELM_EC`.

## Exposed Sensors

```text
fan1_label = CPU Fan
fan1_input = EC V9 register 0x41B

fan2_label = System Fan 1
fan2_input = EC V9 register 0x41E

fan3_label = System Fan 2
fan3_input = EC V9 register 0x421
```

The module is read-only. It does not expose PWM controls and does not write EC registers.

## Build

Install build prerequisites on the target system:

```bash
sudo apt update
sudo apt install -y gcc make "linux-headers-$(uname -r)"
```

Build:

```bash
make
```

Load for testing:

```bash
sudo insmod intel_nuc_ec_hwmon.ko
sensors
sudo rmmod intel_nuc_ec_hwmon
```

## DKMS Install

Install DKMS and build tools:

```bash
sudo apt update
sudo apt install -y gcc make dkms "linux-headers-$(uname -r)"
```

Install:

```bash
sudo ./scripts/install.sh
```

Uninstall:

```bash
sudo ./scripts/uninstall.sh
```

## Validate

Find the hwmon device:

```bash
for h in /sys/class/hwmon/hwmon*; do
  [ "$(cat "$h/name" 2>/dev/null)" = "intel_nuc_ec" ] || continue
  echo "hwmon=$h"
  cat "$h"/fan*_label
  cat "$h"/fan*_input
done
```

Expected labels:

```text
CPU Fan
System Fan 1
System Fan 2
```

## Safety

This module only reads EC MMIO registers. It does not write fan control registers, does not change fan curves, and does not expose `pwm*` controls.
```

- [ ] **Step 2: Check README includes scope and safety**

Run:

```bash
rg -n "read-only|NUC9|SPG_EC|fan1_input|pwm|DKMS|Safety" /Users/timandes/Projects/fnos/nuc9-ec-hwmon/README.md
```

Expected: output includes all key sections.

- [ ] **Step 3: Commit if repository exists**

Run:

```bash
cd /Users/timandes/Projects/fnos/nuc9-ec-hwmon
if git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
  git add README.md
  git commit -m "docs: document intel nuc ec hwmon"
else
  echo "not a git repository; skipping commit"
fi
```

---

### Task 7: End-To-End NAS-11 Validation

**Files:**
- No local source changes expected.

- [ ] **Step 1: Copy project to NAS-11**

Run from local machine:

```bash
rsync -av --delete \
  --exclude '.git' \
  /Users/timandes/Projects/fnos/nuc9-ec-hwmon/ \
  nas-11.timandes.net:/tmp/nuc9-ec-hwmon/
```

Expected: files appear under `/tmp/nuc9-ec-hwmon` on NAS-11.

- [ ] **Step 2: Ensure prerequisites on NAS-11**

Run:

```bash
ssh nas-11.timandes.net 'command -v gcc || echo missing-gcc; command -v dkms || echo missing-dkms; test -d /lib/modules/$(uname -r)/build && echo headers-ok'
```

Expected: `headers-ok`. If `missing-gcc` or `missing-dkms` appears, install:

```bash
ssh -t nas-11.timandes.net 'sudo apt update && sudo apt install -y gcc make dkms linux-headers-$(uname -r)'
```

- [ ] **Step 3: Build on NAS-11**

Run:

```bash
ssh nas-11.timandes.net 'cd /tmp/nuc9-ec-hwmon && make clean && make'
```

Expected: `intel_nuc_ec_hwmon.ko` is produced without compiler errors.

- [ ] **Step 4: Load and verify hwmon manually**

Run:

```bash
ssh -t nas-11.timandes.net 'cd /tmp/nuc9-ec-hwmon && sudo insmod intel_nuc_ec_hwmon.ko && for h in /sys/class/hwmon/hwmon*; do [ "$(cat "$h/name" 2>/dev/null)" = "intel_nuc_ec" ] || continue; echo "hwmon=$h"; cat "$h"/fan*_label; cat "$h"/fan*_input; done; sensors | sed -n "/intel_nuc_ec/,/^$/p"; sudo rmmod intel_nuc_ec_hwmon'
```

Expected:

```text
CPU Fan
System Fan 1
System Fan 2
```

Expected RPM values are non-zero and broadly close to:

```text
1755
2067
2195
```

Values may differ because fan speed changes over time.

- [ ] **Step 5: Test DKMS install**

Run:

```bash
ssh -t nas-11.timandes.net 'cd /tmp/nuc9-ec-hwmon && sudo ./scripts/install.sh && modinfo intel_nuc_ec_hwmon && sensors | sed -n "/intel_nuc_ec/,/^$/p"'
```

Expected:

- `modinfo` shows module `intel_nuc_ec_hwmon`.
- `sensors` shows three fan entries under `intel_nuc_ec`.

- [ ] **Step 6: Test DKMS uninstall**

Run:

```bash
ssh -t nas-11.timandes.net 'cd /tmp/nuc9-ec-hwmon && sudo ./scripts/uninstall.sh && ! lsmod | grep -q intel_nuc_ec_hwmon && echo unloaded'
```

Expected: output includes `unloaded`.

- [ ] **Step 7: Record validation output in README if needed**

If NAS-11 output reveals a different adapter label or sysfs path, update `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/README.md` with the exact observed output. Use this commit command if repository exists:

```bash
cd /Users/timandes/Projects/fnos/nuc9-ec-hwmon
if git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
  git add README.md
  git commit -m "docs: add nas validation notes"
else
  echo "not a git repository; skipping commit"
fi
```

---

## Self-Review

- Spec coverage: The plan covers the module source, DMI/EC guards, hwmon ABI export, read-only scope, DKMS packaging, scripts, README, and NAS-11 runtime validation.
- Placeholder scan: No `TBD`, `TODO`, or undefined "fill in later" steps remain. All code-producing steps include concrete code.
- Type consistency: The module name is consistently `intel_nuc_ec_hwmon`; the hwmon name is consistently `intel_nuc_ec`; exported ABI uses `fan1_input` through `fan3_input` and labels only.
