import pathlib
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[1]


class StaticProjectTests(unittest.TestCase):
    def read(self, relative_path):
        path = ROOT / relative_path
        self.assertTrue(path.exists(), f"{relative_path} should exist")
        return path.read_text()

    def test_module_skeleton_declares_expected_identity(self):
        source = self.read("intel_nuc_ec_hwmon.c")

        self.assertIn('#define DRIVER_NAME "intel_nuc_ec_hwmon"', source)
        self.assertIn('#define HWMON_NAME "intel_nuc_ec"', source)
        self.assertIn('MODULE_AUTHOR("Timandes White")', source)
        self.assertIn('MODULE_LICENSE("GPL")', source)

    def test_project_declares_gpl_2_license(self):
        license_text = self.read("LICENSE")

        self.assertIn("GNU GENERAL PUBLIC LICENSE", license_text)
        self.assertIn("Version 2, June 1991", license_text)
        self.assertIn("Copyright", license_text)

    def test_makefile_builds_expected_module(self):
        makefile = self.read("Makefile")

        self.assertIn("obj-m += intel_nuc_ec_hwmon.o", makefile)
        self.assertIn("KDIR ?= /lib/modules/$(KERNEL_VERSION)/build", makefile)
        self.assertIn("install -D -m 0644 intel_nuc_ec_hwmon.ko", makefile)

    def test_driver_has_nuc9_dmi_guard(self):
        source = self.read("intel_nuc_ec_hwmon.c")

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
            self.assertIn(model, source)

        self.assertIn("DMI_MATCH(DMI_BOARD_NAME", source)
        self.assertIn("DMI_MATCH(DMI_PRODUCT_NAME", source)
        self.assertIn("dmi_check_system(nuc_ec_dmi_table)", source)

    def test_driver_registers_platform_device_and_driver(self):
        source = self.read("intel_nuc_ec_hwmon.c")

        self.assertIn("static struct platform_driver nuc_ec_driver", source)
        self.assertIn("platform_driver_register(&nuc_ec_driver)", source)
        self.assertIn("platform_device_register_simple(DRIVER_NAME", source)
        self.assertIn("platform_device_unregister(nuc_ec_pdev)", source)
        self.assertIn("platform_driver_unregister(&nuc_ec_driver)", source)

    def test_driver_defines_ec_mmio_constants(self):
        source = self.read("intel_nuc_ec_hwmon.c")

        for constant in (
            "#define NUC_EC_LPC_BUS 0",
            "#define NUC_EC_LPC_DEV 31",
            "#define NUC_EC_LPC_FUNC 0",
            "#define NUC_EC_LGMR_OFFSET 0x98",
            "#define NUC_EC_LGMR_ENABLE BIT(0)",
            "#define NUC_EC_LGMR_BASE_MASK 0xFFFF0000U",
            "#define NUC_EC_MMIO_SIZE 0x10000",
            "#define NUC_EC_IDENTIFIER_OFFSET 0x400",
        ):
            self.assertIn(constant, source)

    def test_driver_maps_lpc_ec_mmio_and_validates_identifier(self):
        source = self.read("intel_nuc_ec_hwmon.c")

        self.assertIn("pci_get_domain_bus_and_slot(0, NUC_EC_LPC_BUS", source)
        self.assertIn("pci_read_config_dword(data->lpc, NUC_EC_LGMR_OFFSET", source)
        self.assertIn("lgmr & NUC_EC_LGMR_ENABLE", source)
        self.assertIn("lgmr & NUC_EC_LGMR_BASE_MASK", source)
        self.assertIn("ioremap(base, NUC_EC_MMIO_SIZE)", source)
        self.assertIn('!strcmp(id, "SPG_EC") || !strcmp(id, "ELM_EC")', source)
        self.assertIn("nuc_ec_unmap_mmio(data)", source)
        self.assertIn("pci_dev_put(data->lpc)", source)

    def test_driver_exposes_three_read_only_hwmon_fans(self):
        source = self.read("intel_nuc_ec_hwmon.c")

        for constant in (
            "#define NUC_EC_FAN1_RPM_OFFSET 0x41B",
            "#define NUC_EC_FAN2_RPM_OFFSET 0x41E",
            "#define NUC_EC_FAN3_RPM_OFFSET 0x421",
        ):
            self.assertIn(constant, source)

        for label in ("CPU Fan", "System Fan 1", "System Fan 2"):
            self.assertIn(f'"{label}"', source)

        self.assertIn("static const struct hwmon_ops nuc_ec_hwmon_ops", source)
        self.assertIn("hwmon_fan_input", source)
        self.assertIn("hwmon_fan_label", source)
        self.assertIn("return 0444", source)
        self.assertIn("devm_hwmon_device_register_with_info", source)
        self.assertIn("nuc_ec_read_be16(data->ec_base", source)

    def test_dkms_metadata_targets_expected_module(self):
        dkms = self.read("dkms.conf")

        self.assertIn('PACKAGE_NAME="intel-nuc-ec-hwmon"', dkms)
        self.assertIn('PACKAGE_VERSION="0.1.0"', dkms)
        self.assertIn('BUILT_MODULE_NAME[0]="intel_nuc_ec_hwmon"', dkms)
        self.assertIn('MAKE[0]="make KERNEL_VERSION=${kernelver}"', dkms)
        self.assertIn('AUTOINSTALL="yes"', dkms)

    def test_install_scripts_manage_dkms_package(self):
        install = self.read("scripts/install.sh")
        uninstall = self.read("scripts/uninstall.sh")

        self.assertIn('PACKAGE_NAME="intel-nuc-ec-hwmon"', install)
        self.assertIn('MODULE_NAME="intel_nuc_ec_hwmon"', install)
        self.assertIn("dkms add", install)
        self.assertIn("dkms build", install)
        self.assertIn("dkms install", install)
        self.assertIn('modprobe "$MODULE_NAME"', install)
        self.assertIn("dkms remove", uninstall)
        self.assertIn('modprobe -r "$MODULE_NAME"', uninstall)

    def test_readme_documents_scope_and_safety(self):
        readme = self.read("README.md")

        for text in (
            "Intel NUC9 EC V9",
            "fan1_input",
            "fan2_input",
            "fan3_input",
            "SPG_EC",
            "ELM_EC",
            "read-only",
            "does not expose PWM controls",
        ):
            self.assertIn(text, readme)

        self.assertIn("[INSTALL.md](INSTALL.md)", readme)
        self.assertIn("[README.zh_CN.md](README.zh_CN.md)", readme)

    def test_install_document_covers_install_and_uninstall(self):
        install_doc = self.read("INSTALL.md")

        for text in (
            "sudo apt install -y gcc make dkms",
            "sudo ./scripts/install.sh",
            "sudo ./scripts/uninstall.sh",
            "make",
            "sudo insmod intel_nuc_ec_hwmon.ko",
            "sudo rmmod intel_nuc_ec_hwmon",
            "/usr/sbin/modinfo intel_nuc_ec_hwmon",
            "sensors | sed -n '/intel_nuc_ec/,/^$/p'",
        ):
            self.assertIn(text, install_doc)

        self.assertIn("[INSTALL.zh_CN.md](INSTALL.zh_CN.md)", install_doc)

    def test_zh_cn_readme_is_complete_and_links_to_zh_cn_install(self):
        readme = self.read("README.zh_CN.md")

        for text in (
            "只读 Linux hwmon 驱动",
            "支持的硬件",
            "暴露的传感器",
            "fan1_input",
            "SPG_EC",
            "ELM_EC",
            "[INSTALL.zh_CN.md](INSTALL.zh_CN.md)",
            "GNU General Public License version 2",
        ):
            self.assertIn(text, readme)

        self.assertNotIn("[INSTALL.md](INSTALL.md)", readme)

    def test_zh_cn_install_is_complete_and_links_to_zh_cn_readme(self):
        install_doc = self.read("INSTALL.zh_CN.md")

        for text in (
            "[README.zh_CN.md](README.zh_CN.md)",
            "安装依赖",
            "DKMS 安装",
            "DKMS 卸载",
            "手动构建和测试加载",
            "手动清理",
            "sudo ./scripts/install.sh",
            "sudo ./scripts/uninstall.sh",
            "sudo insmod intel_nuc_ec_hwmon.ko",
            "sudo rmmod intel_nuc_ec_hwmon",
        ):
            self.assertIn(text, install_doc)

        self.assertNotIn("[README.md](README.md)", install_doc)

    def test_changelog_documents_initial_release(self):
        changelog = self.read("CHANGELOG.md")

        for text in (
            "# Changelog",
            "## [0.1.0]",
            "Initial release",
            "Intel NUC9 EC V9",
            "fan1_input",
            "DKMS",
            "GPL-2.0",
        ):
            self.assertIn(text, changelog)


if __name__ == "__main__":
    unittest.main()
