/* SPDX-License-Identifier: GPL-2.0 */
#ifndef PFH_IT8613E_SIO_H
#define PFH_IT8613E_SIO_H

#include <linux/types.h>

#include "../core/pfh-core.h"

struct pfh_it8613e_sio_config {
	u16 sio_addr;
	u16 expected_hwm_base;
};

struct pfh_it8613e_sio_fan {
	u8 low_reg;
	u8 high_reg;
};

extern const struct pfh_backend_ops pfh_it8613e_sio_backend_ops;

#endif /* PFH_IT8613E_SIO_H */
