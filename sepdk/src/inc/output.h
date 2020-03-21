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





#ifndef _OUTPUT_H_
#define _OUTPUT_H_

#include <linux/timer.h>
#include <linux/vmalloc.h>

/*
 * Initial allocation
 * Size of buffer     = 512KB (2^19)
 * number of buffers  = 2
 * The max size of the buffer cannot exceed 1<<22 i.e. 4MB
 */
#define OUTPUT_SMALL_BUFFER        (1<<15)
#define OUTPUT_LARGE_BUFFER        (1<<19)
#define OUTPUT_CP_BUFFER           (1<<22)
#define OUTPUT_EMON_BUFFER         (1<<25)
#define OUTPUT_MEMORY_THRESHOLD    0x8000000

extern U32                         output_buffer_size;
extern U32                         saved_buffer_size;
#define OUTPUT_BUFFER_SIZE         output_buffer_size
#define OUTPUT_NUM_BUFFERS         2
#if defined (DRV_ANDROID)
#define MODULE_BUFF_SIZE           1
#else
#define MODULE_BUFF_SIZE           2
#endif


/*
 *  Data type declarations and accessors macros
 */
typedef struct {
    spinlock_t  buffer_lock;
    U32         remaining_buffer_size;
    U32         current_buffer;
    U32         total_buffer_size;
    U32         next_buffer[OUTPUT_NUM_BUFFERS];
    U32         buffer_full[OUTPUT_NUM_BUFFERS];
    U8         *buffer[OUTPUT_NUM_BUFFERS];
    U32         signal_full;
    DRV_BOOL    tasklet_queued;
} OUTPUT_NODE, *OUTPUT;

#define OUTPUT_buffer_lock(x)            (x)->buffer_lock
#define OUTPUT_remaining_buffer_size(x)  (x)->remaining_buffer_size
#define OUTPUT_total_buffer_size(x)      (x)->total_buffer_size
#define OUTPUT_buffer(x,y)               (x)->buffer[(y)]
#define OUTPUT_buffer_full(x,y)          (x)->buffer_full[(y)]
#define OUTPUT_current_buffer(x)         (x)->current_buffer
#define OUTPUT_signal_full(x)            (x)->signal_full
#define OUTPUT_tasklet_queued(x)         (x)->tasklet_queued
/*
 *  Add an array of control buffer for per-cpu
 */
typedef struct {
    wait_queue_head_t queue;
    OUTPUT_NODE      outbuf;
    U32              sample_count;
} BUFFER_DESC_NODE, *BUFFER_DESC;

#define BUFFER_DESC_queue(a)          (a)->queue
#define BUFFER_DESC_outbuf(a)         (a)->outbuf
#define BUFFER_DESC_sample_count(a)   (a)->sample_count

extern BUFFER_DESC   cpu_buf;  // actually an array of BUFFER_DESC_NODE
extern BUFFER_DESC   unc_buf;
extern BUFFER_DESC   module_buf;
extern BUFFER_DESC   cpu_sideband_buf;
extern BUFFER_DESC   emon_buf;
/*
 *  Interface Functions
 */

extern int       OUTPUT_Module_Fill (PVOID data, U16 size, U8 in_notification);
extern OS_STATUS OUTPUT_Initialize (void);
extern OS_STATUS OUTPUT_Initialize_UNC (void);
extern OS_STATUS OUTPUT_Initialize_EMON (void);
extern void      OUTPUT_Cleanup (VOID);
extern void      OUTPUT_Cleanup (VOID);
extern int       OUTPUT_Destroy (VOID);
extern int       OUTPUT_Flush (VOID);
extern int       OUTPUT_Flush_EMON (VOID);
extern ssize_t   OUTPUT_Module_Read (struct file *filp, char *buf, size_t count, loff_t *f_pos);
extern ssize_t   OUTPUT_Sample_Read (struct file *filp, char *buf, size_t count, loff_t *f_pos);
extern ssize_t   OUTPUT_UncSample_Read (struct file *filp, char *buf, size_t count, loff_t *f_pos);
extern ssize_t   OUTPUT_SidebandInfo_Read (struct file *filp, char *buf, size_t count, loff_t *f_pos);
extern ssize_t   OUTPUT_Emon_Read (struct file *filp, char *buf, size_t count, loff_t *f_pos);
extern void*     OUTPUT_Reserve_Buffer_Space (BUFFER_DESC  bd, U32 size, DRV_BOOL defer, U8 in_notification);
extern void*     OUTPUT_Get_Buffer (BUFFER_DESC  bd);

#endif

