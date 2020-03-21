/*COPYRIGHT**
    Copyright (C) 2013-2020 Intel Corporation.  All Rights Reserved.

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





#ifndef _PERFVER4_H_
#define _PERFVER4_H_

#include "msrdefs.h"

extern DISPATCH_NODE  perfver4_dispatch;
extern DISPATCH_NODE  perfver4_dispatch_htoff_mode;
extern DISPATCH_NODE  perfver4_dispatch_nonht_mode;

#define PERFVER4_UNC_BLBYPASS_BITMASK      0x00000001
#define PERFVER4_UNC_DISABLE_BL_BYPASS_MSR 0x39C

#if defined(DRV_IA32)
#define PERFVER4_LBR_DATA_BITS             32
#else
#define PERFVER4_LBR_DATA_BITS             57
#endif

#define PERFVER4_LBR_BITMASK               ((1ULL << PERFVER4_LBR_DATA_BITS) -1)

#define PERFVER4_FROZEN_BIT_MASK           0xc00000000000000ULL
#define PERFVER4_OVERFLOW_BIT_MASK_HT_ON   0x600000070000000FULL
#define PERFVER4_OVERFLOW_BIT_MASK_HT_OFF  0x60000007000000FFULL
#define PERFVER4_OVERFLOW_BIT_MASK_NON_HT  0x6000000F000000FFULL

#endif

