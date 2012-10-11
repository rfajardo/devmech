VERSION			:=	trunk
MODULENAME		:=	pwc
CONFIG_PWC	:=	m

KVER  := $(shell uname -r)
DEST  := /lib/modules/$(KVER)/kernel/kernel/drivers/misc/$(MODULENAME)

EXTRA_CFLAGS=-DCONFIG_USB_PWC_DEBUG

EXTRA_SYMBOLS := ../../pwcmech ../../../../usbif ../../../../devif
KBUILD_EXTRA_SYMBOLS := $(addprefix $(PWD)/, $(addsuffix /Module.symvers, $(EXTRA_SYMBOLS)))

INCLUDE_FILES=

ifneq ($(KERNELRELEASE),)

$(MODULENAME)-objs := pwc-if.o pwc-misc.o pwc-ctrl.o pwc-v4l.o pwc-uncompress.o
$(MODULENAME)-objs += pwc-dec1.o pwc-dec23.o pwc-kiara.o pwc-timon.o

obj-$(CONFIG_PWC) += pwc.o

else

KVER  := $(shell uname -r)
KLINK := $(shell test -e /lib/modules/${KVER}/build/ && echo "build" || echo "source")
KSRC  := /lib/modules/$(KVER)/$(KLINK)
PWD   := $(shell pwd)
DEST  := /lib/modules/$(KVER)/kernel/drivers/media/video/$(MODULENAME)
INCLUDE_DIR := $(KSRC)/include/$(MODULENAME)
INCLUDE_DIRFILES := $(addprefix $(INCLUDE_DIR)/, $(INCLUDE_FILES))

all:
	$(MAKE) -C $(KSRC) SUBDIRS=$(PWD) modules
	
clean:
	$(MAKE) -C $(KSRC) SUBDIRS=$(PWD) clean

install:
ifneq ($(INCLUDE_FILES),)
	install -d $(INCLUDE_DIR)
	install -t $(INCLUDE_DIR) $(INCLUDE_FILES)
endif
	install -d $(DEST)
	install -m 644 -c $(MODULENAME).ko $(DEST)
	-/sbin/depmod -a
	echo "FINISHED"

uninstall:
ifneq ($(INCLUDE_FILES),)
	rm -f $(INCLUDE_DIRFILES)
	rmdir $(INCLUDE_DIR)
endif
	rm -f $(DEST)/$(MODULENAME).ko	
	-/sbin/depmod -a
	echo "FINISHED"
	
endif
