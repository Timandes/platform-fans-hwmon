KERNEL_VERSION ?= $(shell uname -r)
KDIR ?= /lib/modules/$(KERNEL_VERSION)/build
PWD := $(shell pwd)

obj-m += intel_nuc_ec_hwmon.o

.PHONY: all clean install uninstall

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

install: all
	install -D -m 0644 intel_nuc_ec_hwmon.ko /lib/modules/$(KERNEL_VERSION)/extra/intel_nuc_ec_hwmon.ko
	depmod -a $(KERNEL_VERSION)

uninstall:
	rm -f /lib/modules/$(KERNEL_VERSION)/extra/intel_nuc_ec_hwmon.ko
	depmod -a $(KERNEL_VERSION)
