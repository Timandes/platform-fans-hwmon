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

    def test_release_version_is_0_2_0_in_code_and_changelog(self):
        changelog = self.read("CHANGELOG.md")
        dkms = self.read("dkms.conf")
        install = self.read("scripts/install.sh")
        uninstall = self.read("scripts/uninstall.sh")

        self.assert_contains_all(
            changelog,
            (
                "## [0.2.0]",
                "Platform Fans hwmon",
                "platform_fans_hwmon",
                "intel-nuc-ec-v9",
                "EC MMIO backend",
                "platform-scoped install",
                "Runtime validated on NAS-11",
            ),
        )

        for text in (dkms, install, uninstall):
            self.assertIn('PACKAGE_VERSION="0.2.0"', text)
            self.assertNotIn('PACKAGE_VERSION="0.1.0"', text)

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
                "const struct pfh_platform_desc pfh_intel_nuc_ec_v9",
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
