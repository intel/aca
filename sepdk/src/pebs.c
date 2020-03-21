/*COPYRIGHT**
    Copyright (C) 2012-2020 Intel Corporation.  All Rights Reserved.

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





#include "lwpmudrv_defines.h"
#include <linux/version.h>
#include <linux/percpu.h>
#include <linux/mm.h>
#include <linux/uaccess.h>
#include <asm/segment.h>
#include <asm/page.h>

#include "lwpmudrv_types.h"
#include "rise_errors.h"
#include "lwpmudrv_ecb.h"
#include "lwpmudrv_struct.h"
#include "lwpmudrv.h"
#include "control.h"
#include "core2.h"
#include "utility.h"
#include "output.h"
#include "ecb_iterators.h"
#include "pebs.h"

static PVOID                          pebs_global_memory      = NULL;
static size_t                         pebs_global_memory_size = 0;

extern DRV_CONFIG              drv_cfg;
extern DRV_SETUP_INFO_NODE     req_drv_setup_info;



/* ------------------------------------------------------------------------- */
/*!
 * @fn          VOID pebs_Corei7_Initialize_Threshold (dts, LWPMU_DEVICE_pebs_record_size(&devices[dev_idx]))
 *
 * @brief       The nehalem specific initialization
 *
 * @param       dts  - dts description
 *
 * @return      NONE
 *
 * <I>Special Notes:</I>
 */
static VOID
pebs_Corei7_Initialize_Threshold (
    DTS_BUFFER_EXT   dts
)
{
    U32         this_cpu;
    U32         dev_idx;
    DEV_CONFIG  pcfg;

    SEP_DRV_LOG_TRACE_IN("Dts: %p.", dts);

    this_cpu = CONTROL_THIS_CPU();
    dev_idx  = core_to_dev_map[this_cpu];
    pcfg     = LWPMU_DEVICE_pcfg(&devices[dev_idx]);

    DTS_BUFFER_EXT_pebs_threshold(dts)  = DTS_BUFFER_EXT_pebs_base(dts) + (LWPMU_DEVICE_pebs_record_size(&devices[dev_idx]) * DEV_CONFIG_pebs_record_num(pcfg));

    SEP_DRV_LOG_TRACE_OUT("");
    return;
}

/* ------------------------------------------------------------------------- */
/*!
 * @fn          VOID pebs_Corei7_Overflow ()
 *
 * @brief       The Nehalem specific overflow check
 *
 * @param       this_cpu        - cpu id
 *              overflow_status - overflow status
 *              rec_index       - record index
 *
 * @return      NONE
 *
 * <I>Special Notes:</I>
 *    Check the global overflow field of the buffer descriptor.
 *    Precise events can be allocated on any of the 4 general purpose
 *    registers.
 */
static U64
pebs_Corei7_Overflow (
    S32  this_cpu,
    U64  overflow_status,
    U32  rec_index
)
{
    DTS_BUFFER_EXT   dtes;
    S8              *pebs_base, *pebs_index, *pebs_ptr;
    PEBS_REC_EXT     pb;
    U8               pebs_ptr_check = FALSE;
    U32              dev_idx = core_to_dev_map[this_cpu];

    SEP_DRV_LOG_TRACE_IN("This_cpu: %d, overflow_status: %llx, rec_index: %u.", this_cpu, overflow_status, rec_index);

    dtes = CPU_STATE_dts_buffer(&pcb[this_cpu]);

    SEP_DRV_LOG_TRACE("This_cpu: %d, dtes %p.", this_cpu, dtes);

    if (!dtes) {
        return overflow_status;
    }
    pebs_base      = (S8 *)(UIOP)DTS_BUFFER_EXT_pebs_base(dtes);
    SEP_DRV_LOG_TRACE("This_cpu: %d, pebs_base %p.", this_cpu, pebs_base);
    pebs_index     = (S8 *)(UIOP)DTS_BUFFER_EXT_pebs_index(dtes);
    pebs_ptr       = (S8 *)((UIOP)DTS_BUFFER_EXT_pebs_base(dtes) + (rec_index * LWPMU_DEVICE_pebs_record_size(&devices[dev_idx])));
    pebs_ptr_check = (pebs_ptr && pebs_base != pebs_index && pebs_ptr < pebs_index);
    if (pebs_ptr_check) {
        pb = (PEBS_REC_EXT)pebs_ptr;
        overflow_status |= PEBS_REC_EXT_glob_perf_overflow(pb);
    }

    SEP_DRV_LOG_TRACE_OUT("Res: %llx.", overflow_status);
    return overflow_status;
}

/* ------------------------------------------------------------------------- */
/*!
 * @fn          VOID pebs_Corei7_Overflow_APEBS ()
 *
 * @brief       Overflow check
 *
 * @param       this_cpu        - cpu id
 *              overflow_status - overflow status
 *              rec_index       - record index
 *
 * @return      NONE
 *
 * <I>Special Notes:</I>
 *    Check the global overflow field of the buffer descriptor.
 *    Precise events can be allocated on any of the 8 general purpose
 *    registers or 4 fixed registers.
 */
static U64
pebs_Corei7_Overflow_APEBS (
    S32  this_cpu,
    U64  overflow_status,
    U32  rec_index
)
{
    S8                         *pebs_base, *pebs_index, *pebs_ptr;
    ADAPTIVE_PEBS_BASIC_INFO    pb;
    DTS_BUFFER_EXT1             dtes = CPU_STATE_dts_buffer(&pcb[this_cpu]);
    U8                          pebs_ptr_check = FALSE;
    U32                         dev_idx = core_to_dev_map[this_cpu];
    DEV_CONFIG                  pcfg = LWPMU_DEVICE_pcfg(&devices[dev_idx]);

    if (!dtes) {
        return overflow_status;
    }
    pebs_base      = (S8 *)(UIOP)DTS_BUFFER_EXT1_pebs_base(dtes);
    pebs_index     = (S8 *)(UIOP)DTS_BUFFER_EXT1_pebs_index(dtes);
    pebs_ptr       = (S8 *)((UIOP)DTS_BUFFER_EXT1_pebs_base(dtes) + (rec_index * LWPMU_DEVICE_pebs_record_size(&devices[dev_idx])));
    pebs_ptr_check = (pebs_ptr && pebs_base != pebs_index && pebs_ptr < pebs_index);

    if (pebs_ptr_check && DEV_CONFIG_enable_adaptive_pebs(pcfg)) {
        pb = (ADAPTIVE_PEBS_BASIC_INFO)pebs_ptr;
        overflow_status |= ADAPTIVE_PEBS_BASIC_INFO_applicable_counters(pb);
    }

    return overflow_status;
}


/* ------------------------------------------------------------------------- */
/*!
 * @fn          VOID pebs_Core2_Initialize_Threshold (dts, LWPMU_DEVICE_pebs_record_size(&devices[dev_idx]))
 *
 * @brief       The Core2 specific initialization
 *
 * @param       dts - dts description
 *
 * @return      NONE
 *
 * <I>Special Notes:</I>
 */
static VOID
pebs_Core2_Initialize_Threshold (
    DTS_BUFFER_EXT   dts
)
{
    SEP_DRV_LOG_TRACE_IN("Dts: %p.", dts);

    DTS_BUFFER_EXT_pebs_threshold(dts)  = DTS_BUFFER_EXT_pebs_base(dts);

    SEP_DRV_LOG_TRACE_OUT("");
    return;
}

/* ------------------------------------------------------------------------- */
/*!
 * @fn          VOID pebs_Core2_Overflow (dts, LWPMU_DEVICE_pebs_record_size(&devices[dev_idx]))
 *
 * @brief       The Core2 specific overflow check
 *
 * @param       this_cpu        - cpu id
 *              overflow_status - overflow status
 *              rec_index       - record index
 *
 * @return      NONE
 *
 * <I>Special Notes:</I>
 *    Check the base and the index fields of the circular buffer, if they are
 *    not the same, then a precise event has overflowed.  Precise events are
 *    allocated only on register#0.
 */
static U64
pebs_Core2_Overflow (
    S32  this_cpu,
    U64  overflow_status,
    U32  rec_index
)
{
    DTS_BUFFER_EXT   dtes;
    U8               status   = FALSE;

    SEP_DRV_LOG_TRACE_IN("This_cpu: %d, overflow_status: %llx, rec_index: %u.", this_cpu, overflow_status, rec_index);

    dtes = CPU_STATE_dts_buffer(&pcb[this_cpu]);

    if (!dtes) {
        SEP_DRV_LOG_ERROR_TRACE_OUT("Res: %llx (dtes is NULL!).", overflow_status);
        return overflow_status;
    }
    status = (U8)((dtes) && (DTS_BUFFER_EXT_pebs_index(dtes) != DTS_BUFFER_EXT_pebs_base(dtes)));
    if (status) {
        // Merom allows only for general purpose register 0 to be precise capable
        overflow_status  |= 0x1;
    }

    SEP_DRV_LOG_TRACE_OUT("Res: %llx.", overflow_status);
    return overflow_status;
}

/* ------------------------------------------------------------------------- */
/*!
 * @fn          VOID pebs_Modify_IP (sample, is_64bit_addr)
 *
 * @brief       Change the IP field in the sample to that in the PEBS record
 *
 * @param       sample        - sample buffer
 * @param       is_64bit_addr - are we in a 64 bit module
 *
 * @return      NONE
 *
 * <I>Special Notes:</I>
 *              <NONE>
 */
static VOID
pebs_Modify_IP (
    void        *sample,
    DRV_BOOL     is_64bit_addr,
    U32          rec_index
)
{
    SampleRecordPC  *psamp = sample;
    DTS_BUFFER_EXT   dtes;
    S8              *pebs_base, *pebs_index, *pebs_ptr;
    PEBS_REC_EXT     pb;
    U8               pebs_ptr_check = FALSE;
    U32              this_cpu;
    U32              dev_idx;

    SEP_DRV_LOG_TRACE_IN("Sample: %p, is_64bit_addr: %u, rec_index: %u.", sample, is_64bit_addr, rec_index);

    this_cpu = CONTROL_THIS_CPU();
    dev_idx  = core_to_dev_map[this_cpu];
    dtes     = CPU_STATE_dts_buffer(&pcb[this_cpu]);

    if (!dtes || !psamp) {
        return;
    }
    SEP_DRV_LOG_TRACE("In PEBS Fill Buffer: cpu %d.", CONTROL_THIS_CPU());
    pebs_base      = (S8 *)(UIOP)DTS_BUFFER_EXT_pebs_base(dtes);
    pebs_index     = (S8 *)(UIOP)DTS_BUFFER_EXT_pebs_index(dtes);
    pebs_ptr       = (S8 *)((UIOP)DTS_BUFFER_EXT_pebs_base(dtes) + (rec_index * LWPMU_DEVICE_pebs_record_size(&devices[dev_idx])));
    pebs_ptr_check = (pebs_ptr && pebs_base != pebs_index && pebs_ptr < pebs_index);
    if (pebs_ptr_check) {
        pb = (PEBS_REC_EXT)pebs_ptr;
        if (is_64bit_addr) {
            SAMPLE_RECORD_iip(psamp)    = PEBS_REC_EXT_linear_ip(pb);
            SAMPLE_RECORD_ipsr(psamp)   = PEBS_REC_EXT_r_flags(pb);
        }
        else {
            SAMPLE_RECORD_eip(psamp)    = PEBS_REC_EXT_linear_ip(pb) & 0xFFFFFFFF;
            SAMPLE_RECORD_eflags(psamp) = PEBS_REC_EXT_r_flags(pb) & 0xFFFFFFFF;
        }
    }
    SEP_DRV_LOG_TRACE_OUT("");
    return;
}

/* ------------------------------------------------------------------------- */
/*!
 * @fn          VOID pebs_Modify_IP_With_Eventing_IP (sample, is_64bit_addr)
 *
 * @brief       Change the IP field in the sample to that in the PEBS record
 *
 * @param       sample        - sample buffer
 * @param       is_64bit_addr - are we in a 64 bit module
 *
 * @return      NONE
 *
 * <I>Special Notes:</I>
 *              <NONE>
 */
static VOID
pebs_Modify_IP_With_Eventing_IP (
    void        *sample,
    DRV_BOOL     is_64bit_addr,
    U32          rec_index
)
{
    SampleRecordPC             *psamp = sample;
    DTS_BUFFER_EXT              dtes;
    S8                         *pebs_ptr, *pebs_base, *pebs_index;
    U64                         ip = 0, flags = 0;
    U8                          pebs_ptr_check = FALSE;
    U32                         this_cpu;
    U32                         dev_idx;
    DEV_CONFIG                  pcfg;

    SEP_DRV_LOG_TRACE_IN("Sample: %p, is_64bit_addr: %u, rec_index: %u.", sample, is_64bit_addr, rec_index);

    this_cpu = CONTROL_THIS_CPU();
    dev_idx  = core_to_dev_map[this_cpu];
    pcfg     = LWPMU_DEVICE_pcfg(&devices[dev_idx]);
    dtes     = CPU_STATE_dts_buffer(&pcb[this_cpu]);

    if (!dtes || !psamp) {
        return;
    }

    pebs_base      = (S8 *)(UIOP)DTS_BUFFER_EXT_pebs_base(dtes);
    pebs_index     = (S8 *)(UIOP)DTS_BUFFER_EXT_pebs_index(dtes);
    pebs_ptr       = (S8 *)((UIOP)DTS_BUFFER_EXT_pebs_base(dtes) + (rec_index * LWPMU_DEVICE_pebs_record_size(&devices[dev_idx])));
    pebs_ptr_check = (pebs_ptr && pebs_base != pebs_index && pebs_ptr < pebs_index);

    if (!pebs_ptr_check) {
        return;
    }
    if (DEV_CONFIG_enable_adaptive_pebs(pcfg)) {
        ip = ADAPTIVE_PEBS_BASIC_INFO_eventing_ip((ADAPTIVE_PEBS_BASIC_INFO)pebs_ptr);
        if (DEV_CONFIG_apebs_collect_gpr(pcfg)) {
            flags = ADAPTIVE_PEBS_GPR_INFO_rflags((ADAPTIVE_PEBS_GPR_INFO)
                                           (pebs_ptr + LWPMU_DEVICE_apebs_gpr_offset(&devices[dev_idx])));
        }
    }
    else {
        ip    = PEBS_REC_EXT1_eventing_ip((PEBS_REC_EXT1)pebs_ptr);
        flags = PEBS_REC_EXT1_r_flags((PEBS_REC_EXT1)pebs_ptr);
    }
    if (is_64bit_addr) {
        SAMPLE_RECORD_iip(psamp) = ip;
        SAMPLE_RECORD_ipsr(psamp) = flags;
    }
    else {
        SAMPLE_RECORD_eip(psamp)    = ip & 0xFFFFFFFF;
        SAMPLE_RECORD_eflags(psamp) = flags & 0xFFFFFFFF;
    }
    SEP_DRV_LOG_TRACE_OUT("");
    return;
}

/* ------------------------------------------------------------------------- */
/*!
 * @fn          VOID pebs_Modify_TSC (sample)
 *
 * @brief       Change the TSC field in the sample to that in the PEBS record
 *
 * @param       sample        - sample buffer
 *              rec_index     - record index
 * @return      NONE
 *
 * <I>Special Notes:</I>
 *              <NONE>
 */
static VOID
pebs_Modify_TSC (
    void        *sample,
    U32          rec_index
)
{
    SampleRecordPC *psamp = sample;
    DTS_BUFFER_EXT  dtes;
    S8             *pebs_base, *pebs_index, *pebs_ptr;
    U64             tsc;
    U8              pebs_ptr_check = FALSE;
    U32             this_cpu;
    U32             dev_idx;
    DEV_CONFIG      pcfg;

    SEP_DRV_LOG_TRACE_IN("Sample: %p, rec_index: %u.", sample, rec_index);

    this_cpu = CONTROL_THIS_CPU();
    dev_idx  = core_to_dev_map[this_cpu];
    pcfg     = LWPMU_DEVICE_pcfg(&devices[dev_idx]);
    dtes     = CPU_STATE_dts_buffer(&pcb[this_cpu]);

    if (!dtes || !psamp) {
        return;
    }
    pebs_base      = (S8 *)(UIOP)DTS_BUFFER_EXT_pebs_base(dtes);
    pebs_index     = (S8 *)(UIOP)DTS_BUFFER_EXT_pebs_index(dtes);
    pebs_ptr       = (S8 *)((UIOP)DTS_BUFFER_EXT_pebs_base(dtes) + (rec_index * LWPMU_DEVICE_pebs_record_size(&devices[dev_idx])));
    pebs_ptr_check = (pebs_ptr && pebs_base != pebs_index && pebs_ptr < pebs_index);
    if (!pebs_ptr_check) {
        return;
    }

    if (DEV_CONFIG_enable_adaptive_pebs(pcfg)) {
        tsc = ADAPTIVE_PEBS_BASIC_INFO_tsc((ADAPTIVE_PEBS_BASIC_INFO)pebs_ptr);
    }
    else {
        tsc = PEBS_REC_EXT2_tsc((PEBS_REC_EXT2)pebs_ptr);
    }
    SAMPLE_RECORD_tsc(psamp) = tsc;
    SEP_DRV_LOG_TRACE_OUT("");
    return;
}
/* ------------------------------------------------------------------------- */
/*!
 * @fn          U32 pebs_Get_Num_Records_Filled ()
 *
 * @brief       get number of PEBS records filled in PEBS buffer
 *
 * @param       NONE
 *
 * @return      NONE
 *
 * <I>Special Notes:</I>
 *              <NONE>
 */
static U32
pebs_Get_Num_Records_Filled (
    VOID
)
{
    U32              num = 0;
    DTS_BUFFER_EXT   dtes;
    S8              *pebs_base, *pebs_index;
    U32              this_cpu;
    U32              dev_idx;

    SEP_DRV_LOG_TRACE_IN("");

    this_cpu = CONTROL_THIS_CPU();
    dev_idx  = core_to_dev_map[this_cpu];
    dtes     = CPU_STATE_dts_buffer(&pcb[this_cpu]);

    if (!dtes) {
        return num;
    }
    pebs_base  = (S8 *)(UIOP)DTS_BUFFER_EXT_pebs_base(dtes);
    pebs_index = (S8 *)(UIOP)DTS_BUFFER_EXT_pebs_index(dtes);
    if (pebs_base != pebs_index) {
        num = (U32)(pebs_index - pebs_base) / LWPMU_DEVICE_pebs_record_size(&devices[dev_idx]);
    }

    SEP_DRV_LOG_TRACE_OUT("Res: %u.", num);
    return num;
}

/*
 * Initialize the pebs micro dispatch tables
 */
PEBS_DISPATCH_NODE  core2_pebs =
{
     pebs_Core2_Initialize_Threshold,
     pebs_Core2_Overflow,
     pebs_Modify_IP,
     NULL,
     pebs_Get_Num_Records_Filled
};

PEBS_DISPATCH_NODE  core2p_pebs =
{
     pebs_Corei7_Initialize_Threshold,
     pebs_Core2_Overflow,
     pebs_Modify_IP,
     NULL,
     pebs_Get_Num_Records_Filled
};

PEBS_DISPATCH_NODE  corei7_pebs =
{
     pebs_Corei7_Initialize_Threshold,
     pebs_Corei7_Overflow,
     pebs_Modify_IP,
     NULL,
     pebs_Get_Num_Records_Filled
};

PEBS_DISPATCH_NODE  haswell_pebs =
{
     pebs_Corei7_Initialize_Threshold,
     pebs_Corei7_Overflow,
     pebs_Modify_IP_With_Eventing_IP,
     NULL,
     pebs_Get_Num_Records_Filled
};

PEBS_DISPATCH_NODE  perfver4_pebs =
{
     pebs_Corei7_Initialize_Threshold,
     pebs_Corei7_Overflow,
     pebs_Modify_IP_With_Eventing_IP,
     pebs_Modify_TSC,
     pebs_Get_Num_Records_Filled
};

PEBS_DISPATCH_NODE perfver4_apebs = // adaptive PEBS
{
    pebs_Corei7_Initialize_Threshold,
    pebs_Corei7_Overflow_APEBS,
    pebs_Modify_IP_With_Eventing_IP,
    pebs_Modify_TSC,
    pebs_Get_Num_Records_Filled
};

#define PER_CORE_BUFFER_SIZE(dts_size, record_size, record_num) (dts_size + (record_num + 1) * (record_size) + 64)
/* ------------------------------------------------------------------------- */
/*!
 * @fn          VOID* pebs_Alloc_DTS_Buffer (VOID)
 *
 * @brief       Allocate buffers used for latency and pebs sampling
 *
 * @param       NONE
 *
 * @return      NONE
 *
 * <I>Special Notes:</I>
 *              Allocate the memory needed to hold the DTS and PEBS records buffer.
 *              This routine is called by a thread that corresponds to a single core
 */
static VOID*
pebs_Alloc_DTS_Buffer (
    VOID
)
{
    UIOP            pebs_base;
    U32             dts_size;
    PVOID           dts_buffer = NULL;
    DTS_BUFFER_EXT  dts;
    int             this_cpu;
    CPU_STATE       pcpu;
    U32             dev_idx;
    DEV_CONFIG      pcfg;
    PEBS_DISPATCH   pebs_dispatch;

    SEP_DRV_LOG_TRACE_IN("");

    /*
     * one PEBS record... need 2 records so that
     * threshold can be less than absolute max
     */
    preempt_disable();
    this_cpu = CONTROL_THIS_CPU();
    preempt_enable();
    dts_size      = sizeof(DTS_BUFFER_EXT_NODE);
    pcpu          = &pcb[this_cpu];
    dev_idx       = core_to_dev_map[this_cpu];
    pcfg          = LWPMU_DEVICE_pcfg(&devices[dev_idx]);
    pebs_dispatch = LWPMU_DEVICE_pebs_dispatch(&devices[dev_idx]);

    if (DEV_CONFIG_enable_adaptive_pebs(pcfg) ||
        DEV_CONFIG_collect_fixed_counter_pebs(pcfg)) {
        dts_size = sizeof(DTS_BUFFER_EXT1_NODE);
    }

    /*
     * account for extra bytes to align PEBS base to cache line boundary
     */
    if (DRV_SETUP_INFO_page_table_isolation(&req_drv_setup_info) == DRV_SETUP_INFO_PTI_KPTI) {
        SEP_DRV_LOG_ERROR_TRACE_OUT("KPTI is enabled without PAGE_TABLE_ISOLATION kernel configuration!");
        return NULL;
    }
    else {
        dts_buffer = (char *)pebs_global_memory + CPU_STATE_dts_buffer_offset(pcpu);
        if (!dts_buffer) {
            SEP_DRV_LOG_ERROR_TRACE_OUT("NULL (failed to allocate space for DTS buffer!).");
            return NULL;
        }
        pebs_base = (UIOP)(dts_buffer) + dts_size;

        CPU_STATE_dts_buffer(pcpu)          = dts_buffer;
        CPU_STATE_dts_buffer_size(pcpu)     = PER_CORE_BUFFER_SIZE(dts_size, LWPMU_DEVICE_pebs_record_size(&devices[dev_idx]), DEV_CONFIG_pebs_record_num(pcfg));

        //  Make 32 byte aligned
        if ((pebs_base & 0x000001F) != 0x0) {
            pebs_base = ALIGN_32(pebs_base);
        }

        dts = (DTS_BUFFER_EXT)dts_buffer;
    }

    /*
     * Program the DTES Buffer for Precise EBS.
     * Set PEBS buffer for one PEBS record
     */
    DTS_BUFFER_EXT_base(dts)            = 0;
    DTS_BUFFER_EXT_index(dts)           = 0;
    DTS_BUFFER_EXT_max(dts)             = 0;
    DTS_BUFFER_EXT_threshold(dts)       = 0;
    DTS_BUFFER_EXT_pebs_base(dts)       = pebs_base;
    DTS_BUFFER_EXT_pebs_index(dts)      = pebs_base;
    DTS_BUFFER_EXT_pebs_max(dts)        = pebs_base + (DEV_CONFIG_pebs_record_num(pcfg) + 1) * LWPMU_DEVICE_pebs_record_size(&devices[dev_idx]);

    pebs_dispatch->initialize_threshold(dts);

    SEP_DRV_LOG_TRACE("base --- %llx.", DTS_BUFFER_EXT_pebs_base(dts));
    SEP_DRV_LOG_TRACE("index --- %llu.", DTS_BUFFER_EXT_pebs_index(dts));
    SEP_DRV_LOG_TRACE("max --- %llu.", DTS_BUFFER_EXT_pebs_max(dts));
    SEP_DRV_LOG_TRACE("threahold --- %llu.", DTS_BUFFER_EXT_pebs_threshold(dts));
    SEP_DRV_LOG_TRACE("DTES buffer allocated for PEBS: %p.", dts_buffer);

    SEP_DRV_LOG_TRACE_OUT("Res: %p.", dts_buffer);
    return dts;
}

/* ------------------------------------------------------------------------- */
/*!
 * @fn          VOID* pebs_Allocate_Buffers (VOID *params)
 *
 * @brief       Allocate memory and set up MSRs in preparation for PEBS
 *
 * @param       NONE
 *
 * @return      NONE
 *
 * <I>Special Notes:</I>
 *              Set up the DS area and program the DS_AREA msrs in preparation
 *              for a PEBS run.  Save away the old value in the DS_AREA.
 *              This routine is called via the parallel thread call.
 */
static VOID
pebs_Allocate_Buffers (
    VOID  *params
)
{
    U64         value;
    U32         this_cpu;
    CPU_STATE   pcpu;
    U32         dev_idx;
    DEV_CONFIG  pcfg;
    PVOID       dts_ptr = NULL;

    SEP_DRV_LOG_TRACE_IN("Params: %p.", params);

    this_cpu = CONTROL_THIS_CPU();
    pcpu     = &pcb[this_cpu];
    dev_idx  = core_to_dev_map[this_cpu];
    pcfg     = LWPMU_DEVICE_pcfg(&devices[dev_idx]);

    if (!DEV_CONFIG_pebs_mode(pcfg)) {
        return;
    }

    SYS_Write_MSR(IA32_PEBS_ENABLE, 0LL);
    value = SYS_Read_MSR(IA32_MISC_ENABLE);
    if ((value & 0x80) && !(value & 0x1000)) {
        CPU_STATE_old_dts_buffer(pcpu) = (PVOID)(UIOP)SYS_Read_MSR(IA32_DS_AREA);
        dts_ptr     = pebs_Alloc_DTS_Buffer();
        if (!dts_ptr) {
            SEP_DRV_LOG_ERROR_TRACE_OUT("dts_ptr is NULL!");
            return;
        }
        SEP_DRV_LOG_TRACE("Old dts buffer - %p.", CPU_STATE_old_dts_buffer(pcpu));
        SEP_DRV_LOG_TRACE("New dts buffer - %p.", dts_ptr);
        SYS_Write_MSR(IA32_DS_AREA, (U64)(UIOP)dts_ptr);
    }

    SEP_DRV_LOG_TRACE_OUT("");
    return;
}

/* ------------------------------------------------------------------------- */
/*!
 * @fn          VOID pebs_Dellocate_Buffers (VOID *params)
 *
 * @brief       Clean up PEBS buffers and restore older values into the DS_AREA
 *
 * @param       NONE
 *
 * @return      NONE
 *
 * <I>Special Notes:</I>
 *              Clean up the DS area and all restore state prior to the sampling run
 *              This routine is called via the parallel thread call.
 */
static VOID
pebs_Deallocate_Buffers (
    VOID  *params
)
{
    CPU_STATE   pcpu;
    U32         this_cpu;
    U32         dev_idx;
    DEV_CONFIG  pcfg;

    SEP_DRV_LOG_TRACE_IN("Params: %p.", params);

    this_cpu = CONTROL_THIS_CPU();
    pcpu     = &pcb[this_cpu];
    dev_idx  = core_to_dev_map[this_cpu];
    pcfg     = LWPMU_DEVICE_pcfg(&devices[dev_idx]);

    if (!DEV_CONFIG_pebs_mode(pcfg)) {
        SEP_DRV_LOG_TRACE_OUT("");
        return;
    }

    SEP_DRV_LOG_TRACE("Entered deallocate buffers.");
    SYS_Write_MSR(IA32_DS_AREA, (U64)(UIOP)CPU_STATE_old_dts_buffer(pcpu));

    SEP_DRV_LOG_TRACE_OUT("");
    return;
}

/* ------------------------------------------------------------------------- */
/*!
 * @fn          U64 PEBS_Overflowed (this_cpu, overflow_status)
 *
 * @brief       Figure out if the PEBS event caused an overflow
 *
 * @param       this_cpu        -- the current cpu
 *              overflow_status -- current value of the global overflow status
 *
 * @return      updated overflow_status
 *
 * <I>Special Notes:</I>
 *              Figure out if the PEBS area has data that need to be transferred
 *              to the output sample.
 *              Update the overflow_status that is passed and return this value.
 *              The overflow_status defines the events/status to be read
 */
extern U64
PEBS_Overflowed (
    S32  this_cpu,
    U64  overflow_status,
    U32  rec_index
)
{
    U64           res;
    U32           dev_idx;
    PEBS_DISPATCH pebs_dispatch;

    SEP_DRV_LOG_TRACE_IN("This_cpu: %d, overflow_status: %llx, rec_index: %u.", this_cpu, overflow_status, rec_index);

    dev_idx       = core_to_dev_map[this_cpu];
    pebs_dispatch = LWPMU_DEVICE_pebs_dispatch(&devices[dev_idx]);

    res = pebs_dispatch->overflow(this_cpu, overflow_status, rec_index);

    SEP_DRV_LOG_TRACE_OUT("Res: %llx.", overflow_status);
    return res;
}

/* ------------------------------------------------------------------------- */
/*!
 * @fn          VOID PEBS_Reset_Index (this_cpu)
 *
 * @brief       Reset the PEBS index pointer
 *
 * @param       this_cpu        -- the current cpu
 *
 * @return      NONE
 *
 * <I>Special Notes:</I>
 *              reset index to next PEBS record to base of buffer
 */
extern VOID
PEBS_Reset_Index (
    S32    this_cpu
)
{
    DTS_BUFFER_EXT   dtes;

    SEP_DRV_LOG_TRACE_IN("This_cpu: %d.", this_cpu);

    dtes = CPU_STATE_dts_buffer(&pcb[this_cpu]);

    if (!dtes) {
        return;
    }
    SEP_DRV_LOG_TRACE("PEBS Reset Index: %d.", this_cpu);
    DTS_BUFFER_EXT_pebs_index(dtes) = DTS_BUFFER_EXT_pebs_base(dtes);

    SEP_DRV_LOG_TRACE_OUT("");
    return;
}

extern U32 pmi_Get_CSD (U32, U32*, U32*);
#define EFLAGS_V86_MASK       0x00020000L

/* ------------------------------------------------------------------------- */
/*!
 * @fn          VOID PEBS_Flush_Buffer (VOID * param)
 *
 * @brief       generate sampling records from PEBS records in PEBS buffer
 *
 * @param       param        -- not used
 *
 * @return      NONE
 *
 * <I>Special Notes:</I>
 */
extern VOID
PEBS_Flush_Buffer(
    VOID * param
)
{
    U32 i, this_cpu, index, desc_id;
    U64              pebs_overflow_status = 0;
    U64              lbr_tos_from_ip      = 0ULL;
    DRV_BOOL         counter_overflowed   = FALSE;
    ECB              pecb;
    CPU_STATE        pcpu;
    EVENT_DESC       evt_desc;
    BUFFER_DESC      bd;
    SampleRecordPC  *psamp_pebs;
    U32              is_64bit_addr    = FALSE;
    U32              u32PebsRecordNumFilled;
#if defined(DRV_IA32)
    U32              seg_cs;
    U32              csdlo;
    U32              csdhi;
#endif
    U32              dev_idx;
    DEV_CONFIG       pcfg;
    U32              cur_grp;
    DRV_BOOL         multi_pebs_enabled;

    SEP_DRV_LOG_TRACE_IN("Param: %p.", param);

    this_cpu = CONTROL_THIS_CPU();
    pcpu     = &pcb[this_cpu];
    bd       = &cpu_buf[this_cpu];
    dev_idx  = core_to_dev_map[this_cpu];
    pcfg     = LWPMU_DEVICE_pcfg(&devices[dev_idx]);
    cur_grp  = CPU_STATE_current_group(pcpu);
    multi_pebs_enabled = (DEV_CONFIG_pebs_mode(pcfg) &&
                          (DEV_CONFIG_pebs_record_num(pcfg) > 1) &&
                          (DRV_SETUP_INFO_page_table_isolation(&req_drv_setup_info) == DRV_SETUP_INFO_PTI_DISABLED));

    if (!DEV_CONFIG_pebs_mode(pcfg)) {
        SEP_DRV_LOG_TRACE_OUT("PEBS is not enabled");
        return;
    }

    if (!multi_pebs_enabled) {
        SEP_DRV_LOG_TRACE_OUT("PEBS_Flush_Buffer is not supported.");
        return;
    }

    u32PebsRecordNumFilled = PEBS_Get_Num_Records_Filled();
    for (i = 0; i < u32PebsRecordNumFilled; i++) {
        pebs_overflow_status = PEBS_Overflowed(this_cpu, 0, i);
        SEP_DRV_LOG_TRACE("Pebs_overflow_status = 0x%llx, i=%d.", pebs_overflow_status, i);

        pecb = LWPMU_DEVICE_PMU_register_data(&devices[dev_idx])[cur_grp];
        FOR_EACH_DATA_REG(pecb, j) {
            if ((!DEV_CONFIG_enable_adaptive_pebs(pcfg) && !ECB_entries_is_gp_reg_get(pecb, j)) ||
                 !ECB_entries_precise_get(pecb, j)) {
                continue;
            }
            if (ECB_entries_fixed_reg_get(pecb, j)) {
                index = ECB_entries_reg_id(pecb, j) - IA32_FIXED_CTR0;
                if (pebs_overflow_status & ((U64)1 << (32 + index))) {
                    counter_overflowed = TRUE;
                }
            }
            else {
                index = ECB_entries_reg_id(pecb, j) - IA32_PMC0;
                if (pebs_overflow_status & (U64)1 << index) {
                    counter_overflowed = TRUE;
                }
            }
            if (counter_overflowed) {
                desc_id  = ECB_entries_event_id_index(pecb, j);
                evt_desc = desc_data[desc_id];
                SEP_DRV_LOG_TRACE("Event_id_index=%u, desc_id=%u.", ECB_entries_event_id_index(pecb, j), desc_id);
                psamp_pebs = (SampleRecordPC *)OUTPUT_Reserve_Buffer_Space(bd, EVENT_DESC_sample_size(evt_desc), (NMI_mode)? TRUE:FALSE, !SEP_IN_NOTIFICATION);
                if (!psamp_pebs) {
                    SEP_DRV_LOG_ERROR("Could not generate samples from PEBS records.");
                    continue;
                }

                lbr_tos_from_ip                             = 0ULL;
                CPU_STATE_num_samples(&pcb[this_cpu])      += 1;
                SAMPLE_RECORD_descriptor_id(psamp_pebs)     = desc_id;
                SAMPLE_RECORD_event_index(psamp_pebs)       = ECB_entries_event_id_index(pecb, j);
                SAMPLE_RECORD_pid_rec_index(psamp_pebs)     = (U32)-1;
                SAMPLE_RECORD_pid_rec_index_raw(psamp_pebs) = 1;
                SAMPLE_RECORD_tid(psamp_pebs)               = (U32)-1;
                SAMPLE_RECORD_cpu_num(psamp_pebs)           = (U16) this_cpu;
                SAMPLE_RECORD_osid(psamp_pebs)              = 0;

#if defined (DRV_IA32)
                PEBS_Modify_IP((S8 *)psamp_pebs, is_64bit_addr, i);
                SAMPLE_RECORD_cs(psamp_pebs)                 = __KERNEL_CS;
                if (SAMPLE_RECORD_eflags(psamp_pebs) & EFLAGS_V86_MASK) {
                    csdlo = 0;
                    csdhi = 0;
                }
                else {
                    seg_cs = SAMPLE_RECORD_cs(psamp_pebs);
                    SYS_Get_CSD(seg_cs, &csdlo, &csdhi);
                }
                SAMPLE_RECORD_csd(psamp_pebs).u1.lowWord  = csdlo;
                SAMPLE_RECORD_csd(psamp_pebs).u2.highWord = csdhi;
#elif defined (DRV_EM64T)
                SAMPLE_RECORD_cs(psamp_pebs)                = __KERNEL_CS;
                pmi_Get_CSD(SAMPLE_RECORD_cs(psamp_pebs),
                            &SAMPLE_RECORD_csd(psamp_pebs).u1.lowWord,
                            &SAMPLE_RECORD_csd(psamp_pebs).u2.highWord);
                is_64bit_addr = (SAMPLE_RECORD_csd(psamp_pebs).u2.s2.reserved_0 == 1);
                if (is_64bit_addr) {
                    SAMPLE_RECORD_ia64_pc(psamp_pebs)       = TRUE;
                }
                else {
                    SAMPLE_RECORD_ia64_pc(psamp_pebs)       = FALSE;

                    SEP_DRV_LOG_TRACE("SAMPLE_RECORD_eip(psamp_pebs) 0x%x.", SAMPLE_RECORD_eip(psamp_pebs));
                    SEP_DRV_LOG_TRACE("SAMPLE_RECORD_eflags(psamp_pebs) %x.", SAMPLE_RECORD_eflags(psamp_pebs));
                }
#endif
                if (EVENT_DESC_pebs_offset(evt_desc)
                    || EVENT_DESC_latency_offset_in_sample(evt_desc)) {
                    lbr_tos_from_ip = PEBS_Fill_Buffer((S8 *)psamp_pebs, evt_desc, i);
                }
                PEBS_Modify_IP((S8 *)psamp_pebs, is_64bit_addr, i);
                PEBS_Modify_TSC((S8 *)psamp_pebs, i);
                if (ECB_entries_branch_evt_get(pecb, j) &&
                    DEV_CONFIG_precise_ip_lbrs(pcfg) && lbr_tos_from_ip) {
                    if (is_64bit_addr) {
                        SAMPLE_RECORD_iip(psamp_pebs)       = lbr_tos_from_ip;
                        SEP_DRV_LOG_TRACE("UPDATED SAMPLE_RECORD_iip(psamp) 0x%llx.", SAMPLE_RECORD_iip(psamp_pebs));
                    }
                    else {
                        SAMPLE_RECORD_eip(psamp_pebs)       = (U32) lbr_tos_from_ip;
                        SEP_DRV_LOG_TRACE("UPDATED SAMPLE_RECORD_eip(psamp) 0x%x.", SAMPLE_RECORD_eip(psamp_pebs));
                    }
                }
            }
        } END_FOR_EACH_DATA_REG;
    }
    PEBS_Reset_Index(this_cpu);

    SEP_DRV_LOG_TRACE_OUT("");
}

/* ------------------------------------------------------------------------- */
/*!
 * @fn          VOID PEBS_Reset_Counter (this_cpu, index, value)
 *
 * @brief       set reset value for PMC after overflow
 *
 * @param       this_cpu        -- the current cpu
 *              index           -- PMC register index
 *              value           -- reset value for PMC after overflow
 *
 * @return      NONE
 *
 * <I>Special Notes:</I>
 */
extern VOID
PEBS_Reset_Counter (
    S32        this_cpu,
    U32        index,
    U64        value
)
{
    DTS_BUFFER_EXT  dts;
    DTS_BUFFER_EXT1 dts_ext = NULL;
    U32             dev_idx;
    DEV_CONFIG      pcfg;

    SEP_DRV_LOG_TRACE_IN("This_cpu: %d, index: %u, value: %llx.", this_cpu, index, value);

    dev_idx  = core_to_dev_map[this_cpu];
    pcfg     = LWPMU_DEVICE_pcfg(&devices[dev_idx]);
    dts      = CPU_STATE_dts_buffer(&pcb[this_cpu]);

    if (!dts) {
        return;
    }
    SEP_DRV_LOG_TRACE("PEBS Reset GP Counters[0:4]: cpu %d, index=%u, value=%llx.",
        this_cpu, index, value);
    switch(index) {
        case 0:
            DTS_BUFFER_EXT_counter_reset0(dts)  = value;
            break;
        case 1:
            DTS_BUFFER_EXT_counter_reset1(dts)  = value;
            break;
        case 2:
            DTS_BUFFER_EXT_counter_reset2(dts)  = value;
            break;
        case 3:
            DTS_BUFFER_EXT_counter_reset3(dts)  = value;
            break;
    }

    if (DEV_CONFIG_enable_adaptive_pebs(pcfg) ||
        DEV_CONFIG_collect_fixed_counter_pebs(pcfg)) {
        dts_ext = CPU_STATE_dts_buffer(&pcb[this_cpu]);
    }
    if (!dts_ext) {
        return;
    }
    SEP_DRV_LOG_TRACE("PEBS Reset Fixed Counters and GP Counters[4:7]: cpu %d, index=%u, value=%llx.",
        this_cpu, index, value);
    switch(index) {
        case 4:
            DTS_BUFFER_EXT1_counter_reset4(dts_ext)  = value;
            break;
        case 5:
            DTS_BUFFER_EXT1_counter_reset5(dts_ext)  = value;
            break;
        case 6:
            DTS_BUFFER_EXT1_counter_reset6(dts_ext)  = value;
            break;
        case 7:
            DTS_BUFFER_EXT1_counter_reset7(dts_ext)  = value;
            break;
        case 8:
            DTS_BUFFER_EXT1_fixed_counter_reset0(dts_ext) = value;
            break;
        case 9:
            DTS_BUFFER_EXT1_fixed_counter_reset1(dts_ext) = value;
            break;
        case 10:
            DTS_BUFFER_EXT1_fixed_counter_reset2(dts_ext) = value;
            break;
        case 11:
            DTS_BUFFER_EXT1_fixed_counter_reset3(dts_ext) = value;
            break;
    }

    SEP_DRV_LOG_TRACE_OUT("");
}

/* ------------------------------------------------------------------------- */
/*!
 * @fn          VOID PEBS_Modify_IP (sample, is_64bit_addr)
 *
 * @brief       Change the IP field in the sample to that in the PEBS record
 *
 * @param       sample        - sample buffer
 * @param       is_64bit_addr - are we in a 64 bit module
 *
 * @return      NONE
 *
 * <I>Special Notes:</I>
 *              <NONE>
 */
extern VOID
PEBS_Modify_IP (
    void        *sample,
    DRV_BOOL     is_64bit_addr,
    U32          rec_index
)
{
    U32           this_cpu;
    U32           dev_idx;    
    PEBS_DISPATCH pebs_dispatch;

    SEP_DRV_LOG_TRACE_IN("Sample: %p, is_64bit_addr: %u, rec_index: %u.", sample, is_64bit_addr, rec_index);

    this_cpu      = CONTROL_THIS_CPU();
    dev_idx       = core_to_dev_map[this_cpu];
    pebs_dispatch = LWPMU_DEVICE_pebs_dispatch(&devices[dev_idx]);

    pebs_dispatch->modify_ip(sample, is_64bit_addr, rec_index);

    SEP_DRV_LOG_TRACE_OUT("");
    return;
}


/* ------------------------------------------------------------------------- */
/*!
 * @fn          VOID PEBS_Modify_TSC (sample)
 *
 * @brief       Change the TSC field in the sample to that in the PEBS record
 *
 * @param       sample        - sample buffer
 *
 * @return      NONE
 *
 * <I>Special Notes:</I>
 *              <NONE>
 */
extern VOID
PEBS_Modify_TSC (
    void        *sample,
    U32          rec_index
)
{
    U32           this_cpu;
    U32           dev_idx;    
    PEBS_DISPATCH pebs_dispatch;

    SEP_DRV_LOG_TRACE_IN("Sample: %p, rec_index: %u.", sample, rec_index);

    this_cpu      = CONTROL_THIS_CPU();
    dev_idx       = core_to_dev_map[this_cpu];
    pebs_dispatch = LWPMU_DEVICE_pebs_dispatch(&devices[dev_idx]);

    if (pebs_dispatch->modify_tsc != NULL) {
        pebs_dispatch->modify_tsc(sample, rec_index);
    }

    SEP_DRV_LOG_TRACE_OUT("");
    return;
}

extern U32
PEBS_Get_Num_Records_Filled (
    VOID
)
{
    U32           this_cpu;
    U32           dev_idx;    
    PEBS_DISPATCH pebs_dispatch;
    U32           num = 0;

    SEP_DRV_LOG_TRACE_IN("");

    this_cpu      = CONTROL_THIS_CPU();
    dev_idx       = core_to_dev_map[this_cpu];
    pebs_dispatch = LWPMU_DEVICE_pebs_dispatch(&devices[dev_idx]);

    if (pebs_dispatch->get_num_records_filled != NULL) {
        num = pebs_dispatch->get_num_records_filled();
        SEP_DRV_LOG_TRACE("Num=%u.", num);
    }

    SEP_DRV_LOG_TRACE_OUT("Res: %u.", num);
    return num;
}
/* ------------------------------------------------------------------------- */
/*!
 * @fn          VOID PEBS_Fill_Phy_Addr (LATENCY_INFO latency_info)
 *
 * @brief       Fill latency node with phy addr when applicable
 *
 * @param       latency_info             - pointer to LATENCY_INFO struct
 *
 * @return      NONE 
 *
 * <I>Special Notes:</I>
 *              <NONE>
 */

extern VOID
PEBS_Fill_Phy_Addr (
    LATENCY_INFO latency_info
)
{
#if defined(DRV_EM64T) && LINUX_VERSION_CODE >= KERNEL_VERSION(3,6,0)
    U64                 lin_addr;
    U64                 offset;
    struct page        *page;

    if (!DRV_CONFIG_virt_phys_translation(drv_cfg)) {
        return;
    }
    lin_addr = (U64)LATENCY_INFO_linear_address(latency_info);
    if (lin_addr != 0) {
        offset = (U64)(lin_addr & 0x0FFF);
        if (__virt_addr_valid(lin_addr)) {
            LATENCY_INFO_phys_addr(latency_info) = (U64)__pa(lin_addr);
        }
        else if (lin_addr < __PAGE_OFFSET) {
            pagefault_disable();
            if (__get_user_pages_fast(lin_addr, 1, 1, &page)) {
                LATENCY_INFO_phys_addr(latency_info) = (U64)page_to_phys(page) + offset;
                put_page(page);
            }
            pagefault_enable();
        }
    }
#endif
    return;
}
/* ------------------------------------------------------------------------- */
/*!
 * @fn          U64 PEBS_Fill_Buffer (S8 *buffer, EVENT_DESC evt_desc, U32 rec_index)
 *
 * @brief       Fill the buffer with the pebs data
 *
 * @param       buffer                   -  area to write the data into
 *              event_desc               -  event descriptor of the pebs event
                rec_index                - current pebs record index
 *
 * @return      if APEBS return LBR_TOS_FROM_IP else return 0
 *
 * <I>Special Notes:</I>
 *              <NONE>
 */
extern U64
PEBS_Fill_Buffer (
    S8           *buffer,
    EVENT_DESC    evt_desc,
    U32           rec_index
)
{
    DTS_BUFFER_EXT      dtes;
    LATENCY_INFO_NODE   latency_info  = {0};
    PEBS_REC_EXT1       pebs_base_ext1;
    PEBS_REC_EXT2       pebs_base_ext2;
    S8                 *pebs_base, *pebs_index, *pebs_ptr;
    U8                  pebs_ptr_check = FALSE;
    U64                 lbr_tos_from_ip = 0ULL;
    U32                 this_cpu;
    U32                 dev_idx;
    DEV_CONFIG          pcfg;

    SEP_DRV_LOG_TRACE_IN("Buffer: %p, evt_desc: %p, rec_index: %u.",
        buffer, evt_desc, rec_index);

    this_cpu = CONTROL_THIS_CPU();
    dev_idx  = core_to_dev_map[this_cpu];
    pcfg     = LWPMU_DEVICE_pcfg(&devices[dev_idx]);
    dtes     = CPU_STATE_dts_buffer(&pcb[this_cpu]);

    if (DEV_CONFIG_enable_adaptive_pebs(pcfg)) {
        lbr_tos_from_ip =  APEBS_Fill_Buffer(buffer,
                                             evt_desc,
                                             rec_index);
        return lbr_tos_from_ip;
    }

    SEP_DRV_LOG_TRACE("In PEBS Fill Buffer: cpu %d.", CONTROL_THIS_CPU());

    if (!dtes) {
        return lbr_tos_from_ip;
    }
    pebs_base      = (S8 *)(UIOP)DTS_BUFFER_EXT_pebs_base(dtes);
    pebs_index     = (S8 *)(UIOP)DTS_BUFFER_EXT_pebs_index(dtes);
    pebs_ptr       = (S8 *)((UIOP)DTS_BUFFER_EXT_pebs_base(dtes) + (rec_index *  LWPMU_DEVICE_pebs_record_size(&devices[dev_idx])));
    pebs_ptr_check = (pebs_ptr && pebs_base != pebs_index && pebs_ptr < pebs_index);
    if (!pebs_ptr_check) {
        return lbr_tos_from_ip;
    }
    pebs_base = pebs_ptr;
    if (EVENT_DESC_pebs_offset(evt_desc)) {
        SEP_DRV_LOG_TRACE("PEBS buffer has data available.");
        memcpy(buffer + EVENT_DESC_pebs_offset(evt_desc),
               pebs_base,
               EVENT_DESC_pebs_size(evt_desc));
    }
    if (EVENT_DESC_eventing_ip_offset(evt_desc)) {
        pebs_base_ext1 = (PEBS_REC_EXT1)pebs_base;
        *(U64*)(buffer + EVENT_DESC_eventing_ip_offset(evt_desc)) = PEBS_REC_EXT1_eventing_ip(pebs_base_ext1);
    }
    if (EVENT_DESC_hle_offset(evt_desc)) {
        pebs_base_ext1 = (PEBS_REC_EXT1)pebs_base;
        *(U64*)(buffer + EVENT_DESC_hle_offset(evt_desc)) = PEBS_REC_EXT1_hle_info(pebs_base_ext1);
    }
    if (EVENT_DESC_latency_offset_in_sample(evt_desc)) {
        pebs_base_ext1 = (PEBS_REC_EXT1)pebs_base;
        memcpy(&latency_info,
                pebs_base + EVENT_DESC_latency_offset_in_pebs_record(evt_desc),
                EVENT_DESC_latency_size_from_pebs_record(evt_desc));
        memcpy(&LATENCY_INFO_stack_pointer(&latency_info),
               &PEBS_REC_EXT1_rsp(pebs_base_ext1),
               sizeof(U64));

        LATENCY_INFO_phys_addr(&latency_info) = 0;
        PEBS_Fill_Phy_Addr(&latency_info);

        memcpy(buffer + EVENT_DESC_latency_offset_in_sample(evt_desc),
               &latency_info,
               sizeof(LATENCY_INFO_NODE) );
    }
    if (EVENT_DESC_pebs_tsc_offset(evt_desc)) {
        pebs_base_ext2 = (PEBS_REC_EXT2)pebs_base;
        *(U64*)(buffer + EVENT_DESC_pebs_tsc_offset(evt_desc)) = PEBS_REC_EXT2_tsc(pebs_base_ext2);
    }

    SEP_DRV_LOG_TRACE_OUT("");
    return lbr_tos_from_ip;
}

/* ------------------------------------------------------------------------- */
/*!
 * @fn          U64 APEBS_Fill_Buffer (S8 *buffer, EVENT_DESC evt_desc, U32 rec_index)
 *
 * @brief       Fill the buffer with the pebs data
 *
 * @param       buffer                   -  area to write the data into
 *              event_desc               -  event descriptor of the pebs event
 *              rec_index                - current pebs record index
 *
 * @return      LBR_TOS_FROM_IP
 *
 * <I>Special Notes:</I>
 *              <NONE>
 */
extern U64
APEBS_Fill_Buffer (
    S8           *buffer,
    EVENT_DESC    evt_desc,
    U32           rec_index
)
{
    DTS_BUFFER_EXT1             dtes;
    LATENCY_INFO_NODE           latency_info       = {0};
    U64                         dtes_record_size   = 0;
    U64                         dtes_record_format = 0;
    ADAPTIVE_PEBS_MEM_INFO      apebs_mem          = NULL;
    ADAPTIVE_PEBS_GPR_INFO      apebs_gpr          = NULL;
    ADAPTIVE_PEBS_BASIC_INFO    apebs_basic        = NULL;
    S8                         *pebs_base, *pebs_index, *pebs_ptr;
    U8                          pebs_ptr_check     = FALSE;
    U64                         lbr_tos_from_ip    = 0ULL;
    U32                         this_cpu;
    U32                         dev_idx;
    DEV_CONFIG                  pcfg;

    SEP_DRV_LOG_TRACE_IN("Buffer: %p, evt_desc: %p, rec_index: %u.",
        buffer, evt_desc, rec_index);

    this_cpu = CONTROL_THIS_CPU();
    dev_idx  = core_to_dev_map[this_cpu];
    pcfg     = LWPMU_DEVICE_pcfg(&devices[dev_idx]);
    dtes     = CPU_STATE_dts_buffer(&pcb[this_cpu]);

    SEP_DRV_LOG_TRACE("In APEBS Fill Buffer: cpu %d.", this_cpu);

    if (!dtes || !DEV_CONFIG_enable_adaptive_pebs(pcfg)) {
        return lbr_tos_from_ip;
    }

    pebs_base      = (S8 *)(UIOP)DTS_BUFFER_EXT1_pebs_base(dtes);
    pebs_index     = (S8 *)(UIOP)DTS_BUFFER_EXT1_pebs_index(dtes);
    pebs_ptr       = (S8 *)((UIOP)DTS_BUFFER_EXT1_pebs_base(dtes) + (rec_index *  LWPMU_DEVICE_pebs_record_size(&devices[dev_idx])));
    pebs_ptr_check = (pebs_ptr && pebs_base != pebs_index && pebs_ptr < pebs_index);
    if (!pebs_ptr_check) {
        return lbr_tos_from_ip;
    }

    pebs_base = pebs_ptr;
    apebs_basic        = (ADAPTIVE_PEBS_BASIC_INFO)(pebs_base + LWPMU_DEVICE_apebs_basic_offset(&devices[dev_idx]));
    dtes_record_size   = (ADAPTIVE_PEBS_BASIC_INFO_record_info(apebs_basic) & APEBS_RECORD_SIZE_MASK) >> 48; // [63:48]
    dtes_record_format = (ADAPTIVE_PEBS_BASIC_INFO_record_info(apebs_basic) & APEBS_RECORD_FORMAT_MASK); // [47:0]

    if (dtes_record_size != LWPMU_DEVICE_pebs_record_size(&devices[dev_idx])) {
        SEP_DRV_LOG_TRACE("PEBS record size does not match with ucode\n");
    }
    if (EVENT_DESC_pebs_offset(evt_desc)) {
        *(U64*)(buffer + EVENT_DESC_pebs_offset(evt_desc)) = ADAPTIVE_PEBS_BASIC_INFO_record_info(apebs_basic);
    }
    if (EVENT_DESC_eventing_ip_offset(evt_desc)) {
        *(U64*)(buffer + EVENT_DESC_eventing_ip_offset(evt_desc)) = ADAPTIVE_PEBS_BASIC_INFO_eventing_ip(apebs_basic);
    }
    if (EVENT_DESC_pebs_tsc_offset(evt_desc)) {
        *(U64*)(buffer + EVENT_DESC_pebs_tsc_offset(evt_desc)) = ADAPTIVE_PEBS_BASIC_INFO_tsc(apebs_basic);
    }
    if (EVENT_DESC_applicable_counters_offset(evt_desc)) {
        *(U64*)(buffer + EVENT_DESC_applicable_counters_offset(evt_desc)) = ADAPTIVE_PEBS_BASIC_INFO_applicable_counters(apebs_basic);
    }
    if (DEV_CONFIG_apebs_collect_gpr(pcfg) && EVENT_DESC_gpr_info_offset(evt_desc)) {
        if (!(dtes_record_format & APEBS_GPR_RECORD_FORMAT_MASK)) {
            SEP_DRV_LOG_WARNING("GPR info not found in DS PEBS record.");
        }
        memcpy(buffer    + EVENT_DESC_gpr_info_offset(evt_desc),
               pebs_base + LWPMU_DEVICE_apebs_gpr_offset(&devices[dev_idx]),
               EVENT_DESC_gpr_info_size(evt_desc));
    }
    if (DEV_CONFIG_apebs_collect_mem_info(pcfg) && EVENT_DESC_latency_offset_in_sample(evt_desc)) {
        if (!(dtes_record_format & APEBS_MEM_RECORD_FORMAT_MASK)) {
            SEP_DRV_LOG_WARNING("MEM info not found in DS PEBS record.");
        }
        apebs_mem = (ADAPTIVE_PEBS_MEM_INFO)(pebs_base + LWPMU_DEVICE_apebs_mem_offset(&devices[dev_idx]));
        memcpy(&LATENCY_INFO_linear_address(&latency_info),
               &ADAPTIVE_PEBS_MEM_INFO_data_linear_address(apebs_mem),
               sizeof(U64));
        memcpy(&LATENCY_INFO_data_source(&latency_info),
               &ADAPTIVE_PEBS_MEM_INFO_data_source(apebs_mem),
               sizeof(U64));
        memcpy(&LATENCY_INFO_latency(&latency_info),
               &ADAPTIVE_PEBS_MEM_INFO_latency(apebs_mem),
               sizeof(U64));
        LATENCY_INFO_stack_pointer(&latency_info) = 0;
        if (DEV_CONFIG_apebs_collect_gpr(pcfg)) {
            apebs_gpr = (ADAPTIVE_PEBS_GPR_INFO)(pebs_base + LWPMU_DEVICE_apebs_gpr_offset(&devices[dev_idx]));
            memcpy(&LATENCY_INFO_stack_pointer(&latency_info),
                   &ADAPTIVE_PEBS_GPR_INFO_rsp(apebs_gpr),
                   sizeof(U64));
        }

        LATENCY_INFO_phys_addr(&latency_info) = 0;
        PEBS_Fill_Phy_Addr(&latency_info);
        memcpy(buffer + EVENT_DESC_latency_offset_in_sample(evt_desc),
               &latency_info,
               sizeof(LATENCY_INFO_NODE));
    }
    if (DEV_CONFIG_apebs_collect_mem_info(pcfg) && EVENT_DESC_hle_offset(evt_desc)) {
        *(U64*)(buffer + EVENT_DESC_hle_offset(evt_desc)) =  ADAPTIVE_PEBS_MEM_INFO_hle_info((ADAPTIVE_PEBS_MEM_INFO)(pebs_base + LWPMU_DEVICE_apebs_mem_offset(&devices[dev_idx])));
    }
    if (DEV_CONFIG_apebs_collect_xmm(pcfg) && EVENT_DESC_xmm_info_offset(evt_desc)) {
        if (!(dtes_record_format & APEBS_XMM_RECORD_FORMAT_MASK)) {
            SEP_DRV_LOG_WARNING("XMM info not found in DS PEBS record.");
        }
        memcpy(buffer    + EVENT_DESC_xmm_info_offset(evt_desc),
               pebs_base + LWPMU_DEVICE_apebs_xmm_offset(&devices[dev_idx]),
               EVENT_DESC_xmm_info_size(evt_desc));
    }
    if (DEV_CONFIG_apebs_collect_lbrs(pcfg) && EVENT_DESC_lbr_offset(evt_desc)) {
        if (!(dtes_record_format & APEBS_LBR_RECORD_FORMAT_MASK)) {
            SEP_DRV_LOG_WARNING("LBR info not found in DS PEBS record\n");
        }
        if ((dtes_record_format >> 24) != (DEV_CONFIG_apebs_num_lbr_entries(pcfg)-1)) {
            SEP_DRV_LOG_WARNING("DRV_CONFIG_apebs_num_lbr_entries does not match with PEBS record\n");
        }
        *(U64*)(buffer + EVENT_DESC_lbr_offset(evt_desc)) =
                                       DEV_CONFIG_apebs_num_lbr_entries(pcfg)-1; //Top-of-Stack(TOS) pointing to last entry
        //Populating lbr callstack as SST_ENTRY_N to SST_ENTRY_0 in tb util, hence setting TOS to SST_ENTRY_N
        memcpy(buffer    + EVENT_DESC_lbr_offset(evt_desc) + sizeof(U64),
               pebs_base + LWPMU_DEVICE_apebs_lbr_offset(&devices[dev_idx]),
               EVENT_DESC_lbr_info_size(evt_desc)-sizeof(U64));
        lbr_tos_from_ip = ADAPTIVE_PEBS_LBR_INFO_lbr_from((ADAPTIVE_PEBS_LBR_INFO)(pebs_base + LWPMU_DEVICE_apebs_lbr_offset(&devices[dev_idx])));
    }
    return lbr_tos_from_ip;
}

/* ------------------------------------------------------------------------- */
/*!
 * @fn          OS_STATUS PEBS_Initialize (DEV_CONFIG pcfg)
 *
 * @brief       Initialize the pebs buffers
 *
 * @param       dev_idx -  Device index
 *
 * @return      status
 *
 * <I>Special Notes:</I>
 *              If the user is asking for PEBS information.  Allocate the DS area
 */
extern OS_STATUS
PEBS_Initialize (
    U32         dev_idx
)
{
    DEV_CONFIG pcfg = LWPMU_DEVICE_pcfg(&devices[dev_idx]);

    SEP_DRV_LOG_TRACE_IN("Pcfg: %p.", pcfg);

    if (!DEV_CONFIG_pebs_mode(pcfg)) {
        SEP_DRV_LOG_TRACE_OUT("PEBS is not enabled");
        return OS_SUCCESS;
    }

    switch (DEV_CONFIG_pebs_mode(pcfg)) {
        case 1:
            SEP_DRV_LOG_INIT("Set up the Core2 dispatch table.");
            LWPMU_DEVICE_pebs_dispatch(&devices[dev_idx]) = &core2_pebs;
            LWPMU_DEVICE_pebs_record_size(&devices[dev_idx]) = sizeof(PEBS_REC_NODE);
            break;
        case 2:
            SEP_DRV_LOG_INIT("Set up the Nehalem dispatch.");
            LWPMU_DEVICE_pebs_dispatch(&devices[dev_idx]) = &corei7_pebs;
            LWPMU_DEVICE_pebs_record_size(&devices[dev_idx]) = sizeof(PEBS_REC_EXT_NODE);
            break;
        case 3:
            SEP_DRV_LOG_INIT("Set up the Core2 (PNR) dispatch table.");
            LWPMU_DEVICE_pebs_dispatch(&devices[dev_idx]) = &core2p_pebs;
            LWPMU_DEVICE_pebs_record_size(&devices[dev_idx]) = sizeof(PEBS_REC_NODE);
            break;
        case 4:
            SEP_DRV_LOG_INIT("Set up the Haswell dispatch table.");
            LWPMU_DEVICE_pebs_dispatch(&devices[dev_idx]) = &haswell_pebs;
            LWPMU_DEVICE_pebs_record_size(&devices[dev_idx]) = sizeof(PEBS_REC_EXT1_NODE);
            break;
        case 5:
            SEP_DRV_LOG_INIT("Set up the Perf version4 dispatch table.");
            LWPMU_DEVICE_pebs_dispatch(&devices[dev_idx]) = &perfver4_pebs;
            LWPMU_DEVICE_pebs_record_size(&devices[dev_idx]) = sizeof(PEBS_REC_EXT2_NODE);
            break;
        case 6:
            if (!DEV_CONFIG_enable_adaptive_pebs(pcfg)) {
                SEP_DRV_LOG_TRACE("APEBS need to be enabled in perf version4 SNC dispatch mode.");
            }
            LWPMU_DEVICE_pebs_dispatch(&devices[dev_idx]) = &perfver4_apebs;
            LWPMU_DEVICE_pebs_record_size(&devices[dev_idx]) = sizeof(ADAPTIVE_PEBS_BASIC_INFO_NODE);
            if (DEV_CONFIG_apebs_collect_mem_info(pcfg)) {
                LWPMU_DEVICE_apebs_mem_offset(&devices[dev_idx])  = LWPMU_DEVICE_pebs_record_size(&devices[dev_idx]);
                LWPMU_DEVICE_pebs_record_size(&devices[dev_idx]) += sizeof(ADAPTIVE_PEBS_MEM_INFO_NODE);
            }
            if (DEV_CONFIG_apebs_collect_gpr(pcfg)) {
                LWPMU_DEVICE_apebs_gpr_offset(&devices[dev_idx])  = LWPMU_DEVICE_pebs_record_size(&devices[dev_idx]);
                LWPMU_DEVICE_pebs_record_size(&devices[dev_idx]) += sizeof(ADAPTIVE_PEBS_GPR_INFO_NODE);
            }
            if (DEV_CONFIG_apebs_collect_xmm(pcfg)) {
                LWPMU_DEVICE_apebs_xmm_offset(&devices[dev_idx])  = LWPMU_DEVICE_pebs_record_size(&devices[dev_idx]);
                LWPMU_DEVICE_pebs_record_size(&devices[dev_idx]) += sizeof(ADAPTIVE_PEBS_XMM_INFO_NODE);
            }
            if (DEV_CONFIG_apebs_collect_lbrs(pcfg)) {
                LWPMU_DEVICE_apebs_lbr_offset(&devices[dev_idx])  = LWPMU_DEVICE_pebs_record_size(&devices[dev_idx]);
                LWPMU_DEVICE_pebs_record_size(&devices[dev_idx]) += (sizeof(ADAPTIVE_PEBS_LBR_INFO_NODE) * DEV_CONFIG_apebs_num_lbr_entries(pcfg));
            }
            SEP_DRV_LOG_TRACE("Size of adaptive pebs record - %d.", LWPMU_DEVICE_pebs_record_size(&devices[dev_idx]));
            break;
        default:
            SEP_DRV_LOG_INIT("Unknown PEBS type. Will not collect PEBS information.");
            break;
    }
    if (LWPMU_DEVICE_pebs_dispatch(&devices[dev_idx]) && !DEV_CONFIG_pebs_record_num(pcfg)) {
        DEV_CONFIG_pebs_record_num(pcfg) = 1;
    }

    SEP_DRV_LOG_TRACE_OUT("OS_SUCCESS");
    return OS_SUCCESS;
}

/* ------------------------------------------------------------------------- */
/*!
 * @fn          OS_STATUS PEBS_Allocate (void)
 *
 * @brief       Allocate the pebs related buffers
 *
 * @param       NONE
 *
 * @return      NONE
 *
 * <I>Special Notes:</I>
 *             Allocated the DS area used for PEBS capture
 */
extern OS_STATUS
PEBS_Allocate (
    VOID
)
{
    S32        cpu_num;
    CPU_STATE  pcpu;
    U32        dev_idx;
    U32        dts_size;
    DEV_CONFIG pcfg;

    SEP_DRV_LOG_INIT_IN("");

    for (cpu_num = 0; cpu_num < GLOBAL_STATE_num_cpus(driver_state); cpu_num++) {
        pcpu    = &pcb[cpu_num];
        dev_idx = core_to_dev_map[cpu_num];
        pcfg    = LWPMU_DEVICE_pcfg(&devices[dev_idx]);
        if (!DEV_CONFIG_pebs_mode(pcfg)) {
            continue;
        }
        if (LWPMU_DEVICE_pebs_dispatch(&devices[dev_idx])) {
            dts_size = sizeof(DTS_BUFFER_EXT_NODE);
            if (DEV_CONFIG_enable_adaptive_pebs(pcfg)) {
                dts_size = sizeof(DTS_BUFFER_EXT1_NODE);
            }
            CPU_STATE_dts_buffer_offset(pcpu) = pebs_global_memory_size;
            pebs_global_memory_size += PER_CORE_BUFFER_SIZE(dts_size, LWPMU_DEVICE_pebs_record_size(&devices[dev_idx]), DEV_CONFIG_pebs_record_num(pcfg));
        }
    }
    if (pebs_global_memory_size) {
        if (DRV_SETUP_INFO_page_table_isolation(&req_drv_setup_info) == DRV_SETUP_INFO_PTI_DISABLED) {
            SEP_DRV_LOG_INIT("Allocating global PEBS buffer using regular control routine.");
            pebs_global_memory = (PVOID)CONTROL_Allocate_KMemory(pebs_global_memory_size);
            if (!pebs_global_memory) {
                SEP_DRV_LOG_ERROR_TRACE_OUT("Failed to allocate PEBS buffer!");
                return OS_NO_MEM;
            }
            memset(pebs_global_memory, 0, pebs_global_memory_size);
        }
        else {
            SEP_DRV_LOG_INIT("KAISER or PTI patch is enabled and PEBS feature can't be used.\n");
            return OS_SUCCESS;
        }
    }

    CONTROL_Invoke_Parallel(pebs_Allocate_Buffers, (VOID *)NULL);

    SEP_DRV_LOG_INIT_OUT("");
    return OS_SUCCESS;
}

/* ------------------------------------------------------------------------- */
/*!
 * @fn          VOID PEBS_Destroy (void)
 *
 * @brief       Clean up the pebs related buffers
 *
 * @param       pcfg  -  Driver Configuration
 *
 * @return      NONE
 *
 * <I>Special Notes:</I>
 *             Deallocated the DS area used for PEBS capture
 */
extern VOID
PEBS_Destroy (
    VOID
)
{
    SEP_DRV_LOG_TRACE_IN("");

    CONTROL_Invoke_Parallel(pebs_Deallocate_Buffers, (VOID *)(size_t)0);
    if (pebs_global_memory) {
        if (DRV_SETUP_INFO_page_table_isolation(&req_drv_setup_info) == DRV_SETUP_INFO_PTI_DISABLED) {
            SEP_DRV_LOG_INIT("Freeing PEBS buffer using regular control routine.");
            pebs_global_memory = CONTROL_Free_Memory(pebs_global_memory);
        }
        pebs_global_memory_size = 0;
        SEP_DRV_LOG_INIT("PEBS buffer successfully freed.");
    }

    SEP_DRV_LOG_TRACE_OUT("");
    return;
}

