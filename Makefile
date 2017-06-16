obj-m := yes.o
clean-files := *.o *~

KERNELVER ?= $(shell uname -r)
KERNELDIR ?= /lib/modules/$(KERNELVER)/build

.PHONY: all module clean install

all: module

module:
	$(MAKE) -C $(KERNELDIR) M=`pwd`

clean:
	$(MAKE) -C $(KERNELDIR) M=`pwd` clean

install:
	$(MAKE) -C $(KERNELDIR) M=`pwd` modules_install
