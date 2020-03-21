#
# Version: 1.6
#
# Copyright (C) 2008-2019 Intel Corporation.  All Rights Reserved.
#
#     This file is part of SEP Development Kit
#
#     SEP Development Kit is free software; you can redistribute it
#     and/or modify it under the terms of the GNU General Public License
#     version 2 as published by the Free Software Foundation.
#
#     SEP Development Kit is distributed in the hope that it will be useful,
#     but WITHOUT ANY WARRANTY; without even the implied warranty of
#     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#     GNU General Public License for more details.
#
#     You should have received a copy of the GNU General Public License
#     along with SEP Development Kit; if not, write to the Free Software
#     Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#
#     As a special exception, you may use this file as part of a free software
#     library without restriction.  Specifically, if other files instantiate
#     templates or use macros or inline functions from this file, or you compile
#     this file and link it with other files to produce an executable, this
#     file does not by itself cause the resulting executable to be covered by
#     the GNU General Public License.  This exception does not however
#     invalidate any other reasons why the executable file might be covered by
#     the GNU General Public License.
#

# -------------------- user configurable options ------------------------

# base name of SEP driver
DRIVER_NAME = sep5

# location to install driver
ifeq ($(INSTALL),)
INSTALL = .
endif

# current working directory
DRV_CWD := $(shell pwd)

# If KERNELRELEASE is defined, we've been invoked from the
# kernel build system and can use its language.
# Example flags are "-Werror", "-Wno-error", etc.
EXTRA_CFLAGS += -I$(LDDINCDIR) -I$(LDDINCDIR1)

# if ARCH variable is set, unset it to avoid conflicts with kbuild
unexport ARCH

# platform details
MACH ?= $(shell uname -m)
export MACH
ifeq ($(MACH),x86_64)
PLATFORM=x32_64
endif
ifeq ($(PLATFORM),)
PLATFORM=x32
endif
KERNEL_VERSION ?= $(shell uname -r)
SMP ?= $(shell uname -v | grep SMP)
ARITY=up
ifneq ($(SMP),)
ARITY=smp
endif

ifeq ($(PER_USER_MODE),YES)
SEP_PER_USER=-pu
endif


# this is a temporary work-around for handling the module structure changes across multiple kernels.
# need to find out a proper solution.
ifneq ($(KERNEL_SRC_DIR),)
    PATH_PREFIX = $(KERNEL_SRC_DIR)
endif
PATH_PREFIX ?= /lib/modules/$(shell uname -r)/build
ifneq ($(shell grep -s $(PATH_PREFIX)/include/linux/module.h -we "struct module_layout"),)
    EXTRA_CFLAGS += -DSEP_CONFIG_MODULE_LAYOUT
endif

ifeq ($(shell test -e $(PATH_PREFIX)/include/linux/kaiser.h && echo -n YES),YES)
    EXTRA_CFLAGS += -DKAISER_HEADER_PRESENT
endif

# eventual filename of SEP driver
DRIVER_MODE=$(DRIVER_NAME)$(SEP_PER_USER)
DRIVER_TYPE=$(PLATFORM)-$(KERNEL_VERSION)$(ARITY)
DRIVER_FILENAME=$(DRIVER_MODE)-$(DRIVER_TYPE).ko

# build options ...
ifneq ($(KERNELRELEASE),)
	obj-m := $(DRIVER_NAME).o

ifeq ($(PLATFORM),x32)
	arch-objs :=    core2.o                \
			perfver4.o             \
			sys32.o                \
			silvermont.o           \
			pci.o                  \
			apic.o                 \
			pebs.o                 \
			unc_gt.o               \
			unc_mmio.o             \
			unc_msr.o              \
			unc_common.o           \
			unc_pci.o              \
			sepdrv_p_state.o
endif
ifeq ($(PLATFORM),x32_64)
	arch-objs :=    core2.o                \
			perfver4.o             \
			sys64.o                \
			silvermont.o           \
			pci.o                  \
			apic.o                 \
			pebs.o                 \
			unc_gt.o               \
			unc_mmio.o             \
			unc_msr.o              \
			unc_common.o           \
			unc_pci.o              \
			sepdrv_p_state.o
endif

ifeq ($(PER_USER_MODE),YES)
    EXTRA_CFLAGS += -DSECURE_SEP
endif

EXTRA_CFLAGS += -DDRV_CPU_HOTPLUG -DDRV_USE_TASKLET_WORKAROUND -DDRV_SAFE_MSR -DDRV_DISABLE_PEBS

ifeq ($(MINLOG_MODE),YES)
    EXTRA_CFLAGS += -DDRV_MINIMAL_LOGGING
    $(info **********************************************************************************)
    $(info WARNING: Using minimal logging mode. This may make troubleshooting more difficult.)
    $(info **********************************************************************************)
else
    ifeq ($(MAXLOG_MODE),YES)
        EXTRA_CFLAGS += -DDRV_MAXIMAL_LOGGING
        $(info ****************************************************************)
        $(info WARNING: Using maximal logging mode. This may increase overhead.)
        $(info ****************************************************************)
    endif
endif

ifeq ($(UDEV_AVAILABLE),NO)
    EXTRA_CFLAGS += -DDRV_UDEV_UNAVAILABLE
    $(info ********************************************************************************)
    $(info WARNING: The device manager is NOT available. It create device files separately.)
    $(info ********************************************************************************)
endif
EXTRA_CFLAGS += -DDISABLE_BUILD_SOCPERF
#** <private build only!>
ifeq ($(BUILD_PUBLIC),YES)
else
    EXTRA_CFLAGS += -DENABLE_CPUS
    private-objs := unc_sa.o
endif

#== </private build!>

	$(DRIVER_NAME)-objs :=            \
			lwpmudrv.o        \
			control.o         \
			cpumon.o          \
			eventmux.o        \
			linuxos.o         \
			output.o          \
			pmi.o             \
			sys_info.o        \
			utility.o         \
			valleyview_sochap.o    \
			unc_power.o       \
			pmu_list.o        \
			$(private-objs)   \
			$(arch-objs)

# targets ...

# Otherwise, we were called directly from the command
# line, so the kernel build system will be used.
else
	KERNEL_SRC_DIR ?= /lib/modules/$(shell uname -r)/build
	PWD := $(shell pwd)

all: default

default:
	$(MAKE) -C $(KERNEL_SRC_DIR) M=$(PWD) LDDINCDIR=$(PWD)/../include LDDINCDIR1=$(PWD)/inc modules PWD=$(PWD) -j4
	cp $(DRIVER_NAME).ko $(DRIVER_FILENAME)

endif

install:
	@if [ $(DRV_CWD) != $(INSTALL) ] ; then \
		cp $(DRIVER_FILENAME) $(INSTALL)/$(DRIVER_FILENAME) ; \
		cp insmod-sep $(INSTALL)/insmod-sep ; \
		cp rmmod-sep $(INSTALL)/rmmod-sep ; \
		cp boot-script $(INSTALL)/boot-script ; \
		cp read_dmisysfs.py $(INSTALL)/read_dmisysfs.py ; \
		cp read_slitsysfs.py $(INSTALL)/read_slitsysfs.py ; \
		cp read_sratsysfs.py $(INSTALL)/read_sratsysfs.py ; \
		cp read_smbios.py $(INSTALL)/read_smbios.py ; \
		echo "Installed $(DRIVER_NAME) driver to $(INSTALL)/$(DRIVER_FILENAME) ." ; \
	fi;


clean:
	rm -f *.o .*.o.cmd .*.o.d .*.ko.cmd .*.ko.unsigned.cmd *.gcno .cache.mk *.o.ur-safe .*.o.tmp
	rm -f $(DRIVER_NAME).ko $(DRIVER_NAME).ko.unsigned
	rm -f $(DRIVER_MODE)*$(DRIVER_TYPE).ko
	rm -f Module.symvers Modules.symvers *.mod.c modules.order Module.markers
	rm -rf .tmp_versions

distclean: clean
	rm -f $(DRIVER_NAME)*.ko

