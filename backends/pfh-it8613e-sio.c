// SPDX-License-Identifier: GPL-2.0

#include <linux/errno.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/slab.h>

#include "pfh-it8613e-sio.h"

#define PFH_IT8613E_DEVID 0x8613
#define PFH_IT8613E_DEV_REG 0x07
#define PFH_IT8613E_PME_LDN 0x04
#define PFH_IT8613E_DEVID_REG 0x20
#define PFH_IT8613E_ACT_REG 0x30
#define PFH_IT8613E_BASE_REG 0x60
#define PFH_IT8613E_EXPECTED_BASE 0x0a30
#define PFH_IT8613E_HWM_ADDR_OFFSET 5
#define PFH_IT8613E_HWM_DATA_OFFSET 6

struct pfh_it8613e_sio_data {
	u16 sio_addr;
	u16 hwm_base;
};

static void pfh_it8613e_sio_enter(u16 sio_addr)
{
	outb(0x87, sio_addr);
	outb(0x01, sio_addr);
	outb(0x55, sio_addr);
	outb(0x55, sio_addr);
}

static void pfh_it8613e_sio_exit(u16 sio_addr)
{
	outb(0x02, sio_addr);
	outb(0x02, sio_addr + 1);
}

static u8 pfh_it8613e_sio_inb(u16 sio_addr, u8 reg)
{
	outb(reg, sio_addr);
	return inb(sio_addr + 1);
}

static u16 pfh_it8613e_sio_inw(u16 sio_addr, u8 reg)
{
	u16 high;
	u16 low;

	outb(reg, sio_addr);
	high = inb(sio_addr + 1);
	outb(reg + 1, sio_addr);
	low = inb(sio_addr + 1);

	return (high << 8) | low;
}

static void pfh_it8613e_sio_select(u16 sio_addr, u8 ldn)
{
	outb(PFH_IT8613E_DEV_REG, sio_addr);
	outb(ldn, sio_addr + 1);
}

static u8 pfh_it8613e_hwm_inb(const struct pfh_it8613e_sio_data *data, u8 reg)
{
	outb(reg, data->hwm_base + PFH_IT8613E_HWM_ADDR_OFFSET);
	return inb(data->hwm_base + PFH_IT8613E_HWM_DATA_OFFSET);
}

static u16 pfh_it8613e_sio_read_raw(struct pfh_device *pfh,
				    const struct pfh_fan_channel *fan)
{
	const struct pfh_it8613e_sio_fan *config = fan->read_config;
	const struct pfh_it8613e_sio_data *data = pfh->backend_data;
	u16 high;
	u16 low;

	low = pfh_it8613e_hwm_inb(data, config->low_reg);
	high = pfh_it8613e_hwm_inb(data, config->high_reg);

	return (high << 8) | low;
}

static long pfh_it8613e_sio_raw_to_rpm(u16 raw)
{
	if (raw == 0x0000)
		return -EIO;
	if (raw == 0xffff)
		return 0;
	return 1350000L / ((long)raw * 2L);
}

static int pfh_it8613e_sio_probe(struct pfh_device *pfh)
{
	const struct pfh_it8613e_sio_config *config = pfh->desc->backend_config;
	struct pfh_it8613e_sio_data *data;
	u16 devid;
	u16 hwm_base;
	u8 active;
	int ret = 0;

	data = devm_kzalloc(pfh->dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	if (!request_muxed_region(config->sio_addr, 2, "pfh-it8613e-sio"))
		return -EBUSY;

	pfh_it8613e_sio_enter(config->sio_addr);
	devid = pfh_it8613e_sio_inw(config->sio_addr, PFH_IT8613E_DEVID_REG);
	if (devid != PFH_IT8613E_DEVID) {
		dev_err(pfh->dev, "unexpected IT8613E device id 0x%04x\n", devid);
		ret = -ENODEV;
		goto out_sio;
	}

	pfh_it8613e_sio_select(config->sio_addr, PFH_IT8613E_PME_LDN);
	active = pfh_it8613e_sio_inb(config->sio_addr, PFH_IT8613E_ACT_REG);
	if (!(active & 0x01)) {
		dev_err(pfh->dev, "IT8613E environmental monitor is inactive\n");
		ret = -ENODEV;
		goto out_sio;
	}

	hwm_base = pfh_it8613e_sio_inw(config->sio_addr, PFH_IT8613E_BASE_REG);
	if (hwm_base != config->expected_hwm_base ||
	    hwm_base != PFH_IT8613E_EXPECTED_BASE) {
		dev_err(pfh->dev, "unexpected IT8613E HWM base 0x%04x\n", hwm_base);
		ret = -ENODEV;
		goto out_sio;
	}

out_sio:
	pfh_it8613e_sio_exit(config->sio_addr);
	release_region(config->sio_addr, 2);
	if (ret)
		return ret;

	data->sio_addr = config->sio_addr;
	data->hwm_base = hwm_base;

	if (!request_region(data->hwm_base + PFH_IT8613E_HWM_ADDR_OFFSET, 2,
			    "pfh-it8613e-hwm"))
		return -EBUSY;

	pfh->backend_data = data;

	dev_info(pfh->dev, "using IT8613E HWM base 0x%04x for platform %s\n",
		 hwm_base, pfh->desc->id);
	return 0;
}

static bool pfh_it8613e_sio_fan_visible(struct pfh_device *pfh,
					const struct pfh_fan_channel *fan)
{
	u16 raw;

	if (!pfh->backend_data || !fan->read_config)
		return false;

	raw = pfh_it8613e_sio_read_raw(pfh, fan);
	return raw != 0x0000;
}

static int pfh_it8613e_sio_read_fan(struct pfh_device *pfh,
				    const struct pfh_fan_channel *fan,
				    long *rpm)
{
	long converted;
	u16 raw;

	if (!pfh->backend_data || !fan->read_config)
		return -ENODEV;

	raw = pfh_it8613e_sio_read_raw(pfh, fan);
	converted = pfh_it8613e_sio_raw_to_rpm(raw);
	if (converted < 0)
		return converted;

	*rpm = converted;
	return 0;
}

static void pfh_it8613e_sio_remove(struct pfh_device *pfh)
{
	struct pfh_it8613e_sio_data *data = pfh->backend_data;

	if (!data)
		return;

	if (data->hwm_base)
		release_region(data->hwm_base + PFH_IT8613E_HWM_ADDR_OFFSET, 2);

	pfh->backend_data = NULL;
}

const struct pfh_backend_ops pfh_it8613e_sio_backend_ops = {
	.probe = pfh_it8613e_sio_probe,
	.fan_visible = pfh_it8613e_sio_fan_visible,
	.read_fan = pfh_it8613e_sio_read_fan,
	.remove = pfh_it8613e_sio_remove,
};
