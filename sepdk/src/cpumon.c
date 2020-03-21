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





/*
 *  CVS_Id="$Id$"
 */

#include "lwpmudrv_defines.h"
#include <linux/version.h>
#include <linux/interrupt.h>
#if defined(DRV_EM64T)
#include <asm/desc.h>
#endif

#include "lwpmudrv_types.h"
#include "rise_errors.h"
#include "lwpmudrv_ecb.h"
#include "apic.h"
#include "lwpmudrv.h"
#include "control.h"
#include "utility.h"
#include "cpumon.h"
#include "pmi.h"
#include "sys_info.h"

#include <linux/ptrace.h>
#include <asm/nmi.h>

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,2,0))
#include <linux/notifier.h>
static int
cpumon_NMI_Handler (
    unsigned int cmd,
    struct pt_regs *regs
)
{
    U32 captured_state = GET_DRIVER_STATE();

    if (DRIVER_STATE_IN(captured_state, STATE_BIT_RUNNING | STATE_BIT_PAUSING | STATE_BIT_PREPARE_STOP | STATE_BIT_TERMINATING)) {
        if (captured_state != DRV_STATE_TERMINATING) {
            PMI_Interrupt_Handler(regs);
        }
        return NMI_HANDLED;
    }
    else {
        return NMI_DONE;
    }
}

#define EBS_NMI_CALLBACK                        cpumon_NMI_Handler

#else
#include <linux/kdebug.h>
static int
cpumon_NMI_Handler (
    struct notifier_block *self,
    unsigned long val, void *data
)
{
    struct die_args *args = (struct die_args *)data;
    U32 captured_state      = GET_DRIVER_STATE();

    if (args) {
        switch (val) {
            case DIE_NMI:
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,38))
            case DIE_NMI_IPI:
#endif
                if (DRIVER_STATE_IN(captured_state, STATE_BIT_RUNNING | STATE_BIT_PAUSING | STATE_BIT_PREPARE_STOP | STATE_BIT_TERMINATING)) {
                    if (captured_state != DRV_STATE_TERMINATING) {
                        PMI_Interrupt_Handler(args->regs);
                    }
                    return NOTIFY_STOP;
                }
        }
    }
    return NOTIFY_DONE;
}

static struct notifier_block cpumon_notifier = {
        .notifier_call = cpumon_NMI_Handler,
        .next = NULL,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,38))
        .priority = 2
#else
        .priority = NMI_LOCAL_LOW_PRIOR,
#endif
};
#endif


static volatile S32   cpuhook_installed = 0;

/*
 * CPU Monitoring Functionality
 */


/*
 * General per-processor initialization
 */
#if defined(DRV_CPU_HOTPLUG)
/* ------------------------------------------------------------------------- */
/*!
 * @fn       DRV_BOOL CPUMON_is_Online_Allowed()
 *
 * @param    None
 *
 * @return   DRV_BOOL TRUE if cpu is allowed to go Online, else FALSE
 *
 * @brief    Checks if the cpu is allowed to go online during the
 * @brief    current driver state
 *
 */
extern DRV_BOOL
CPUMON_is_Online_Allowed()
{
    U32      cur_driver_state;
    DRV_BOOL is_allowed = FALSE;

    SEP_DRV_LOG_TRACE_IN("");

    cur_driver_state = GET_DRIVER_STATE();

    switch (cur_driver_state) {
       case DRV_STATE_IDLE:
       case DRV_STATE_PAUSED:
       case DRV_STATE_RUNNING:
       case DRV_STATE_PAUSING:
           is_allowed = TRUE;
           break;
       default:
           SEP_DRV_LOG_TRACE("CPU is prohibited to online in driver state %d.", cur_driver_state);
           break;
    }

    SEP_DRV_LOG_TRACE_OUT("Res: %u.", is_allowed);
    return is_allowed;
}

/* ------------------------------------------------------------------------- */
/*!
 * @fn       DRV_BOOL CPUMON_is_Offline_Allowed()
 *
 * @param    None
 *
 * @return   DRV_BOOL TRUE if cpu is allowed to go Offline, else FALSE
 *
 * @brief    Checks if the cpu is allowed to go offline during the
 * @brief    current driver state
 *
 */
extern DRV_BOOL
CPUMON_is_Offline_Allowed()
{
    U32      cur_driver_state;
    DRV_BOOL is_allowed = FALSE;

    SEP_DRV_LOG_TRACE_IN("");

    cur_driver_state = GET_DRIVER_STATE();

    switch (cur_driver_state) {
       case DRV_STATE_PAUSED:
       case DRV_STATE_RUNNING:
       case DRV_STATE_PAUSING:
           is_allowed = TRUE;
           break;
       default:
           SEP_DRV_LOG_TRACE("CPU is prohibited to offline in driver state %d.", cur_driver_state);
           break;
    }

    SEP_DRV_LOG_TRACE_OUT("Res: %u.", is_allowed);
    return is_allowed;
}

/* ------------------------------------------------------------------------- */
/*!
 * @fn       VOID CPUMON_Online_Cpu(
 *               PVOID param)
 *
 * @param    PVOID parm
 *
 * @return   None
 *
 * @brief    Sets a cpu online, initialize APIC on it,
 * @brief    Build the sys_info for this cpu
 *
 */
extern VOID
CPUMON_Online_Cpu (
    PVOID parm
)
{
    U32           this_cpu;
    CPU_STATE     pcpu;

    SEP_DRV_LOG_TRACE_IN("Dummy parm: %p.", parm);

    preempt_disable();
    this_cpu = CONTROL_THIS_CPU();
    preempt_enable();
    pcpu = &pcb[this_cpu];
    if (pcpu == NULL) {
        SEP_DRV_LOG_WARNING_TRACE_OUT("Unable to set CPU %d online!", this_cpu);
        return;
    }
    SEP_DRV_LOG_INIT("Setting CPU %d online, PCPU = %p.", this_cpu, pcpu);
    CPU_STATE_offlined(pcpu)           = FALSE;
    CPU_STATE_accept_interrupt(pcpu)   = 1;
    CPU_STATE_initial_mask(pcpu)       = 1;
    CPU_STATE_group_swap(pcpu)         = 1;
    APIC_Init(NULL);
    APIC_Install_Interrupt_Handler(NULL);

    SYS_INFO_Build_Cpu(NULL);

    SEP_DRV_LOG_TRACE_OUT("");
    return;
}

/* ------------------------------------------------------------------------- */
/*!
 * @fn       VOID CPUMON_Offline_Cpu(
 *               PVOID param)
 *
 * @param    PVOID parm
 *
 * @return   None
 *
 * @brief    Sets a cpu offline
 *
 */
extern VOID
CPUMON_Offline_Cpu (
    PVOID parm
)
{
    U32       cpu_idx;
    CPU_STATE pcpu;

    SEP_DRV_LOG_TRACE_IN("Dummy parm: %p.", parm);

    cpu_idx = *(U32 *) parm;
    pcpu    = &pcb[cpu_idx];

    if (pcpu == NULL) {
        SEP_DRV_LOG_WARNING_TRACE_OUT("Unable to set CPU %d offline.", cpu_idx);
        return;
    }
    SEP_DRV_LOG_INIT("Setting CPU %d offline.", cpu_idx);
    CPU_STATE_offlined(pcpu) = TRUE;

    SEP_DRV_LOG_TRACE_OUT("");
    return;
}
#endif


/* ------------------------------------------------------------------------- */
/*!
 * @fn extern void CPUMON_Install_Cpuhooks(void)
 *
 * @param    None
 *
 * @return   None     No return needed
 *
 * @brief  set up the interrupt handler (on a per-processor basis)
 * @brief  Initialize the APIC in two phases (current CPU, then others)
 *
 */
extern VOID
CPUMON_Install_Cpuhooks (
    void
)
{
    S32   me        = 0;

    SEP_DRV_LOG_TRACE_IN("");

    if (cpuhook_installed) {
        SEP_DRV_LOG_WARNING_TRACE_OUT("Cpuhook already installed.");
        return;
    }

    CONTROL_Invoke_Parallel(APIC_Init, NULL);
    CONTROL_Invoke_Parallel(APIC_Install_Interrupt_Handler, (PVOID)(size_t)me);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,2,0))
    register_nmi_handler(NMI_LOCAL, EBS_NMI_CALLBACK, 0, "sep_pmi");
#else
    register_die_notifier(&cpumon_notifier);
#endif

    cpuhook_installed = 1;

    SEP_DRV_LOG_TRACE_OUT("");
    return;
}

/* ------------------------------------------------------------------------- */
/*!
 * @fn extern void CPUMON_Remove_Cpuhools(void)
 *
 * @param    None
 *
 * @return   None     No return needed
 *
 * @brief  De-Initialize the APIC in phases
 * @brief  clean up the interrupt handler (on a per-processor basis)
 *
 */
extern VOID
CPUMON_Remove_Cpuhooks (
    void
)
{
    SEP_DRV_LOG_TRACE_IN("");
    CONTROL_Invoke_Parallel(APIC_Restore_LVTPC, NULL);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,2,0))
    unregister_nmi_handler(NMI_LOCAL, "sep_pmi");
#else
    unregister_die_notifier(&cpumon_notifier);
#endif

    cpuhook_installed = 0;

    SEP_DRV_LOG_TRACE_OUT("");
    return;
}

