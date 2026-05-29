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
