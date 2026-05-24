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
#include <linux/string.h>

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
#define NUC_EC_FAN1_RPM_OFFSET 0x41B
#define NUC_EC_FAN2_RPM_OFFSET 0x41E
#define NUC_EC_FAN3_RPM_OFFSET 0x421

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
	resource_size_t base;
	u32 lgmr;
	int ret;

	data->lpc = pci_get_domain_bus_and_slot(0, NUC_EC_LPC_BUS,
						PCI_DEVFN(NUC_EC_LPC_DEV,
							  NUC_EC_LPC_FUNC));
	if (!data->lpc) {
		dev_err(dev, "LPC device 0000:00:1f.0 not found\n");
		return -ENODEV;
	}

	ret = pci_read_config_dword(data->lpc, NUC_EC_LGMR_OFFSET, &lgmr);
	if (ret) {
		dev_err(dev, "failed to read LGMR config register: %d\n", ret);
		goto out_put_lpc;
	}

	if (!(lgmr & NUC_EC_LGMR_ENABLE)) {
		dev_err(dev, "EC MMIO window is disabled: LGMR=0x%08x\n", lgmr);
		ret = -ENODEV;
		goto out_put_lpc;
	}

	base = lgmr & NUC_EC_LGMR_BASE_MASK;
	if (!base) {
		dev_err(dev, "EC MMIO base is zero: LGMR=0x%08x\n", lgmr);
		ret = -ENODEV;
		goto out_put_lpc;
	}

	data->ec_base = ioremap(base, NUC_EC_MMIO_SIZE);
	if (!data->ec_base) {
		ret = -ENOMEM;
		goto out_put_lpc;
	}

	if (!nuc_ec_identifier_valid(data->ec_base)) {
		dev_err(dev, "unsupported Intel NUC EC identifier\n");
		ret = -ENODEV;
		goto out_unmap;
	}

	dev_info(dev, "mapped EC MMIO at 0x%pa\n", &base);
	return 0;

out_unmap:
	iounmap(data->ec_base);
	data->ec_base = NULL;
out_put_lpc:
	pci_dev_put(data->lpc);
	data->lpc = NULL;
	return ret;
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

static int nuc_ec_hwmon_read(struct device *dev,
			     enum hwmon_sensor_types type,
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
				    u32 attr, int channel, const char **str)
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

	data->hwmon_dev = devm_hwmon_device_register_with_info(
		&pdev->dev, HWMON_NAME, data, &nuc_ec_hwmon_chip_info, NULL);
	if (IS_ERR(data->hwmon_dev)) {
		ret = PTR_ERR(data->hwmon_dev);
		nuc_ec_unmap_mmio(data);
		return ret;
	}

	dev_info(&pdev->dev, "Intel NUC EC V9 hwmon registered\n");
	return 0;
}

static void nuc_ec_remove(struct platform_device *pdev)
{
	struct nuc_ec_data *data = platform_get_drvdata(pdev);

	nuc_ec_unmap_mmio(data);
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

	nuc_ec_pdev = platform_device_register_simple(DRIVER_NAME,
						      PLATFORM_DEVID_NONE,
						      NULL, 0);
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
