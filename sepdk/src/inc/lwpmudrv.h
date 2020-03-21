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






#ifndef _LWPMUDRV_H_
#define _LWPMUDRV_H_

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/compat.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,12,0)
#include <asm/uaccess.h>
#else
#include <linux/uaccess.h>
#endif
#include <asm/cpufeature.h>

#include "lwpmudrv_defines.h"
#include "lwpmudrv_types.h"
#include "lwpmudrv_ecb.h"
#include "lwpmudrv_version.h"
#include "lwpmudrv_struct.h"
#include "pebs.h"

#if defined(X86_FEATURE_KAISER) || defined(CONFIG_KAISER) || defined(KAISER_HEADER_PRESENT)
#define  DRV_USE_KAISER
#elif defined(X86_FEATURE_PTI)
#define  DRV_USE_PTI
#endif

/*
 * Print macros for driver messages
 */

#if defined(MYDEBUG)
#define SEP_PRINT_DEBUG(fmt,args...) { printk(KERN_INFO SEP_MSG_PREFIX" [DEBUG] " fmt,##args); }
#else
#define SEP_PRINT_DEBUG(fmt,args...) {;}
#endif

#define SEP_PRINT(fmt,args...) { printk(KERN_INFO SEP_MSG_PREFIX" " fmt,##args); }

#define SEP_PRINT_WARNING(fmt,args...) { printk(KERN_ALERT SEP_MSG_PREFIX" [Warning] " fmt,##args); }

#define SEP_PRINT_ERROR(fmt,args...) { printk(KERN_CRIT SEP_MSG_PREFIX" [ERROR] " fmt,##args); }

// Macro to return the thread group id
#define GET_CURRENT_TGID() (current->tgid)

#define OVERFLOW_ARGS  U64*, U64*

typedef struct DRV_EVENT_MASK_NODE_S  DRV_EVENT_MASK_NODE;
typedef        DRV_EVENT_MASK_NODE    *DRV_EVENT_MASK;

struct DRV_EVENT_MASK_NODE_S {
    U16 event_idx;    // 0 <= index < MAX_EVENTS
    U16 desc_id;
    union {
        U32 bitFields1;
        struct {
            U32 precise              : 1;
            U32 lbr_capture          : 1;
            U32 dear_capture         : 1;  // Indicates which events need to have additional registers read
                                           // because they are DEAR events.
            U32 iear_capture         : 1;  // Indicates which events need to have additional registers read
                                           // because they are IEAR events.
            U32 btb_capture          : 1;  // Indicates which events need to have additional registers read
                                           // because they are BTB events.
            U32 ipear_capture        : 1;  // Indicates which events need to have additional registers read
                                           // because they are IPEAR events.
            U32 uncore_capture       : 1;
            U32 branch               : 1;  // Indicates whether the event is related to branch opertion or
                                           // not
            U32 perf_metrics_capture : 1;  // Indicates whether the event is related to perf_metrics or not
            U32 trigger              : 1;  // Indicates if the event interrupted is the trigger event, flag used
                                           // in ebc mode and uncore interrupt mode
            U32 collect_on_ctx_sw    : 1;  // Indicates to collect the data only if context switch occurs.
            U32 reserved             : 21;
        } s1;
    } u1;
};

#define DRV_EVENT_MASK_event_idx(d)             (d)->event_idx
#define DRV_EVENT_MASK_desc_id(d)               (d)->desc_id
#define DRV_EVENT_MASK_bitFields1(d)            (d)->u1.bitFields1
#define DRV_EVENT_MASK_precise(d)               (d)->u1.s1.precise
#define DRV_EVENT_MASK_lbr_capture(d)           (d)->u1.s1.lbr_capture
#define DRV_EVENT_MASK_dear_capture(d)          (d)->u1.s1.dear_capture
#define DRV_EVENT_MASK_iear_capture(d)          (d)->u1.s1.iear_capture
#define DRV_EVENT_MASK_btb_capture(d)           (d)->u1.s1.btb_capture
#define DRV_EVENT_MASK_ipear_capture(d)         (d)->u1.s1.ipear_capture
#define DRV_EVENT_MASK_uncore_capture(d)        (d)->u1.s1.uncore_capture
#define DRV_EVENT_MASK_branch(d)                (d)->u1.s1.branch
#define DRV_EVENT_MASK_perf_metrics_capture(d)  (d)->u1.s1.perf_metrics_capture
#define DRV_EVENT_MASK_trigger(d)               (d)->u1.s1.trigger
#define DRV_EVENT_MASK_collect_on_ctx_sw(d)     (d)->u1.s1.collect_on_ctx_sw

#define MAX_OVERFLOW_EVENTS 16    // This defines the maximum number of overflow events per interrupt.
                                  // In order to reduce memory footprint, the value should be at least
                                  // the number of fixed and general PMU registers.
                                  // Sandybridge with HT off has 11 PMUs(3 fixed and 8 generic)

typedef struct DRV_MASKS_NODE_S  DRV_MASKS_NODE;
typedef        DRV_MASKS_NODE    *DRV_MASKS;

/*
 * @macro DRV_EVENT_MASK_NODE_S
 * @brief
 * The structure is used to store overflow events when handling PMU interrupt.
 * This approach should be more efficient than checking all event masks
 * if there are many events to be monitored
 * and only a few events among them have overflow per interrupt.
 */
struct DRV_MASKS_NODE_S {
    DRV_EVENT_MASK_NODE eventmasks[MAX_OVERFLOW_EVENTS];
    U8                  masks_num; // 0 <= mask_num <= MAX_OVERFLOW_EVENTS
};

#define DRV_MASKS_masks_num(d)           (d)->masks_num
#define DRV_MASKS_eventmasks(d)          (d)->eventmasks


/*
 *  Dispatch table for virtualized functions.
 *  Used to enable common functionality for different
 *  processor microarchitectures
 */
typedef struct DISPATCH_NODE_S  DISPATCH_NODE;
typedef        DISPATCH_NODE   *DISPATCH;

struct DISPATCH_NODE_S {
    VOID (*init)(PVOID);
    VOID (*fini)(PVOID);
    VOID (*write)(PVOID);
    VOID (*freeze)(PVOID);
    VOID (*restart)(PVOID);
    VOID (*read_data)(PVOID, U32);
    VOID (*check_overflow)(DRV_MASKS);
    VOID (*swap_group)(DRV_BOOL);
    U64  (*read_lbrs)(PVOID);
    VOID (*cleanup)(PVOID);
    VOID (*hw_errata)(VOID);
    VOID (*read_power)(PVOID);
    U64  (*check_overflow_errata)(ECB, U32, U64);
    VOID (*read_counts)(PVOID, U32);
    U64  (*check_overflow_gp_errata)(ECB,U64*);
    VOID (*read_ro)(PVOID, U32, U32);
    VOID (*platform_info)(PVOID);
    VOID (*trigger_read)(PVOID, U32, U32);    // Counter reads triggered/initiated by User mode timer
    VOID (*scan_for_uncore)(PVOID);
    VOID (*read_metrics)(PVOID);
};

extern VOID         **PMU_register_data;
extern VOID         **desc_data;
extern U64           *prev_counter_data;

/*!
 * @struct LWPMU_DEVICE_NODE_S
 * @brief  Struct to hold fields per device
 *           PMU_register_data_unc - MSR info
 *           dispatch_unc          - dispatch table
 *           em_groups_counts_unc  - # groups
 *           pcfg_unc              - config struct
 */
typedef struct LWPMU_DEVICE_NODE_S  LWPMU_DEVICE_NODE;
typedef        LWPMU_DEVICE_NODE   *LWPMU_DEVICE;

struct LWPMU_DEVICE_NODE_S {
    VOID         **PMU_register_data;
    DISPATCH       dispatch;
    S32            em_groups_count;
    VOID          *pcfg;
    U64          **unc_prev_value;
    U64         ***unc_acc_value;
    U64            counter_mask;
    U64            num_events;
    U32            num_units;
    VOID          *ec;
    S32           *cur_group;
    S32            pci_dev_node_index;
    U32            device_type;
    LBR            lbr;
    PEBS_INFO_NODE pebs_info_node;
};

#define LWPMU_DEVICE_PMU_register_data(dev)   (dev)->PMU_register_data
#define LWPMU_DEVICE_dispatch(dev)            (dev)->dispatch
#define LWPMU_DEVICE_em_groups_count(dev)     (dev)->em_groups_count
#define LWPMU_DEVICE_pcfg(dev)                (dev)->pcfg
#define LWPMU_DEVICE_prev_value(dev)          (dev)->unc_prev_value
#define LWPMU_DEVICE_acc_value(dev)           (dev)->unc_acc_value
#define LWPMU_DEVICE_counter_mask(dev)        (dev)->counter_mask
#define LWPMU_DEVICE_num_events(dev)          (dev)->num_events
#define LWPMU_DEVICE_num_units(dev)           (dev)->num_units
#define LWPMU_DEVICE_ec(dev)                  (dev)->ec
#define LWPMU_DEVICE_cur_group(dev)           (dev)->cur_group
#define LWPMU_DEVICE_pci_dev_node_index(dev)  (dev)->pci_dev_node_index
#define LWPMU_DEVICE_device_type(dev)         (dev)->device_type
#define LWPMU_DEVICE_lbr(dev)                 (dev)->lbr
#define LWPMU_DEVICE_pebs_dispatch(dev)       (dev)->pebs_info_node.pebs_dispatch
#define LWPMU_DEVICE_pebs_record_size(dev)    (dev)->pebs_info_node.pebs_record_size
#define LWPMU_DEVICE_apebs_basic_offset(dev)  (dev)->pebs_info_node.apebs_basic_offset
#define LWPMU_DEVICE_apebs_mem_offset(dev)    (dev)->pebs_info_node.apebs_mem_offset
#define LWPMU_DEVICE_apebs_gpr_offset(dev)    (dev)->pebs_info_node.apebs_gpr_offset
#define LWPMU_DEVICE_apebs_xmm_offset(dev)    (dev)->pebs_info_node.apebs_xmm_offset
#define LWPMU_DEVICE_apebs_lbr_offset(dev)    (dev)->pebs_info_node.apebs_lbr_offset


extern U32            num_devices;
extern U32            cur_devices;
extern LWPMU_DEVICE   devices;
extern U64           *pmu_state;
extern U32            num_core_devs;

typedef struct UNC_EM_DESC_NODE_S  UNC_EM_DESC_NODE;
typedef        UNC_EM_DESC_NODE   *UNC_EM_DESC;

struct UNC_EM_DESC_NODE_S {
    struct timer_list *read_timer;
    U32                em_factor;
};

#define UNC_EM_read_timer(desc)  (desc)->read_timer
#define UNC_EM_em_factor(desc)   (desc)->em_factor

typedef struct EMON_DESC_NODE_S  EMON_DESC_NODE;
typedef        EMON_DESC_NODE   *EMON_DESC;

struct EMON_DESC_NODE_S {
    struct timer_list *read_timer;
    U32                call_count;
    U32                buf_index;
};

#define EMON_read_timer(desc)  (desc)->read_timer
#define EMON_call_count(desc)  (desc)->call_count
#define EMON_buf_index(desc)   (desc)->buf_index

// Handy macro
#define TSC_SKEW(this_cpu)     (cpu_tsc[this_cpu] - cpu_tsc[0])

/*
 *  The IDT / GDT descriptor for use in identifying code segments
 */
#if defined(DRV_EM64T)
#pragma pack(push,1)
typedef struct _idtgdtDesc {
    U16    idtgdt_limit;
    PVOID  idtgdt_base;
} IDTGDT_DESC;
#pragma pack(pop)

extern IDTGDT_DESC         gdt_desc;
#endif

extern DRV_BOOL            NMI_mode;
extern DRV_BOOL            KVM_guest_mode;

#endif

