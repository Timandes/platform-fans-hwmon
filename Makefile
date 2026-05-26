KERNEL_VERSION ?= $(shell uname -r)
KDIR ?= /lib/modules/$(KERNEL_VERSION)/build
PWD := $(shell pwd)

obj-m += platform_fans_hwmon.o

platform_fans_hwmon-y := \
	core/pfh-core.o \
	backends/pfh-ec-mmio.o \
	backends/pfh-it8613e-sio.o \
	platforms/intel/pfh-intel-nuc-ec-v9.o \
	platforms/ite/pfh-ds2308-it8613e.o

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

install:
	install -D -m 0644 platform_fans_hwmon.ko /lib/modules/$(KERNEL_VERSION)/extra/platform_fans_hwmon.ko
	depmod -a $(KERNEL_VERSION)

uninstall:
	rm -f /lib/modules/$(KERNEL_VERSION)/extra/platform_fans_hwmon.ko
	depmod -a $(KERNEL_VERSION)

.PHONY: all clean install uninstall
