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
	bool (*fan_visible)(struct pfh_device *pfh,
			    const struct pfh_fan_channel *fan);
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
extern const struct pfh_platform_desc pfh_ds2308_it8613e;

#endif /* PFH_CORE_H */
