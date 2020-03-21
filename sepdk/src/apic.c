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
#include <linux/interrupt.h>
#include <asm/msr.h>
#include <asm/apic.h>
#if defined(CONFIG_XEN) && LINUX_VERSION_CODE > KERNEL_VERSION(2,6,32)
#include <xen/xen.h>
#endif
#if defined(CONFIG_XEN_DOM0) && LINUX_VERSION_CODE > KERNEL_VERSION(3,3,0)
#include <xen/interface/platform.h>
#include <asm/xen/hypercall.h>
#endif

#include "lwpmudrv_types.h"
#include "rise_errors.h"
#include "lwpmudrv_ecb.h"
#include "apic.h"
#include "lwpmudrv.h"
#include "control.h"
#include "utility.h"

static DEFINE_PER_CPU(unsigned long, saved_apic_lvtpc);



/*!
 * @fn          VOID apic_Get_APIC_ID(S32 cpu)
 *
 * @brief       Obtain APIC ID
 *
 * @param       S32 cpuid - cpu index
 *
 * @return      U32 APIC ID
 */
VOID
apic_Get_APIC_ID(S32 cpu)
{
    U32             apic_id = 0;
    CPU_STATE       pcpu;

    SEP_DRV_LOG_TRACE_IN("CPU: %d.", cpu);
    pcpu = &pcb[cpu];

#if defined(CONFIG_XEN_DOM0) && LINUX_VERSION_CODE > KERNEL_VERSION(3,3,0)
    if (xen_initial_domain()) {
        S32 ret = 0;
        struct xen_platform_op op = {
                .cmd                   = XENPF_get_cpuinfo,
                .interface_version     = XENPF_INTERFACE_VERSION,
                .u.pcpu_info.xen_cpuid = cpu,
        };

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,5,0)
        ret = HYPERVISOR_platform_op(&op);
#else
        ret = HYPERVISOR_dom0_op(&op);
#endif
        if (ret) {
            SEP_DRV_LOG_ERROR("apic_Get_APIC_ID: Error in reading APIC ID on Xen PV.");
            apic_id = 0;
        } else {
            apic_id = op.u.pcpu_info.apic_id;
        }
    } else {
#endif
        apic_id = read_apic_id();
#if defined(CONFIG_XEN_DOM0) && LINUX_VERSION_CODE > KERNEL_VERSION(3,3,0)
    }
#endif

    CPU_STATE_apic_id(pcpu) = apic_id;

    SEP_DRV_LOG_TRACE_OUT("Apic_id[%d] is %d.", cpu, CPU_STATE_apic_id(pcpu));
}


/*!
 * @fn          extern VOID APIC_Init(param)
 *
 * @brief       initialize the local APIC
 *
 * @param       int cpu_idx - The cpu to deinit
 *
 * @return      None
 *
 * <I>Special Notes:</I>
 *              This routine is expected to be called via the CONTROL_Parallel routine
 */
extern VOID
APIC_Init (PVOID param)
{
    int             me;

    SEP_DRV_LOG_TRACE_IN("Param: %p.", param);

    preempt_disable();
    me      = CONTROL_THIS_CPU();
    preempt_enable();

    apic_Get_APIC_ID(me);

    SEP_DRV_LOG_TRACE_OUT("");
}

/*!
 * @fn          extern VOID APIC_Install_Interrupt_Handler(param)
 *
 * @brief       Install the interrupt handler
 *
 * @param       int param - The linear address of the Local APIC
 *
 * @return      None
 *
 * <I>Special Notes:</I>
 *             The linear address is necessary if the LAPIC is used.  If X2APIC is
 *             used the linear address is not necessary.
 */
extern VOID
APIC_Install_Interrupt_Handler (PVOID param)
{
    SEP_DRV_LOG_TRACE_IN("Param: %p.", param);

    per_cpu(saved_apic_lvtpc, CONTROL_THIS_CPU()) = apic_read(APIC_LVTPC);
    apic_write(APIC_LVTPC, APIC_DM_NMI);

    SEP_DRV_LOG_TRACE_OUT("");
}


/*!
 * @fn          extern VOID APIC_Enable_PMI(void)
 * 
 * @brief       Enable the PMU interrupt
 *
 * @param       None
 * 
 * @return      None
 *
 * <I>Special Notes:</I>
 *             <NONE>
 */
extern VOID
APIC_Enable_Pmi(VOID)
{
    SEP_DRV_LOG_TRACE_IN("");

    apic_write(APIC_LVTPC, APIC_DM_NMI);

    SEP_DRV_LOG_TRACE_OUT("");
}


/*!
 * @fn          extern VOID APIC_Restore_LVTPC(void)
 *
 * @brief       Restore APIC LVTPC value
 *
 * @param       None
 *
 * @return      None
 *
 * <I>Special Notes:</I>
 *             <NONE>
 */
extern VOID
APIC_Restore_LVTPC(PVOID param)
{
    SEP_DRV_LOG_TRACE_IN("");

    apic_write(APIC_LVTPC, per_cpu(saved_apic_lvtpc, CONTROL_THIS_CPU()));

    SEP_DRV_LOG_TRACE_OUT("");
}

