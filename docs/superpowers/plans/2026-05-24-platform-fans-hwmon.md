# Platform Fans hwmon Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Migrate the current Intel NUC9-only EC hwmon driver into a generic `platform-fans-hwmon` framework with a reusable core, an EC MMIO backend, and the current NUC9 EC V9 platform as the first platform descriptor.

**Architecture:** The new module is named `platform_fans_hwmon` and is split into a core hwmon/platform-driver layer, backend files that implement hardware access methods, and platform descriptor files that provide DMI matching, secondary identity checks, fan labels, and per-channel read metadata. The first backend is EC MMIO, and the first platform descriptor preserves the current NUC9 behavior and exported hwmon name `intel_nuc_ec`.

**Tech Stack:** Linux kernel module C, Linux hwmon API, DMI matching, PCI config access, `ioremap`, DKMS, POSIX shell, Python `unittest` static tests.

---

## Scope Check

This plan implements one coherent migration: the existing NUC9 driver becomes the first platform in a generalized platform-fan hwmon framework. It does not add any non-NUC9 platform, any unsafe probing, or any fan-control writes. New access backends such as ACPI, WMI, Super I/O, SMBus, and IPMI are documented extension points only; they are not created as empty source files.

## File Structure

- Modify: `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/tests/test_static_project.py`
  - Static regression tests for the new names, split files, read-only behavior, install scripts, and documentation.
- Delete: `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/intel_nuc_ec_hwmon.c`
  - Replaced by split core/backend/platform files.
- Create: `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/core/pfh-core.h`
  - Shared structs, constants, helper declarations, module parameter declaration, and platform registry declarations.
- Create: `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/core/pfh-core.c`
  - Platform driver lifecycle, platform matching, hwmon registration, hwmon read callbacks, module init/exit.
- Create: `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/backends/pfh-ec-mmio.h`
  - EC MMIO backend config structs and backend ops declaration.
- Create: `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/backends/pfh-ec-mmio.c`
  - LPC/LGMR lookup, EC MMIO mapping, EC identifier validation, big-endian RPM reads, cleanup.
- Create: `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/platforms/intel/pfh-intel-nuc-ec-v9.c`
  - NUC9 DMI table, EC MMIO config, fan channel table, platform descriptor.
- Modify: `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/Makefile`
  - Build `platform_fans_hwmon.o` from split objects.
- Modify: `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/dkms.conf`
  - Rename package and module to `platform-fans-hwmon` / `platform_fans_hwmon`.
- Modify: `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/scripts/install.sh`
  - Install renamed DKMS package, copy split source tree, support `--platform intel-nuc-ec-v9`, and load `platform_fans_hwmon`.
- Modify: `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/scripts/uninstall.sh`
  - Remove renamed DKMS package and unload `platform_fans_hwmon`.
- Modify: `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/README.md`
  - Document generalized scope, current supported platform, safety policy, and links.
- Modify: `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/README.zh_CN.md`
  - Chinese equivalent of README scope and links.
- Modify: `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/INSTALL.md`
  - Updated install/build/manual test commands.
- Modify: `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/INSTALL.zh_CN.md`
  - Chinese equivalent of install/build/manual test commands.
- Create: `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/docs/contributing-platform.md`
  - Platform contribution workflow and evidence requirements.
- Create: `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/docs/platform-template.md`
  - Copyable platform support template.
- Create: `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/docs/supported-platforms.md`
  - Supported platform matrix, initially Intel NUC9 EC V9.
- Create: `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/tools/collect-platform-info.sh`
  - Read-only data collector for DMI, existing hwmon, ACPI hints, PCI hints, loaded modules, and recent kernel logs.

## Task 1: Rewrite Static Tests For Generalized Project

**Files:**
- Modify: `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/tests/test_static_project.py`

- [ ] **Step 1: Replace the static test file**

Replace `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/tests/test_static_project.py` with:

```python
import pathlib
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[1]


class StaticProjectTests(unittest.TestCase):
    def read(self, relative_path):
        path = ROOT / relative_path
        self.assertTrue(path.exists(), f"{relative_path} should exist")
        return path.read_text()

    def assert_contains_all(self, text, expected):
        for item in expected:
            self.assertIn(item, text)

    def test_project_declares_gpl_2_license(self):
        license_text = self.read("LICENSE")

        self.assert_contains_all(
            license_text,
            (
                "GNU GENERAL PUBLIC LICENSE",
                "Version 2, June 1991",
                "Copyright",
            ),
        )

    def test_module_identity_uses_generalized_names(self):
        core = self.read("core/pfh-core.c")
        header = self.read("core/pfh-core.h")

        self.assert_contains_all(
            core,
            (
                '#define DRIVER_NAME "platform_fans_hwmon"',
                'MODULE_DESCRIPTION("Platform fan RPM hwmon framework")',
                'MODULE_AUTHOR("Timandes White")',
                'MODULE_LICENSE("GPL")',
                "module_param_named(platform, pfh_platform_param, charp, 0444)",
            ),
        )
        self.assert_contains_all(
            header,
            (
                "struct pfh_backend_ops",
                "struct pfh_fan_channel",
                "struct pfh_platform_desc",
                "struct pfh_device",
                "pfh_intel_nuc_ec_v9",
            ),
        )

    def test_project_uses_split_core_backend_platform_layout(self):
        for relative_path in (
            "core/pfh-core.c",
            "core/pfh-core.h",
            "backends/pfh-ec-mmio.c",
            "backends/pfh-ec-mmio.h",
            "platforms/intel/pfh-intel-nuc-ec-v9.c",
        ):
            self.assertTrue((ROOT / relative_path).exists(), f"{relative_path} should exist")

        self.assertFalse((ROOT / "intel_nuc_ec_hwmon.c").exists())

    def test_makefile_builds_generalized_module_from_split_objects(self):
        makefile = self.read("Makefile")

        self.assert_contains_all(
            makefile,
            (
                "obj-m += platform_fans_hwmon.o",
                "platform_fans_hwmon-y :=",
                "core/pfh-core.o",
                "backends/pfh-ec-mmio.o",
                "platforms/intel/pfh-intel-nuc-ec-v9.o",
                "KDIR ?= /lib/modules/$(KERNEL_VERSION)/build",
                "install -D -m 0644 platform_fans_hwmon.ko",
            ),
        )
        self.assertNotIn("intel_nuc_ec_hwmon.o", makefile)

    def test_dkms_metadata_targets_generalized_module(self):
        dkms = self.read("dkms.conf")

        self.assert_contains_all(
            dkms,
            (
                'PACKAGE_NAME="platform-fans-hwmon"',
                'PACKAGE_VERSION="0.2.0"',
                'BUILT_MODULE_NAME[0]="platform_fans_hwmon"',
                'MAKE[0]="make KERNEL_VERSION=${kernelver}"',
                'AUTOINSTALL="yes"',
            ),
        )
        self.assertNotIn("intel-nuc-ec-hwmon", dkms)
        self.assertNotIn("intel_nuc_ec_hwmon", dkms)

    def test_core_registers_platform_driver_and_hwmon(self):
        core = self.read("core/pfh-core.c")

        self.assert_contains_all(
            core,
            (
                "static const struct pfh_platform_desc * const pfh_platforms[]",
                "&pfh_intel_nuc_ec_v9",
                "static const struct pfh_platform_desc *pfh_match_platform(void)",
                "dmi_check_system(desc->dmi_table)",
                "devm_hwmon_device_register_with_info",
                "platform_driver_register(&pfh_driver)",
                "platform_device_register_simple(DRIVER_NAME",
                "platform_device_unregister(pfh_pdev)",
                "platform_driver_unregister(&pfh_driver)",
            ),
        )

    def test_core_keeps_platform_forcing_safe_by_default(self):
        core = self.read("core/pfh-core.c")

        self.assert_contains_all(
            core,
            (
                "pfh_platform_param",
                "strcmp(pfh_platform_param, desc->id)",
                "dmi_check_system(desc->dmi_table)",
                "dev_err(dev, \"requested platform %s did not match this machine",
            ),
        )

    def test_core_exposes_read_only_hwmon_fans(self):
        core = self.read("core/pfh-core.c")

        self.assert_contains_all(
            core,
            (
                "hwmon_fan_input",
                "hwmon_fan_label",
                "return 0444",
                "pfh_hwmon_read",
                "pfh_hwmon_read_string",
                "pfh->desc->backend->read_fan",
            ),
        )
        self.assertNotIn("hwmon_pwm", core)
        self.assertNotIn("pwm", core.lower())

    def test_ec_mmio_backend_preserves_lpc_lgmr_and_identifier_logic(self):
        backend = self.read("backends/pfh-ec-mmio.c")
        backend_header = self.read("backends/pfh-ec-mmio.h")

        self.assert_contains_all(
            backend_header,
            (
                "struct pfh_ec_mmio_config",
                "struct pfh_ec_mmio_fan",
                "extern const struct pfh_backend_ops pfh_ec_mmio_backend_ops",
            ),
        )
        self.assert_contains_all(
            backend,
            (
                "pci_get_domain_bus_and_slot(0, config->lpc_bus",
                "pci_read_config_dword(data->lpc, config->lgmr_offset",
                "lgmr & config->lgmr_enable",
                "lgmr & config->lgmr_base_mask",
                "ioremap(base, config->mmio_size)",
                "pfh_ec_mmio_identifier_valid",
                "pci_dev_put(data->lpc)",
                "pfh_ec_mmio_read_be16",
                ".read_fan = pfh_ec_mmio_read_fan",
            ),
        )

    def test_nuc9_platform_preserves_current_dmi_guard_and_fan_mapping(self):
        platform = self.read("platforms/intel/pfh-intel-nuc-ec-v9.c")

        self.assert_contains_all(
            platform,
            (
                'const struct pfh_platform_desc pfh_intel_nuc_ec_v9',
                '.id = "intel-nuc-ec-v9"',
                '.hwmon_name = "intel_nuc_ec"',
                ".backend = &pfh_ec_mmio_backend_ops",
                '"SPG_EC"',
                '"ELM_EC"',
                "#define NUC_EC_FAN1_RPM_OFFSET 0x41B",
                "#define NUC_EC_FAN2_RPM_OFFSET 0x41E",
                "#define NUC_EC_FAN3_RPM_OFFSET 0x421",
                '"CPU Fan"',
                '"System Fan 1"',
                '"System Fan 2"',
            ),
        )

        for model in (
            "NUC9i5QNB",
            "NUC9i7QNB",
            "NUC9i9QNB",
            "NUC9V7QNB",
            "NUC9VXQNB",
            "NUC9i5QNX",
            "NUC9i7QNX",
            "NUC9i9QNX",
            "NUC9V7QNX",
            "NUC9VXQNX",
        ):
            self.assertIn(model, platform)

        self.assert_contains_all(
            platform,
            (
                "DMI_MATCH(DMI_BOARD_NAME",
                "DMI_MATCH(DMI_PRODUCT_NAME",
                "MODULE_DEVICE_TABLE(dmi, pfh_intel_nuc_ec_v9_dmi_table)",
            ),
        )

    def test_install_scripts_manage_generalized_dkms_package(self):
        install = self.read("scripts/install.sh")
        uninstall = self.read("scripts/uninstall.sh")

        self.assert_contains_all(
            install,
            (
                'PACKAGE_NAME="platform-fans-hwmon"',
                'MODULE_NAME="platform_fans_hwmon"',
                'SUPPORTED_PLATFORM="intel-nuc-ec-v9"',
                "--platform",
                "PFH_PLATFORM_FILTER",
                "dkms add",
                "dkms build",
                "dkms install",
                'modprobe "$MODULE_NAME"',
            ),
        )
        self.assert_contains_all(
            uninstall,
            (
                'PACKAGE_NAME="platform-fans-hwmon"',
                'MODULE_NAME="platform_fans_hwmon"',
                "dkms remove",
                'modprobe -r "$MODULE_NAME"',
            ),
        )
        self.assertNotIn("intel-nuc-ec-hwmon", install + uninstall)
        self.assertNotIn("intel_nuc_ec_hwmon", install + uninstall)

    def test_docs_explain_platform_framework_scope_and_links(self):
        readme = self.read("README.md")
        install_doc = self.read("INSTALL.md")
        contributing = self.read("docs/contributing-platform.md")
        template = self.read("docs/platform-template.md")
        supported = self.read("docs/supported-platforms.md")

        self.assert_contains_all(
            readme,
            (
                "Platform Fans hwmon",
                "platform-fans-hwmon",
                "platform_fans_hwmon",
                "Intel NUC9 EC V9",
                "read-only",
                "does not expose PWM controls",
                "[INSTALL.md](INSTALL.md)",
                "[README.zh_CN.md](README.zh_CN.md)",
                "[docs/contributing-platform.md](docs/contributing-platform.md)",
                "[docs/supported-platforms.md](docs/supported-platforms.md)",
            ),
        )
        self.assert_contains_all(
            install_doc,
            (
                "sudo ./scripts/install.sh",
                "sudo ./scripts/install.sh --platform intel-nuc-ec-v9",
                "sudo modprobe platform_fans_hwmon",
                "sudo modprobe platform_fans_hwmon platform=intel-nuc-ec-v9",
                "sudo insmod platform_fans_hwmon.ko",
                "sudo rmmod platform_fans_hwmon",
                "/usr/sbin/modinfo platform_fans_hwmon",
                "sensors | sed -n '/intel_nuc_ec/,/^$/p'",
                "[INSTALL.zh_CN.md](INSTALL.zh_CN.md)",
            ),
        )
        self.assert_contains_all(
            contributing,
            (
                "tools/collect-platform-info.sh",
                "DMI",
                "secondary identifier",
                "real hardware validation",
                "No speculative scanning",
            ),
        )
        self.assert_contains_all(
            template,
            (
                "Platform ID",
                "Backend",
                "Fan channels",
                "Validation evidence",
            ),
        )
        self.assert_contains_all(
            supported,
            (
                "intel-nuc-ec-v9",
                "intel_nuc_ec",
                "NUC9i7QNX",
                "SPG_EC",
                "ELM_EC",
            ),
        )

    def test_zh_cn_docs_use_generalized_names_and_localized_links(self):
        readme = self.read("README.zh_CN.md")
        install_doc = self.read("INSTALL.zh_CN.md")

        self.assert_contains_all(
            readme,
            (
                "Platform Fans hwmon",
                "platform-fans-hwmon",
                "platform_fans_hwmon",
                "平台特定风扇转速",
                "Intel NUC9 EC V9",
                "只读",
                "[INSTALL.zh_CN.md](INSTALL.zh_CN.md)",
            ),
        )
        self.assert_contains_all(
            install_doc,
            (
                "[README.zh_CN.md](README.zh_CN.md)",
                "sudo ./scripts/install.sh",
                "sudo ./scripts/install.sh --platform intel-nuc-ec-v9",
                "sudo modprobe platform_fans_hwmon",
                "sudo insmod platform_fans_hwmon.ko",
                "sudo rmmod platform_fans_hwmon",
            ),
        )
        self.assertNotIn("[INSTALL.md](INSTALL.md)", readme)
        self.assertNotIn("[README.md](README.md)", install_doc)

    def test_collect_platform_info_script_is_read_only(self):
        script = self.read("tools/collect-platform-info.sh")

        self.assert_contains_all(
            script,
            (
                "#!/bin/sh",
                "dmidecode",
                "/sys/class/hwmon",
                "/sys/bus/acpi/devices",
                "lspci",
                "lsmod",
                "dmesg",
            ),
        )
        forbidden = (" iowrite", " outb", " setpci -s", " wrmsr", " modprobe ")
        for item in forbidden:
            self.assertNotIn(item, script)


if __name__ == "__main__":
    unittest.main()
```

- [ ] **Step 2: Run the static tests and verify they fail**

Run:

```bash
cd /Users/timandes/Projects/fnos/nuc9-ec-hwmon
python3 -m unittest discover -s tests -q
```

Expected: FAIL. The first failures should mention missing `core/pfh-core.c`, `core/pfh-core.h`, `backends/pfh-ec-mmio.c`, `backends/pfh-ec-mmio.h`, or `platforms/intel/pfh-intel-nuc-ec-v9.c`.

- [ ] **Step 3: Commit the failing tests**

```bash
git add tests/test_static_project.py
git commit -m "test: define platform fans hwmon migration"
```

## Task 2: Add Core Interfaces And Build Metadata

**Files:**
- Create: `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/core/pfh-core.h`
- Modify: `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/Makefile`
- Modify: `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/dkms.conf`

- [ ] **Step 1: Create the shared core header**

Create `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/core/pfh-core.h`:

```c
/* SPDX-License-Identifier: GPL-2.0 */
#ifndef PFH_CORE_H
#define PFH_CORE_H

#include <linux/dmi.h>
#include <linux/hwmon.h>
#include <linux/platform_device.h>
#include <linux/types.h>

struct pfh_device;
struct pfh_fan_channel;
struct pfh_platform_desc;

struct pfh_backend_ops {
	int (*probe)(struct pfh_device *pfh);
	int (*read_fan)(struct pfh_device *pfh,
			const struct pfh_fan_channel *fan,
			long *rpm);
	void (*remove)(struct pfh_device *pfh);
};

struct pfh_fan_channel {
	const char *label;
	const void *read_config;
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

struct pfh_device {
	struct device *dev;
	struct device *hwmon_dev;
	const struct pfh_platform_desc *desc;
	void *backend_data;
};

extern const struct pfh_platform_desc pfh_intel_nuc_ec_v9;

#endif /* PFH_CORE_H */
```

- [ ] **Step 2: Update the Makefile**

Replace `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/Makefile` with:

```makefile
KERNEL_VERSION ?= $(shell uname -r)
KDIR ?= /lib/modules/$(KERNEL_VERSION)/build
PWD := $(shell pwd)

obj-m += platform_fans_hwmon.o

platform_fans_hwmon-y := \
	core/pfh-core.o \
	backends/pfh-ec-mmio.o \
	platforms/intel/pfh-intel-nuc-ec-v9.o

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

install:
	install -D -m 0644 platform_fans_hwmon.ko /lib/modules/$(KERNEL_VERSION)/extra/platform_fans_hwmon.ko
	depmod -a $(KERNEL_VERSION)

uninstall:
	rm -f /lib/modules/$(KERNEL_VERSION)/extra/platform_fans_hwmon.ko
	depmod -a $(KERNEL_VERSION)

.PHONY: all clean install uninstall
```

- [ ] **Step 3: Update DKMS metadata**

Replace `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/dkms.conf` with:

```bash
PACKAGE_NAME="platform-fans-hwmon"
PACKAGE_VERSION="0.2.0"
BUILT_MODULE_NAME[0]="platform_fans_hwmon"
DEST_MODULE_LOCATION[0]="/updates/dkms"
MAKE[0]="make KERNEL_VERSION=${kernelver}"
CLEAN="make clean KERNEL_VERSION=${kernelver}"
AUTOINSTALL="yes"
```

- [ ] **Step 4: Run tests and verify the expected partial failure**

Run:

```bash
cd /Users/timandes/Projects/fnos/nuc9-ec-hwmon
python3 -m unittest discover -s tests -q
```

Expected: FAIL. The missing header/Makefile/DKMS failures should be resolved. Failures should now focus on missing `core/pfh-core.c`, EC MMIO backend, NUC9 platform file, scripts, and docs.

- [ ] **Step 5: Commit**

```bash
git add core/pfh-core.h Makefile dkms.conf
git commit -m "refactor: add platform fans hwmon build identity"
```

## Task 3: Add EC MMIO Backend

**Files:**
- Create: `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/backends/pfh-ec-mmio.h`
- Create: `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/backends/pfh-ec-mmio.c`

- [ ] **Step 1: Create EC MMIO backend header**

Create `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/backends/pfh-ec-mmio.h`:

```c
/* SPDX-License-Identifier: GPL-2.0 */
#ifndef PFH_EC_MMIO_H
#define PFH_EC_MMIO_H

#include <linux/types.h>

#include "../core/pfh-core.h"

struct pfh_ec_mmio_identifier {
	u16 offset;
	const char *value;
};

struct pfh_ec_mmio_config {
	unsigned int lpc_bus;
	unsigned int lpc_dev;
	unsigned int lpc_func;
	u16 lgmr_offset;
	u32 lgmr_enable;
	u32 lgmr_base_mask;
	resource_size_t mmio_size;
	const struct pfh_ec_mmio_identifier *identifiers;
	int num_identifiers;
};

struct pfh_ec_mmio_fan {
	u16 rpm_offset;
};

extern const struct pfh_backend_ops pfh_ec_mmio_backend_ops;

#endif /* PFH_EC_MMIO_H */
```

- [ ] **Step 2: Create EC MMIO backend implementation**

Create `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/backends/pfh-ec-mmio.c`:

```c
// SPDX-License-Identifier: GPL-2.0

#include <linux/err.h>
#include <linux/io.h>
#include <linux/pci.h>
#include <linux/slab.h>
#include <linux/string.h>

#include "pfh-ec-mmio.h"

struct pfh_ec_mmio_data {
	void __iomem *ec_base;
	struct pci_dev *lpc;
};

static u16 pfh_ec_mmio_read_be16(void __iomem *base, u16 offset)
{
	u16 high = readb(base + offset);
	u16 low = readb(base + offset + 1);

	return (high << 8) | low;
}

static bool pfh_ec_mmio_identifier_valid(void __iomem *ec_base,
					 const struct pfh_ec_mmio_config *config)
{
	char id[16];
	int i;

	if (!config->identifiers || config->num_identifiers <= 0)
		return true;

	for (i = 0; i < config->num_identifiers; i++) {
		size_t len = strlen(config->identifiers[i].value);
		int j;

		if (len >= sizeof(id))
			return false;

		for (j = 0; j < len; j++)
			id[j] = readb(ec_base + config->identifiers[i].offset + j);
		id[len] = '\0';

		if (!strcmp(id, config->identifiers[i].value))
			return true;
	}

	return false;
}

static int pfh_ec_mmio_probe(struct pfh_device *pfh)
{
	const struct pfh_ec_mmio_config *config = pfh->desc->backend_config;
	struct pfh_ec_mmio_data *data;
	resource_size_t base;
	u32 lgmr;
	int ret;

	data = devm_kzalloc(pfh->dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	data->lpc = pci_get_domain_bus_and_slot(0, config->lpc_bus,
						PCI_DEVFN(config->lpc_dev,
							  config->lpc_func));
	if (!data->lpc) {
		dev_err(pfh->dev, "LPC device 0000:%02x:%02x.%u not found\n",
			config->lpc_bus, config->lpc_dev, config->lpc_func);
		return -ENODEV;
	}

	ret = pci_read_config_dword(data->lpc, config->lgmr_offset, &lgmr);
	if (ret) {
		dev_err(pfh->dev, "failed to read LGMR config register: %d\n", ret);
		goto out_put_lpc;
	}

	if (!(lgmr & config->lgmr_enable)) {
		dev_err(pfh->dev, "EC MMIO window is disabled: LGMR=0x%08x\n", lgmr);
		ret = -ENODEV;
		goto out_put_lpc;
	}

	base = lgmr & config->lgmr_base_mask;
	if (!base) {
		dev_err(pfh->dev, "EC MMIO base is zero: LGMR=0x%08x\n", lgmr);
		ret = -ENODEV;
		goto out_put_lpc;
	}

	data->ec_base = ioremap(base, config->mmio_size);
	if (!data->ec_base) {
		ret = -ENOMEM;
		goto out_put_lpc;
	}

	if (!pfh_ec_mmio_identifier_valid(data->ec_base, config)) {
		dev_err(pfh->dev, "unsupported EC identifier for platform %s\n",
			pfh->desc->id);
		ret = -ENODEV;
		goto out_unmap;
	}

	pfh->backend_data = data;
	dev_info(pfh->dev, "mapped EC MMIO at 0x%pa for platform %s\n",
		 &base, pfh->desc->id);
	return 0;

out_unmap:
	iounmap(data->ec_base);
	data->ec_base = NULL;
out_put_lpc:
	pci_dev_put(data->lpc);
	data->lpc = NULL;
	return ret;
}

static int pfh_ec_mmio_read_fan(struct pfh_device *pfh,
				const struct pfh_fan_channel *fan,
				long *rpm)
{
	struct pfh_ec_mmio_data *data = pfh->backend_data;
	const struct pfh_ec_mmio_fan *config = fan->read_config;

	if (!data || !data->ec_base || !config)
		return -ENODEV;

	*rpm = pfh_ec_mmio_read_be16(data->ec_base, config->rpm_offset);
	return 0;
}

static void pfh_ec_mmio_remove(struct pfh_device *pfh)
{
	struct pfh_ec_mmio_data *data = pfh->backend_data;

	if (!data)
		return;

	if (data->ec_base) {
		iounmap(data->ec_base);
		data->ec_base = NULL;
	}

	if (data->lpc) {
		pci_dev_put(data->lpc);
		data->lpc = NULL;
	}

	pfh->backend_data = NULL;
}

const struct pfh_backend_ops pfh_ec_mmio_backend_ops = {
	.probe = pfh_ec_mmio_probe,
	.read_fan = pfh_ec_mmio_read_fan,
	.remove = pfh_ec_mmio_remove,
};
```

- [ ] **Step 3: Run tests and verify EC backend expectations pass**

Run:

```bash
cd /Users/timandes/Projects/fnos/nuc9-ec-hwmon
python3 -m unittest discover -s tests -q
```

Expected: FAIL. EC backend-specific failures should be resolved. Remaining failures should focus on missing core implementation, NUC9 platform file, scripts, docs, and removal of the old monolithic source.

- [ ] **Step 4: Commit**

```bash
git add backends/pfh-ec-mmio.h backends/pfh-ec-mmio.c
git commit -m "feat: add ec mmio fan read backend"
```

## Task 4: Add NUC9 EC V9 Platform Descriptor

**Files:**
- Create: `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/platforms/intel/pfh-intel-nuc-ec-v9.c`

- [ ] **Step 1: Create Intel NUC9 platform descriptor**

Create `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/platforms/intel/pfh-intel-nuc-ec-v9.c`:

```c
// SPDX-License-Identifier: GPL-2.0
/*
 * Platform descriptor for Intel NUC9 EC V9 fan tachometers.
 */

#include <linux/dmi.h>
#include <linux/module.h>

#include "../../backends/pfh-ec-mmio.h"
#include "../../core/pfh-core.h"

#define NUC_EC_LPC_BUS 0
#define NUC_EC_LPC_DEV 31
#define NUC_EC_LPC_FUNC 0
#define NUC_EC_LGMR_OFFSET 0x98
#define NUC_EC_LGMR_ENABLE BIT(0)
#define NUC_EC_LGMR_BASE_MASK 0xFFFF0000U
#define NUC_EC_MMIO_SIZE 0x10000
#define NUC_EC_IDENTIFIER_OFFSET 0x400
#define NUC_EC_FAN1_RPM_OFFSET 0x41B
#define NUC_EC_FAN2_RPM_OFFSET 0x41E
#define NUC_EC_FAN3_RPM_OFFSET 0x421

static const struct dmi_system_id pfh_intel_nuc_ec_v9_dmi_table[] = {
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
MODULE_DEVICE_TABLE(dmi, pfh_intel_nuc_ec_v9_dmi_table);

static const struct pfh_ec_mmio_identifier pfh_intel_nuc_ec_v9_identifiers[] = {
	{ .offset = NUC_EC_IDENTIFIER_OFFSET, .value = "SPG_EC" },
	{ .offset = NUC_EC_IDENTIFIER_OFFSET, .value = "ELM_EC" },
};

static const struct pfh_ec_mmio_config pfh_intel_nuc_ec_v9_ec_config = {
	.lpc_bus = NUC_EC_LPC_BUS,
	.lpc_dev = NUC_EC_LPC_DEV,
	.lpc_func = NUC_EC_LPC_FUNC,
	.lgmr_offset = NUC_EC_LGMR_OFFSET,
	.lgmr_enable = NUC_EC_LGMR_ENABLE,
	.lgmr_base_mask = NUC_EC_LGMR_BASE_MASK,
	.mmio_size = NUC_EC_MMIO_SIZE,
	.identifiers = pfh_intel_nuc_ec_v9_identifiers,
	.num_identifiers = ARRAY_SIZE(pfh_intel_nuc_ec_v9_identifiers),
};

static const struct pfh_ec_mmio_fan pfh_intel_nuc_ec_v9_fan1 = {
	.rpm_offset = NUC_EC_FAN1_RPM_OFFSET,
};

static const struct pfh_ec_mmio_fan pfh_intel_nuc_ec_v9_fan2 = {
	.rpm_offset = NUC_EC_FAN2_RPM_OFFSET,
};

static const struct pfh_ec_mmio_fan pfh_intel_nuc_ec_v9_fan3 = {
	.rpm_offset = NUC_EC_FAN3_RPM_OFFSET,
};

static const struct pfh_fan_channel pfh_intel_nuc_ec_v9_fans[] = {
	{
		.label = "CPU Fan",
		.read_config = &pfh_intel_nuc_ec_v9_fan1,
	},
	{
		.label = "System Fan 1",
		.read_config = &pfh_intel_nuc_ec_v9_fan2,
	},
	{
		.label = "System Fan 2",
		.read_config = &pfh_intel_nuc_ec_v9_fan3,
	},
};

const struct pfh_platform_desc pfh_intel_nuc_ec_v9 = {
	.id = "intel-nuc-ec-v9",
	.hwmon_name = "intel_nuc_ec",
	.dmi_table = pfh_intel_nuc_ec_v9_dmi_table,
	.backend = &pfh_ec_mmio_backend_ops,
	.backend_config = &pfh_intel_nuc_ec_v9_ec_config,
	.fans = pfh_intel_nuc_ec_v9_fans,
	.num_fans = ARRAY_SIZE(pfh_intel_nuc_ec_v9_fans),
};
```

- [ ] **Step 2: Run tests and verify NUC9 platform expectations pass**

Run:

```bash
cd /Users/timandes/Projects/fnos/nuc9-ec-hwmon
python3 -m unittest discover -s tests -q
```

Expected: FAIL. NUC9 platform-specific failures should be resolved. Remaining failures should focus on missing core implementation, scripts, docs, and old monolithic source removal.

- [ ] **Step 3: Commit**

```bash
git add platforms/intel/pfh-intel-nuc-ec-v9.c
git commit -m "feat: add intel nuc ec v9 platform"
```

## Task 5: Implement Core hwmon Framework

**Files:**
- Create: `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/core/pfh-core.c`
- Delete: `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/intel_nuc_ec_hwmon.c`

- [ ] **Step 1: Create core implementation**

Create `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/core/pfh-core.c`:

```c
// SPDX-License-Identifier: GPL-2.0
/*
 * Read-only platform fan RPM hwmon framework.
 */

#include <linux/err.h>
#include <linux/hwmon.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/string.h>

#include "pfh-core.h"

#define DRIVER_NAME "platform_fans_hwmon"

static char *pfh_platform_param;
module_param_named(platform, pfh_platform_param, charp, 0444);
MODULE_PARM_DESC(platform, "Optional platform id to select, for example intel-nuc-ec-v9");

static const struct pfh_platform_desc * const pfh_platforms[] = {
	&pfh_intel_nuc_ec_v9,
	NULL
};

static const struct hwmon_channel_info *pfh_hwmon_info[2];
static struct hwmon_channel_info pfh_fan_channel_info;
static u32 pfh_fan_config[32];

static const struct pfh_platform_desc *pfh_match_platform(void)
{
	const struct pfh_platform_desc *desc;
	int i;

	for (i = 0; pfh_platforms[i]; i++) {
		desc = pfh_platforms[i];

		if (pfh_platform_param && strcmp(pfh_platform_param, desc->id))
			continue;

		if (desc->dmi_table && !dmi_check_system(desc->dmi_table))
			continue;

		if (desc->match && !desc->match(desc))
			continue;

		return desc;
	}

	return NULL;
}

static umode_t pfh_hwmon_is_visible(const void *drvdata,
				    enum hwmon_sensor_types type,
				    u32 attr, int channel)
{
	const struct pfh_device *pfh = drvdata;

	if (type != hwmon_fan)
		return 0;

	if (channel < 0 || channel >= pfh->desc->num_fans)
		return 0;

	switch (attr) {
	case hwmon_fan_input:
	case hwmon_fan_label:
		return 0444;
	default:
		return 0;
	}
}

static int pfh_hwmon_read(struct device *dev,
			  enum hwmon_sensor_types type,
			  u32 attr, int channel, long *val)
{
	struct pfh_device *pfh = dev_get_drvdata(dev);

	if (type != hwmon_fan || attr != hwmon_fan_input)
		return -EOPNOTSUPP;

	if (channel < 0 || channel >= pfh->desc->num_fans)
		return -EINVAL;

	return pfh->desc->backend->read_fan(pfh, &pfh->desc->fans[channel], val);
}

static int pfh_hwmon_read_string(struct device *dev,
				 enum hwmon_sensor_types type,
				 u32 attr, int channel, const char **str)
{
	struct pfh_device *pfh = dev_get_drvdata(dev);

	if (type != hwmon_fan || attr != hwmon_fan_label)
		return -EOPNOTSUPP;

	if (channel < 0 || channel >= pfh->desc->num_fans)
		return -EINVAL;

	*str = pfh->desc->fans[channel].label;
	return 0;
}

static const struct hwmon_ops pfh_hwmon_ops = {
	.is_visible = pfh_hwmon_is_visible,
	.read = pfh_hwmon_read,
	.read_string = pfh_hwmon_read_string,
};

static struct hwmon_chip_info pfh_hwmon_chip_info = {
	.ops = &pfh_hwmon_ops,
	.info = pfh_hwmon_info,
};

static int pfh_prepare_hwmon_info(const struct pfh_platform_desc *desc)
{
	int i;

	if (desc->num_fans <= 0 || desc->num_fans >= ARRAY_SIZE(pfh_fan_config))
		return -EINVAL;

	for (i = 0; i < desc->num_fans; i++)
		pfh_fan_config[i] = HWMON_F_INPUT | HWMON_F_LABEL;
	pfh_fan_config[desc->num_fans] = 0;

	pfh_fan_channel_info.type = hwmon_fan;
	pfh_fan_channel_info.config = pfh_fan_config;
	pfh_hwmon_info[0] = &pfh_fan_channel_info;
	pfh_hwmon_info[1] = NULL;

	return 0;
}

static int pfh_probe(struct platform_device *pdev)
{
	const struct pfh_platform_desc *desc;
	struct pfh_device *pfh;
	int ret;

	desc = pfh_match_platform();
	if (!desc) {
		if (pfh_platform_param)
			dev_err(&pdev->dev,
				"requested platform %s did not match this machine\n",
				pfh_platform_param);
		return -ENODEV;
	}

	if (!desc->backend || !desc->backend->probe || !desc->backend->read_fan)
		return -EINVAL;

	ret = pfh_prepare_hwmon_info(desc);
	if (ret)
		return ret;

	pfh = devm_kzalloc(&pdev->dev, sizeof(*pfh), GFP_KERNEL);
	if (!pfh)
		return -ENOMEM;

	pfh->dev = &pdev->dev;
	pfh->desc = desc;
	platform_set_drvdata(pdev, pfh);

	ret = desc->backend->probe(pfh);
	if (ret)
		return ret;

	pfh->hwmon_dev = devm_hwmon_device_register_with_info(
		&pdev->dev, desc->hwmon_name, pfh, &pfh_hwmon_chip_info, NULL);
	if (IS_ERR(pfh->hwmon_dev)) {
		ret = PTR_ERR(pfh->hwmon_dev);
		desc->backend->remove(pfh);
		return ret;
	}

	dev_info(&pdev->dev, "registered platform %s as hwmon %s\n",
		 desc->id, desc->hwmon_name);
	return 0;
}

static void pfh_remove(struct platform_device *pdev)
{
	struct pfh_device *pfh = platform_get_drvdata(pdev);

	if (pfh && pfh->desc && pfh->desc->backend && pfh->desc->backend->remove)
		pfh->desc->backend->remove(pfh);
}

static struct platform_driver pfh_driver = {
	.driver = {
		.name = DRIVER_NAME,
	},
	.probe = pfh_probe,
	.remove = pfh_remove,
};

static struct platform_device *pfh_pdev;

static int __init pfh_init(void)
{
	int ret;

	ret = platform_driver_register(&pfh_driver);
	if (ret)
		return ret;

	pfh_pdev = platform_device_register_simple(DRIVER_NAME,
						   PLATFORM_DEVID_NONE,
						   NULL, 0);
	if (IS_ERR(pfh_pdev)) {
		ret = PTR_ERR(pfh_pdev);
		platform_driver_unregister(&pfh_driver);
		return ret;
	}

	return 0;
}

static void __exit pfh_exit(void)
{
	platform_device_unregister(pfh_pdev);
	platform_driver_unregister(&pfh_driver);
}

module_init(pfh_init);
module_exit(pfh_exit);

MODULE_DESCRIPTION("Platform fan RPM hwmon framework");
MODULE_AUTHOR("Timandes White");
MODULE_LICENSE("GPL");
```

- [ ] **Step 2: Remove the old monolithic driver source**

Run:

```bash
cd /Users/timandes/Projects/fnos/nuc9-ec-hwmon
git rm intel_nuc_ec_hwmon.c
```

Expected: `rm 'intel_nuc_ec_hwmon.c'`.

- [ ] **Step 3: Run tests and verify source-layout tests pass**

Run:

```bash
cd /Users/timandes/Projects/fnos/nuc9-ec-hwmon
python3 -m unittest discover -s tests -q
```

Expected: FAIL. Core/layout/backend/platform tests should pass. Remaining failures should focus on scripts and docs.

- [ ] **Step 4: Run a kernel build if headers are available**

Run:

```bash
cd /Users/timandes/Projects/fnos/nuc9-ec-hwmon
make clean
make
```

Expected when kernel headers are installed: build produces `platform_fans_hwmon.ko`. If `/lib/modules/$(uname -r)/build` is missing, record the exact error and continue with static tests; runtime build validation will happen on the target NAS.

- [ ] **Step 5: Commit**

```bash
git add core/pfh-core.c
git commit -m "refactor: split platform fan hwmon core"
```

## Task 6: Update DKMS Install And Uninstall Scripts

**Files:**
- Modify: `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/scripts/install.sh`
- Modify: `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/scripts/uninstall.sh`

- [ ] **Step 1: Replace install script**

Replace `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/scripts/install.sh` with:

```sh
#!/bin/sh
set -eu

PACKAGE_NAME="platform-fans-hwmon"
PACKAGE_VERSION="0.2.0"
MODULE_NAME="platform_fans_hwmon"
SUPPORTED_PLATFORM="intel-nuc-ec-v9"
PFH_PLATFORM_FILTER=""

usage() {
  echo "Usage: $0 [--platform intel-nuc-ec-v9]" >&2
}

while [ "$#" -gt 0 ]; do
  case "$1" in
    --platform)
      if [ "$#" -lt 2 ]; then
        usage
        exit 2
      fi
      PFH_PLATFORM_FILTER="$2"
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      usage
      exit 2
      ;;
  esac
done

if [ -n "$PFH_PLATFORM_FILTER" ] && [ "$PFH_PLATFORM_FILTER" != "$SUPPORTED_PLATFORM" ]; then
  echo "Unsupported platform filter: $PFH_PLATFORM_FILTER" >&2
  echo "Supported platform filter: $SUPPORTED_PLATFORM" >&2
  exit 2
fi

command -v dkms >/dev/null 2>&1 || {
  echo "dkms is required" >&2
  exit 1
}

PROJECT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
SRC_DIR="/usr/src/${PACKAGE_NAME}-${PACKAGE_VERSION}"

if /usr/sbin/modinfo "$MODULE_NAME" >/dev/null 2>&1 || modinfo "$MODULE_NAME" >/dev/null 2>&1; then
  echo "Module $MODULE_NAME already exists; continuing with DKMS reinstall" >&2
fi

if dkms status -m "$PACKAGE_NAME" -v "$PACKAGE_VERSION" >/dev/null 2>&1; then
  dkms remove -m "$PACKAGE_NAME" -v "$PACKAGE_VERSION" --all || true
fi

rm -rf "$SRC_DIR"
mkdir -p "$SRC_DIR"
mkdir -p "$SRC_DIR/core" "$SRC_DIR/backends" "$SRC_DIR/platforms/intel"

cp "$PROJECT_DIR"/Makefile "$SRC_DIR"/
cp "$PROJECT_DIR"/dkms.conf "$SRC_DIR"/
cp "$PROJECT_DIR"/core/pfh-core.c "$SRC_DIR"/core/
cp "$PROJECT_DIR"/core/pfh-core.h "$SRC_DIR"/core/
cp "$PROJECT_DIR"/backends/pfh-ec-mmio.c "$SRC_DIR"/backends/
cp "$PROJECT_DIR"/backends/pfh-ec-mmio.h "$SRC_DIR"/backends/
cp "$PROJECT_DIR"/platforms/intel/pfh-intel-nuc-ec-v9.c "$SRC_DIR"/platforms/intel/

dkms add -m "$PACKAGE_NAME" -v "$PACKAGE_VERSION"
dkms build -m "$PACKAGE_NAME" -v "$PACKAGE_VERSION"
dkms install -m "$PACKAGE_NAME" -v "$PACKAGE_VERSION"
depmod -a

if [ -n "$PFH_PLATFORM_FILTER" ]; then
  modprobe "$MODULE_NAME" platform="$PFH_PLATFORM_FILTER"
else
  modprobe "$MODULE_NAME"
fi
```

- [ ] **Step 2: Replace uninstall script**

Replace `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/scripts/uninstall.sh` with:

```sh
#!/bin/sh
set -eu

PACKAGE_NAME="platform-fans-hwmon"
PACKAGE_VERSION="0.2.0"
MODULE_NAME="platform_fans_hwmon"
SRC_DIR="/usr/src/${PACKAGE_NAME}-${PACKAGE_VERSION}"

if lsmod | awk '{print $1}' | grep -qx "$MODULE_NAME"; then
  modprobe -r "$MODULE_NAME"
fi

if command -v dkms >/dev/null 2>&1; then
  if dkms status -m "$PACKAGE_NAME" -v "$PACKAGE_VERSION" >/dev/null 2>&1; then
    dkms remove -m "$PACKAGE_NAME" -v "$PACKAGE_VERSION" --all
  fi
fi

rm -rf "$SRC_DIR"
depmod -a
```

- [ ] **Step 3: Verify shell syntax**

Run:

```bash
cd /Users/timandes/Projects/fnos/nuc9-ec-hwmon
sh -n scripts/install.sh
sh -n scripts/uninstall.sh
```

Expected: both commands exit 0 with no output.

- [ ] **Step 4: Run static tests**

Run:

```bash
cd /Users/timandes/Projects/fnos/nuc9-ec-hwmon
python3 -m unittest discover -s tests -q
```

Expected: FAIL. Script failures should be resolved. Remaining failures should focus on documentation and collection tool.

- [ ] **Step 5: Commit**

```bash
git add scripts/install.sh scripts/uninstall.sh
git commit -m "feat: update dkms scripts for platform fans hwmon"
```

## Task 7: Add Contribution Docs And Platform Collection Tool

**Files:**
- Create: `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/docs/contributing-platform.md`
- Create: `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/docs/platform-template.md`
- Create: `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/docs/supported-platforms.md`
- Create: `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/tools/collect-platform-info.sh`

- [ ] **Step 1: Create platform contribution guide**

Create `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/docs/contributing-platform.md`:

```markdown
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
```

- [ ] **Step 2: Create platform template**

Create `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/docs/platform-template.md`:

```markdown
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
```

- [ ] **Step 3: Create supported platform matrix**

Create `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/docs/supported-platforms.md`:

```markdown
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
```

- [ ] **Step 4: Create read-only collection tool**

Create `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/tools/collect-platform-info.sh`:

```sh
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
```

- [ ] **Step 5: Make collection tool executable and check syntax**

Run:

```bash
cd /Users/timandes/Projects/fnos/nuc9-ec-hwmon
chmod +x tools/collect-platform-info.sh
sh -n tools/collect-platform-info.sh
```

Expected: `sh -n` exits 0 with no output.

- [ ] **Step 6: Run static tests**

Run:

```bash
cd /Users/timandes/Projects/fnos/nuc9-ec-hwmon
python3 -m unittest discover -s tests -q
```

Expected: FAIL. Contribution-doc and tool failures should be resolved. Remaining failures should focus on README and INSTALL docs.

- [ ] **Step 7: Commit**

```bash
git add docs/contributing-platform.md docs/platform-template.md docs/supported-platforms.md tools/collect-platform-info.sh
git commit -m "docs: add platform contribution workflow"
```

## Task 8: Update User Documentation

**Files:**
- Modify: `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/README.md`
- Modify: `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/README.zh_CN.md`
- Modify: `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/INSTALL.md`
- Modify: `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/INSTALL.zh_CN.md`

- [ ] **Step 1: Replace English README**

Replace `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/README.md` with:

```markdown
# Platform Fans hwmon

`platform-fans-hwmon` is a read-only Linux hwmon framework for platform-specific fan RPM sensors that are not already exposed by existing kernel drivers.

The kernel module is `platform_fans_hwmon`. It provides a common hwmon core and platform-specific descriptors. The first supported platform is Intel NUC9 EC V9, exported as hwmon name `intel_nuc_ec`.

This project exposes fan RPM values through the standard hwmon ABI so tools such as `sensors`, CoolerControl, and monitoring agents can read `fan*_input` and `fan*_label`.

## Supported Platforms

See [docs/supported-platforms.md](docs/supported-platforms.md).

Current platform:

- `intel-nuc-ec-v9`
  - Intel NUC9 Extreme Compute Element / Kit
  - Intel NUC9 Pro Compute Element / Kit
  - EC identifiers: `SPG_EC` or `ELM_EC`
  - hwmon name: `intel_nuc_ec`

## Intel NUC9 EC V9 Sensors

```text
fan1_label = CPU Fan
fan1_input = EC V9 register 0x41B

fan2_label = System Fan 1
fan2_input = EC V9 register 0x41E

fan3_label = System Fan 2
fan3_input = EC V9 register 0x421
```

## Install

See [INSTALL.md](INSTALL.md).

Default DKMS install:

```bash
sudo ./scripts/install.sh
sudo modprobe platform_fans_hwmon
```

Platform-scoped install:

```bash
sudo ./scripts/install.sh --platform intel-nuc-ec-v9
```

## Contributing Platform Support

See [docs/contributing-platform.md](docs/contributing-platform.md) and [docs/platform-template.md](docs/platform-template.md).

Platform support must be explicitly matched and validated on real hardware. The driver does not perform speculative scanning of unknown EC, MMIO, or IO-port ranges.

## Safety

Default platform support is read-only. This module does not expose PWM controls, does not change fan curves, and does not write fan-control registers.

Fan control is a separate risk category and is not part of the default module.

## Chinese Documentation

- [README.zh_CN.md](README.zh_CN.md)
- [INSTALL.zh_CN.md](INSTALL.zh_CN.md)

## License

This project is licensed under the GNU General Public License version 2.
```

- [ ] **Step 2: Replace Chinese README**

Replace `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/README.zh_CN.md` with:

```markdown
# Platform Fans hwmon

`platform-fans-hwmon` 是一个只读 Linux hwmon 框架，用于暴露现有内核驱动尚未支持的平台特定风扇转速传感器。

内核模块名为 `platform_fans_hwmon`。它提供通用 hwmon core 和平台特定描述。第一个支持的平台是 Intel NUC9 EC V9，导出的 hwmon name 保持为 `intel_nuc_ec`。

本项目通过标准 hwmon ABI 暴露风扇 RPM，使 `sensors`、CoolerControl 和监控系统可以读取 `fan*_input` 与 `fan*_label`。

## 支持的平台

见 [docs/supported-platforms.md](docs/supported-platforms.md)。

当前平台：

- `intel-nuc-ec-v9`
  - Intel NUC9 Extreme Compute Element / Kit
  - Intel NUC9 Pro Compute Element / Kit
  - EC identifier：`SPG_EC` 或 `ELM_EC`
  - hwmon name：`intel_nuc_ec`

## Intel NUC9 EC V9 传感器

```text
fan1_label = CPU Fan
fan1_input = EC V9 register 0x41B

fan2_label = System Fan 1
fan2_input = EC V9 register 0x41E

fan3_label = System Fan 2
fan3_input = EC V9 register 0x421
```

## 安装

见 [INSTALL.zh_CN.md](INSTALL.zh_CN.md)。

默认 DKMS 安装：

```bash
sudo ./scripts/install.sh
sudo modprobe platform_fans_hwmon
```

限定平台安装：

```bash
sudo ./scripts/install.sh --platform intel-nuc-ec-v9
```

## 贡献平台支持

见 [docs/contributing-platform.md](docs/contributing-platform.md) 和 [docs/platform-template.md](docs/platform-template.md)。

平台支持必须有明确匹配条件，并在真实硬件上验证。本驱动不会扫描未知 EC、MMIO 或 IO port 范围来猜测风扇转速。

## 安全

默认平台支持是只读的。本模块不暴露 PWM 控制，不修改风扇曲线，也不写入风扇控制寄存器。

风扇控制属于另一类风险，不属于默认模块范围。

## 许可证

本项目使用 GNU General Public License version 2。
```

- [ ] **Step 3: Replace English install doc**

Replace `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/INSTALL.md` with:

```markdown
# Install

Back to [README.md](README.md). Chinese version: [INSTALL.zh_CN.md](INSTALL.zh_CN.md).

## Dependencies

```bash
sudo apt update
sudo apt install -y gcc make dkms "linux-headers-$(uname -r)"
```

On minimal Debian or fnOS shells, `modinfo` may live outside the default user `PATH`. Use `/usr/sbin/modinfo` when needed.

## DKMS Install

Default install:

```bash
sudo ./scripts/install.sh
sudo modprobe platform_fans_hwmon
```

Platform-scoped install for the current first platform:

```bash
sudo ./scripts/install.sh --platform intel-nuc-ec-v9
```

Diagnostic runtime platform selection:

```bash
sudo modprobe platform_fans_hwmon platform=intel-nuc-ec-v9
```

The `platform=` parameter still requires the platform identity checks to pass.

## Validate

```bash
/usr/sbin/modinfo platform_fans_hwmon
sensors | sed -n '/intel_nuc_ec/,/^$/p'
```

Find the hwmon device:

```bash
for h in /sys/class/hwmon/hwmon*; do
  [ "$(cat "$h/name" 2>/dev/null)" = "intel_nuc_ec" ] || continue
  echo "hwmon=$h"
  cat "$h"/fan*_label
  cat "$h"/fan*_input
done
```

Expected Intel NUC9 labels:

```text
CPU Fan
System Fan 1
System Fan 2
```

## DKMS Uninstall

```bash
sudo ./scripts/uninstall.sh
```

## Manual Build And Test Load

```bash
make
sudo insmod platform_fans_hwmon.ko
sensors | sed -n '/intel_nuc_ec/,/^$/p'
sudo rmmod platform_fans_hwmon
```

## Manual Cleanup

```bash
make clean
```
```

- [ ] **Step 4: Replace Chinese install doc**

Replace `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/INSTALL.zh_CN.md` with:

```markdown
# 安装

返回 [README.zh_CN.md](README.zh_CN.md)。英文版：[INSTALL.md](INSTALL.md)。

## 安装依赖

```bash
sudo apt update
sudo apt install -y gcc make dkms "linux-headers-$(uname -r)"
```

在最小 Debian 或 fnOS shell 中，`modinfo` 可能不在默认用户 `PATH` 中。需要时使用 `/usr/sbin/modinfo`。

## DKMS 安装

默认安装：

```bash
sudo ./scripts/install.sh
sudo modprobe platform_fans_hwmon
```

限定当前第一个平台安装：

```bash
sudo ./scripts/install.sh --platform intel-nuc-ec-v9
```

诊断时指定运行平台：

```bash
sudo modprobe platform_fans_hwmon platform=intel-nuc-ec-v9
```

`platform=` 参数默认仍要求平台身份检查通过。

## 验证

```bash
/usr/sbin/modinfo platform_fans_hwmon
sensors | sed -n '/intel_nuc_ec/,/^$/p'
```

查找 hwmon 设备：

```bash
for h in /sys/class/hwmon/hwmon*; do
  [ "$(cat "$h/name" 2>/dev/null)" = "intel_nuc_ec" ] || continue
  echo "hwmon=$h"
  cat "$h"/fan*_label
  cat "$h"/fan*_input
done
```

Intel NUC9 预期标签：

```text
CPU Fan
System Fan 1
System Fan 2
```

## DKMS 卸载

```bash
sudo ./scripts/uninstall.sh
```

## 手动构建和测试加载

```bash
make
sudo insmod platform_fans_hwmon.ko
sensors | sed -n '/intel_nuc_ec/,/^$/p'
sudo rmmod platform_fans_hwmon
```

## 手动清理

```bash
make clean
```
```

- [ ] **Step 5: Run static tests**

Run:

```bash
cd /Users/timandes/Projects/fnos/nuc9-ec-hwmon
python3 -m unittest discover -s tests -q
```

Expected: PASS with 0 failures.

- [ ] **Step 6: Commit**

```bash
git add README.md README.zh_CN.md INSTALL.md INSTALL.zh_CN.md
git commit -m "docs: document platform fans hwmon usage"
```

## Task 9: Build And Runtime Validation On Target

**Files:**
- No source edits unless validation exposes a compile or runtime bug.

- [ ] **Step 1: Run final static tests locally**

Run:

```bash
cd /Users/timandes/Projects/fnos/nuc9-ec-hwmon
python3 -m unittest discover -s tests -q
```

Expected: PASS with output similar to:

```text
----------------------------------------------------------------------
Ran 14 tests in 0.001s

OK
```

- [ ] **Step 2: Run shell syntax checks**

Run:

```bash
cd /Users/timandes/Projects/fnos/nuc9-ec-hwmon
sh -n scripts/install.sh
sh -n scripts/uninstall.sh
sh -n tools/collect-platform-info.sh
```

Expected: all commands exit 0 with no output.

- [ ] **Step 3: Run local build when kernel headers are available**

Run:

```bash
cd /Users/timandes/Projects/fnos/nuc9-ec-hwmon
make clean
make
```

Expected when headers are available: `platform_fans_hwmon.ko` is produced. If local macOS or missing Linux headers prevent this, skip local build and perform target build in Step 4.

- [ ] **Step 4: Copy to NAS-11 and build**

Run:

```bash
rsync -av --delete \
  /Users/timandes/Projects/fnos/nuc9-ec-hwmon/ \
  nas-11.timandes.net:/tmp/platform-fans-hwmon/

ssh nas-11.timandes.net 'cd /tmp/platform-fans-hwmon && make clean && make'
```

Expected: `platform_fans_hwmon.ko` is produced without compiler errors.

- [ ] **Step 5: Load manually and verify hwmon output**

Run:

```bash
ssh -t nas-11.timandes.net 'cd /tmp/platform-fans-hwmon && sudo insmod platform_fans_hwmon.ko && for h in /sys/class/hwmon/hwmon*; do [ "$(cat "$h/name" 2>/dev/null)" = "intel_nuc_ec" ] || continue; echo "hwmon=$h"; cat "$h"/fan*_label; cat "$h"/fan*_input; done; sensors | sed -n "/intel_nuc_ec/,/^$/p"; sudo rmmod platform_fans_hwmon'
```

Expected:

```text
hwmon=/sys/class/hwmon/hwmonN
CPU Fan
System Fan 1
System Fan 2
<non-zero fan1 RPM>
<non-zero fan2 RPM>
<non-zero fan3 RPM>
intel_nuc_ec-isa-0000
```

RPM values may differ from previous documentation because fan speed changes over time.

- [ ] **Step 6: Validate DKMS install and uninstall**

Run:

```bash
ssh -t nas-11.timandes.net 'cd /tmp/platform-fans-hwmon && sudo ./scripts/install.sh --platform intel-nuc-ec-v9 && /usr/sbin/modinfo platform_fans_hwmon && sensors | sed -n "/intel_nuc_ec/,/^$/p"'
ssh -t nas-11.timandes.net 'cd /tmp/platform-fans-hwmon && sudo ./scripts/uninstall.sh && ! lsmod | grep -q platform_fans_hwmon && echo unloaded'
```

Expected:

```text
filename: /lib/modules/<kernel>/updates/dkms/platform_fans_hwmon.ko
intel_nuc_ec-isa-0000
unloaded
```

- [ ] **Step 7: Commit validation doc updates if observed output differs**

If runtime output differs only in paths, adapter label, or RPM values, update README/INSTALL examples with the exact observed stable output and commit:

```bash
git add README.md README.zh_CN.md INSTALL.md INSTALL.zh_CN.md docs/supported-platforms.md
git commit -m "docs: update platform fans hwmon validation output"
```

If no doc update is needed, do not create an empty commit.

## Task 10: Final Review And Integration Prep

**Files:**
- No planned source edits.

- [ ] **Step 1: Inspect final diff**

Run:

```bash
cd /Users/timandes/Projects/fnos/nuc9-ec-hwmon
git status --short --branch
git log --oneline --decorate --max-count=12
git diff --stat origin/main...HEAD
```

Expected: branch contains the migration commits and no uncommitted files.

- [ ] **Step 2: Search for stale names**

Run:

```bash
cd /Users/timandes/Projects/fnos/nuc9-ec-hwmon
rg -n "intel-nuc-ec-hwmon|intel_nuc_ec_hwmon|nuc9-ec-hwmon" . || true
```

Expected: no stale package/module names remain. The string `intel_nuc_ec` may remain because it is the intended NUC9 hwmon device name.

- [ ] **Step 3: Search for forbidden commit trailers**

Run:

```bash
cd /Users/timandes/Projects/fnos/nuc9-ec-hwmon
git log --format=%B origin/main..HEAD | rg -n "Co-Authored-By" && exit 1 || true
```

Expected: no output and exit 0.

- [ ] **Step 4: Run final verification**

Run:

```bash
cd /Users/timandes/Projects/fnos/nuc9-ec-hwmon
python3 -m unittest discover -s tests -q
sh -n scripts/install.sh
sh -n scripts/uninstall.sh
sh -n tools/collect-platform-info.sh
```

Expected: tests pass and shell checks produce no output.

- [ ] **Step 5: Prepare merge or PR**

If the user wants a merge, use a non-interactive merge from the main checkout after confirming there are no unrelated working tree changes that would be overwritten:

```bash
cd /Users/timandes/Projects/fnos/nuc9-ec-hwmon
git status --short --branch
git merge --no-ff feature/platform-fans-hwmon-design -m "feat: generalize platform fans hwmon"
```

If the user wants a PR branch, push the feature branch without rewriting history:

```bash
cd /Users/timandes/Projects/fnos/nuc9-ec-hwmon/.worktrees/feature-platform-fans-hwmon-design
git push -u origin feature/platform-fans-hwmon-design
```

Do not include `Co-Authored-By` in any commit message.

## Self-Review

- Spec coverage: The plan covers generalized naming, core/backend/platform split, NUC9 behavior preservation, safe runtime matching, read-only default support, platform-scoped install, contribution docs, supported platform docs, and validation.
- Placeholder scan: The plan contains no placeholder markers or unspecified implementation steps.
- Type consistency: The plan uses `pfh_backend_ops`, `pfh_fan_channel`, `pfh_platform_desc`, `pfh_device`, `pfh_ec_mmio_config`, and `pfh_ec_mmio_fan` consistently across the header, backend, platform descriptor, and core implementation.
