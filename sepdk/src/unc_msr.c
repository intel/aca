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

extern U64                        *read_counter_info;
extern EMON_BUFFER_DRIVER_HELPER   emon_buffer_driver_helper;
extern DRV_CONFIG                  drv_cfg;
extern U64                        *prev_counter_data;

/*!
 * @fn          static VOID UNC_COMMON_MSR_Write_PMU(VOID*)
 *
 * @brief       Initial write of PMU registers
 *              Walk through the enties and write the value of the register accordingly.
 *              When current_group = 0, then this is the first time this routine is called,
 *
 * @param       None
 *
 * @return      None
 *
 * <I>Special Notes:</I>
 */
static VOID
UNC_MSR_Write_PMU (
    PVOID            param
)
{
    U32        dev_idx;
    U32        this_cpu;
    CPU_STATE  pcpu;

    SEP_DRV_LOG_TRACE_IN("Param: %p.", param);

    dev_idx  = *((U32*)param);
    this_cpu = CONTROL_THIS_CPU();
    pcpu     = &pcb[this_cpu];

    if (!CPU_STATE_socket_master(pcpu)) {
        SEP_DRV_LOG_TRACE_OUT("Early exit (!CPU_STATE_socket_master).");
        return;
    }

    FOR_EACH_REG_UNC_OPERATION(pecb, dev_idx, idx, PMU_OPERATION_WRITE) {
        SYS_Write_MSR(ECB_entries_reg_id(pecb,idx), ECB_entries_reg_value(pecb,idx));
    } END_FOR_EACH_REG_UNC_OPERATION;

    FOR_EACH_REG_UNC_OPERATION(pecb, dev_idx, idx, PMU_OPERATION_READ) {
        SYS_Write_MSR(ECB_entries_reg_id(pecb,idx), 0ULL);
        if (LWPMU_DEVICE_counter_mask(&devices[dev_idx]) == 0) {
            LWPMU_DEVICE_counter_mask(&devices[dev_idx]) = (U64)ECB_entries_max_bits(pecb,idx);
        }
    } END_FOR_EACH_REG_UNC_OPERATION;

    SEP_DRV_LOG_TRACE_OUT("");
    return;
}

/*!
 * @fn         static VOID UNC_MSR_Enable_PMU(PVOID)
 *
 * @brief      Set the enable bit for all the evsel registers
 *
 * @param      None
 *
 * @return     None
 *
 * <I>Special Notes:</I>
 */
static VOID
UNC_MSR_Enable_PMU (
    PVOID param
)
{
    U32        j;
    U32        dev_idx;
    U32        this_cpu;
    CPU_STATE  pcpu;
    U64        reg_val       = 0;
    U32        package_num   = 0;

    SEP_DRV_LOG_TRACE_IN("Param: %p.", param);

    dev_idx     = *((U32*)param);
    this_cpu    = CONTROL_THIS_CPU();
    pcpu        = &pcb[this_cpu];
    package_num = core_to_package_map[this_cpu];

    if (!CPU_STATE_socket_master(pcpu)) {
        SEP_DRV_LOG_TRACE_OUT("Early exit (!CPU_STATE_socket_master).");
        return;
    }

    FOR_EACH_REG_UNC_OPERATION(pecb, dev_idx, idx, PMU_OPERATION_ENABLE) {
        reg_val = ECB_entries_reg_value(pecb,idx);
        if (ECB_entries_reg_rw_type(pecb, idx)  == PMU_REG_RW_READ_WRITE) {
            reg_val = SYS_Read_MSR(ECB_entries_reg_id(pecb,idx));
            if (ECB_entries_reg_type(pecb,idx) == PMU_REG_UNIT_CTRL) {
                reg_val &= ECB_entries_reg_value(pecb,idx);
            }
            else {
                reg_val |= ECB_entries_reg_value(pecb,idx);
            }
        }
        SYS_Write_MSR(ECB_entries_reg_id(pecb,idx), reg_val);
    } END_FOR_EACH_REG_UNC_OPERATION;


    FOR_EACH_REG_UNC_OPERATION(pecb, dev_idx, idx, PMU_OPERATION_READ) {
        if (ECB_entries_counter_type(pecb,idx) == FREERUN_COUNTER) {
            U64          tmp_value           = 0;
            if (ECB_entries_event_scope(pecb,idx) == SYSTEM_EVENT) {
                j = ECB_entries_uncore_buffer_offset_in_system(pecb, idx);
            }
            else {
                j = EMON_BUFFER_UNCORE_PACKAGE_EVENT_OFFSET(package_num, EMON_BUFFER_DRIVER_HELPER_num_entries_per_package(emon_buffer_driver_helper),
                                                        ECB_entries_uncore_buffer_offset_in_package(pecb,idx));
            }
            tmp_value = SYS_Read_MSR(ECB_entries_reg_id(pecb,idx));
            if (ECB_entries_max_bits(pecb,idx)) {
                tmp_value &= ECB_entries_max_bits(pecb,idx);
            }
            prev_counter_data[j] = tmp_value;
            SEP_DRV_LOG_TRACE("j=%u, value=%llu, cpu=%u", j, prev_counter_data[j], this_cpu);
        }
    } END_FOR_EACH_REG_UNC_OPERATION;

    SEP_DRV_LOG_TRACE_OUT("");
    return;
}


/*!
 * @fn         static VOID UNC_MSR_Disable_PMU(PVOID)
 *
 * @brief      Set the enable bit for all the evsel registers
 *
 * @param      None
 *
 * @return     None
 *
 * <I>Special Notes:</I>
 */
static VOID
UNC_MSR_Disable_PMU (
    PVOID param
)
{
    U32        dev_idx;
    U32        this_cpu;
    CPU_STATE  pcpu;
    U64        reg_val = 0;

    SEP_DRV_LOG_TRACE_IN("Param: %p.", param);

    dev_idx  = *((U32*)param);
    this_cpu = CONTROL_THIS_CPU();
    pcpu     = &pcb[this_cpu];

    if (!CPU_STATE_socket_master(pcpu)) {
        SEP_DRV_LOG_TRACE_OUT("Early exit (!CPU_STATE_socket_master).");
        return;
    }

    FOR_EACH_REG_UNC_OPERATION(pecb, dev_idx, idx, PMU_OPERATION_DISABLE) {
        if (ECB_entries_reg_type(pecb, idx) == PMU_REG_GLOBAL_CTRL) {
            continue;
        }
        reg_val = ECB_entries_reg_value(pecb,idx);
        if (ECB_entries_reg_rw_type(pecb, idx)  == PMU_REG_RW_READ_WRITE) {
            reg_val = SYS_Read_MSR(ECB_entries_reg_id(pecb,idx));
            if (ECB_entries_reg_type(pecb,idx) == PMU_REG_UNIT_CTRL) {
                reg_val |= ECB_entries_reg_value(pecb,idx);
            }
            else {
                reg_val &= ECB_entries_reg_value(pecb,idx);
            }
        }
        SYS_Write_MSR(ECB_entries_reg_id(pecb,idx), reg_val);
    } END_FOR_EACH_REG_UNC_OPERATION;

    SEP_DRV_LOG_TRACE_OUT("");
    return;
}

/*!
 * @fn static VOID UNC_MSR_Read_PMU_Data(param)
 *
 * @param    param    The read thread node to process
 * @param    id       The id refers to the device index
 *
 * @return   None     No return needed
 *
 * @brief    Read the Uncore count data and store into the buffer
 *           Let us say we have 2 core events in a dual socket JKTN;
 *           The start_index will be at 32 as it will 2 events in 16 CPU per socket
 *           The position for first event of QPI will be computed based on its event
 *
 */
static VOID
UNC_MSR_Read_PMU_Data (
    PVOID  param,
    U32    dev_idx
)
{
    U32         j             = 0;
    U32         this_cpu;
    U32         package_num   = 0;
    U64        *buffer;
    CPU_STATE   pcpu;
    U32         cur_grp;
    ECB         pecb;
    U64        *prev_buffer  = prev_counter_data;
    U64         tmp_value    = 0ULL;

    SEP_DRV_LOG_TRACE_IN("Param: %p.", param);

    this_cpu    = CONTROL_THIS_CPU();
    buffer      = (U64 *)param;
    pcpu        = &pcb[this_cpu];
    package_num = core_to_package_map[this_cpu];
    cur_grp     = LWPMU_DEVICE_cur_group(&devices[(dev_idx)])[package_num];
    pecb        = LWPMU_DEVICE_PMU_register_data(&devices[(dev_idx)])[cur_grp];

    // NOTE THAT the read_pmu function on for EMON collection.
    if (!DRV_CONFIG_emon_mode(drv_cfg)) {
        SEP_DRV_LOG_TRACE_OUT("Early exit (!emon_mode).");
        return;
    }
    if (!CPU_STATE_socket_master(pcpu)) {
        SEP_DRV_LOG_TRACE_OUT("Early exit (!CPU_STATE_socket_master).");
        return;
    }
    if (!pecb) {
        SEP_DRV_LOG_TRACE_OUT("Early exit (!pecb).");
        return;
    }

    //Read in the counts into temporary buffer
    FOR_EACH_REG_UNC_OPERATION(pecb, dev_idx, idx, PMU_OPERATION_READ) {

        if (ECB_entries_event_scope(pecb,idx) == SYSTEM_EVENT) {
            j = ECB_entries_uncore_buffer_offset_in_system(pecb, idx);
        }
        else {
            j = EMON_BUFFER_UNCORE_PACKAGE_EVENT_OFFSET(package_num, EMON_BUFFER_DRIVER_HELPER_num_entries_per_package(emon_buffer_driver_helper),
                                                        ECB_entries_uncore_buffer_offset_in_package(pecb,idx));
        }
        tmp_value = SYS_Read_MSR(ECB_entries_reg_id(pecb,idx));
        if (ECB_entries_counter_type(pecb,idx) == FREERUN_COUNTER) {
            if (ECB_entries_max_bits(pecb,idx)) {
                tmp_value &= ECB_entries_max_bits(pecb,idx);
            }
            if (tmp_value >= prev_buffer[j]) {
                buffer[j] = tmp_value - prev_buffer[j];
            }
            else {
                buffer[j] = tmp_value + (ECB_entries_max_bits(pecb,idx) - prev_buffer[j]);
            }
        }
        else {
            buffer[j] = tmp_value;
        }
        SEP_DRV_LOG_TRACE("j=%u, value=%llu, cpu=%u", j, buffer[j], this_cpu);

    } END_FOR_EACH_REG_UNC_OPERATION;

    SEP_DRV_LOG_TRACE_OUT("");
    return;
}

/* ------------------------------------------------------------------------- */
/*!
 * @fn       static VOID UNC_MSR_Trigger_Read(param, id, read_from_intr)
 *
 * @param    param          Pointer to populate read data
 * @param    id             Device index
 * @param    read_from_intr Read data from interrupt or timer
 *
 * @return   None     No return needed
 *
 * @brief    Read the Uncore data from counters and store into buffer
 */
static VOID
UNC_MSR_Trigger_Read (
    PVOID  param,
    U32    id,
    U32    read_from_intr
)
{
    U32             this_cpu;
    U32             package_num;
    U32             cur_grp;
    ECB             pecb;
    U32             index = 0;
    U64             diff  = 0;
    U64             value;
    U64            *data;

    SEP_DRV_LOG_TRACE_IN("Param: %p, id: %u, intr mode: %u.", param, id, read_from_intr);

    this_cpu    = CONTROL_THIS_CPU();
    package_num = core_to_package_map[this_cpu];
    cur_grp     = LWPMU_DEVICE_cur_group(&devices[id])[package_num];
    pecb        = LWPMU_DEVICE_PMU_register_data(&devices[id])[cur_grp];

    //Read in the counts into uncore buffer
    FOR_EACH_REG_UNC_OPERATION(pecb, id, idx, PMU_OPERATION_READ) {
        // If the function is invoked from pmi, the event we are
        // reading counts must be an unc intr event.
        // If the function is invoked from timer, the event must not be
        // an interrupt read event.
        if ((read_from_intr && !ECB_entries_unc_evt_intr_read_get(pecb, idx)) ||
            (!read_from_intr && ECB_entries_unc_evt_intr_read_get(pecb, idx))) {
            index++;
            continue;
        }
        // Write GroupID based on interrupt read event or timer event into
        // the respective groupd id offsets
        if (read_from_intr) {
            data = (U64*)((S8*)param + ECB_group_id_offset_in_trigger_evt_desc(pecb));
        }
        else {
            data = (U64*)((S8*)param + ECB_group_offset(pecb));
        }
        *data = cur_grp + 1;
        value = SYS_Read_MSR(ECB_entries_reg_id(pecb,idx));
        //check for overflow
        if (value < LWPMU_DEVICE_prev_value(&devices[id])[package_num][index]) {
            diff = LWPMU_DEVICE_counter_mask(&devices[id]) - LWPMU_DEVICE_prev_value(&devices[id])[package_num][index];
            diff += value;
        }
        else {
            diff = value - LWPMU_DEVICE_prev_value(&devices[id])[package_num][index];
        }
        LWPMU_DEVICE_acc_value(&devices[id])[package_num][cur_grp][index] += diff;
        LWPMU_DEVICE_prev_value(&devices[id])[package_num][index] = value;
        data  = (U64 *)((S8*)param + ECB_entries_counter_event_offset(pecb,idx));
        *data = LWPMU_DEVICE_acc_value(&devices[id])[package_num][cur_grp][index];
        index++;
    } END_FOR_EACH_REG_UNC_OPERATION;

    SEP_DRV_LOG_TRACE_OUT("");
    return;
}


/*
 * Initialize the dispatch table
 */
DISPATCH_NODE  unc_msr_dispatch =
{
    NULL,                                // initialize
    NULL,                                // destroy
    UNC_MSR_Write_PMU,                   // write
    UNC_MSR_Disable_PMU,                 // freeze
    UNC_MSR_Enable_PMU,                  // restart
    UNC_MSR_Read_PMU_Data,               // read
    NULL,                                // check for overflow
    NULL,                                // swap group
    NULL,                                // read lbrs
    UNC_COMMON_MSR_Clean_Up,             // cleanup
    NULL,                                // hw errata
    NULL,                                // read power
    NULL,                                // check overflow errata
    NULL,                                // read counts
    NULL,                                // check overflow gp errata
    NULL,                                // read_ro
    NULL,                                // platform info
    UNC_MSR_Trigger_Read,                // trigger read
    NULL,                                // scan for uncore
    NULL                                 // read metrics
};



