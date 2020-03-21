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






#ifndef _JKTUNC_QPILL_H_INC_
#define _JKTUNC_QPILL_H_INC_

/*
 * Local to this architecture: JKT uncore QPILL unit 
 * 
 */
#define JKTUNC_QPILL0_DID                  0x3C41 // --- QPILL0 PerfMon DID --- B:D 1:8:2
#define JKTUNC_QPILL_MM0_DID               0x3C86 // --- QPILL0 PerfMon MM Config DID --- B:D 1:8:6
#define JKTUNC_QPILL1_DID                  0x3C42 // --- QPILL1 PerfMon DID --- B:D 1:9:2
#define JKTUNC_QPILL2_DID                  0x3C44 // --- QPILL0 PerfMon DID --- B:D 1:8:2
#define JKTUNC_QPILL3_DID                  0x3C45 // --- QPILL0 PerfMon DID --- B:D 1:8:2
#define JKTUNC_QPILL_MM1_DID               0x3C96 // --- QPILL1 PerfMon MM Config DID --- B:D 1:9:6
#define JKTUNC_QPILL_MCFG_DID              0x3C28 // --- QPILL1 PerfMon MCFG DID --- B:D 0:5:0
#define JKTUNC_QPILL0_D2C_DID              0x3C80 // --- D2C QPILL Port 1 config DID B:D:F X:8:0
#define JKTUNC_QPILL1_D2C_DID              0x3C90 // --- D2C QPILL Port 2 config DID B:D:F X:9:0

#define JKTUNC_QPILL_PERF_GLOBAL_CTRL      0x391

#define IA32_DEBUG_CTRL                    0x1D9

#define JKTUNC_QPILL_D2C_OFFSET            0x80
#define JKTUNC_QPILL_D2C_BITMASK           0x00000002
#define JKTUNC_QPILL_FUNC_NO               2
#define JKTUNC_QPILL_D2C_FUNC_NO           0

extern DISPATCH_NODE  jktunc_qpill_dispatch;

#endif 

