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






#ifndef _PMU_INFO_PCI_H_INC_
#define _PMU_INFO_PCI_H_INC_

static U16 common_reg_list[] = {
    0xa0, 0xa8, 0xb0, 0xb8, 0xd8, 0xdc, 0xe0, 0xe4, 0xf4, 0xf8, 0
};

static U16 knl_uclk_reg_list[] = {
    0x400, 0x408, 0x410, 0x418, 0x420, 0x424, 0x428, 0x42c, 0x430, 0x434, 0x44c, 0x454, 0
};

// iMC
static U16 common_imc_reg_list[] = {
    0xa0, 0xa8, 0xb0, 0xb8, 0xd0, 0xd8, 0xdc, 0xe0, 0xe4, 0xf0, 0xf4, 0xf8, 0
};

static U16 knl_imc_reg_list[] = {
    0xb00, 0xb08, 0xb10, 0xb18, 0xb20, 0xb24, 0xb28, 0xb2c, 0xb30, 0xb34, 0xb3c, 0xb44, 0
};

static U16 imc_dimm_reg_list[] = {
    0x80, 0x84, 0x88, 0
};

// EDC
static U16 knl_edc_reg_list[] = {
    0xa00, 0xa08, 0xa10, 0xa18, 0xa20, 0xa24, 0xa28, 0xa2c, 0xa30, 0xa34, 0xa3c, 0xa44, 0
};

// QPI
static U16 qpi_rx_reg_list[] = {
    0x228, 0x22c, 0x238, 0x23c, 0
};

static U16 qpi_rx_tx_reg_list[] = {
    0x200, 0x204, 0x210, 0x214, 0x228, 0x22c, 0x238, 0x23c, 0
};

static U16 qpi_lp_reg_list[] = {
    0x4c, 0x50, 0x54, 0x58, 0xC0, 0
};

// UPI
static U16 skx_upi_reg_list[] = {
    0x318, 0x320, 0x328, 0x330, 0x350, 0x358, 0x360, 0x368, 0x37c, 0x378, 0
};

static U16 upi_rx_tx_reg_list[] = {
    0x200, 0x204, 0x210, 0x214, 0x23C, 0x240, 0x244, 0x248, 0x24c, 0x250, 0x254,
    0x258, 0x25c, 0x260, 0x264, 0x268, 0x26c, 0x270, 0
};

static U16 upi_lp_reg_list[] = {
    0x94, 0x120, 0
};

// IRP
static U16 ivt_irp_reg_list[] = {
    0xa0, 0xb0, 0xb8, 0xc0, 0xd8, 0xdc, 0xe0, 0xe4, 0xf4, 0xf8, 0
};

static U16 knl_irp_reg_list[] = {
    0xa0, 0xa8, 0xd8, 0xdc, 0xf0, 0xf4, 0
};

// R3QPI/M3UPI
static U16 common_r3qpi_reg_list[] = {
    0xa0, 0xa8, 0xb0, 0xd8, 0xdc, 0xe0, 0xf4, 0xf8, 0
};

// M2M
static U16 skx_m2m_reg_list[] = {
    0x200, 0x208, 0x210, 0x218, 0x228, 0x230, 0x238, 0x240, 0x258, 0
};

static U16 snr_m2m_reg_list[] = {
    0x438, 0x440, 0x448, 0x450, 0x458, 0x460, 0x468, 0x470, 0x478, 0x480, 0x488, 0
};

static PMU_PCI_UNIT_INFO_NODE jkt_pci_list[] = {
	// HA
	{14, 1, common_reg_list}, {28, 1, common_reg_list},
	// iMC
	{16, 0, common_imc_reg_list}, {16, 1, common_imc_reg_list}, {16, 4, common_imc_reg_list}, {16, 5, common_imc_reg_list}, 
	{15, 2, imc_dimm_reg_list}, {15, 3, imc_dimm_reg_list}, {15, 4, imc_dimm_reg_list}, {15, 5, imc_dimm_reg_list},
	// QPI
	{8, 2, common_reg_list}, {9, 2, common_reg_list}, {19, 5, common_reg_list}, {19, 6, common_reg_list},
	// End of list
	{0}
};

static PMU_PCI_UNIT_INFO_NODE ivt_pci_list[] = {
	// HA
	{14, 1, common_reg_list}, {28, 1, common_reg_list},
	// iMC
	{16, 0, common_imc_reg_list}, {16, 1, common_imc_reg_list}, {16, 4, common_imc_reg_list}, {16, 5, common_imc_reg_list}, 
	{30, 0, common_imc_reg_list}, {30, 1, common_imc_reg_list}, {30, 4, common_imc_reg_list}, {30, 5, common_imc_reg_list},
	{15, 2, imc_dimm_reg_list}, {15, 3, imc_dimm_reg_list}, {15, 4, imc_dimm_reg_list}, {15, 5, imc_dimm_reg_list},
	{29, 2, imc_dimm_reg_list}, {29, 3, imc_dimm_reg_list}, {29, 4, imc_dimm_reg_list}, {29, 5, imc_dimm_reg_list},
	// QPI
	{8, 2, common_reg_list}, {9, 2, common_reg_list}, {24, 2, common_reg_list},
	{8, 6, qpi_rx_reg_list}, {9, 6, qpi_rx_reg_list}, {24, 6, qpi_rx_reg_list},
	// IRP
	{5, 6, ivt_irp_reg_list},
	// R2PCIe
    {19, 1, common_reg_list},
	// R3QPI
	{19, 5, common_r3qpi_reg_list}, {19, 6, common_r3qpi_reg_list},
	// End of list
	{0}
};

static PMU_PCI_UNIT_INFO_NODE hsx_pci_list[] = {
	// HA
	{18, 1, common_reg_list}, {18, 5, common_reg_list},
	// iMC
	{20, 0, common_imc_reg_list}, {20, 1, common_imc_reg_list}, {21, 0, common_imc_reg_list}, {21, 1, common_imc_reg_list}, 
	{23, 0, common_imc_reg_list}, {23, 1, common_imc_reg_list}, {24, 0, common_imc_reg_list}, {24, 1, common_imc_reg_list},
	{19, 2, imc_dimm_reg_list}, {19, 3, imc_dimm_reg_list}, {19, 4, imc_dimm_reg_list}, {19, 5, imc_dimm_reg_list},
	{22, 2, imc_dimm_reg_list}, {22, 3, imc_dimm_reg_list}, {22, 4, imc_dimm_reg_list}, {22, 5, imc_dimm_reg_list},
	// QPI
	{8, 2, common_reg_list}, {9, 2, common_reg_list}, {10, 2, common_reg_list},
	{8, 6, qpi_rx_tx_reg_list}, {9, 6, qpi_rx_tx_reg_list}, {10, 6, qpi_rx_tx_reg_list},
	{8, 0, qpi_lp_reg_list}, {9, 0, qpi_lp_reg_list}, {10, 0, qpi_lp_reg_list},
	// IRP
	{5, 6, common_reg_list},
	// R2PCIe
    {16, 1, common_reg_list},
	// R3QPI
	{11, 1, common_r3qpi_reg_list}, {11, 2, common_r3qpi_reg_list}, {11, 5, common_r3qpi_reg_list},
	// End of list
	{0}
};

static PMU_PCI_UNIT_INFO_NODE bdw_de_pci_list[] = {
	// HA
	{18, 1, common_reg_list}, {18, 0, common_reg_list},
	// iMC
	{20, 0, common_imc_reg_list}, {20, 1, common_imc_reg_list}, {21, 0, common_imc_reg_list}, {21, 1, common_imc_reg_list}, 
	{23, 0, common_imc_reg_list}, {23, 1, common_imc_reg_list}, {24, 0, common_imc_reg_list}, {24, 1, common_imc_reg_list},
	{19, 2, imc_dimm_reg_list}, {19, 3, imc_dimm_reg_list}, {19, 4, imc_dimm_reg_list}, {19, 5, imc_dimm_reg_list},
	// IRP
	{5, 6, common_reg_list},
	// R2PCIe
	{16, 0, common_reg_list}, {16, 1, common_reg_list},
	// R3QPI
	{11, 1, common_reg_list}, {11, 2, common_reg_list}, {11, 5, common_reg_list},
	// End of list
	{0}
};

static PMU_PCI_UNIT_INFO_NODE bdx_pci_list[] = {
	// HA
	{18, 1, common_reg_list}, {18, 5, common_reg_list},
	// iMC
	{20, 0, common_imc_reg_list}, {20, 1, common_imc_reg_list}, {21, 0, common_imc_reg_list}, {21, 1, common_imc_reg_list}, 
	{23, 0, common_imc_reg_list}, {23, 1, common_imc_reg_list}, {24, 0, common_imc_reg_list}, {24, 1, common_imc_reg_list},
	{19, 2, imc_dimm_reg_list}, {19, 3, imc_dimm_reg_list}, {19, 4, imc_dimm_reg_list}, {19, 5, imc_dimm_reg_list},
	{22, 2, imc_dimm_reg_list}, {22, 3, imc_dimm_reg_list}, {22, 4, imc_dimm_reg_list}, {22, 5, imc_dimm_reg_list},
	// QPI
	{8, 2, common_reg_list}, {9, 2, common_reg_list}, {10, 2, common_reg_list},
	{8, 6, qpi_rx_tx_reg_list}, {9, 6, qpi_rx_tx_reg_list}, {10, 6, qpi_rx_tx_reg_list},
	{8, 0, qpi_lp_reg_list}, {9, 0, qpi_lp_reg_list}, {10, 0, qpi_lp_reg_list},
	// IRP
	{5, 6, common_reg_list},
	// R2PCIe
	{16, 0, common_reg_list}, {16, 1, common_reg_list},
	// R3QPI
	{11, 1, common_reg_list}, {11, 2, common_reg_list}, {11, 5, common_reg_list},
	// End of list
	{0}
};

static PMU_PCI_UNIT_INFO_NODE knl_pci_list[] = {
	// iMC
	{8, 2, knl_imc_reg_list}, {8, 3, knl_imc_reg_list}, {8, 4, knl_imc_reg_list},
	{9, 2, knl_imc_reg_list}, {9, 3, knl_imc_reg_list}, {9, 4, knl_imc_reg_list},
    // iMC_UCLK
    {10, 0, knl_uclk_reg_list}, {11, 0, knl_uclk_reg_list},
	// EDC
	{24, 2, knl_edc_reg_list}, {25, 2, knl_edc_reg_list}, {26, 2, knl_edc_reg_list}, {27, 2, knl_edc_reg_list},
	{28, 2, knl_edc_reg_list}, {29, 2, knl_edc_reg_list}, {30, 2, knl_edc_reg_list}, {31, 2, knl_edc_reg_list},
	// EDC_UCLK
	{15, 0, knl_uclk_reg_list}, {16, 0, knl_uclk_reg_list}, {17, 0, knl_uclk_reg_list}, {18, 0, knl_uclk_reg_list},
	{19, 0, knl_uclk_reg_list}, {20, 0, knl_uclk_reg_list}, {21, 0, knl_uclk_reg_list}, {22, 0, knl_uclk_reg_list},
    // IRP
	{5, 6, knl_irp_reg_list},
	// M2PCIe
	{12, 1, common_reg_list},
	// End of list
	{0}
};

static PMU_PCI_UNIT_INFO_NODE skx_pci_list[] = {
	// iMC
	{10, 2, common_imc_reg_list}, {10, 6, common_imc_reg_list}, {11, 2, common_imc_reg_list},
	{12, 2, common_imc_reg_list}, {12, 6, common_imc_reg_list}, {13, 2, common_imc_reg_list},
	{10, 0, imc_dimm_reg_list}, {10, 4, imc_dimm_reg_list}, {11, 0, imc_dimm_reg_list},
	{12, 0, imc_dimm_reg_list}, {12, 4, imc_dimm_reg_list}, {13, 0, imc_dimm_reg_list},
	// UPI
	{14, 0, skx_upi_reg_list}, {15, 0, skx_upi_reg_list}, {16, 0, skx_upi_reg_list},
	{14, 0, upi_rx_tx_reg_list}, {15, 0, upi_rx_tx_reg_list}, {16, 0, upi_rx_tx_reg_list},
	{14, 0, upi_lp_reg_list}, {15, 0, upi_lp_reg_list}, {16, 0, upi_lp_reg_list},
	// M2PCIe
	{21, 1, common_reg_list}, {22, 1, common_reg_list}, {23, 1, common_reg_list}, {22, 5, common_reg_list},
	// M3UPI
	{18, 1, common_r3qpi_reg_list}, {18, 2, common_r3qpi_reg_list}, {18, 5, common_r3qpi_reg_list},
	// M2M
	{8, 0, skx_m2m_reg_list}, {9, 0, skx_m2m_reg_list},
	// End of list
	{0}
};

static PMU_PCI_UNIT_INFO_NODE snr_pci_list[] = {
	// iMC
	{10, 0, imc_dimm_reg_list}, {10, 4, imc_dimm_reg_list}, {11, 0, imc_dimm_reg_list},
	{12, 0, imc_dimm_reg_list}, {12, 4, imc_dimm_reg_list}, {13, 0, imc_dimm_reg_list},
	// M2M
	{12, 0, snr_m2m_reg_list}, {13, 0, snr_m2m_reg_list}, {14, 0, snr_m2m_reg_list}, {15, 0, snr_m2m_reg_list},
	// End of list
	{0}
};

#endif

