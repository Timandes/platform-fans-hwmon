// SPDX-License-Identifier: GPL-2.0

#include <linux/err.h>
#include <linux/io.h>
#include <linux/pci.h>
#include <linux/slab.h>
#include <linux/string.h>

#include "pfh-ec-mmio.h"

struct pfh_ec_mmio_data {
	void __iomem *ec_base;
	struct pci_dev *lpc;
};

static u16 pfh_ec_mmio_read_be16(void __iomem *base, u16 offset)
{
	u16 high = readb(base + offset);
	u16 low = readb(base + offset + 1);

	return (high << 8) | low;
}

static bool pfh_ec_mmio_identifier_valid(void __iomem *ec_base,
					 const struct pfh_ec_mmio_config *config)
{
	char id[16];
	int i;

	if (!config->identifiers || config->num_identifiers <= 0)
		return true;

	for (i = 0; i < config->num_identifiers; i++) {
		size_t len = strlen(config->identifiers[i].value);
		int j;

		if (len >= sizeof(id))
			return false;

		for (j = 0; j < len; j++)
			id[j] = readb(ec_base + config->identifiers[i].offset + j);
		id[len] = '\0';

		if (!strcmp(id, config->identifiers[i].value))
			return true;
	}

	return false;
}

static int pfh_ec_mmio_probe(struct pfh_device *pfh)
{
	const struct pfh_ec_mmio_config *config = pfh->desc->backend_config;
	struct pfh_ec_mmio_data *data;
	resource_size_t base;
	u32 lgmr;
	int ret;

	data = devm_kzalloc(pfh->dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	data->lpc = pci_get_domain_bus_and_slot(0, config->lpc_bus,
						PCI_DEVFN(config->lpc_dev,
							  config->lpc_func));
	if (!data->lpc) {
		dev_err(pfh->dev, "LPC device 0000:%02x:%02x.%u not found\n",
			config->lpc_bus, config->lpc_dev, config->lpc_func);
		return -ENODEV;
	}

	ret = pci_read_config_dword(data->lpc, config->lgmr_offset, &lgmr);
	if (ret) {
		dev_err(pfh->dev, "failed to read LGMR config register: %d\n", ret);
		goto out_put_lpc;
	}

	if (!(lgmr & config->lgmr_enable)) {
		dev_err(pfh->dev, "EC MMIO window is disabled: LGMR=0x%08x\n", lgmr);
		ret = -ENODEV;
		goto out_put_lpc;
	}

	base = lgmr & config->lgmr_base_mask;
	if (!base) {
		dev_err(pfh->dev, "EC MMIO base is zero: LGMR=0x%08x\n", lgmr);
		ret = -ENODEV;
		goto out_put_lpc;
	}

	data->ec_base = ioremap(base, config->mmio_size);
	if (!data->ec_base) {
		ret = -ENOMEM;
		goto out_put_lpc;
	}

	if (!pfh_ec_mmio_identifier_valid(data->ec_base, config)) {
		dev_err(pfh->dev, "unsupported EC identifier for platform %s\n",
			pfh->desc->id);
		ret = -ENODEV;
		goto out_unmap;
	}

	pfh->backend_data = data;
	dev_info(pfh->dev, "mapped EC MMIO at 0x%pa for platform %s\n",
		 &base, pfh->desc->id);
	return 0;

out_unmap:
	iounmap(data->ec_base);
	data->ec_base = NULL;
out_put_lpc:
	pci_dev_put(data->lpc);
	data->lpc = NULL;
	return ret;
}

static int pfh_ec_mmio_read_fan(struct pfh_device *pfh,
				const struct pfh_fan_channel *fan,
				long *rpm)
{
	struct pfh_ec_mmio_data *data = pfh->backend_data;
	const struct pfh_ec_mmio_fan *config = fan->read_config;

	if (!data || !data->ec_base || !config)
		return -ENODEV;

	*rpm = pfh_ec_mmio_read_be16(data->ec_base, config->rpm_offset);
	return 0;
}

static void pfh_ec_mmio_remove(struct pfh_device *pfh)
{
	struct pfh_ec_mmio_data *data = pfh->backend_data;

	if (!data)
		return;

	if (data->ec_base) {
		iounmap(data->ec_base);
		data->ec_base = NULL;
	}

	if (data->lpc) {
		pci_dev_put(data->lpc);
		data->lpc = NULL;
	}

	pfh->backend_data = NULL;
}

const struct pfh_backend_ops pfh_ec_mmio_backend_ops = {
	.probe = pfh_ec_mmio_probe,
	.read_fan = pfh_ec_mmio_read_fan,
	.remove = pfh_ec_mmio_remove,
};
