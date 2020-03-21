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
#include "lwpmudrv_types.h"
#include "rise_errors.h"
#include "lwpmudrv_ecb.h"
#include "lwpmudrv_struct.h"

#include "inc/ecb_iterators.h"
#include "inc/control.h"
#include "inc/unc_common.h"
#include "inc/utility.h"
#include "inc/pci.h"
#include "inc/unc_gt.h"



extern U64                       *read_counter_info;
extern EMON_BUFFER_DRIVER_HELPER  emon_buffer_driver_helper;

static U64                        unc_gt_virtual_address = 0;
static SEP_MMIO_NODE              unc_gt_map;
static U32                        unc_gt_rc6_reg1;
static U32                        unc_gt_rc6_reg2;
static U32                        unc_gt_clk_gt_reg1;
static U32                        unc_gt_clk_gt_reg2;
static U32                        unc_gt_clk_gt_reg3;
static U32                        unc_gt_clk_gt_reg4;

/*!
 * @fn          static VOID unc_gt_Initialize(VOID*)
 *
 * @brief       Initial write of PMU registers
 *
 * @param       device id
 *
 * @return      None
 *
 * <I>Special Notes:</I>
 */
static VOID
unc_gt_Initialize  (
     PVOID   param
)
{
    U64                        bar;
    U32                        dev_idx           = 0;
    U32                        cur_grp           = 0;
    ECB                        pecb              = NULL;
    MMIO_BAR_INFO              mmio_bar_list     = NULL;

    SEP_DRV_LOG_TRACE_IN("Param: %p.", param);
    if (param == NULL) {
        SEP_DRV_LOG_TRACE_OUT("Early exit (!param).");
        return;
    }

    dev_idx         = *((U32*)param);
    cur_grp         = LWPMU_DEVICE_cur_group(&devices[(dev_idx)])[0];
    pecb            = LWPMU_DEVICE_PMU_register_data(&devices[dev_idx])[cur_grp];
    if (!pecb) {
        SEP_DRV_LOG_TRACE_OUT("Early exit (!pecb).");
    }

    mmio_bar_list   = &ECB_mmio_bar_list(pecb, 0);
    if (!mmio_bar_list) {
        SEP_DRV_LOG_TRACE_OUT("Early exit (!mmio_bar_info).");
        return;
    }

    bar = (U64)PCI_Read_U32(MMIO_BAR_INFO_bus_no(mmio_bar_list),
                            MMIO_BAR_INFO_dev_no(mmio_bar_list),
                            MMIO_BAR_INFO_func_no(mmio_bar_list),
                            MMIO_BAR_INFO_main_bar_offset(mmio_bar_list));

    bar &= UNC_GT_BAR_MASK;

    PCI_Map_Memory(&unc_gt_map, bar, GT_MMIO_SIZE);
    unc_gt_virtual_address  = SEP_MMIO_NODE_virtual_address(&unc_gt_map);

    SEP_DRV_LOG_TRACE_OUT("");

    return;
}

/*!
 * @fn          static VOID unc_gt_Write_PMU(VOID*)
 *
 * @brief       Walk through the enties and write the value of the register accordingly.
 *
 * @param       device id
 *
 * @return      None
 *
 * <I>Special Notes:</I>
 */
static VOID
unc_gt_Write_PMU (
    VOID  *param
)
{
    U32                        dev_idx;
    ECB                        pecb;
    U32                        offset_delta;
    U32                        tmp_value;
    U32                        this_cpu;
    CPU_STATE                  pcpu;

    SEP_DRV_LOG_TRACE_IN("Param: %p.", param);
    if (param == NULL) {
        SEP_DRV_LOG_TRACE_OUT("Early exit (!param).");
        return;
    }
    if (!unc_gt_virtual_address) {
        SEP_DRV_LOG_TRACE_OUT("Early exit (!unc_gt_virtual_address).");
        return;
    }

    dev_idx  = *((U32*)param);
    pecb     = LWPMU_DEVICE_PMU_register_data(&devices[dev_idx])[0];
    this_cpu = CONTROL_THIS_CPU();
    pcpu     = &pcb[this_cpu];

    if (!CPU_STATE_system_master(pcpu)) {
        SEP_DRV_LOG_TRACE_OUT("Early exit (!system_master).");
        return;
    }

    FOR_EACH_REG_UNC_OPERATION(pecb, dev_idx, i, PMU_OPERATION_READ) {
        offset_delta = ECB_entries_reg_offset(pecb, i);
        // this is needed for overflow detection of the accumulators.
        if (LWPMU_DEVICE_counter_mask(&devices[dev_idx]) == 0) {
            LWPMU_DEVICE_counter_mask(&devices[dev_idx]) = (U64)ECB_entries_max_bits(pecb,i);
        }
    } END_FOR_EACH_REG_UNC_OPERATION;

    //enable the global control to clear the counter first
    SYS_Write_MSR(PERF_GLOBAL_CTRL, ECB_entries_reg_value(pecb,0));
    FOR_EACH_REG_UNC_OPERATION(pecb, dev_idx, i, PMU_OPERATION_WRITE) {
        offset_delta = ECB_entries_reg_offset(pecb, i);
        if  (offset_delta == PERF_GLOBAL_CTRL) {
           continue;
        }
        PCI_MMIO_Write_U32(unc_gt_virtual_address,
                                offset_delta,
                                GT_CLEAR_COUNTERS);

        SEP_DRV_LOG_TRACE("CCCR offset delta is 0x%x W is clear ctrs.", offset_delta);
    } END_FOR_EACH_REG_UNC_OPERATION;

    //disable the counters
    SYS_Write_MSR(PERF_GLOBAL_CTRL, 0LL);

    FOR_EACH_REG_UNC_OPERATION(pecb, dev_idx, i, PMU_OPERATION_WRITE) {
        offset_delta           = ECB_entries_reg_offset(pecb, i);
        if  (offset_delta == PERF_GLOBAL_CTRL) {
           continue;
        }
        PCI_MMIO_Write_U32(unc_gt_virtual_address,
                                offset_delta,
                                ((U32)ECB_entries_reg_value(pecb,i)));
        tmp_value = PCI_MMIO_Read_U32(unc_gt_virtual_address, offset_delta);

        // remove compiler warning on unused variables
        if (tmp_value) {
        }

        SEP_DRV_LOG_TRACE("CCCR offset delta is 0x%x R is 0x%x W is 0x%llx.", offset_delta, tmp_value, ECB_entries_reg_value(pecb, i));
    } END_FOR_EACH_REG_UNC_OPERATION;

    SEP_DRV_LOG_TRACE_OUT("");
    return;
}

/*!
 * @fn          static VOID unc_gt_Disable_RC6_Clock_Gating(VOID)
 *
 * @brief       This snippet of code allows GT events to count by
 *              disabling settings related to clock gating/power
 * @param       none
 *
 * @return      None
 *
 * <I>Special Notes:</I>
 */
static VOID
unc_gt_Disable_RC6_Clock_Gating (
    VOID
)
{
    U32 tmp = 0;

    SEP_DRV_LOG_TRACE_IN("");

    // Disable RC6
    unc_gt_rc6_reg1  =  PCI_MMIO_Read_U32(unc_gt_virtual_address, UNC_GT_RC6_REG1);
    tmp              =  unc_gt_rc6_reg1 | UNC_GT_RC6_REG1_OR_VALUE;
    unc_gt_rc6_reg2  =  PCI_MMIO_Read_U32(unc_gt_virtual_address, UNC_GT_RC6_REG2);

    PCI_MMIO_Write_U32(unc_gt_virtual_address,
                       UNC_GT_RC6_REG2,
                       UNC_GT_RC6_REG2_VALUE);
    PCI_MMIO_Write_U32(unc_gt_virtual_address,
                       UNC_GT_RC6_REG1,
                       tmp);

    SEP_DRV_LOG_TRACE("Original value of RC6 rc6_1 = 0x%x, rc6_2 = 0x%x.", unc_gt_rc6_reg1, unc_gt_rc6_reg2);

    // Disable clock gating
    // Save
    unc_gt_clk_gt_reg1 = PCI_MMIO_Read_U32(unc_gt_virtual_address, UNC_GT_GCPUNIT_REG1);
    unc_gt_clk_gt_reg2 = PCI_MMIO_Read_U32(unc_gt_virtual_address, UNC_GT_GCPUNIT_REG2);
    unc_gt_clk_gt_reg3 = PCI_MMIO_Read_U32(unc_gt_virtual_address, UNC_GT_GCPUNIT_REG3);
    unc_gt_clk_gt_reg4 = PCI_MMIO_Read_U32(unc_gt_virtual_address, UNC_GT_GCPUNIT_REG4);

    SEP_DRV_LOG_TRACE("Original value of RC6 ck_1 = 0x%x, ck_2 = 0x%x.", unc_gt_clk_gt_reg1, unc_gt_clk_gt_reg2);
    SEP_DRV_LOG_TRACE("Original value of RC6 ck_3 = 0x%x, ck_4 = 0x%x.", unc_gt_clk_gt_reg3, unc_gt_clk_gt_reg4);

    // Disable
    PCI_MMIO_Write_U32(unc_gt_virtual_address,
                       UNC_GT_GCPUNIT_REG1,
                       UNC_GT_GCPUNIT_REG1_VALUE);
    PCI_MMIO_Write_U32(unc_gt_virtual_address,
                       UNC_GT_GCPUNIT_REG2,
                       UNC_GT_GCPUNIT_REG2_VALUE);
    PCI_MMIO_Write_U32(unc_gt_virtual_address,
                       UNC_GT_GCPUNIT_REG3,
                       UNC_GT_GCPUNIT_REG3_VALUE);
    PCI_MMIO_Write_U32(unc_gt_virtual_address,
                       UNC_GT_GCPUNIT_REG4,
                       UNC_GT_GCPUNIT_REG4_VALUE);

    SEP_DRV_LOG_TRACE_OUT("");
    return;
}


/*!
 * @fn          static VOID unc_gt_Restore_RC6_Clock_Gating(VOID)
 *
 * @brief       This snippet of code restores the system settings
 *              for clock gating/power
 * @param       none
 *
 * @return      None
 *
 * <I>Special Notes:</I>
 */
static VOID
unc_gt_Restore_RC6_Clock_Gating (
    VOID
)
{
    SEP_DRV_LOG_TRACE_IN("");

    PCI_MMIO_Write_U32(unc_gt_virtual_address,
                       UNC_GT_RC6_REG2,
                       unc_gt_rc6_reg2);
    PCI_MMIO_Write_U32(unc_gt_virtual_address,
                       UNC_GT_RC6_REG1,
                       unc_gt_rc6_reg1);

    PCI_MMIO_Write_U32(unc_gt_virtual_address,
                       UNC_GT_GCPUNIT_REG1,
                       unc_gt_clk_gt_reg1);
    PCI_MMIO_Write_U32(unc_gt_virtual_address,
                       UNC_GT_GCPUNIT_REG2,
                       unc_gt_clk_gt_reg2);
    PCI_MMIO_Write_U32(unc_gt_virtual_address,
                       UNC_GT_GCPUNIT_REG3,
                       unc_gt_clk_gt_reg3);
    PCI_MMIO_Write_U32(unc_gt_virtual_address,
                       UNC_GT_GCPUNIT_REG4,
                       unc_gt_clk_gt_reg4);

    SEP_DRV_LOG_TRACE_OUT("");
    return;
}

/*!
 * @fn         static VOID unc_gt_Enable_PMU(PVOID)
 *
 * @brief      Disable the clock gating and Set the global enable
 *
 * @param      device_id
 *
 * @return     None
 *
 * <I>Special Notes:</I>
 */
static VOID
unc_gt_Enable_PMU (
    PVOID   param
)
{
    U32          dev_idx;
    ECB          pecb;
    U32          this_cpu;
    CPU_STATE    pcpu;

    SEP_DRV_LOG_TRACE_IN("Param: %p.", param);

    dev_idx     = *((U32*)param);
    pecb        = LWPMU_DEVICE_PMU_register_data(&devices[dev_idx])[0];
    this_cpu    = CONTROL_THIS_CPU();
    pcpu        = &pcb[this_cpu];

    if (!CPU_STATE_system_master(pcpu)) {
        SEP_DRV_LOG_TRACE_OUT("Early exit (!system_master).");
        return;
    }

    unc_gt_Disable_RC6_Clock_Gating();

    if (pecb && GET_DRIVER_STATE() == DRV_STATE_RUNNING) {
        SYS_Write_MSR(PERF_GLOBAL_CTRL, ECB_entries_reg_value(pecb,0));
        SEP_DRV_LOG_TRACE("Enabling GT Global control = 0x%llx.", ECB_entries_reg_value(pecb, 0));
    }

    SEP_DRV_LOG_TRACE_OUT("");
    return;
}
/*!
 * @fn         static VOID unc_gt_Disable_PMU(PVOID)
 *
 * @brief      Unmap the virtual address when sampling/driver stops
 *             and restore system values for clock gating settings
 *
 * @param      None
 *
 * @return     None
 *
 * <I>Special Notes:</I>
 */
static VOID
unc_gt_Disable_PMU (
    PVOID  param
)
{
    U32          this_cpu;
    CPU_STATE    pcpu;
    U32          cur_driver_state;

    SEP_DRV_LOG_TRACE_IN("Dummy param: %p.", param);

    this_cpu         = CONTROL_THIS_CPU();
    pcpu             = &pcb[this_cpu];
    cur_driver_state = GET_DRIVER_STATE();

    if (!CPU_STATE_system_master(pcpu)) {
        SEP_DRV_LOG_TRACE_OUT("Early exit (!system_master).");
        return;
    }
    unc_gt_Restore_RC6_Clock_Gating();

    if (unc_gt_virtual_address                   &&
        (cur_driver_state == DRV_STATE_STOPPED      ||
         cur_driver_state == DRV_STATE_PREPARE_STOP ||
         cur_driver_state == DRV_STATE_TERMINATING)) {
        SYS_Write_MSR(PERF_GLOBAL_CTRL, 0LL);
        PCI_Unmap_Memory(&unc_gt_map);
        unc_gt_virtual_address = 0;
    }

    SEP_DRV_LOG_TRACE_OUT("");
    return;
}


/*!
 * @fn unc_gt_Trigger_Read(param, id, read_from_intr)
 *
 * @param    param          The read thread node to process
 * @param    id             The id refers to the device index
 * @param    read_from_intr Trigger read coming from intr or timer
 *
 * @return   None     No return needed
 *
 * @brief    Read the Uncore count data and store into the buffer param;
 *
 */
static VOID
unc_gt_Trigger_Read (
    PVOID  param,
    U32    id,
    U32    read_from_intr
)
{
    U32                this_cpu            = 0;
    U32                package_num         = 0;
    U32                cur_grp             = 0;
    U32                offset_delta        = 0;
    U32                tmp_value_lo        = 0;
    U32                tmp_value_hi        = 0;
    U64               *data;
    ECB                pecb;
    GT_CTR_NODE        gt_ctr_value;

    SEP_DRV_LOG_TRACE_IN("Param: %p, id: %u.", param, id);

    this_cpu    = CONTROL_THIS_CPU();
    package_num = core_to_package_map[this_cpu];
    cur_grp     = LWPMU_DEVICE_cur_group(&devices[id])[package_num];
    pecb        = LWPMU_DEVICE_PMU_register_data(&devices[id])[cur_grp];

    GT_CTR_NODE_value_reset(gt_ctr_value);

    //Read in the counts into temporary buffer
    FOR_EACH_REG_UNC_OPERATION(pecb, id, i, PMU_OPERATION_READ) {
        // If the function is invoked from pmi, the event we are
        // reading counts must be an unc intr event.
        // If the function is invoked from timer, the event must not be
        // an interrupt read event.
        if ((read_from_intr && !ECB_entries_unc_evt_intr_read_get(pecb, i)) ||
            (!read_from_intr && ECB_entries_unc_evt_intr_read_get(pecb, i))) {
            continue;
        }
        // Write GroupID based on interrupt read event or timer event into
        // the respective groupd id offsets
        if (read_from_intr) {
            data    = (U64*)((S8*)param + ECB_group_id_offset_in_trigger_evt_desc(pecb));
        }
        else {
            data    = (U64*)((S8*)param + ECB_group_offset(pecb));
        }
        *data   = cur_grp + 1;

        offset_delta                     = ECB_entries_reg_offset(pecb, i);
        tmp_value_lo                     = PCI_MMIO_Read_U32(unc_gt_virtual_address, offset_delta);
        offset_delta                     = offset_delta + NEXT_ADDR_OFFSET;
        tmp_value_hi                     = PCI_MMIO_Read_U32(unc_gt_virtual_address, offset_delta);
        data                             = (U64 *)((S8*)param + ECB_entries_counter_event_offset(pecb,i));
        GT_CTR_NODE_low(gt_ctr_value)    = tmp_value_lo;
        GT_CTR_NODE_high(gt_ctr_value)   = tmp_value_hi;
        *data                            = GT_CTR_NODE_value(gt_ctr_value);
        SEP_DRV_LOG_TRACE("DATA offset delta is 0x%x R is 0x%llx.", offset_delta, GT_CTR_NODE_value(gt_ctr_value));
    } END_FOR_EACH_REG_UNC_OPERATION;

    SEP_DRV_LOG_TRACE_OUT("");
    return;
}


static VOID
unc_gt_Read_PMU_Data (
    PVOID  param,
    U32    dev_idx
)
{
    U32                j                = 0;
    U32                this_cpu         = 0;
    U32                package_num      = 0;
    U32                cur_grp          = 0;
    U32                offset_delta     = 0;
    U32                tmp_value_lo     = 0;
    U32                tmp_value_hi     = 0;
    U64               *buffer           = (U64 *)param;
    CPU_STATE          pcpu;
    ECB                pecb;
    GT_CTR_NODE        gt_ctr_value;

    SEP_DRV_LOG_TRACE_IN("Param: %p.", param);

    this_cpu     = CONTROL_THIS_CPU();
    pcpu         = &pcb[this_cpu];

    if (!CPU_STATE_system_master(pcpu)) {
        SEP_DRV_LOG_TRACE_OUT("Early exit (!system_master).");
        return;
    }

    package_num = core_to_package_map[this_cpu];
    cur_grp     = LWPMU_DEVICE_cur_group(&devices[(dev_idx)])[package_num];
    pecb        = LWPMU_DEVICE_PMU_register_data(&devices[(dev_idx)])[(cur_grp)];
    GT_CTR_NODE_value_reset(gt_ctr_value);


    FOR_EACH_REG_UNC_OPERATION(pecb, dev_idx, i, PMU_OPERATION_READ) {
        j = EMON_BUFFER_UNCORE_PACKAGE_EVENT_OFFSET(package_num, EMON_BUFFER_DRIVER_HELPER_num_entries_per_package(emon_buffer_driver_helper),
                                                    ECB_entries_uncore_buffer_offset_in_package(pecb, i));
        offset_delta                     = ECB_entries_reg_offset(pecb, i);
        tmp_value_lo                     = PCI_MMIO_Read_U32(unc_gt_virtual_address, offset_delta);
        offset_delta                     = offset_delta + NEXT_ADDR_OFFSET;
        tmp_value_hi                     = PCI_MMIO_Read_U32(unc_gt_virtual_address, offset_delta);
        GT_CTR_NODE_low(gt_ctr_value)    = tmp_value_lo;
        GT_CTR_NODE_high(gt_ctr_value)   = tmp_value_hi;
        buffer[j] =  GT_CTR_NODE_value(gt_ctr_value);
        SEP_DRV_LOG_TRACE("j=%u, value=%llu, cpu=%u", j, buffer[j], this_cpu);

    } END_FOR_EACH_REG_UNC_OPERATION;

    SEP_DRV_LOG_TRACE_OUT("");
    return;
}

/*
 * Initialize the dispatch table
 */
DISPATCH_NODE  unc_gt_dispatch =
{
    unc_gt_Initialize,          // initialize
    NULL,                       // destroy
    unc_gt_Write_PMU,           // write
    unc_gt_Disable_PMU,         // freeze
    unc_gt_Enable_PMU,          // restart
    unc_gt_Read_PMU_Data,       // read
    NULL,                       // check for overflow
    NULL,                       // swap_group
    NULL,                       // read_lbrs
    NULL,                       // cleanup
    NULL,                       // hw_errata
    NULL,                       // read_power
    NULL,                       // check_overflow_errata
    NULL,                       // read_counts
    NULL,                       // check_overflow_gp_errata
    NULL,                       // read_ro
    NULL,                       // platform_info
    unc_gt_Trigger_Read,        // trigger read
    NULL,                       // scan for uncore
    NULL                        // read metrics
};

