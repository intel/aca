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





#ifndef _CONTROL_H_
#define _CONTROL_H_

#include <linux/smp.h>
#include <linux/timer.h>
#if defined(DRV_IA32)
#include <asm/apic.h>
#endif
#include <asm/io.h>
#if defined(DRV_IA32)
#include <asm/msr.h>
#endif
#include <asm/atomic.h>
#include <linux/interrupt.h>

#include "lwpmudrv_defines.h"
#include "lwpmudrv.h"
#include "lwpmudrv_types.h"

// large memory allocation will be used if the requested size (in bytes) is
// above this threshold
#define  MAX_KMALLOC_SIZE ((1<<17)-1)
#define  SEP_DRV_MEMSET   memset

// check whether Linux driver should use unlocked ioctls (not protected by BKL)
#if defined(HAVE_UNLOCKED_IOCTL)
#define DRV_USE_UNLOCKED_IOCTL
#endif
#if defined(DRV_USE_UNLOCKED_IOCTL)
#define IOCTL_OP .unlocked_ioctl
#define IOCTL_OP_TYPE long
#define IOCTL_USE_INODE
#else
#define IOCTL_OP .ioctl
#define IOCTL_OP_TYPE S32
#define IOCTL_USE_INODE struct   inode  *inode,
#endif

// Information about the state of the driver
typedef struct GLOBAL_STATE_NODE_S  GLOBAL_STATE_NODE;
typedef        GLOBAL_STATE_NODE   *GLOBAL_STATE;
struct GLOBAL_STATE_NODE_S {
    volatile S32    cpu_count;
    volatile S32    dpc_count;

    S32             num_cpus;       // Number of CPUs in the system
    S32             active_cpus;    // Number of active CPUs - some cores can be
                                    // deactivated by the user / admin
    S32             num_em_groups;
    S32             num_descriptors;
    volatile S32    current_phase;
    U32             num_modules;
};

// Access Macros
#define  GLOBAL_STATE_num_cpus(x)          ((x).num_cpus)
#define  GLOBAL_STATE_active_cpus(x)       ((x).active_cpus)
#define  GLOBAL_STATE_cpu_count(x)         ((x).cpu_count)
#define  GLOBAL_STATE_dpc_count(x)         ((x).dpc_count)
#define  GLOBAL_STATE_num_em_groups(x)     ((x).num_em_groups)
#define  GLOBAL_STATE_num_descriptors(x)   ((x).num_descriptors)
#define  GLOBAL_STATE_current_phase(x)     ((x).current_phase)
#define  GLOBAL_STATE_sampler_id(x)        ((x).sampler_id)
#define  GLOBAL_STATE_num_modules(x)       ((x).num_modules)

/*
 *
 *
 * CPU State data structure and access macros
 *
 */
typedef struct CPU_STATE_NODE_S  CPU_STATE_NODE;
typedef        CPU_STATE_NODE   *CPU_STATE;
struct CPU_STATE_NODE_S {
    S32         apic_id;             // Processor ID on the system bus
    PVOID       apic_linear_addr;    // linear address of local apic
    PVOID       apic_physical_addr;  // physical address of local apic

    PVOID       idt_base;            // local IDT base address
    atomic_t    in_interrupt;

#if defined(DRV_IA32)
    U64         saved_ih;            // saved perfvector to restore
#endif
#if defined(DRV_EM64T)
    PVOID       saved_ih;            // saved perfvector to restore
#endif

    U64         last_mperf;          // previous value of MPERF, needed for calculating delta MPERF
    U64         last_aperf;          // previous value of APERF, needed for calculating delta MPERF
    DRV_BOOL    last_p_state_valid;  // are the previous values valid? (e.g., the first measurement does not have
                                     // a previous value for calculating the delta's.
    DRV_BOOL    p_state_counting;    // Flag to mark PMI interrupt from fixed event

    S64        *em_tables;           // holds the data that is saved/restored
                                     // during event multiplexing
    U32         em_table_offset;

    struct timer_list *em_timer;
    U32         current_group;
    S32         trigger_count;
    S32         trigger_event_num;

    DISPATCH    dispatch;
    PVOID       lbr_area;
    PVOID       old_dts_buffer;
    PVOID       dts_buffer;
    U32         dts_buffer_size;
    U32         dts_buffer_offset;
    U32         initial_mask;
    U32         accept_interrupt;

    U64        *pmu_state;           // holds PMU state (e.g., MSRs) that will be
                                     // saved before and restored after collection
    S32         socket_master;
    S32         core_master;
    S32         thr_master;
    U64         num_samples;
    U64         reset_mask;
    U64         group_swap;
    U64         last_visa_count[16];
    U16         cpu_module_num;
    U16         cpu_module_master;
    S32         system_master;
    DRV_BOOL    offlined;
    U32         nmi_handled;
    struct tasklet_struct nmi_tasklet;
    U32         em_timer_delay;
    U32         core_type;
    U32         last_thread_id;
};

#define CPU_STATE_apic_id(cpu)              (cpu)->apic_id
#define CPU_STATE_apic_linear_addr(cpu)     (cpu)->apic_linear_addr
#define CPU_STATE_apic_physical_addr(cpu)   (cpu)->apic_physical_addr
#define CPU_STATE_idt_base(cpu)             (cpu)->idt_base
#define CPU_STATE_in_interrupt(cpu)         (cpu)->in_interrupt
#define CPU_STATE_saved_ih(cpu)             (cpu)->saved_ih
#define CPU_STATE_saved_ih_hi(cpu)          (cpu)->saved_ih_hi
#define CPU_STATE_dpc(cpu)                  (cpu)->dpc
#define CPU_STATE_em_tables(cpu)            (cpu)->em_tables
#define CPU_STATE_em_table_offset(cpu)      (cpu)->em_table_offset
#define CPU_STATE_pmu_state(cpu)            (cpu)->pmu_state
#define CPU_STATE_em_dpc(cpu)               (cpu)->em_dpc
#define CPU_STATE_em_timer(cpu)             (cpu)->em_timer
#define CPU_STATE_current_group(cpu)        (cpu)->current_group
#define CPU_STATE_trigger_count(cpu)        (cpu)->trigger_count
#define CPU_STATE_trigger_event_num(cpu)    (cpu)->trigger_event_num
#define CPU_STATE_dispatch(cpu)             (cpu)->dispatch
#define CPU_STATE_lbr(cpu)                  (cpu)->lbr
#define CPU_STATE_old_dts_buffer(cpu)       (cpu)->old_dts_buffer
#define CPU_STATE_dts_buffer(cpu)           (cpu)->dts_buffer
#define CPU_STATE_dts_buffer_size(cpu)      (cpu)->dts_buffer_size
#define CPU_STATE_dts_buffer_offset(cpu)    (cpu)->dts_buffer_offset
#define CPU_STATE_initial_mask(cpu)         (cpu)->initial_mask
#define CPU_STATE_accept_interrupt(cpu)     (cpu)->accept_interrupt
#define CPU_STATE_msr_value(cpu)            (cpu)->msr_value
#define CPU_STATE_msr_addr(cpu)             (cpu)->msr_addr
#define CPU_STATE_socket_master(cpu)        (cpu)->socket_master
#define CPU_STATE_core_master(cpu)          (cpu)->core_master
#define CPU_STATE_thr_master(cpu)           (cpu)->thr_master
#define CPU_STATE_num_samples(cpu)          (cpu)->num_samples
#define CPU_STATE_reset_mask(cpu)           (cpu)->reset_mask
#define CPU_STATE_group_swap(cpu)           (cpu)->group_swap
#define CPU_STATE_last_mperf(cpu)           (cpu)->last_mperf
#define CPU_STATE_last_aperf(cpu)           (cpu)->last_aperf
#define CPU_STATE_last_p_state_valid(cpu)   (cpu)->last_p_state_valid
#define CPU_STATE_cpu_module_num(cpu)       (cpu)->cpu_module_num
#define CPU_STATE_cpu_module_master(cpu)    (cpu)->cpu_module_master
#define CPU_STATE_p_state_counting(cpu)     (cpu)->p_state_counting
#define CPU_STATE_system_master(cpu)        (cpu)->system_master
#define CPU_STATE_offlined(cpu)             (cpu)->offlined
#define CPU_STATE_nmi_handled(cpu)          (cpu)->nmi_handled
#define CPU_STATE_nmi_tasklet(cpu)          (cpu)->nmi_tasklet
#define CPU_STATE_em_timer_delay(cpu)       (cpu)->em_timer_delay
#define CPU_STATE_core_type(cpu)            (cpu)->core_type
#define CPU_STATE_last_thread_id(cpu)       (cpu)->last_thread_id

/*
 * For storing data for --read/--write-msr command line options
 */
typedef struct MSR_DATA_NODE_S MSR_DATA_NODE;
typedef        MSR_DATA_NODE  *MSR_DATA;
struct MSR_DATA_NODE_S {
    U64         value;             // Used for emon, for read/write-msr value
    U64         addr;
};

#define MSR_DATA_value(md)   (md)->value
#define MSR_DATA_addr(md)    (md)->addr

/*
 * Memory Allocation tracker
 *
 * Currently used to track large memory allocations
 */

typedef struct MEM_EL_NODE_S  MEM_EL_NODE;
typedef        MEM_EL_NODE   *MEM_EL;
struct MEM_EL_NODE_S {
    PVOID     address;         // pointer to piece of memory we're tracking
    S32       size;            // size (bytes) of the piece of memory
    U32       is_addr_vmalloc; // flag to check if the memory is allocated using vmalloc
};

// accessors for MEM_EL defined in terms of MEM_TRACKER below

#define MEM_EL_MAX_ARRAY_SIZE  32   // minimum is 1, nominal is 64

typedef struct MEM_TRACKER_NODE_S  MEM_TRACKER_NODE;
typedef        MEM_TRACKER_NODE   *MEM_TRACKER;
struct MEM_TRACKER_NODE_S {
    U16         max_size;            // MAX number of elements in the array (default: MEM_EL_MAX_ARRAY_SIZE)
    U16         elements;            // number of elements available in this array
    U16         node_vmalloc;        // flag to check whether the node struct is allocated using vmalloc
    U16         array_vmalloc;       // flag to check whether the list of mem el is allocated using vmalloc
    MEM_EL      mem;                 // array of large memory items we're tracking
    MEM_TRACKER prev,next;           // enables bi-directional scanning of linked list
};
#define MEM_TRACKER_max_size(mt)         ((mt)->max_size)
#define MEM_TRACKER_node_vmalloc(mt)     ((mt)->node_vmalloc)
#define MEM_TRACKER_array_vmalloc(mt)    ((mt)->array_vmalloc)
#define MEM_TRACKER_elements(mt)         ((mt)->elements)
#define MEM_TRACKER_mem(mt)              ((mt)->mem)
#define MEM_TRACKER_prev(mt)             ((mt)->prev)
#define MEM_TRACKER_next(mt)             ((mt)->next)
#define MEM_TRACKER_mem_address(mt, i)   ((MEM_TRACKER_mem(mt)[(i)].address))
#define MEM_TRACKER_mem_size(mt, i)      ((MEM_TRACKER_mem(mt)[(i)].size))
#define MEM_TRACKER_mem_vmalloc(mt, i)   ((MEM_TRACKER_mem(mt)[(i)].is_addr_vmalloc))

/****************************************************************************
 ** Global State variables exported
 ***************************************************************************/
extern   CPU_STATE            pcb;
extern   U64                 *cpu_tsc;
extern   GLOBAL_STATE_NODE    driver_state;
extern   MSR_DATA             msr_data;
extern   U32                 *core_to_package_map;
extern   U32                 *core_to_dev_map;
extern   U32                 *core_to_phys_core_map;
extern   U32                 *core_to_thread_map;
extern   U32                 *threads_per_core;
extern   U32                  num_packages;
extern   U64                 *restore_bl_bypass;
extern   U32                 **restore_ha_direct2core;
extern   U32                 **restore_qpi_direct2core;
/****************************************************************************
 **  Handy Short cuts
 ***************************************************************************/

/*
 * CONTROL_THIS_CPU()
 *     Parameters
 *         None
 *     Returns
 *         CPU number of the processor being executed on
 *
 */
#define CONTROL_THIS_CPU()      smp_processor_id()

/*
 * CONTROL_THIS_RAW_CPU()
 *     Parameters
 *         None
 *     Returns
 *         CPU number of the processor being executed on
 *
 */
#define CONTROL_THIS_RAW_CPU() (raw_smp_processor_id())
/****************************************************************************
 **  Interface definitions
 ***************************************************************************/

/*
 *  Execution Control Functions
 */

extern VOID
CONTROL_Invoke_Cpu (
    S32   cpuid,
    VOID  (*func)(PVOID),
    PVOID ctx
);

/*
 * @fn VOID CONTROL_Invoke_Parallel_Service(func, ctx, blocking, exclude)
 *
 * @param    func     - function to be invoked by each core in the system
 * @param    ctx      - pointer to the parameter block for each function invocation
 * @param    blocking - Wait for invoked function to complete
 * @param    exclude  - exclude the current core from executing the code
 *
 * @returns  none
 *
 * @brief    Service routine to handle all kinds of parallel invoke on all CPU calls
 *
 * <I>Special Notes:</I>
 *         Invoke the function provided in parallel in either a blocking/non-blocking mode.
 *         The current core may be excluded if desired.
 *         NOTE - Do not call this function directly from source code.  Use the aliases
 *         CONTROL_Invoke_Parallel(), CONTROL_Invoke_Parallel_NB(), CONTROL_Invoke_Parallel_XS().
 *
 */
extern VOID
CONTROL_Invoke_Parallel_Service (
        VOID   (*func)(PVOID),
        PVOID  ctx,
        S32    blocking,
        S32    exclude
);

/*
 * @fn VOID CONTROL_Invoke_Parallel(func, ctx)
 *
 * @param    func     - function to be invoked by each core in the system
 * @param    ctx      - pointer to the parameter block for each function invocation
 *
 * @returns  none
 *
 * @brief    Invoke the named function in parallel. Wait for all the functions to complete.
 *
 * <I>Special Notes:</I>
 *        Invoke the function named in parallel, including the CPU that the control is
 *        being invoked on
 *        Macro built on the service routine
 *
 */
#define CONTROL_Invoke_Parallel(a,b)      CONTROL_Invoke_Parallel_Service((a),(b),TRUE,FALSE)

/*
 * @fn VOID CONTROL_Invoke_Parallel_NB(func, ctx)
 *
 * @param    func     - function to be invoked by each core in the system
 * @param    ctx      - pointer to the parameter block for each function invocation
 *
 * @returns  none
 *
 * @brief    Invoke the named function in parallel. DO NOT Wait for all the functions to complete.
 *
 * <I>Special Notes:</I>
 *        Invoke the function named in parallel, including the CPU that the control is
 *        being invoked on
 *        Macro built on the service routine
 *
 */
#define CONTROL_Invoke_Parallel_NB(a,b)   CONTROL_Invoke_Parallel_Service((a),(b),FALSE,FALSE)

/*
 * @fn VOID CONTROL_Invoke_Parallel_XS(func, ctx)
 *
 * @param    func     - function to be invoked by each core in the system
 * @param    ctx      - pointer to the parameter block for each function invocation
 *
 * @returns  none
 *
 * @brief    Invoke the named function in parallel. Wait for all the functions to complete.
 *
 * <I>Special Notes:</I>
 *        Invoke the function named in parallel, excluding the CPU that the control is
 *        being invoked on
 *        Macro built on the service routine
 *
 */
#define CONTROL_Invoke_Parallel_XS(a,b)   CONTROL_Invoke_Parallel_Service((a),(b),TRUE,TRUE)


/*
 * @fn VOID CONTROL_Memory_Tracker_Init(void)
 *
 * @param    None
 *
 * @returns  None
 *
 * @brief    Initializes Memory Tracker
 *
 * <I>Special Notes:</I>
 *           This should only be called when the
 *           the driver is being loaded.
 */
extern VOID
CONTROL_Memory_Tracker_Init (
    VOID
);

/*
 * @fn VOID CONTROL_Memory_Tracker_Free(void)
 *
 * @param    None
 *
 * @returns  None
 *
 * @brief    Frees memory used by Memory Tracker
 *
 * <I>Special Notes:</I>
 *           This should only be called when the
 *           driver is being unloaded.
 */
extern VOID
CONTROL_Memory_Tracker_Free (
    VOID
);

/*
 * @fn VOID CONTROL_Memory_Tracker_Compaction(void)
 *
 * @param    None
 *
 * @returns  None
 *
 * @brief    Compacts the memory allocator if holes are detected
 *
 * <I>Special Notes:</I>
 *           At end of collection (or at other safe sync point),
 *           reclaim/compact space used by mem tracker
 */
extern VOID
CONTROL_Memory_Tracker_Compaction (
    void
);

/*
 * @fn PVOID CONTROL_Allocate_Memory(size)
 *
 * @param    IN size     - size of the memory to allocate
 *
 * @returns  char*       - pointer to the allocated memory block
 *
 * @brief    Allocate and zero memory
 *
 * <I>Special Notes:</I>
 *           Allocate memory in the GFP_KERNEL pool.
 *
 *           Use this if memory is to be allocated within a context where
 *           the allocator can block the allocation (e.g., by putting
 *           the caller to sleep) while it tries to free up memory to
 *           satisfy the request.  Otherwise, if the allocation must
 *           occur atomically (e.g., caller cannot sleep), then use
 *           CONTROL_Allocate_KMemory instead.
 */
extern PVOID
CONTROL_Allocate_Memory (
    size_t    size
);

/*
 * @fn PVOID CONTROL_Allocate_KMemory(size)
 *
 * @param    IN size     - size of the memory to allocate
 *
 * @returns  char*       - pointer to the allocated memory block
 *
 * @brief    Allocate and zero memory
 *
 * <I>Special Notes:</I>
 *           Allocate memory in the GFP_ATOMIC pool.
 *
 *           Use this if memory is to be allocated within a context where
 *           the allocator cannot block the allocation (e.g., by putting
 *           the caller to sleep) as it tries to free up memory to
 *           satisfy the request.  Examples include interrupt handlers,
 *           process context code holding locks, etc.
 */
extern PVOID
CONTROL_Allocate_KMemory (
    size_t  size
);

/*
 * @fn PVOID CONTROL_Free_Memory(location)
 *
 * @param    IN location  - size of the memory to allocate
 *
 * @returns  pointer to the allocated memory block
 *
 * @brief    Frees the memory block
 *
 * <I>Special Notes:</I>
 *           Does not try to free memory if fed with a NULL pointer
 *           Expected usage:
 *               ptr = CONTROL_Free_Memory(ptr);
 */
extern PVOID
CONTROL_Free_Memory (
    PVOID    location
);

#endif

