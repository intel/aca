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






#ifndef _PMU_INFO_MMIO_H_INC_
#define _PMU_INFO_MMIO_H_INC_
/*	Public	*/

static U32 mmio_client_imc_common_offset[] = {
	0x5040, 0x5044, 0x5048, 0x5050, 0x5054, 0x5058, 0
};

static U32 mmio_fpga_common_offset[] = {
	0x3008, 0x3010, 0x3018, 0x3020, 0x3028, 0x1008, 0x1010, 0x1018, 0x2008,
	0
};

static U32 mmio_hfi_common_offset[] = {
	0x500,	0x508,	0x400,	0x408,	0x418,	0x420,	0x428,	0x430,	0x438,	0x440,
	0x448,	0x450,	0x458,	0x460,	0x468,	0x470,	0x478,	0x480,	0x488,	0x490,
	0x498,	0x4a0,	0
};

static U32 mmio_bxt_dvt_imc_offset[] = {
        0x6868, 0x686C, 0
};

static U32 mmio_dual_bar_imc_offset[] = {
	0x22800, 0x26800, 0x22840, 0x22844, 0x22848, 0x2284c, 0x22850, 0x26840, 0x26844,
	0x26848, 0x2684c, 0x26850, 0x2285c, 0x2685c, 0x22808, 0x22810, 0x22818, 0x22820,
	0x22828, 0x26808, 0x26810, 0x26818, 0x26820, 0x26828, 0x22854, 0x26854, 0x22838,
	0x26838, 0x2290, 0x2298, 0x22A0, 0x22A8, 0x22B0, 0x4000, 0x8, 0x40, 0x4008,
	0x4040, 0x4044, 0x54, 0x4054, 0x4038, 0x38, 0xF8, 0x44, 0x48, 0x10, 0x4010,
        0x2290, 0x4C, 0x4048, 0x404C, 0x18, 0x20, 0x4018, 0x4020, 0
};


static PMU_MMIO_UNIT_INFO_NODE client_common_mmio_list[] = {
	{
		{{{0, 0, 0, 0x48}}, 0, MMIO_SINGLE_BAR_TYPE, 0, 0x0007FFFFF8000ULL},
		{{{0, 0, 0, 0}}, 0, MMIO_SINGLE_BAR_TYPE, 0, 0x0},
		mmio_client_imc_common_offset
	},
	{
		{{{0, 0, 0, 0}}, 0, 0, 0, 0x0},
		{{{0, 0, 0, 0}}, 0, 0, 0, 0x0},
		NULL
	}
};

static PMU_MMIO_UNIT_INFO_NODE skx_mmio_list[] = {
	{
		{{{0, 0, 0, 0x10}}, 0, MMIO_SINGLE_BAR_TYPE, 0, 0x3FFFFFFF8000ULL},
		{{{0, 0, 0, 0}}, 0, MMIO_SINGLE_BAR_TYPE, 0, 0x0},
		mmio_fpga_common_offset
	},
	{
		{{{0, 0, 0, 0x10}}, 0, MMIO_SINGLE_BAR_TYPE, 0, 0x3ffffffff0000ULL},
		{{{0, 0, 0, 0}}, 0, MMIO_SINGLE_BAR_TYPE, 0, 0x0},
		mmio_hfi_common_offset
	},
	{
		{{{0, 0, 0, 0}}, 0, 0, 0, 0x0},
		{{{0, 0, 0, 0}}, 0, 0, 0, 0x0},
		NULL
	}
};

static PMU_MMIO_UNIT_INFO_NODE server_common_mmio_list[] = {
	{
		{{{0, 0, 0, 0x10}}, 0, MMIO_SINGLE_BAR_TYPE, 0, 0x3ffffffff0000ULL},
		{{{0, 0, 0, 0}}, 0, MMIO_SINGLE_BAR_TYPE, 0, 0x0},
		mmio_hfi_common_offset
	},
	{
		{{{0, 0, 0, 0}}, 0, 0, 0, 0x0},
		{{{0, 0, 0, 0}}, 0, 0, 0, 0x0},
		NULL
	}
};

static PMU_MMIO_UNIT_INFO_NODE bxt_dvt_mmio_info_list[] = {
        {
                {{{0, 0, 0, 0x48}}, 0, MMIO_SINGLE_BAR_TYPE, 0, 0x0007FFFFF8000LL},
                {{{0, 0, 0, 0}}, 0, MMIO_SINGLE_BAR_TYPE, 0, 0x0},
                mmio_bxt_dvt_imc_offset
        },
        {
                {{{0, 0, 0, 0}}, 0, 0, 0, 0x0},
                {{{0, 0, 0, 0}}, 0, 0, 0, 0x0},
                NULL
        }
};

static PMU_MMIO_UNIT_INFO_NODE snr_mmio_info_list[] = {
        {
                {{{0, 0, 1, 0xD0}}, 23, MMIO_DUAL_BAR_TYPE, 0, 0x1FFFFFFFULL},
                {{{0, 0, 1, 0xD8}}, 12, MMIO_DUAL_BAR_TYPE, 0, 0x7FFULL},
                mmio_dual_bar_imc_offset
        },
        {
                {{{0, 0, 1, 0xD0}}, 23, MMIO_DUAL_BAR_TYPE, 0, 0x1FFFFFFFULL},
                {{{0, 0, 1, 0xDC}}, 12, MMIO_DUAL_BAR_TYPE, 0, 0x7FFULL},
                mmio_dual_bar_imc_offset
        },
        {
                {{{0, 0, 0, 0x0}}, 0, MMIO_DUAL_BAR_TYPE, 0, 0x0},
                {{{0, 0, 0, 0x0}}, 0, MMIO_DUAL_BAR_TYPE, 0, 0x0},
                mmio_dual_bar_imc_offset
        },
        {
                {{{0, 0, 0, 0}}, 0, 0, 0, 0x0},
                {{{0, 0, 0, 0}}, 0, 0, 0, 0x0},
                NULL
        }
};

#endif

