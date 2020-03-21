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

extern U64                        *read_counter_info;
extern UNCORE_TOPOLOGY_INFO_NODE   uncore_topology;
extern EMON_BUFFER_DRIVER_HELPER   emon_buffer_driver_helper;
extern DRV_CONFIG                  drv_cfg;


/*!
 * @fn          static VOID unc_pci_Write_PMU(VOID*)
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
unc_pci_Write_PMU (
    PVOID            param
)
{
    U32         device_id;
    U32         dev_idx;
    U32         value;
    U32         vendor_id;
    U32         this_cpu;
    CPU_STATE   pcpu;
    U32         package_num   = 0;
    U32         dev_node      = 0;
    U32         cur_grp;
    ECB         pecb;
    U32         busno;

    SEP_DRV_LOG_TRACE_IN("Param: %p.", param);

    dev_idx     = *((U32*)param);
    this_cpu    = CONTROL_THIS_CPU();
    pcpu        = &pcb[this_cpu];
    package_num = core_to_package_map[this_cpu];
    cur_grp     = LWPMU_DEVICE_cur_group(&devices[(dev_idx)])[package_num];
    pecb        = LWPMU_DEVICE_PMU_register_data(&devices[(dev_idx)])[cur_grp];

    if (!CPU_STATE_socket_master(pcpu)) {
        SEP_DRV_LOG_TRACE_OUT("Early exit (!CPU_STATE_socket_master).");
        return;
    }
    if (!pecb) {
        SEP_DRV_LOG_TRACE_OUT("Early exit (!pecb).");
        return;
    }

    // first, figure out which package maps to which bus
    dev_node = ECB_dev_node(pecb);
    if (!IS_BUS_MAP_VALID(dev_node, package_num)) {
        SEP_DRV_LOG_ERROR_TRACE_OUT("No UNC_PCIDEV bus map for %u!", dev_node);
        return;
    }

    busno = GET_BUS_MAP(dev_node, package_num);

    LWPMU_DEVICE_pci_dev_node_index(&devices[dev_idx]) = dev_node;

    FOR_EACH_REG_UNC_OPERATION(pecb, dev_idx, idx, PMU_OPERATION_WRITE) {
        if (ECB_entries_reg_type(pecb, idx)  == PMU_REG_GLOBAL_CTRL) {
             //Check if we need to zero this MSR out
             SYS_Write_MSR(ECB_entries_reg_id(pecb,idx), 0LL);
             continue;
        }

        // otherwise, we have a valid entry
        // now we just need to find the corresponding bus #
        ECB_entries_bus_no(pecb,idx) = busno;
        value = PCI_Read_U32(busno, ECB_entries_dev_no(pecb,idx), ECB_entries_func_no(pecb,idx), 0);

        CONTINUE_IF_NOT_GENUINE_INTEL_DEVICE(value, vendor_id, device_id);

        if (ECB_entries_reg_type(pecb, idx)  == PMU_REG_UNIT_CTRL) {
            // busno can not be stored in ECB because different sockets have different bus no.
            PCI_Write_U32(busno,
                      ECB_entries_dev_no(pecb,idx),
                      ECB_entries_func_no(pecb,idx),
                      ECB_entries_reg_id(pecb,idx),
                      (U32)ECB_entries_reg_value(pecb,idx));
            continue;
        }

        // now program at the corresponding offset
        PCI_Write_U32(busno,
                  ECB_entries_dev_no(pecb,idx),
                  ECB_entries_func_no(pecb,idx),
                  ECB_entries_reg_id(pecb,idx),
                  (U32)ECB_entries_reg_value(pecb,idx));

        if ((ECB_entries_reg_value(pecb,idx) >> NEXT_ADDR_SHIFT) != 0) {
            PCI_Write_U32(busno,
                      ECB_entries_dev_no(pecb,idx),
                      ECB_entries_func_no(pecb,idx),
                      ECB_entries_reg_id(pecb,idx) + NEXT_ADDR_OFFSET,
                      (U32)(ECB_entries_reg_value(pecb,idx) >> NEXT_ADDR_SHIFT));
        }

    } END_FOR_EACH_REG_UNC_OPERATION;

    FOR_EACH_REG_UNC_OPERATION(pecb, dev_idx, idx, PMU_OPERATION_READ) {
        PCI_Write_U64(busno,
                  ECB_entries_dev_no(pecb,idx),
                  ECB_entries_func_no(pecb,idx),
                  ECB_entries_reg_id(pecb,idx),
                  0);

        // this is needed for overflow detection of the accumulators.
        if (LWPMU_DEVICE_counter_mask(&devices[dev_idx]) == 0) {
             LWPMU_DEVICE_counter_mask(&devices[dev_idx]) = (U64)ECB_entries_max_bits(pecb,idx);
        }
    } END_FOR_EACH_REG_UNC_OPERATION;

    SEP_DRV_LOG_TRACE_OUT("");
    return;
}

/*!
 * @fn         static VOID unc_pci_Enable_PMU(PVOID)
 *
 * @brief      Set the enable bit for all the EVSEL registers
 *
 * @param      Device Index of this PMU unit
 *
 * @return     None
 *
 * <I>Special Notes:</I>
 */
static VOID
unc_pci_Enable_PMU (
    PVOID               param
)
{
    U32            dev_idx;
    U32            this_cpu;
    CPU_STATE      pcpu;
    U32            package_num   = 0;
    U32            dev_node;
    U32            reg_val       = 0;
    U32            busno;

    SEP_DRV_LOG_TRACE_IN("Param: %p.", param);

    dev_idx  = *((U32 *)param);
    this_cpu = CONTROL_THIS_CPU();
    pcpu     = &pcb[this_cpu];
    dev_node = LWPMU_DEVICE_pci_dev_node_index(&devices[dev_idx]);

    if (!CPU_STATE_socket_master(pcpu)) {
        SEP_DRV_LOG_TRACE_OUT("Early exit (!CPU_STATE_socket_master).");
        return;
    }

    package_num         = core_to_package_map[this_cpu];

    if (!IS_BUS_MAP_VALID(dev_node, package_num)) {
        SEP_DRV_LOG_ERROR_TRACE_OUT("No UNC_PCIDEV bus map for %u!", dev_node);
        return;
    }

    busno = GET_BUS_MAP(dev_node, package_num);

    FOR_EACH_REG_UNC_OPERATION(pecb, dev_idx, idx, PMU_OPERATION_ENABLE) {
        if (ECB_entries_reg_type(pecb, idx)  == PMU_REG_GLOBAL_CTRL) {
            SYS_Write_MSR(ECB_entries_reg_id(pecb,idx), ECB_entries_reg_value(pecb,idx));
            continue;
        }
        reg_val = (U32)ECB_entries_reg_value(pecb,idx);
        if (ECB_entries_reg_rw_type(pecb, idx)  == PMU_REG_RW_READ_WRITE) {
            reg_val = PCI_Read_U32(busno,
                             ECB_entries_dev_no(pecb,idx),
                             ECB_entries_func_no(pecb,idx),
                             ECB_entries_reg_id(pecb,idx));
            reg_val &= ECB_entries_reg_value(pecb,idx);
        }
        PCI_Write_U32(busno,
               ECB_entries_dev_no(pecb,idx),
               ECB_entries_func_no(pecb,idx),
               ECB_entries_reg_id(pecb,idx),
               reg_val);
    } END_FOR_EACH_REG_UNC_OPERATION;

    SEP_DRV_LOG_TRACE_OUT("");
    return;
}

/*!
 * @fn           static VOID unc_pci_Disable_PMU(PVOID)
 *
 * @brief        Disable the per unit global control to stop the PMU counters.
 *
 * @param        Device Index of this PMU unit
 * @control_msr  Control MSR address
 * @enable_val   If counter freeze bit does not work, counter enable bit should be cleared
 * @disable_val  Disable collection
 *
 * @return       None
 *
 * <I>Special Notes:</I>
 */
static VOID
unc_pci_Disable_PMU (
    PVOID               param
)
{
    U32            dev_idx;
    U32            this_cpu;
    CPU_STATE      pcpu;
    U32            package_num   = 0;
    U32            dev_node;
    U32            reg_val       = 0;
    U32            busno;

    SEP_DRV_LOG_TRACE_IN("Param: %p.", param);

    dev_idx      = *((U32 *)param);
    this_cpu     = CONTROL_THIS_CPU();
    pcpu         = &pcb[this_cpu];
    dev_node     = LWPMU_DEVICE_pci_dev_node_index(&devices[dev_idx]);

    if (!CPU_STATE_socket_master(pcpu)) {
        SEP_DRV_LOG_TRACE_OUT("Early exit (!CPU_STATE_socket_master).");
        return;
    }

    package_num         = core_to_package_map[this_cpu];

    if (!IS_BUS_MAP_VALID(dev_node, package_num)) {
        SEP_DRV_LOG_ERROR_TRACE_OUT("No UNC_PCIDEV bus map for %u!", dev_node);
        return;
    }

    busno = GET_BUS_MAP(dev_node, package_num);

    FOR_EACH_REG_UNC_OPERATION(pecb, dev_idx, idx, PMU_OPERATION_DISABLE) {
        if (ECB_entries_reg_type(pecb, idx)  == PMU_REG_GLOBAL_CTRL) {
            continue;
        }
        reg_val = (U32)ECB_entries_reg_value(pecb,idx);
        if (ECB_entries_reg_rw_type(pecb, idx)  == PMU_REG_RW_READ_WRITE) {
            reg_val = PCI_Read_U32(busno,
                             ECB_entries_dev_no(pecb,idx),
                             ECB_entries_func_no(pecb,idx),
                             ECB_entries_reg_id(pecb,idx));
            reg_val |= ECB_entries_reg_value(pecb,idx);
        }
        PCI_Write_U32(busno,
               ECB_entries_dev_no(pecb,idx),
               ECB_entries_func_no(pecb,idx),
               ECB_entries_reg_id(pecb,idx),
               reg_val);
    } END_FOR_EACH_REG_UNC_OPERATION;

    SEP_DRV_LOG_TRACE_OUT("");
    return;
}


/* ------------------------------------------------------------------------- */
/*!
 * @fn       static VOID unc_pci_Trigger_Read(param, id, read_from_intr)
 *
 * @param    param          Pointer to populate read data
 * @param    id             Device index
 * @param    read_from_intr Read data from interrupt or timer
 *
 * @return   None     No return needed
 *
 * @brief    Read the Uncore data from counters and store into buffer
 */
static  VOID
unc_pci_Trigger_Read (
    PVOID  param,
    U32    id,
    U32    read_from_intr
)
{
    U32             this_cpu      = 0;
    U32             package_num   = 0;
    U32             dev_node      = 0;
    U32             cur_grp       = 0;
    ECB             pecb          = NULL;
    U32             index         = 0;
    U64             value_low     = 0;
    U64             value_high    = 0;
    U64             diff          = 0;
    U64             value;
    U64            *data;
    U32             busno;

    SEP_DRV_LOG_TRACE_IN("Param: %p, id: %u.", param, id);

    this_cpu      = CONTROL_THIS_CPU();
    package_num   = core_to_package_map[this_cpu];
    dev_node      = LWPMU_DEVICE_pci_dev_node_index(&devices[id]);
    cur_grp       = LWPMU_DEVICE_cur_group(&devices[id])[package_num];
    pecb          = LWPMU_DEVICE_PMU_register_data(&devices[id])[cur_grp];

    if (!IS_BUS_MAP_VALID(dev_node, package_num)) {
        SEP_DRV_LOG_ERROR_TRACE_OUT("No UNC_PCIDEV bus map for %u!", dev_node);
        return;
    }

    busno = GET_BUS_MAP(dev_node, package_num);

    // Read the counts into uncore buffer
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

        // read lower 4 bytes
        value_low = PCI_Read_U32(busno,
                             ECB_entries_dev_no(pecb,idx),
                             ECB_entries_func_no(pecb,idx),
                             ECB_entries_reg_id(pecb,idx));
        value = LOWER_4_BYTES_MASK & value_low;

        // read upper 4 bytes
        value_high = PCI_Read_U32(busno,
                              ECB_entries_dev_no(pecb,idx),
                              ECB_entries_func_no(pecb,idx),
                              (ECB_entries_reg_id(pecb,idx) + NEXT_ADDR_OFFSET));
        value |= value_high << NEXT_ADDR_SHIFT;
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

/*!
 * @fn       static   unc_pci_Read_PMU_Data(param)
 *
 * @param    param    The device index
 *
 * @return   None     No return needed
 *
 * @brief    Read the Uncore count data and store into the buffer;
 */
static VOID
unc_pci_Read_PMU_Data(
    PVOID           param,
    U32             dev_idx
)
{
    U32             j                   = 0;
    U32             this_cpu;
    U64            *buffer              = (U64 *)param;
    CPU_STATE       pcpu;
    U32             cur_grp;
    ECB             pecb;
    U32             dev_node;
    U32             package_num         = 0;
    U32             busno;

    SEP_DRV_LOG_TRACE_IN("Param: %p.", param);

    this_cpu    = CONTROL_THIS_CPU();
    pcpu        = &pcb[this_cpu];
    package_num = core_to_package_map[this_cpu];
    cur_grp     = LWPMU_DEVICE_cur_group(&devices[(dev_idx)])[package_num];
    pecb        = LWPMU_DEVICE_PMU_register_data(&devices[(dev_idx)])[cur_grp];
    dev_node    = LWPMU_DEVICE_pci_dev_node_index(&devices[dev_idx]);

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

    if (!IS_BUS_MAP_VALID(dev_node, package_num)) {
        SEP_DRV_LOG_ERROR_TRACE_OUT("No UNC_PCIDEV bus map for %u!", dev_node);
        return;
    }

    busno = GET_BUS_MAP(dev_node, package_num);

    //Read in the counts into temporary buffer
    FOR_EACH_REG_UNC_OPERATION(pecb, dev_idx, idx, PMU_OPERATION_READ) {

        if (ECB_entries_event_scope(pecb,idx) == SYSTEM_EVENT) {
            j = ECB_entries_uncore_buffer_offset_in_system(pecb,idx);
        }
        else {
            j = EMON_BUFFER_UNCORE_PACKAGE_EVENT_OFFSET(package_num, EMON_BUFFER_DRIVER_HELPER_num_entries_per_package(emon_buffer_driver_helper),
                                                        ECB_entries_uncore_buffer_offset_in_package(pecb,idx));
        }

        buffer[j] = PCI_Read_U64(busno,
                                 ECB_entries_dev_no(pecb,idx),
                                 ECB_entries_func_no(pecb,idx),
                                 ECB_entries_reg_id(pecb,idx));

        SEP_DRV_LOG_TRACE("j=%u, value=%llu, cpu=%u", j, buffer[j], this_cpu);

    } END_FOR_EACH_REG_UNC_OPERATION;

    SEP_DRV_LOG_TRACE_OUT("");
    return;
}

/*
 * Initialize the dispatch table
 */
DISPATCH_NODE  unc_pci_dispatch =
{
    NULL,                                // initialize
    NULL,                                // destroy
    unc_pci_Write_PMU,                   // write
    unc_pci_Disable_PMU,                 // freeze
    unc_pci_Enable_PMU,                  // restart
    unc_pci_Read_PMU_Data,               // read
    NULL,                                // check for overflow
    NULL,                                // swap group
    NULL,                                // read lbrs
    NULL,                                // cleanup
    NULL,                                // hw errata
    NULL,                                // read power
    NULL,                                // check overflow errata
    NULL,                                // read counts
    NULL,                                // check overflow gp errata
    NULL,                                // read_ro
    NULL,                                // platform info
    unc_pci_Trigger_Read,                // trigger read
    NULL,                                // scan for uncore
    NULL                                 // read metrics
};


