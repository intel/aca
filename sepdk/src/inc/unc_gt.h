/*COPYRIGHT**
    Copyright (C) 2005-2020 Intel Corporation.  All Rights Reserved.

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





#ifndef _UNC_GT_H_INC_
#define _UNC_GT_H_INC_

/*
 * Local to this architecture: SNB uncore GT unit
 *
 */
#define GT_MMIO_SIZE                   0x200000
#define NEXT_ADDR_OFFSET               4
#define UNC_GT_BAR_MASK                0x7FFF000000
#define PERF_GLOBAL_CTRL               0x391
#define GT_CLEAR_COUNTERS              0xFFFF0000

#define GT_DID_1                       0x102
#define INTEL_VENDOR_ID                0x8086
#define DRV_GET_PCI_VENDOR_ID(value)   value & 0x0000FFFF
#define DRV_GET_PCI_DEVICE_ID(value)   (value & 0xFFFF0000) >> 16
#define DRV_IS_INTEL_VENDOR_ID(value)  value == INTEL_VENDOR_ID
#define DRV_IS_GT_DEVICE_ID(value)     value == GT_DID_1

//clock gating disable values
#define UNC_GT_GCPUNIT_REG1            0x9400
#define UNC_GT_GCPUNIT_REG2            0x9404
#define UNC_GT_GCPUNIT_REG3            0x9408
#define UNC_GT_GCPUNIT_REG4            0x940c
#define UNC_GT_GCPUNIT_REG1_VALUE      0xffffffff
#define UNC_GT_GCPUNIT_REG2_VALUE      0xffffffff
#define UNC_GT_GCPUNIT_REG3_VALUE      0xffe3ffff
#define UNC_GT_GCPUNIT_REG4_VALUE      0x00000003
//RC6 disable
#define UNC_GT_RC6_REG1                0xa090
#define UNC_GT_RC6_REG2                0xa094
#define UNC_GT_RC6_REG1_OR_VALUE       0x80000000
#define UNC_GT_RC6_REG2_VALUE          0x00000000
extern  DISPATCH_NODE                  unc_gt_dispatch;

typedef struct GT_CTR_NODE_S  GT_CTR_NODE;
typedef        GT_CTR_NODE   *GT_CTR;
struct GT_CTR_NODE_S
{
    union
    {
        struct
        {
          U32 low: 32;
          U32 high : 12;
        } bits;
       U64 value;
    } u;
};

#define GT_CTR_NODE_value(x)        x.u.value
#define GT_CTR_NODE_low(x)          x.u.bits.low
#define GT_CTR_NODE_high(x)         x.u.bits.high
#define GT_CTR_NODE_value_reset(x)  x.u.value = 0

#endif

