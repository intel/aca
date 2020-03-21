/*COPYRIGHT**
    Copyright (C) 2019-2020 Intel Corporation.  All Rights Reserved.

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.






**COPYRIGHT*/






#ifndef _PMU_INFO_H_INC_
#define _PMU_INFO_H_INC_

U32 drv_type = DRV_TYPE_PUBLIC;
S8 *drv_type_str = "PUBLIC";

static const PMU_INFO_NODE pmu_info_list[] = {
	// CTI_Broadwell_Server = 0x406F0
	{0x6, 0x4F, 0x0, 0xF, bdx_msr_list, bdx_pci_list, skx_mmio_list},

	// CTI_Skylake = 0x506E0
	{0x6, 0x5E, 0x0, 0xF, skl_msr_list, NULL, client_common_mmio_list},

	// CTI_Skylake_ULT = 0x406E0
	{0x6, 0x4E, 0x0, 0xF, skl_msr_list, NULL, client_common_mmio_list},

	// CTI_Kabylake = 0x906E0
	{0x6, 0x9E, 0x0, 0x9, skl_msr_list, NULL, client_common_mmio_list},

	// CTI_Kabylake_ULX = 0x806E0
	{0x6, 0x8E, 0x0, 0xF, skl_msr_list, NULL, client_common_mmio_list},

	// CTI_Skylake_Server = 0x50650
	{0x6, 0x55, 0x0, 0x4, skx_msr_list, skx_pci_list, skx_mmio_list},

	// CTI_Coffeelake = 0x906E0
	{0x6, 0x9E, 0xA, 0xF, skl_msr_list, NULL, client_common_mmio_list},

	// CTI_Cascadelake_Server = 0x50655
	{0x6, 0x55, 0x5, 0x9, clx_msr_list, skx_pci_list, server_common_mmio_list},

	// CTI_Snowridge = 0x80660
	{0x6, 0x86, 0x0, 0xF, snr_msr_list, snr_pci_list, snr_mmio_info_list},

	// CTI_Icelake = 0x706E0
	{0x6, 0x7E, 0x0, 0xF, icl_msr_list, NULL, client_common_mmio_list},

	// CTI_Geminilake = 0x706A0
	{0x6, 0x7A, 0x0, 0xF, gml_msr_list, NULL, NULL},

	// CTI_CougarMountain = 0x606E0
	{0x6, 0x6E, 0x0, 0xF, slm_msr_list, NULL, NULL},

	// CTI_Denverton = 0x506F0
	{0x6, 0x5F, 0x0, 0xF, dnv_msr_list, NULL, bxt_dvt_mmio_info_list},

	// CTI_Broxton = 0x506C0
	{0x6, 0x5C, 0x0, 0xF, bxt_msr_list, NULL, bxt_dvt_mmio_info_list},

	// CTI_Broadwell_DE = 0x50660
	{0x6, 0x56, 0x0, 0xF, bdx_msr_list, bdw_de_pci_list, client_common_mmio_list},

	// CTI_Broadwell_H = 0x40670
	{0x6, 0x47, 0x0, 0xF, hsw_msr_list, NULL, client_common_mmio_list},

	// CTI_Knightslanding = 0x50670
	{0x6, 0x57, 0x0, 0xF, knl_msr_list, knl_pci_list, server_common_mmio_list},

	// CTI_Knightsmill = 0x80650
	{0x6, 0x85, 0x0, 0xF, knl_msr_list, knl_pci_list, server_common_mmio_list},

	// CTI_Anniedale = 0x506A0
	{0x6, 0x5A, 0x0, 0xF, and_msr_list, NULL, NULL},

	// CTI_Cherryview = 0x406C0
	{0x6, 0x4C, 0x0, 0xF, slm_msr_list, NULL, NULL},

	// CTI_Tangier = 0x406A0
	{0x6, 0x4A, 0x0, 0xF, slm_msr_list, NULL, NULL},

	// CTI_Haswell_Server = 0x306F0
	{0x6, 0x3F, 0x0, 0xF, hsx_msr_list, hsx_pci_list, server_common_mmio_list},

	// CTI_Broadwell = 0x306D0
	{0x6, 0x3D, 0x0, 0xF, hsw_msr_list, NULL, client_common_mmio_list},

	// CTI_Avoton = 0x406D0
	{0x6, 0x4D, 0x0, 0xF, avt_msr_list, NULL, NULL},

	// CTI_Crystalwell = 0x40660
	{0x6, 0x46, 0x0, 0xF, hsw_msr_list, NULL, client_common_mmio_list},

	// CTI_Haswell_ULT = 0x40650
	{0x6, 0x45, 0x0, 0xF, hsw_ult_msr_list, NULL, client_common_mmio_list},

	// CTI_Ivytown = 0x306E0
	{0x6, 0x3E, 0x0, 0xF, ivt_msr_list, ivt_pci_list, server_common_mmio_list},

	// CTI_Silvermont = 0x30670
	{0x6, 0x37, 0x0, 0xF, slm_msr_list, NULL, NULL},

	// CTI_Haswell = 0x306c0
	{0x6, 0x3C, 0x0, 0xF, hsw_msr_list, NULL, client_common_mmio_list},

	// CTI_Ivybridge = 0x306a0
	{0x6, 0x3A, 0x0, 0xF, snb_msr_list, NULL, client_common_mmio_list},

	// CTI_Jaketown = 0x206d0
	{0x6, 0x2D, 0x0, 0xF, jkt_msr_list, jkt_pci_list, server_common_mmio_list},

	// CTI_Sandybridge = 0x206a0
	{0x6, 0x2A, 0x0, 0xF, snb_msr_list, NULL, client_common_mmio_list},

	// CTI_Cometlake = 0xA0660
	{0x6, 0xA6, 0x0, 0xF, skl_msr_list, NULL, client_common_mmio_list},

	// Last
	{0x0, 0x0, 0x0, 0x0, NULL, NULL, NULL}
};

#endif


