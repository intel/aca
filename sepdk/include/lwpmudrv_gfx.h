/****
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






****/

#ifndef _LWPMUDRV_GFX_H_
#define _LWPMUDRV_GFX_H_

#if defined(__cplusplus)
extern "C" {
#endif

#define GFX_BASE_ADDRESS       0xFF200000
#define GFX_BASE_NEW_OFFSET    0x00080000
#define GFX_PERF_REG           0x040         // location of GFX counter relative to base
#define GFX_NUM_COUNTERS       9             // max number of GFX counters per counter group
#define GFX_CTR_OVF_VAL        0xFFFFFFFF    // overflow value for GFX counters

#define GFX_REG_CTR_CTRL       0x01FF
#define GFX_CTRL_DISABLE       0x1E00

//#define GFX_COMPUTE_DELTAS     1             // use event count deltas instead of raw counts

#if defined(__cplusplus)
}
#endif

#endif

