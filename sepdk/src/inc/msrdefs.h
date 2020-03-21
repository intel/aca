/*COPYRIGHT**
    Copyright (C) 2011-2020 Intel Corporation.  All Rights Reserved.

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






#ifndef _MSRDEFS_H_
#define _MSRDEFS_H_

/*
 * Arch Perf monitoring version 3
 */
#define IA32_PMC0                   0x0C1
#define IA32_PMC1                   0x0C2
#define IA32_PMC2                   0x0C3
#define IA32_PMC3                   0x0C4
#define IA32_PMC4                   0x0C5
#define IA32_PMC5                   0x0C6
#define IA32_PMC6                   0x0C7
#define IA32_PMC7                   0x0C8
#define IA32_FULL_PMC0              0x4C1
#define IA32_FULL_PMC1              0x4C2
#define IA32_PERFEVTSEL0            0x186
#define IA32_PERFEVTSEL1            0x187
#define IA32_FIXED_CTR0             0x309
#define IA32_FIXED_CTR1             0x30A
#define IA32_FIXED_CTR2             0x30B
#define IA32_FIXED_CTR3             0x30C
#define IA32_PERF_CAPABILITIES      0x345
#define IA32_FIXED_CTRL             0x38D
#define IA32_PERF_GLOBAL_STATUS     0x38E
#define IA32_PERF_GLOBAL_CTRL       0x38F
#define IA32_PERF_GLOBAL_OVF_CTRL   0x390
#define IA32_PEBS_ENABLE            0x3F1
#define IA32_MISC_ENABLE            0x1A0
#define IA32_DS_AREA                0x600
#define IA32_DEBUG_CTRL             0x1D9
#undef  IA32_LBR_FILTER_SELECT
#define IA32_LBR_FILTER_SELECT      0x1c8
#define IA32_PEBS_FRONTEND          0x3F7
#define IA32_PERF_METRICS           0x329

#define COMPOUND_CTR_CTL            0x306
#define COMPOUND_PERF_CTR           0x307
#define COMPOUND_CTR_OVF_BIT        0x800
#define COMPOUND_CTR_OVF_SHIFT      12

#define FIXED_CORE_CYCLE_GLOBAL_CTRL_MASK       0x200000000
#define FIXED_CORE_CYCLE_FIXED_CTRL_MASK        0xF0

// REG INDEX inside GLOBAL CTRL SECTION
enum {
    GLOBAL_CTRL_REG_INDEX = 0,
    GLOBAL_OVF_CTRL_REG_INDEX,
    PEBS_ENABLE_REG_INDEX,
    DEBUG_CTRL_REG_INDEX,
    FIXED_CTRL_REG_INDEX,
};

// REG INDEX inside GLOBAL STATUS SECTION
enum {
    GLOBAL_STATUS_REG_INDEX = 0,
};

#endif

