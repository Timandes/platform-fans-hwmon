// SPDX-License-Identifier: GPL-2.0
/*
 * Platform descriptor for DS2308 IT8613E fan tachometers.
 */

#include <linux/array_size.h>
#include <linux/dmi.h>
#include <linux/module.h>

#include "../../backends/pfh-it8613e-sio.h"
#include "../../core/pfh-core.h"

static const struct dmi_system_id pfh_ds2308_it8613e_dmi_table[] = {
	{
		.matches = {
			DMI_MATCH(DMI_PRODUCT_NAME, "DS2308"),
			DMI_MATCH(DMI_BOARD_NAME, "Dinson"),
		},
	},
	{}
};
MODULE_DEVICE_TABLE(dmi, pfh_ds2308_it8613e_dmi_table);

static const struct pfh_it8613e_sio_config pfh_ds2308_it8613e_config = {
	.sio_addr = 0x2e,
	.expected_hwm_base = 0x0a30,
};

static const struct pfh_it8613e_sio_fan pfh_ds2308_it8613e_fan1 = {
	.low_reg = 0x0d,
	.high_reg = 0x18,
};

static const struct pfh_it8613e_sio_fan pfh_ds2308_it8613e_fan2 = {
	.low_reg = 0x0e,
	.high_reg = 0x19,
};

static const struct pfh_it8613e_sio_fan pfh_ds2308_it8613e_fan3 = {
	.low_reg = 0x0f,
	.high_reg = 0x1a,
};

static const struct pfh_it8613e_sio_fan pfh_ds2308_it8613e_fan4 = {
	.low_reg = 0x80,
	.high_reg = 0x81,
};

static const struct pfh_it8613e_sio_fan pfh_ds2308_it8613e_fan5 = {
	.low_reg = 0x82,
	.high_reg = 0x83,
};

static const struct pfh_fan_channel pfh_ds2308_it8613e_fans[] = {
	{
		.label = "Fan 1",
		.read_config = &pfh_ds2308_it8613e_fan1,
	},
	{
		.label = "Fan 2",
		.read_config = &pfh_ds2308_it8613e_fan2,
	},
	{
		.label = "Fan 3",
		.read_config = &pfh_ds2308_it8613e_fan3,
	},
	{
		.label = "Fan 4",
		.read_config = &pfh_ds2308_it8613e_fan4,
	},
	{
		.label = "Fan 5",
		.read_config = &pfh_ds2308_it8613e_fan5,
	},
};

const struct pfh_platform_desc pfh_ds2308_it8613e = {
	.id = "ds2308-it8613e-sio",
	.hwmon_name = "ds2308_it8613e",
	.dmi_table = pfh_ds2308_it8613e_dmi_table,
	.backend = &pfh_it8613e_sio_backend_ops,
	.backend_config = &pfh_ds2308_it8613e_config,
	.fans = pfh_ds2308_it8613e_fans,
	.num_fans = ARRAY_SIZE(pfh_ds2308_it8613e_fans),
};
