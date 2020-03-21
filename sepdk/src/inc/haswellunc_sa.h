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





#ifndef _HSWUNC_SA_H_INC_
#define _HSWUNC_SA_H_INC_

/*
 * Local to this architecture: Haswell uncore SA unit 
 * 
 */
#define HSWUNC_SA_DESKTOP_DID                 0x000C04
#define HSWUNC_SA_NEXT_ADDR_OFFSET            4
#define HSWUNC_SA_BAR_ADDR_SHIFT              32
#define HSWUNC_SA_BAR_ADDR_MASK               0x0007FFFFFF000LL
#define HSWUNC_SA_MAX_PCI_DEVICES             16
#define HSWUNC_SA_MAX_COUNT                   0x00000000FFFFFFFFLL
#define HSWUNC_SA_MAX_COUNTERS                8

#define HSWUNC_SA_MCHBAR_MMIO_PAGE_SIZE       8*4096
#define HSWUNC_SA_PCIEXBAR_MMIO_PAGE_SIZE     57*4096
#define HSWUNC_SA_OTHER_BAR_MMIO_PAGE_SIZE    4096
#define HSWUNC_SA_GDXCBAR_OFFSET_LO           0x5420
#define HSWUNC_SA_GDXCBAR_OFFSET_HI           0x5424
#define HSWUNC_SA_GDXCBAR_MASK                0x7FFFFFF000LL
#define HSWUNC_SA_CHAP_SAMPLE_DATA            0x00020000
#define HSWUNC_SA_CHAP_STOP                   0x00040000
#define HSWUNC_SA_CHAP_CTRL_REG_OFFSET        0x0

#define HSWUNC_SA_PAGE_MASK                   0xfffffffffffff000
#define HSWUNC_SA_PAGE_OFFSET_MASK            0xfff
#define HSWUNC_SA_PAGE_SIZE                   0x1000


extern DISPATCH_NODE  hswunc_sa_dispatch;

#endif

