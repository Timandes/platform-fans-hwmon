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
	&pfh_ds2308_it8613e,
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
	struct pfh_device *pfh = (struct pfh_device *)drvdata;

	if (type != hwmon_fan)
		return 0;

	if (channel < 0 || channel >= pfh->desc->num_fans)
		return 0;

	if (pfh->desc->backend->fan_visible &&
	    !pfh->desc->backend->fan_visible(pfh, &pfh->desc->fans[channel]))
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
	struct device *dev = &pdev->dev;
	const struct pfh_platform_desc *desc;
	struct pfh_device *pfh;
	int ret;

	desc = pfh_match_platform();
	if (!desc) {
		if (pfh_platform_param)
			dev_err(dev, "requested platform %s did not match this machine\n",
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
