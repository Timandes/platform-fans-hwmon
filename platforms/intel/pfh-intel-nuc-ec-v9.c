// SPDX-License-Identifier: GPL-2.0
/*
 * Platform descriptor for Intel NUC9 EC V9 fan tachometers.
 */

#include <linux/bits.h>
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
