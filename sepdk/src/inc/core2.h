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





#ifndef _CORE2_H_
#define _CORE2_H_

#include "msrdefs.h"

extern DISPATCH_NODE  core2_dispatch;
extern DISPATCH_NODE  corei7_dispatch;
extern DISPATCH_NODE  corei7_dispatch_nehalem;
extern DISPATCH_NODE  corei7_dispatch_htoff_mode;
extern DISPATCH_NODE  corei7_dispatch_2;
extern DISPATCH_NODE  corei7_dispatch_htoff_mode_2;

#define CORE2UNC_BLBYPASS_BITMASK      0x00000001
#define CORE2UNC_DISABLE_BL_BYPASS_MSR 0x39C

#if defined(DRV_IA32)
#define CORE2_LBR_DATA_BITS            32
#else
#define CORE2_LBR_DATA_BITS            48
#endif

#define CORE2_LBR_BITMASK                    ((1ULL << CORE2_LBR_DATA_BITS) -1)

#endif 

