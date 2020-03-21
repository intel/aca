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

#ifndef _COLLECTION_TRACES_H_
#define _COLLECTION_TRACES_H_

// Auto collection mode allows to collect samples
// in a prespecified configuration
// without any communication from host.
// The sampled data will be buffered on target memory
// until host connection is made to fetch them over network.

// Set the default mode to TRUE to enable the auto collection mode
// 0 : No auto collection
// 1 : Fixed counters collection only
// 2 : Fixed counters and LBR data collection
#define DEFAULT_AUTO_COLLECTION_MODE           2

// Set the collection time in second
#define ADV_HOTSPOT_COLLECTION_DURATION_SEC    10

// Set the per-cpu sample buffer size in memory
// Samples captured after buffer gets full will be discarded.
#define AUTO_COLLECTION_SAMP_BUFFER_SIZE       (1<<22)   // 4MB per cpu

#define AUTO_COLLECTION_MOD_BUFFER_CHUNK_SIZE  4096      // Increase by 4KB

#define ADV_HOTSPOT_COLLECTION_TRACE_LENGTH        21
#define ADV_HOTSPOT_LBR_COLLECTION_TRACE_LENGTH    22

#define CMD_TRACE_SIZE    24
#define ARG_TRACE_SIZE    4300

extern S8        adv_hotspot_cmd_traces[ADV_HOTSPOT_COLLECTION_TRACE_LENGTH][CMD_TRACE_SIZE];
extern S8        adv_hotspot_arg_traces[ADV_HOTSPOT_COLLECTION_TRACE_LENGTH][ARG_TRACE_SIZE];
//extern S8       *adv_hotspot_ret_traces[ADV_HOTSPOT_COLLECTION_TRACE_LENGTH];

extern S8        adv_hotspot_lbr_cmd_traces[ADV_HOTSPOT_LBR_COLLECTION_TRACE_LENGTH][CMD_TRACE_SIZE];
extern S8        adv_hotspot_lbr_arg_traces[ADV_HOTSPOT_LBR_COLLECTION_TRACE_LENGTH][ARG_TRACE_SIZE];
//extern S8       *adv_hotspot_lbr_ret_traces[ADV_HOTSPOT_LBR_COLLECTION_TRACE_LENGTH];

extern U32       auto_collection_mode;
extern U32       trace_length;
extern S8        *cmd_traces;
extern S8        *arg_traces;
extern S8        **ret_traces;

#endif
