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





#include "lwpmudrv_defines.h"
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/ptrace.h>
#if defined(DRV_EM64T)
#include <asm/desc.h>
#endif
#include <asm/apic.h>
#include <asm/nmi.h>

#include "lwpmudrv_types.h"
#include "rise_errors.h"
#include "lwpmudrv_ecb.h"
#include "lwpmudrv_struct.h"
#include "apic.h"
#include "lwpmudrv.h"
#include "output.h"
#include "control.h"
#include "pmi.h"
#include "utility.h"
#include "pebs.h"

#include "sepdrv_p_state.h"

// Desc id #0 is used for module records
#define COMPUTE_DESC_ID(index)     ((index))

extern DRV_CONFIG             drv_cfg;
extern uid_t                  uid;
extern DRV_SETUP_INFO_NODE    req_drv_setup_info;
#define EFLAGS_V86_MASK       0x00020000L
extern DRV_BOOL               multi_pebs_enabled;
/*********************************************************************
 * Global Variables / State
 *********************************************************************/

/*********************************************************************
 * Interrupt Handler
 *********************************************************************/

/*
 *  PMI_Interrupt_Handler
 *      Arguments
 *          IntFrame - Pointer to the Interrupt Frame
 *
 *      Returns
 *          None
 *
 *      Description
 *  Grab the data that is needed to populate the sample records
 */
#if defined(DRV_EM64T)
#define IS_LDT_BIT       0x4
#define SEGMENT_SHIFT    3
IDTGDT_DESC              gdt_desc;

U32
pmi_Get_CSD (
    U32     seg,
    U32    *low,
    U32    *high
)
{
    PVOID               gdt_max_addr;
    struct desc_struct *gdt;
    CodeDescriptor     *csd;

    SEP_DRV_LOG_TRACE_IN("Seg: %u, low: %p, high: %p.", seg, low, high);

    gdt_max_addr = (PVOID) (((U64) gdt_desc.idtgdt_base) + gdt_desc.idtgdt_limit);
    gdt          = gdt_desc.idtgdt_base;

    if (seg & IS_LDT_BIT) {
        *low  = 0;
        *high = 0;
        SEP_DRV_LOG_TRACE_OUT("FALSE [%u, %u] (IS_LDT_BIT).", *low, *high);
        return (FALSE);
    }

    // segment offset is based on dropping the bottom 3 bits...
    csd = (CodeDescriptor *) &(gdt[seg >> SEGMENT_SHIFT]);

    if (((PVOID) csd) >= gdt_max_addr) {
        SEP_DRV_LOG_WARNING_TRACE_OUT("FALSE (segment too big in get_CSD(0x%x)!).", seg);
        return FALSE;
    }

    *low  = csd->u1.lowWord;
    *high = csd->u2.highWord;

    SEP_DRV_LOG_TRACE("Seg 0x%x, low %08x, high %08x, reserved_0: %d.",
                     seg, *low, *high, csd->u2.s2.reserved_0);
    SEP_DRV_LOG_TRACE_OUT("TRUE [%u, %u].", *low, *high);

    return TRUE;
}
#endif

asmlinkage VOID
PMI_Interrupt_Handler (
     struct pt_regs *regs
)
{
    SampleRecordPC         *psamp;
    CPU_STATE               pcpu;
    BUFFER_DESC             bd;
#if defined(DRV_IA32)
    U32              csdlo;        // low  half code seg descriptor
    U32              csdhi;        // high half code seg descriptor
    U32              seg_cs;       // code seg selector
#endif
    DRV_MASKS_NODE   event_mask;
    U32              this_cpu;
    U32              dev_idx;
    DISPATCH         dispatch;
    DEV_CONFIG       pcfg;
    U32              i;
    U32              is_64bit_addr    = FALSE;
    U32              pid;
    U32              tid;
    U64              tsc;
    U32              desc_id;
    EVENT_DESC       evt_desc;
    U32              accept_interrupt = 1;
#if defined(SECURE_SEP)
    uid_t            l_uid;
#endif
    U64              lbr_tos_from_ip  = 0;
    U32              unc_dev_idx;
    DEV_UNC_CONFIG   pcfg_unc         = NULL;
    DISPATCH         dispatch_unc     = NULL;
    U32              read_unc_evt_counts_from_intr = 0;

    SEP_DRV_LOG_INTERRUPT_IN("PID: %d, TID: %d.", current->pid, GET_CURRENT_TGID()); // needs to be before function calls for the tracing to make sense
                                                                                     // may later want to separate the INTERRUPT_IN from the PID/TID logging

    this_cpu = CONTROL_THIS_CPU();
    pcpu     = &pcb[this_cpu];
    bd       = &cpu_buf[this_cpu];
    dev_idx  = core_to_dev_map[this_cpu];
    dispatch = LWPMU_DEVICE_dispatch(&devices[dev_idx]);
    pcfg     = LWPMU_DEVICE_pcfg(&devices[dev_idx]);
    SYS_Locked_Inc(&CPU_STATE_in_interrupt(pcpu)); // needs to be before dispatch->freeze to ensure printk is never called from an interrupt

    // Disable the counter control
    dispatch->freeze(NULL);

    CPU_STATE_nmi_handled(&pcb[this_cpu])++;

#if defined(SECURE_SEP)
    l_uid            = DRV_GET_UID(current);
    accept_interrupt = (l_uid == uid);
#endif
    dispatch->check_overflow(&event_mask);
    if (GET_DRIVER_STATE() != DRV_STATE_RUNNING       ||
        CPU_STATE_accept_interrupt(&pcb[this_cpu]) != 1) {
        goto pmi_cleanup;
    }

    pid  = GET_CURRENT_TGID();
    tid  = current->pid;

    if (DRV_CONFIG_target_pid(drv_cfg) > 0 && pid != DRV_CONFIG_target_pid(drv_cfg)) {
        accept_interrupt = 0;
    }

    if (accept_interrupt == 0) {
        goto pmi_cleanup;
    }
    UTILITY_Read_TSC(&tsc);
    if (multi_pebs_enabled
        && PEBS_Get_Num_Records_Filled() > 0) {
        PEBS_Flush_Buffer(NULL);
    }

    SEP_DRV_LOG_TRACE("Nb overflowed events: %d.", event_mask.masks_num);
    for (i = 0; i < event_mask.masks_num; i++) {
        if (DRV_EVENT_MASK_collect_on_ctx_sw(&event_mask.eventmasks[i])) {
            if (CPU_STATE_last_thread_id(pcpu) == tid) {
                continue;
            }
            else {
                CPU_STATE_last_thread_id(pcpu) = tid;
            }
        }
        if (multi_pebs_enabled
            && (DRV_EVENT_MASK_precise(&event_mask.eventmasks[i]))) {
            continue;
        }
        if (DRV_CONFIG_mixed_ebc_available(drv_cfg)) {
            desc_id = DRV_EVENT_MASK_desc_id(&event_mask.eventmasks[i]);
        }
        else {
            if (DRV_CONFIG_event_based_counts(drv_cfg) == 0) {
                desc_id  = COMPUTE_DESC_ID(DRV_EVENT_MASK_event_idx(&event_mask.eventmasks[i]));
            }
            else {
                desc_id = CPU_STATE_current_group(pcpu);
            }
        }

        evt_desc = desc_data[desc_id];
        psamp = (SampleRecordPC *)OUTPUT_Reserve_Buffer_Space(bd, EVENT_DESC_sample_size(evt_desc), (NMI_mode)? TRUE:FALSE, !SEP_IN_NOTIFICATION);

        if (!psamp) {
            continue;
        }
        lbr_tos_from_ip                        = 0;
        CPU_STATE_num_samples(pcpu)           += 1;
        SAMPLE_RECORD_descriptor_id(psamp)     = desc_id;
        SAMPLE_RECORD_tsc(psamp)               = tsc;
        SAMPLE_RECORD_pid_rec_index_raw(psamp) = 1;
        SAMPLE_RECORD_pid_rec_index(psamp)     = pid;
        SAMPLE_RECORD_tid(psamp)               = tid;
        SAMPLE_RECORD_cpu_num(psamp)           = (U16) this_cpu;
#if defined(DRV_IA32)
        SAMPLE_RECORD_eip(psamp)               = REGS_eip(regs);
        SAMPLE_RECORD_eflags(psamp)            = REGS_eflags(regs);
        SAMPLE_RECORD_cs(psamp)                = (U16) REGS_xcs(regs);

        if (SAMPLE_RECORD_eflags(psamp) & EFLAGS_V86_MASK) {
            csdlo = 0;
            csdhi = 0;
        }
        else {
            seg_cs = SAMPLE_RECORD_cs(psamp);
            SYS_Get_CSD(seg_cs, &csdlo, &csdhi);
        }
        SAMPLE_RECORD_csd(psamp).u1.lowWord  = csdlo;
        SAMPLE_RECORD_csd(psamp).u2.highWord = csdhi;
#elif defined(DRV_EM64T)
        SAMPLE_RECORD_cs(psamp)                = (U16) REGS_cs(regs);

        pmi_Get_CSD(SAMPLE_RECORD_cs(psamp),
                &SAMPLE_RECORD_csd(psamp).u1.lowWord,
                &SAMPLE_RECORD_csd(psamp).u2.highWord);
#endif
        SEP_DRV_LOG_TRACE("SAMPLE_RECORD_pid_rec_index(psamp) %x.", SAMPLE_RECORD_pid_rec_index(psamp));
        SEP_DRV_LOG_TRACE("SAMPLE_RECORD_tid(psamp) %x.", SAMPLE_RECORD_tid(psamp));
#if defined(DRV_IA32)
        SEP_DRV_LOG_TRACE("SAMPLE_RECORD_eip(psamp) %x.", SAMPLE_RECORD_eip(psamp));
        SEP_DRV_LOG_TRACE("SAMPLE_RECORD_eflags(psamp) %x.", SAMPLE_RECORD_eflags(psamp));
#endif
        SEP_DRV_LOG_TRACE("SAMPLE_RECORD_cpu_num(psamp) %x.", SAMPLE_RECORD_cpu_num(psamp));
        SEP_DRV_LOG_TRACE("SAMPLE_RECORD_cs(psamp) %x.", SAMPLE_RECORD_cs(psamp));
        SEP_DRV_LOG_TRACE("SAMPLE_RECORD_csd(psamp).lowWord %x.", SAMPLE_RECORD_csd(psamp).u1.lowWord);
        SEP_DRV_LOG_TRACE("SAMPLE_RECORD_csd(psamp).highWord %x.", SAMPLE_RECORD_csd(psamp).u2.highWord);

#if defined(DRV_EM64T)
        is_64bit_addr = (SAMPLE_RECORD_csd(psamp).u2.s2.reserved_0 == 1);
        if (is_64bit_addr) {
            SAMPLE_RECORD_iip(psamp)           = REGS_rip(regs);
            SAMPLE_RECORD_ipsr(psamp)          = (REGS_eflags(regs) & 0xffffffff) |
                (((U64) SAMPLE_RECORD_csd(psamp).u2.s2.dpl) << 32);
            SAMPLE_RECORD_ia64_pc(psamp)       = TRUE;
        }
        else {
            SAMPLE_RECORD_eip(psamp)           = REGS_rip(regs);
            SAMPLE_RECORD_eflags(psamp)        = REGS_eflags(regs);
            SAMPLE_RECORD_ia64_pc(psamp)       = FALSE;

            SEP_DRV_LOG_TRACE("SAMPLE_RECORD_eip(psamp) 0x%x.", SAMPLE_RECORD_eip(psamp));
            SEP_DRV_LOG_TRACE("SAMPLE_RECORD_eflags(psamp) %x.", SAMPLE_RECORD_eflags(psamp));
        }
#endif

        SAMPLE_RECORD_event_index(psamp) = DRV_EVENT_MASK_event_idx(&event_mask.eventmasks[i]);
        if (DEV_CONFIG_pebs_mode(pcfg) && DRV_EVENT_MASK_precise(&event_mask.eventmasks[i])) {
            if (EVENT_DESC_pebs_offset(evt_desc) ||
                EVENT_DESC_latency_offset_in_sample(evt_desc)) {
                lbr_tos_from_ip = PEBS_Fill_Buffer((S8 *)psamp,
                                                    evt_desc,
                                                    0);
            }
            PEBS_Modify_IP((S8 *)psamp, is_64bit_addr, 0);
            PEBS_Modify_TSC((S8 *)psamp, 0);
        }
        if (DEV_CONFIG_collect_lbrs(pcfg) &&
            DRV_EVENT_MASK_lbr_capture(&event_mask.eventmasks[i]) &&
            EVENT_DESC_lbr_offset(evt_desc) &&
           !DEV_CONFIG_apebs_collect_lbrs(pcfg)) {
            lbr_tos_from_ip = dispatch->read_lbrs(!DEV_CONFIG_store_lbrs(pcfg) ? NULL:((S8 *)(psamp)+EVENT_DESC_lbr_offset(evt_desc)));
        }
        if (DRV_EVENT_MASK_branch(&event_mask.eventmasks[i]) &&
            DEV_CONFIG_precise_ip_lbrs(pcfg)                 &&
            lbr_tos_from_ip) {
            if (is_64bit_addr) {
                SAMPLE_RECORD_iip(psamp)       = lbr_tos_from_ip;
                SEP_DRV_LOG_TRACE("UPDATED SAMPLE_RECORD_iip(psamp) 0x%llx.", SAMPLE_RECORD_iip(psamp));
            }
            else {
                SAMPLE_RECORD_eip(psamp)       = (U32) lbr_tos_from_ip;
                SEP_DRV_LOG_TRACE("UPDATED SAMPLE_RECORD_eip(psamp) 0x%x.", SAMPLE_RECORD_eip(psamp));
            }
        }
        if (DEV_CONFIG_power_capture(pcfg)) {
            dispatch->read_power(((S8 *)(psamp)+EVENT_DESC_power_offset_in_sample(evt_desc)));
        }

        if (DRV_CONFIG_event_based_counts(drv_cfg) &&
            DRV_EVENT_MASK_trigger(&event_mask.eventmasks[i])) {
            dispatch->read_counts((S8 *)psamp, DRV_EVENT_MASK_event_idx(&event_mask.eventmasks[i]));
        }
        if (DEV_CONFIG_enable_perf_metrics(pcfg) && DRV_EVENT_MASK_perf_metrics_capture(&event_mask.eventmasks[i])) {
            dispatch->read_metrics((S8 *)(psamp)+EVENT_DESC_perfmetrics_offset(evt_desc));
        }
        if (DRV_CONFIG_enable_p_state(drv_cfg)) {
            if (DRV_CONFIG_read_pstate_msrs(drv_cfg) &&
                (DRV_CONFIG_p_state_trigger_index(drv_cfg) == -1 || SAMPLE_RECORD_event_index(psamp) == DRV_CONFIG_p_state_trigger_index(drv_cfg))) {
                SEPDRV_P_STATE_Read((S8 *)(psamp)+EVENT_DESC_p_state_offset(evt_desc), pcpu);
            }
            if (!DRV_CONFIG_event_based_counts(drv_cfg) && CPU_STATE_p_state_counting(pcpu)) {
                dispatch->read_counts((S8 *) psamp, DRV_EVENT_MASK_event_idx(&event_mask.eventmasks[i]));
            }
        }

        if (DRV_CONFIG_unc_collect_in_intr_enabled(drv_cfg) && DRV_EVENT_MASK_trigger(&event_mask.eventmasks[i])) {
            for (unc_dev_idx = num_core_devs; unc_dev_idx < num_devices; unc_dev_idx++) {
                pcfg_unc = (DEV_UNC_CONFIG)LWPMU_DEVICE_pcfg(&devices[unc_dev_idx]);
                dispatch_unc = LWPMU_DEVICE_dispatch(&devices[unc_dev_idx]);

                if (pcfg_unc && DEV_UNC_CONFIG_device_with_intr_events(pcfg_unc) &&
                    dispatch_unc && dispatch_unc->trigger_read) {
                    read_unc_evt_counts_from_intr = 1;
                    dispatch_unc->trigger_read(psamp, unc_dev_idx, read_unc_evt_counts_from_intr);
                }
            }
        }
    }

pmi_cleanup:
    if (DEV_CONFIG_pebs_mode(pcfg)) {
        if (!multi_pebs_enabled) {
            PEBS_Reset_Index(this_cpu);
        }
        else {
            if (cpu_sideband_buf) {
                OUTPUT outbuf = &BUFFER_DESC_outbuf(&cpu_sideband_buf[this_cpu]);
                if (OUTPUT_signal_full(outbuf) && !OUTPUT_tasklet_queued(outbuf)) {
                    SEP_DRV_LOG_TRACE("Interrupt-driven sideband buffer flush tasklet scheduling.");
                    OUTPUT_tasklet_queued(outbuf) = TRUE;
                    tasklet_schedule(&CPU_STATE_nmi_tasklet(&pcb[this_cpu]));
                }
            }
        }
    }

    // Reset the data counters
    if (CPU_STATE_trigger_count(&pcb[this_cpu]) == 0) {
        dispatch->swap_group(FALSE);
    }
    // Re-enable the counter control
    dispatch->restart(NULL);
    SYS_Locked_Dec(&CPU_STATE_in_interrupt(&pcb[this_cpu])); // do not use SEP_DRV_LOG_X (where X != INTERRUPT) below this

    SEP_DRV_LOG_INTERRUPT_OUT("");
    return;
}


