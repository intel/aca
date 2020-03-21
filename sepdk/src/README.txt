==============================================================================
           HOW TO BUILD AND LOAD THE SEP KERNEL DRIVER UNDER LINUX*
==============================================================================

version: 1.7
updated: 08/30/2011

This document describes the required software environment needed in order
to build and load the SEP kernel driver on the Linux* operating system.
Use this information to build and load the driver in case that the kernel
on your Linux system is not one of the supported kernels listed in the
Release Notes.


1.  Basic Development Environment

In order to compile the driver, the target system must have the following:

    * C compiler that was used to build the kernel, and which is capable
      of compiling programs with anonymous structs/unions. For example,
      GCC 2.96 or later

    * Tools needed to build a C based program. For example, GNU make
      tool, native assembler, linker

    * System headers. For example, /usr/include/

In addition, the kernel must be configured with the following options enabled:

    CONFIG_MODULES=y
    CONFIG_MODULE_UNLOAD=y
    CONFIG_PROFILING=y

These options can be verified by checking the kernel config file
(e.g., /boot/config, /proc/config.gz, /usr/src/linux/.config, etc.).

Normally, these tools are installed, and kernel options enabled, by default.
However, administrators may remove/disable them from deployment systems,
such as servers or embedded systems.


2.  Kernel Development Environment

In addition to the above tools, a proper kernel development environment
must be present on the target system.

  Linux Distributions Based on Kernel 2.6

    * Red Hat Enterprise Linux 5, 6:
      install the kernel-*-devel-*.rpm that is appropriate for the running
      kernel (on 3rd Red Hat install CD)

    * Red Hat Fedora* Core 5 and later:
      download and install the kernel-*-devel-*.rpm that is appropriate for the
      running kernel from, http://download.fedora.redhat.com/pub/fedora/linux/ 
      or by using YUM:

        yum install kernel-devel

    * Novell SuSE Linux Professional 9.x:
      Novell OpenSuSE 10.x:
      install the kernel-source-*.rpm that is appropriate for the
      running kernel (on 3rd or 5th SuSE install CD)

    * Novell SuSE Linux Enterprise Server 9:
      install the kernel-source-*-`uname -m`.rpm that is appropriate for the
      running kernel (on 2nd or 3rd SuSE install CD)

    * Novell SuSE Linux Enterprise Server 10, 11:
      Novell OpenSuSE 11.x:
      install the kernel-source-*-`uname -m`.rpm that is appropriate
      for the running kernel (on 2nd or 3rd SuSE install CD)

    * Red Flag* 5.x:
      install the kernel-*-devel-*.rpm that is appropriate for the running 
      kernel (on 2nd Red Flag install CD)

    * Debian 4.x - 8.x:
      Ubuntu 6.x - 16.04:
      install the GCC and kernel development environment via,

        apt-get update
        apt-get install build-essential
        apt-get install linux-headers-`uname -r`


4.  Other Linux Distributions or Kernels

For kernels or Linux disributions not mentioned above, you need to set
up the kernel build environment manually. This involves configuring the
kernel sources (and hence kernel headers) to match the running kernel
on the target system.

For 2.6 and later kernels, the kernel sources can be configured as follows:

  # boot into the kernel you wish to build driver for
  # and make sure the kernel source tree is placed in
  # /usr/src/linux-`uname -r`
  # 
  cd /usr/src/linux-`uname -r`
  vi Makefile # set EXTRAVERSION to a value corresponding to `uname -r`
  make mrproper
  cp /boot/config-`uname -r` .config
  make oldconfig
  make prepare
  make scripts

Once the configuration completes, make sure that UTS_RELEASE in
/usr/src/linux-`uname -r`/include/linux/version.h or in
/usr/src/linux-`uname -r`/include/linux/utsrelease.h matches `uname -r`.


5.  Building and (re)Loading the Driver

Once the standard development tools and proper kernel development
environment are installed, you can build and load the driver:

  # build the driver:
  cd /path/to/sepdk/sources/
  ./build-driver

  # unload previously loaded driver from the kernel (if any):
  cd /path/to/sepdk/sources/
  ./rmmod-sep

  # load the driver into the kernel:
  cd /path/to/sepdk/sources/
  ./insmod-sep

  # autoload the driver at boot time:
  cd /path/to/sepdk/sources/
  ./boot-script -g users -d /path/to/pre-built-drivers

If any errors occur during the building or loading of the driver, this
may indicate a mismatch between the kernel sources and the running kernel.
For load issues, check the /var/log/messages file or the output of dmesg.


6.  Drill Down into Kernel [ OPTIONAL ]

In order to to drill down into the kernel and see function hotspots, the
kernel file (e.g., "vmlinux") must be uncompressed and not be stripped of
symbols.  In order to further drill down into the kernel sources, the kernel
file must also be compiled with debug information.

Linux vendors typically release kernel files that are compressed and stripped
of symbols (e.g., "vmlinuz").  However, some vendors have released special
debug versions of their kernels, which are also suitable for drill down.

For Red Hat Enterprise Linux 3, 4 and 5 distros, Red Hat provides "debuginfo"
RPMs that can be obtained from http://people.redhat.com/duffy/debuginfo/ .
After installing the RPM, the debug version of the kernel file will be placed
under /usr/lib/debug/boot (EL 3) or /usr/lib/debug/lib/modules (EL 4, 5).

For SuSE Linux Enterprise 9, 10, and 11 distros, SuSE provides "debug" kernel
RPMs (kernel-debug-*.rpm) available on the install CD or from their website.
After installing the RPM, the debug version of the kernel file will be placed
under /boot/vmlinux-*-debug (on IA32 systems with or without
Intel(R) Extended Memory 64 Technology) or under /boot/vmlinuz-*-debug

For all others, after configuring the kernel sources (see step 4 above)
edit the kernel source top-level Makefile and add the "-g" option to the
following variables:

  CFLAGS_KERNEL := -g
  CFLAGS := -g

Then run "make clean; make" to create the vmlinux kernel file with debug
information.

Once a debug version of the kernel is created or obtained, specify that
kernel file as the one to use during drill down to hotspot view.


------------------------------------------------------------------------------

Disclaimer and Legal Information

The information in this document is subject to change without notice and
Intel Corporation assumes no responsibility or liability for any errors
or inaccuracies that may appear in this document or any software that
may be provided in association with this document. This document and the
software described in it are furnished under license and may only be
used or copied in accordance with the terms of the license. No license,
express or implied, by estoppel or otherwise, to any intellectual
property rights is granted by this document. The information in this
document is provided in connection with Intel products and should not be
construed as a commitment by Intel Corporation.

EXCEPT AS PROVIDED IN INTEL'S TERMS AND CONDITIONS OF SALE FOR SUCH
PRODUCTS, INTEL ASSUMES NO LIABILITY WHATSOEVER, AND INTEL DISCLAIMS ANY
EXPRESS OR IMPLIED WARRANTY, RELATING TO SALE AND/OR USE OF INTEL
PRODUCTS INCLUDING LIABILITY OR WARRANTIES RELATING TO FITNESS FOR A
PARTICULAR PURPOSE, MERCHANTABILITY, OR INFRINGEMENT OF ANY PATENT,
COPYRIGHT OR OTHER INTELLECTUAL PROPERTY RIGHT. Intel products are not
intended for use in medical, life saving, life sustaining, critical
control or safety systems, or in nuclear facility applications.

Designers must not rely on the absence or characteristics of any
features or instructions marked "reserved" or "undefined." Intel
reserves these for future definition and shall have no responsibility
whatsoever for conflicts or incompatibilities arising from future
changes to them.

The software described in this document may contain software defects
which may cause the product to deviate from published specifications.
Current characterized software defects are available on request.

Intel, the Intel logo, Intel SpeedStep, Intel NetBurst, Intel
NetStructure, MMX, Intel386, Intel486, Celeron, Intel Centrino, Intel
Xeon, Intel XScale, Itanium, Pentium, Pentium II Xeon, Pentium III Xeon,
Pentium M, and VTune are trademarks or registered trademarks of Intel
Corporation or its subsidiaries in the United States and other countries.

*Other names and brands may be claimed as the property of others.

Copyright(C) 2008-2018 Intel Corporation.  All Rights Reserved.
