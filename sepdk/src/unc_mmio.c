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
extern U64                        *prev_counter_data;
extern DRV_CONFIG                  drv_cfg;
extern EMON_BUFFER_DRIVER_HELPER   emon_buffer_driver_helper;

#define MASK_32BIT             0xffffffff
#define MASK_64BIT             0xffffffff00000000ULL

#define IS_MASTER(device_type, cpu)               (((device_type) == DRV_SINGLE_INSTANCE)? CPU_STATE_system_master(&pcb[cpu]): CPU_STATE_socket_master(&pcb[(cpu)]))
#define GET_PACKAGE_NUM(device_type, cpu)         (((device_type) == DRV_SINGLE_INSTANCE)? 0:core_to_package_map[cpu])
#define IS_64BIT(mask)                            (((mask)>>32) != 0)

#define EVENT_COUNTER_MAX_TRY  30

struct FPGA_CONTROL_NODE_S {
    union {
        struct {
            U64 rst_ctrs         : 1;
            U64 rsvd1            : 7;
            U64 frz              : 1;
            U64 rsvd2            : 7;
            U64 event_select     : 4;
            U64 port_id          : 2;
            U64 rsvd3            : 1;
            U64 port_enable      : 1;
            U64 rsvd4            : 40;
        } bits;
        U64 bit_field;
    } u;
} control_node;


/*!
 * @fn          static VOID unc_mmio_single_bar_Write_PMU(VOID*)
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
unc_mmio_single_bar_Write_PMU (
    VOID  *param
)
{
    U32              dev_idx          = 0;
    U32              offset_delta     = 0;
    U32              event_id         = 0;
    U32              this_cpu         = 0;
    U32              package_num      = 0;
    U32              cur_grp          = 0;
    U32              idx_w            = 0;
    U32              event_code       = 0;
    U32              counter          = 0;
    U32              entry            = 0;
    U32              dev_node         = 0;
    U64              tmp_value        = 0;
    U64              virtual_addr     = 0;
    ECB              pecb;
    DEV_UNC_CONFIG   pcfg_unc;
    MMIO_BAR_INFO    mmio_bar_info;

    SEP_DRV_LOG_TRACE_IN("Param: %p.", param);

    dev_idx      = *((U32*)param);
    this_cpu     = CONTROL_THIS_CPU();
    pcfg_unc     = (DEV_UNC_CONFIG)LWPMU_DEVICE_pcfg(&devices[dev_idx]);
    if (!IS_MASTER(DEV_UNC_CONFIG_device_type(pcfg_unc), this_cpu)) {
        SEP_DRV_LOG_TRACE_OUT("Early exit (!is_master).");
        return;
    }
    package_num  = GET_PACKAGE_NUM(DEV_UNC_CONFIG_device_type(pcfg_unc), this_cpu);
    cur_grp      = LWPMU_DEVICE_cur_group(&devices[(dev_idx)])[package_num];
    pecb         = LWPMU_DEVICE_PMU_register_data(&devices[(dev_idx)])[(cur_grp)];
    if (!pecb) {
        SEP_DRV_LOG_TRACE_OUT("Early exit (!pecb).");
        return;
    }

    dev_node     = ECB_dev_node(pecb);
    entry        = package_num;

    if (!IS_MMIO_MAP_VALID(dev_node, entry)) {
        SEP_DRV_LOG_TRACE_OUT("Early exit (!IS_MMIO_MAP_VALID).");
        return;
    }

    virtual_addr = virtual_address_table(dev_node, entry);

    FOR_EACH_REG_UNC_OPERATION(pecb, dev_idx, idx, PMU_OPERATION_WRITE) {

        PCI_MMIO_Write_U64(virtual_addr, ECB_entries_reg_id(pecb, idx), ECB_entries_reg_value(pecb, idx));
        SEP_DRV_LOG_TRACE("va=0x%llx, ri=%u, rv=0x%llx", virtual_addr, ECB_entries_reg_id(pecb, idx), ECB_entries_reg_value(pecb, idx));

    } END_FOR_EACH_REG_UNC_OPERATION;


    if (DRV_CONFIG_emon_mode(drv_cfg)) {
        SEP_DRV_LOG_TRACE_OUT("Early exit (!event_based_counts).");
        return;
    }

    idx_w = ECB_operations_register_start(pecb, PMU_OPERATION_WRITE);
    FOR_EACH_REG_UNC_OPERATION(pecb, dev_idx, idx, PMU_OPERATION_READ) {

        mmio_bar_info = &ECB_mmio_bar_list(pecb, 0);
        if (!mmio_bar_info) {
            SEP_DRV_LOG_TRACE_OUT("Early exit (!mmio_bar_info).");
            break;
        }

        if (ECB_entries_reg_offset(pecb, idx) >= MMIO_BAR_INFO_base_offset_for_mmio(mmio_bar_info)) {
            offset_delta = ECB_entries_reg_offset(pecb, idx) - (U32)MMIO_BAR_INFO_base_offset_for_mmio(mmio_bar_info);
        }
        else {
            offset_delta = ECB_entries_reg_offset(pecb, idx);
        }
        SEP_DRV_LOG_TRACE("od=0x%x", offset_delta);

        if ((DEV_UNC_CONFIG_device_type(pcfg_unc) == DRV_SINGLE_INSTANCE)  &&
            (GET_NUM_MAP_ENTRIES(dev_node) > 1))   {
            // multiple MMIO mapping per <dev_no, func_no> device, find virtual_addr per mapping.
            entry = ECB_entries_unit_id(pecb, idx);
            virtual_addr = virtual_address_table(dev_node, entry);
        }

        if ((ECB_entries_counter_type(pecb, idx) == PROG_FREERUN_COUNTER) &&
            (ECB_entries_unit_id(pecb, idx) == 0)) {
            //Write event code before reading
            PCI_MMIO_Write_U64(virtual_addr, ECB_entries_reg_id(pecb, idx_w), ECB_entries_reg_value(pecb, idx_w));
            event_code = (U32)control_node.u.bits.event_select;
            idx_w++;
        }

        // this is needed for overflow detection of the accumulators.
        if (IS_64BIT((U64)(ECB_entries_max_bits(pecb, idx)))) {
            if (ECB_entries_counter_type(pecb, idx) == PROG_FREERUN_COUNTER) {
                do {
                    if (counter > EVENT_COUNTER_MAX_TRY) {
                        break;
                    }
                    tmp_value = PCI_MMIO_Read_U64(virtual_addr, offset_delta);
                    counter++;
                } while (event_code != (tmp_value >>60));
            }
            tmp_value = PCI_MMIO_Read_U64(virtual_addr, offset_delta);
        }
        else {
            tmp_value = PCI_MMIO_Read_U32(virtual_addr, offset_delta);
        }
        tmp_value &= (U64)ECB_entries_max_bits(pecb, idx);

        LWPMU_DEVICE_prev_value(&devices[dev_idx])[package_num][event_id] = tmp_value;

        SEP_DRV_LOG_TRACE("cpu=%u, device=%u, package=%u, entry=%u, event_id=%u, value=0x%llu",
                      this_cpu, dev_idx, package_num, entry, event_id, tmp_value);
        event_id++;

        if (LWPMU_DEVICE_counter_mask(&devices[dev_idx]) == 0) {
            LWPMU_DEVICE_counter_mask(&devices[dev_idx]) = (U64)ECB_entries_max_bits(pecb, idx);
        }
    } END_FOR_EACH_REG_UNC_OPERATION;

    SEP_DRV_LOG_TRACE_OUT("");
    return;
}

/*!
 * @fn          static VOID unc_mmio_multiple_bar_Write_PMU(VOID*)
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
unc_mmio_multiple_bar_Write_PMU (
    VOID  *param
)
{
    U32              dev_idx                = 0;
    U32              event_id               = 0;
    U32              this_cpu               = 0;
    U32              package_num            = 0;
    U32              cur_grp                = 0;
    U32              dev_node               = 0;
    U32              entry                  = 0;
    U32              num_mmio_secondary_bar = 0;
    U64              tmp_value              = 0;
    U64              virtual_addr           = 0;
    ECB              pecb;
    DEV_UNC_CONFIG   pcfg_unc;

    SEP_DRV_LOG_TRACE_IN("Param: %p.", param);

    dev_idx      = *((U32*)param);
    this_cpu     = CONTROL_THIS_CPU();
    pcfg_unc     = (DEV_UNC_CONFIG)LWPMU_DEVICE_pcfg(&devices[dev_idx]);
    if (!IS_MASTER(DEV_UNC_CONFIG_device_type(pcfg_unc), this_cpu)) {
        SEP_DRV_LOG_TRACE_OUT("Early exit (!is_master).");
        return;
    }
    package_num  = GET_PACKAGE_NUM(DEV_UNC_CONFIG_device_type(pcfg_unc), this_cpu);
    cur_grp      = LWPMU_DEVICE_cur_group(&devices[(dev_idx)])[package_num];
    pecb         = LWPMU_DEVICE_PMU_register_data(&devices[(dev_idx)])[(cur_grp)];
    if (!pecb) {
        SEP_DRV_LOG_TRACE_OUT("Early exit (!pecb).");
        return;
    }

    dev_node               = ECB_dev_node(pecb);
    entry                  = package_num;
    num_mmio_secondary_bar = GET_NUM_MMIO_SECONDARY_BAR(dev_node);

    FOR_EACH_SCHEDULED_REG_UNC_OPERATION(pecb, dev_idx, idx, PMU_OPERATION_WRITE) {

        if (!IS_MMIO_MAP_VALID(dev_node, entry * num_mmio_secondary_bar + ECB_entries_reg_bar_index(pecb, idx))) {
            SEP_DRV_LOG_TRACE_OUT("Early exit (!IS_MMIO_MAP_VALID).");
            continue;
        }

        virtual_addr = virtual_address_table(dev_node, entry * num_mmio_secondary_bar + ECB_entries_reg_bar_index(pecb, idx));
        SEP_DRV_LOG_TRACE("e=%u, bi=%u, va=0x%llx", entry, ECB_entries_reg_bar_index(pecb, idx), virtual_addr);

        if (ECB_entries_reg_type(pecb, idx)       == PMU_REG_GLOBAL_CTRL &&
            ECB_entries_reg_prog_type(pecb, idx)  == PMU_REG_PROG_MSR) {
            SYS_Write_MSR(ECB_entries_reg_id(pecb, idx), 0LL);
            continue;
        }

        SEP_DRV_LOG_TRACE("va=0x%llx, ro=0x%x, rv=0x%x", virtual_addr, ECB_entries_reg_offset(pecb, idx), ECB_entries_reg_value(pecb, idx));
        PCI_MMIO_Write_U32(virtual_addr, ECB_entries_reg_offset(pecb, idx), (U32)ECB_entries_reg_value(pecb, idx));

    } END_FOR_EACH_SCHEDULED_REG_UNC_OPERATION;


    if (DRV_CONFIG_emon_mode(drv_cfg)) {
        SEP_DRV_LOG_TRACE_OUT("Early exit (!event_based_counts).");
        return;
    }

    FOR_EACH_SCHEDULED_REG_UNC_OPERATION(pecb, dev_idx, idx, PMU_OPERATION_READ) {

        if (!IS_MMIO_MAP_VALID(dev_node, entry * num_mmio_secondary_bar + ECB_entries_reg_bar_index(pecb, idx))) {
            SEP_DRV_LOG_TRACE_OUT("Early exit (!IS_MMIO_MAP_VALID).");
            continue;
        }

        virtual_addr = virtual_address_table(dev_node, entry * num_mmio_secondary_bar + ECB_entries_reg_bar_index(pecb, idx));
        SEP_DRV_LOG_TRACE("e=%u, bi=%u, va=0x%llx", entry, ECB_entries_reg_bar_index(pecb, idx), virtual_addr);

        // this is needed for overflow detection of the accumulators.
        if (IS_64BIT((U64)(ECB_entries_max_bits(pecb, idx)))) {
            tmp_value = PCI_MMIO_Read_U64(virtual_addr, ECB_entries_reg_offset(pecb, idx));
        }
        else {
            tmp_value = PCI_MMIO_Read_U32(virtual_addr, ECB_entries_reg_offset(pecb, idx));
        }
        tmp_value &= (U64)ECB_entries_max_bits(pecb, idx);

        LWPMU_DEVICE_prev_value(&devices[dev_idx])[package_num][event_id] = tmp_value;

        SEP_DRV_LOG_TRACE("cpu=%u, device=%u, package=%u, event_id=%u, value=0x%llu",
                      this_cpu, dev_idx, package_num, event_id, tmp_value);
        event_id++;

        if (LWPMU_DEVICE_counter_mask(&devices[dev_idx]) == 0) {
            LWPMU_DEVICE_counter_mask(&devices[dev_idx]) = (U64)ECB_entries_max_bits(pecb, idx);
        }
    } END_FOR_EACH_SCHEDULED_REG_UNC_OPERATION;

    SEP_DRV_LOG_TRACE_OUT("");
    return;
}

/*!
 * @fn         static VOID unc_mmio_single_bar_Enable_PMU(PVOID)
 *
 * @brief      Capture the previous values to calculate delta later.
 *
 * @param      None
 *
 * @return     None
 *
 * <I>Special Notes:</I>
 */
static void
unc_mmio_single_bar_Enable_PMU (
    PVOID  param
)
{
    U32            j                = 0;
    U32            this_cpu         = 0;
    U32            dev_idx          = 0;
    U32            package_num      = 0;
    U32            offset_delta     = 0;
    U32            cur_grp          = 0;
    U32            idx_w            = 0;
    U32            event_code       = 0;
    U32            counter          = 0;
    U32            num_events       = 0;
    U32            entry            = 0;
    U32            num_pkgs         = num_packages;
    U32            dev_node         = 0;
    U64            virtual_addr     = 0;
    U64            reg_val          = 0;
    U64           *buffer           = prev_counter_data;
    ECB            pecb;
    DEV_UNC_CONFIG pcfg_unc;
    MMIO_BAR_INFO  mmio_bar_info;

    SEP_DRV_LOG_TRACE_IN("Param: %p.", param);

    dev_idx     = *((U32*)param);
    this_cpu    = CONTROL_THIS_CPU();
    pcfg_unc    = (DEV_UNC_CONFIG)LWPMU_DEVICE_pcfg(&devices[dev_idx]);
    if (!IS_MASTER(DEV_UNC_CONFIG_device_type(pcfg_unc), this_cpu)) {
        SEP_DRV_LOG_TRACE_OUT("Early exit (!IS_MASTER).");
        return;
    }

    package_num = GET_PACKAGE_NUM(DEV_UNC_CONFIG_device_type(pcfg_unc), this_cpu);
    cur_grp     = LWPMU_DEVICE_cur_group(&devices[(dev_idx)])[package_num];
    pecb        = LWPMU_DEVICE_PMU_register_data(&devices[(dev_idx)])[(cur_grp)];
    if (!pecb) {
        SEP_DRV_LOG_TRACE_OUT("Early exit (!pecb) for group %u.", cur_grp);
        return;
    }
    SEP_DRV_LOG_TRACE("enable PMU for group %u", cur_grp);
    dev_node    = ECB_dev_node(pecb);
    entry       = package_num;

    if (DEV_UNC_CONFIG_device_type(pcfg_unc) == DRV_SINGLE_INSTANCE) {
        num_pkgs    = 1;
    }

    if (!IS_MMIO_MAP_VALID(dev_node, entry)) {
        SEP_DRV_LOG_TRACE_OUT("Early exit (!IS_MMIO_MAP_VALID).");
        return;
    }

    virtual_addr = virtual_address_table(dev_node, entry);

    // NOTE THAT the enable function currently captures previous values
    // for EMON collection to avoid unnecessary memory copy.
    // capturing previous values enable freerunning counter delta computation
    if (DRV_CONFIG_emon_mode(drv_cfg)) {
        num_events  = ECB_num_events(pecb);
        idx_w = ECB_operations_register_start(pecb, PMU_OPERATION_WRITE);
        FOR_EACH_REG_UNC_OPERATION(pecb, dev_idx, idx, PMU_OPERATION_READ) {

            mmio_bar_info = &ECB_mmio_bar_list(pecb, 0);
            if (!mmio_bar_info) {
                SEP_DRV_LOG_TRACE_OUT("Early exit (!mmio_bar_info).");
                break;
            }

            if (ECB_entries_reg_offset(pecb, idx) >= MMIO_BAR_INFO_base_offset_for_mmio(mmio_bar_info)) {
                offset_delta = ECB_entries_reg_offset(pecb, idx) - (U32)MMIO_BAR_INFO_base_offset_for_mmio(mmio_bar_info);
            }
            else {
                offset_delta = ECB_entries_reg_offset(pecb, idx);
            }

            if ((DEV_UNC_CONFIG_device_type(pcfg_unc) == DRV_SINGLE_INSTANCE)  &&
                (GET_NUM_MAP_ENTRIES(dev_node) > 1))   {
                // multiple MMIO mapping per <dev_no, func_no> device, find virtual_addr per mapping.
                entry = ECB_entries_unit_id(pecb, idx);
                virtual_addr = virtual_address_table(dev_node, entry);
            }

            if ((ECB_entries_counter_type(pecb, idx) == PROG_FREERUN_COUNTER) &&
                (ECB_entries_unit_id(pecb, idx) == 0)) {
                PCI_MMIO_Write_U64(virtual_addr, ECB_entries_reg_id(pecb, idx_w), ECB_entries_reg_value(pecb, idx_w));
                control_node.u.bit_field = ECB_entries_reg_value(pecb, idx_w);
                event_code = (U32)control_node.u.bits.event_select;
                idx_w++;
            }

            if ((ECB_entries_event_scope(pecb, idx) == PACKAGE_EVENT) ||
                (ECB_entries_event_scope(pecb, idx) == SYSTEM_EVENT)) {

                if (ECB_entries_event_scope(pecb, idx) == SYSTEM_EVENT) {
                    j = ECB_entries_uncore_buffer_offset_in_system(pecb, idx);
                }
                else {
                    j = EMON_BUFFER_UNCORE_PACKAGE_EVENT_OFFSET(package_num, EMON_BUFFER_DRIVER_HELPER_num_entries_per_package(emon_buffer_driver_helper),
                                                                ECB_entries_uncore_buffer_offset_in_package(pecb, idx));
                }

                if (IS_64BIT((U64)(ECB_entries_max_bits(pecb, idx)))) {
                    if (ECB_entries_counter_type(pecb, idx) == PROG_FREERUN_COUNTER) {
                        do {
                            if (counter > EVENT_COUNTER_MAX_TRY) {
                                break;
                            }
                            buffer[j] = PCI_MMIO_Read_U64(virtual_addr, offset_delta);
                            counter++;
                        } while (event_code != (buffer[j] >>60));
                    }
                    buffer[j] = PCI_MMIO_Read_U64(virtual_addr, offset_delta);

                }
                else {
                    buffer[j] = PCI_MMIO_Read_U32(virtual_addr, offset_delta);
                }
                buffer[j] &= (U64)ECB_entries_max_bits(pecb, idx);
            }
        } END_FOR_EACH_REG_UNC_OPERATION;
    }

    FOR_EACH_REG_UNC_OPERATION(pecb, dev_idx, idx, PMU_OPERATION_ENABLE) {

        if (ECB_entries_reg_type(pecb, idx)       == PMU_REG_GLOBAL_CTRL &&
            ECB_entries_reg_prog_type(pecb, idx)  == PMU_REG_PROG_MSR) {
            SYS_Write_MSR(ECB_entries_reg_id(pecb, idx), ECB_entries_reg_value(pecb, idx));
            continue;
        }

        // FIXME: EDRAM uncore
        //        1. control register is GT_PMON_CNT_MISC_CTRL 32bit GTTMMADR+11_7268
        //        2. control register is not for write after read pattern
        if (ECB_entries_reg_rw_type(pecb, idx)  == PMU_REG_RW_WRITE) {
            PCI_MMIO_Write_U32(virtual_addr, ECB_entries_reg_id(pecb,idx), (U32)ECB_entries_reg_value(pecb,idx));
        }

        if (ECB_entries_reg_rw_type(pecb, idx)  == PMU_REG_RW_READ_WRITE) {
            reg_val = PCI_MMIO_Read_U64(virtual_addr, ECB_entries_reg_id(pecb,idx));
            reg_val &= ECB_entries_reg_value(pecb,idx);
            PCI_MMIO_Write_U64(virtual_addr, ECB_entries_reg_id(pecb,idx), reg_val);
        }

    } END_FOR_EACH_REG_UNC_OPERATION;

    SEP_DRV_LOG_TRACE_OUT("");
    return;
}

/*!
 * @fn         static VOID unc_mmio_multiple_bar_Enable_PMU(PVOID)
 *
 * @brief      Capture the previous values to calculate delta later.
 *
 * @param      None
 *
 * @return     None
 *
 * <I>Special Notes:</I>
 */
static void
unc_mmio_multiple_bar_Enable_PMU (
    PVOID  param
)
{
    U32            j                      = 0;
    U32            this_cpu               = 0;
    U32            dev_idx                = 0;
    U32            package_num            = 0;
    U32            cur_grp                = 0;
    U32            dev_node               = 0;
    U32            entry                  = 0;
    U32            num_mmio_secondary_bar = 0;
    U64            virtual_addr           = 0;
    U64            reg_val                = 0;
    U64           *buffer                 = prev_counter_data;
    ECB            pecb;
    DEV_UNC_CONFIG pcfg_unc;

    SEP_DRV_LOG_TRACE_IN("Param: %p.", param);

    dev_idx     = *((U32*)param);
    this_cpu    = CONTROL_THIS_CPU();
    pcfg_unc    = (DEV_UNC_CONFIG)LWPMU_DEVICE_pcfg(&devices[dev_idx]);
    if (!IS_MASTER(DEV_UNC_CONFIG_device_type(pcfg_unc), this_cpu)) {
        SEP_DRV_LOG_TRACE_OUT("Early exit (!IS_MASTER).");
        return;
    }

    package_num = GET_PACKAGE_NUM(DEV_UNC_CONFIG_device_type(pcfg_unc), this_cpu);
    cur_grp     = LWPMU_DEVICE_cur_group(&devices[(dev_idx)])[package_num];
    pecb        = LWPMU_DEVICE_PMU_register_data(&devices[(dev_idx)])[(cur_grp)];
    if (!pecb) {
        SEP_DRV_LOG_TRACE_OUT("Early exit (!pecb) for group %u.", cur_grp);
        return;
    }
    SEP_DRV_LOG_TRACE("enable PMU for group %u", cur_grp);

    dev_node               = ECB_dev_node(pecb);
    entry                  = package_num;
    num_mmio_secondary_bar = GET_NUM_MMIO_SECONDARY_BAR(dev_node);

    // NOTE THAT the enable function currently captures previous values
    // for EMON collection to avoid unnecessary memory copy.
    // capturing previous values enable freerunning counter delta computation
    if (DRV_CONFIG_emon_mode(drv_cfg)) {
        FOR_EACH_SCHEDULED_REG_UNC_OPERATION(pecb, dev_idx, idx, PMU_OPERATION_READ) {

            if (!IS_MMIO_MAP_VALID(dev_node, entry * num_mmio_secondary_bar + ECB_entries_reg_bar_index(pecb, idx))) {
                SEP_DRV_LOG_TRACE_OUT("Early exit (!IS_MMIO_MAP_VALID).");
                continue;
            }

            virtual_addr = virtual_address_table(dev_node, entry * num_mmio_secondary_bar + ECB_entries_reg_bar_index(pecb, idx));
            SEP_DRV_LOG_TRACE("e=%u,bi=%u,va=0x%llx", entry, ECB_entries_reg_bar_index(pecb, idx), virtual_addr);

            j = EMON_BUFFER_UNCORE_PACKAGE_EVENT_OFFSET(package_num, EMON_BUFFER_DRIVER_HELPER_num_entries_per_package(emon_buffer_driver_helper),
                                                        ECB_entries_uncore_buffer_offset_in_package(pecb, idx));

            if (IS_64BIT((U64)(ECB_entries_max_bits(pecb, idx)))) {
                buffer[j] = PCI_MMIO_Read_U64(virtual_addr, ECB_entries_reg_offset(pecb, idx));
            }
            else {
                buffer[j] = PCI_MMIO_Read_U32(virtual_addr, ECB_entries_reg_offset(pecb, idx));
            }
            buffer[j] &= (U64)ECB_entries_max_bits(pecb, idx);

        } END_FOR_EACH_SCHEDULED_REG_UNC_OPERATION;
    }

    FOR_EACH_SCHEDULED_REG_UNC_OPERATION(pecb, dev_idx, idx, PMU_OPERATION_ENABLE) {

        if (ECB_entries_reg_type(pecb, idx)       == PMU_REG_GLOBAL_CTRL &&
            ECB_entries_reg_prog_type(pecb, idx)  == PMU_REG_PROG_MSR) {
            SYS_Write_MSR(ECB_entries_reg_id(pecb, idx), ECB_entries_reg_value(pecb, idx));
            continue;
        }

        if (!IS_MMIO_MAP_VALID(dev_node, entry * num_mmio_secondary_bar + ECB_entries_reg_bar_index(pecb, idx))) {
            SEP_DRV_LOG_TRACE_OUT("Early exit (!IS_MMIO_MAP_VALID).");
            continue;
        }

        virtual_addr = virtual_address_table(dev_node, entry * num_mmio_secondary_bar + ECB_entries_reg_bar_index(pecb, idx));
        SEP_DRV_LOG_TRACE("e=%u, bi=%u, va=0x%llx", entry, ECB_entries_reg_bar_index(pecb, idx), virtual_addr);

        if (ECB_entries_reg_rw_type(pecb, idx)  == PMU_REG_RW_WRITE) {
            PCI_MMIO_Write_U32(virtual_addr, ECB_entries_reg_offset(pecb, idx), (U32)ECB_entries_reg_value(pecb, idx));
        }

        if (ECB_entries_reg_rw_type(pecb, idx)  == PMU_REG_RW_READ_WRITE) {
            reg_val = PCI_MMIO_Read_U32(virtual_addr, ECB_entries_reg_offset(pecb, idx));
            reg_val &= ECB_entries_reg_value(pecb, idx);
            SEP_DRV_LOG_TRACE("va=0x%llx, ri=0x%x, ro=0x%x, rv=0x%x", virtual_addr, ECB_entries_reg_id(pecb, idx), ECB_entries_reg_offset(pecb, idx), reg_val);
            PCI_MMIO_Write_U32(virtual_addr, ECB_entries_reg_offset(pecb, idx), (U32)reg_val);
        }
    } END_FOR_EACH_SCHEDULED_REG_UNC_OPERATION;

    SEP_DRV_LOG_TRACE_OUT("");
    return;
}

/*!
 * @fn         static VOID unc_mmio_Disable_PMU(PVOID)
 *
 * @brief      Unmap the virtual address when you stop sampling.
 *
 * @param      None
 *
 * @return     None
 *
 * <I>Special Notes:</I>
 */
static void
unc_mmio_single_bar_Disable_PMU (
    PVOID  param
)
{
    U32            dev_idx          = 0;
    U32            this_cpu         = 0;
    U32            package_num      = 0;
    U32            cur_grp          = 0;
    U32            dev_node         = 0;
    U32            entry            = 0;
    U64            virtual_addr     = 0;
    U64            reg_val          = 0;
    ECB            pecb;
    DEV_UNC_CONFIG pcfg_unc;

    SEP_DRV_LOG_TRACE_IN("Param: %p.", param);

    dev_idx     = *((U32*)param);
    this_cpu    = CONTROL_THIS_CPU();
    pcfg_unc    = (DEV_UNC_CONFIG)LWPMU_DEVICE_pcfg(&devices[dev_idx]);
    if (!IS_MASTER(DEV_UNC_CONFIG_device_type(pcfg_unc), this_cpu)) {
        SEP_DRV_LOG_TRACE_OUT("Early exit (!IS_MASTER).");
        return;
    }

    package_num = GET_PACKAGE_NUM(DEV_UNC_CONFIG_device_type(pcfg_unc), this_cpu);
    cur_grp     = LWPMU_DEVICE_cur_group(&devices[dev_idx])[package_num];
    pecb        = LWPMU_DEVICE_PMU_register_data(&devices[dev_idx])[(cur_grp)];
    if (!pecb) {
        SEP_DRV_LOG_TRACE_OUT("Early exit (!pecb).");
        return;
    }

    dev_node     = ECB_dev_node(pecb);
    entry        = package_num;

    if (!IS_MMIO_MAP_VALID(dev_node, entry)) {
        SEP_DRV_LOG_TRACE_OUT("Early exit (!IS_MMIO_MAP_VALID).");
        return;
    }

    virtual_addr = virtual_address_table(dev_node, entry);

    FOR_EACH_REG_UNC_OPERATION(pecb, dev_idx, idx, PMU_OPERATION_DISABLE) {

        if (ECB_entries_reg_type(pecb, idx)       == PMU_REG_GLOBAL_CTRL &&
            ECB_entries_reg_prog_type(pecb, idx)  == PMU_REG_PROG_MSR) {
            continue;
        }

        // FIXME: EDRAM uncore
        //        1. control register is GT_PMON_CNT_MISC_CTRL 32bit GTTMMADR+11_7268
        //        2. control register is not for write after read pattern
        if (ECB_entries_reg_rw_type(pecb, idx)  == PMU_REG_RW_WRITE) {
            PCI_MMIO_Write_U32(virtual_addr, ECB_entries_reg_id(pecb,idx), (U32)ECB_entries_reg_value(pecb,idx));
        }

        if (ECB_entries_reg_rw_type(pecb, idx)  == PMU_REG_RW_READ_WRITE) {
            reg_val = PCI_MMIO_Read_U64(virtual_addr, ECB_entries_reg_id(pecb,idx));
            reg_val |= ECB_entries_reg_value(pecb,idx);
            PCI_MMIO_Write_U64(virtual_addr, ECB_entries_reg_id(pecb,idx), reg_val);
        }
    } END_FOR_EACH_REG_UNC_OPERATION;

    SEP_DRV_LOG_TRACE_OUT("");
    return;
}

/*!
 * @fn         static VOID unc_mmio_multiple_bar_Disable_PMU(PVOID)
 *
 * @brief      Unmap the virtual address when you stop sampling.
 *
 * @param      None
 *
 * @return     None
 *
 * <I>Special Notes:</I>
 */
static void
unc_mmio_multiple_bar_Disable_PMU (
    PVOID  param
)
{
    U32            dev_idx                = 0;
    U32            this_cpu               = 0;
    U32            package_num            = 0;
    U32            cur_grp                = 0;
    U32            dev_node               = 0;
    U32            entry                  = 0;
    U32            num_mmio_secondary_bar = 0;
    U64            virtual_addr           = 0;
    U64            reg_val                = 0;
    ECB            pecb;
    DEV_UNC_CONFIG pcfg_unc;

    SEP_DRV_LOG_TRACE_IN("Param: %p.", param);

    dev_idx     = *((U32*)param);
    this_cpu    = CONTROL_THIS_CPU();
    pcfg_unc    = (DEV_UNC_CONFIG)LWPMU_DEVICE_pcfg(&devices[dev_idx]);
    if (!IS_MASTER(DEV_UNC_CONFIG_device_type(pcfg_unc), this_cpu)) {
        SEP_DRV_LOG_TRACE_OUT("Early exit (!IS_MASTER).");
        return;
    }

    package_num = GET_PACKAGE_NUM(DEV_UNC_CONFIG_device_type(pcfg_unc), this_cpu);
    cur_grp     = LWPMU_DEVICE_cur_group(&devices[dev_idx])[package_num];
    pecb        = LWPMU_DEVICE_PMU_register_data(&devices[dev_idx])[(cur_grp)];
    if (!pecb) {
        SEP_DRV_LOG_TRACE_OUT("Early exit (!pecb).");
        return;
    }

    dev_node               = ECB_dev_node(pecb);
    entry                  = package_num;
    num_mmio_secondary_bar = GET_NUM_MMIO_SECONDARY_BAR(dev_node);

    FOR_EACH_SCHEDULED_REG_UNC_OPERATION(pecb, dev_idx, idx, PMU_OPERATION_DISABLE) {

        if (ECB_entries_reg_type(pecb, idx)       == PMU_REG_GLOBAL_CTRL &&
            ECB_entries_reg_prog_type(pecb, idx)  == PMU_REG_PROG_MSR) {
            continue;
        }

        if (!IS_MMIO_MAP_VALID(dev_node, entry * num_mmio_secondary_bar + ECB_entries_reg_bar_index(pecb, idx))) {
            SEP_DRV_LOG_TRACE_OUT("Early exit (!IS_MMIO_MAP_VALID).");
            continue;
        }

        virtual_addr = virtual_address_table(dev_node, entry * num_mmio_secondary_bar + ECB_entries_reg_bar_index(pecb, idx));
        SEP_DRV_LOG_TRACE("e=%u, bi=%u, va=0x%llx", entry, ECB_entries_reg_bar_index(pecb, idx), virtual_addr);

        if (ECB_entries_reg_rw_type(pecb, idx)  == PMU_REG_RW_WRITE) {
            PCI_MMIO_Write_U32(virtual_addr, ECB_entries_reg_offset(pecb, idx), (U32)ECB_entries_reg_value(pecb, idx));
        }

        if (ECB_entries_reg_rw_type(pecb, idx)  == PMU_REG_RW_READ_WRITE) {
            reg_val = PCI_MMIO_Read_U32(virtual_addr, ECB_entries_reg_offset(pecb, idx));
            reg_val |= ECB_entries_reg_value(pecb, idx);
            PCI_MMIO_Write_U32(virtual_addr, ECB_entries_reg_offset(pecb, idx), (U32)reg_val);
        }
    } END_FOR_EACH_SCHEDULED_REG_UNC_OPERATION;

    SEP_DRV_LOG_TRACE_OUT("");
    return;
}

/* ------------------------------------------------------------------------- */
/*!
 * @fn       void unc_mmio_single_bar_Trigger_Read(param, id, read_from_intr)
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
unc_mmio_single_bar_Trigger_Read (
    PVOID  param,
    U32    id,
    U32    read_from_intr
)
{
    U32             this_cpu            = 0;
    U32             cur_grp             = 0;
    U32             index               = 0;
    U32             offset_delta        = 0;
    U32             package_num         = 0;
    U32             idx_w               = 0;
    U32             event_code          = 0;
    U32             counter             = 0;
    U32             entry               = 0;
    U32             dev_node            = 0;
    U64             diff                = 0;
    U64             value               = 0ULL;
    U64             virtual_addr        = 0;
    U64            *data;
    ECB             pecb;
    DEV_UNC_CONFIG  pcfg_unc;
    MMIO_BAR_INFO   mmio_bar_info;

    SEP_DRV_LOG_TRACE_IN("Param: %p, id: %u.", param, id);

    this_cpu    = CONTROL_THIS_CPU();
    pcfg_unc    = (DEV_UNC_CONFIG)LWPMU_DEVICE_pcfg(&devices[id]);

    package_num = GET_PACKAGE_NUM(DEV_UNC_CONFIG_device_type(pcfg_unc), this_cpu);
    cur_grp     = LWPMU_DEVICE_cur_group(&devices[id])[package_num];
    pecb        = LWPMU_DEVICE_PMU_register_data(&devices[id])[(cur_grp)];
    if (!pecb) {
        SEP_DRV_LOG_TRACE_OUT("Early exit (!pecb).");
        return;
    }

    dev_node    = ECB_dev_node(pecb);
    entry       = package_num;

    if (!IS_MMIO_MAP_VALID(dev_node, entry)) {
        SEP_DRV_LOG_TRACE_OUT("Early exit (!IS_MMIO_MAP_VALID).");
        return;
    }

    virtual_addr = virtual_address_table(dev_node, entry);

    //Read in the counts into temporary buffer
    idx_w = ECB_operations_register_start(pecb, PMU_OPERATION_WRITE);
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

        mmio_bar_info = &ECB_mmio_bar_list(pecb, 0);

        if (!mmio_bar_info) {
            SEP_DRV_LOG_TRACE_OUT("Early exit (!mmio_bar_info).");
            break;
        }

        if (ECB_entries_reg_offset(pecb, idx) >= MMIO_BAR_INFO_base_offset_for_mmio(mmio_bar_info)) {
            offset_delta = (U32)(ECB_entries_reg_offset(pecb, idx) - MMIO_BAR_INFO_base_offset_for_mmio(mmio_bar_info));
        }
        else {
            offset_delta = ECB_entries_reg_offset(pecb, idx);
        }

        if ((DEV_UNC_CONFIG_device_type(pcfg_unc) == DRV_SINGLE_INSTANCE)  &&
            (GET_NUM_MAP_ENTRIES(dev_node) > 1))   {
            // multiple MMIO mapping per <dev_no, func_no> device
            entry = ECB_entries_unit_id(pecb, idx);
            virtual_addr = virtual_address_table(dev_node, entry);
        }

        if ((ECB_entries_counter_type(pecb, idx) == PROG_FREERUN_COUNTER) &&
            (ECB_entries_unit_id(pecb, idx) == 0)) {
            PCI_MMIO_Write_U64(virtual_addr, ECB_entries_reg_id(pecb, idx_w), ECB_entries_reg_value(pecb, idx_w));
            control_node.u.bit_field = ECB_entries_reg_value(pecb, idx_w);
            event_code = (U32)control_node.u.bits.event_select;
            idx_w++;
        }

        if (IS_64BIT((U64)(ECB_entries_max_bits(pecb, idx)))) {
            if (ECB_entries_counter_type(pecb, idx) == PROG_FREERUN_COUNTER) {
                do {
                    if (counter > EVENT_COUNTER_MAX_TRY) {
                        break;
                    }
                    value = PCI_MMIO_Read_U64(virtual_addr, offset_delta);
                    counter++;
                } while (event_code != (value >>60));
            }
            value = PCI_MMIO_Read_U64(virtual_addr, offset_delta);
        }
        else {
            value = PCI_MMIO_Read_U32(virtual_addr, offset_delta);
        }
        value &= (U64)ECB_entries_max_bits(pecb, idx);

        data = (U64 *)((S8*)param + ECB_entries_counter_event_offset(pecb, idx));
        //check for overflow if not a static counter
        if (ECB_entries_counter_type(pecb, idx) == STATIC_COUNTER) {
            *data = value;
        }
        else {
            if (value < LWPMU_DEVICE_prev_value(&devices[id])[package_num][index]) {
                diff = LWPMU_DEVICE_counter_mask(&devices[id]) - LWPMU_DEVICE_prev_value(&devices[id])[package_num][index];
                diff += value;
            }
            else {
                diff = value - LWPMU_DEVICE_prev_value(&devices[id])[package_num][index];
            }
            LWPMU_DEVICE_acc_value(&devices[id])[package_num][cur_grp][index] += diff;
            LWPMU_DEVICE_prev_value(&devices[id])[package_num][index] = value;
            *data = LWPMU_DEVICE_acc_value(&devices[id])[package_num][cur_grp][index];
        }
        index++;
    } END_FOR_EACH_REG_UNC_OPERATION;

    SEP_DRV_LOG_TRACE_OUT("");
    return;
}

/* ------------------------------------------------------------------------- */
/*!
 * @fn       void unc_mmio_multiple_bar_Trigger_Read(param, id, read_from_intr)
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
unc_mmio_multiple_bar_Trigger_Read (
    PVOID  param,
    U32    id,
    U32    read_from_intr
)
{
    U32             this_cpu               = 0;
    U32             cur_grp                = 0;
    U32             index                  = 0;
    U32             package_num            = 0;
    U32             idx_w                  = 0;
    U32             entry                  = 0;
    U32             dev_node               = 0;
    U32             num_mmio_secondary_bar = 0;
    U64             diff                   = 0;
    U64             value                  = 0ULL;
    U64             virtual_addr           = 0;
    U64            *data;
    ECB             pecb;
    DEV_UNC_CONFIG  pcfg_unc;

    SEP_DRV_LOG_TRACE_IN("Param: %p, id: %u.", param, id);

    this_cpu    = CONTROL_THIS_CPU();
    pcfg_unc    = (DEV_UNC_CONFIG)LWPMU_DEVICE_pcfg(&devices[id]);
    package_num = GET_PACKAGE_NUM(DEV_UNC_CONFIG_device_type(pcfg_unc), this_cpu);
    cur_grp     = LWPMU_DEVICE_cur_group(&devices[id])[package_num];
    pecb        = LWPMU_DEVICE_PMU_register_data(&devices[id])[(cur_grp)];
    if (!pecb) {
        SEP_DRV_LOG_TRACE_OUT("Early exit (!pecb).");
        return;
    }

    dev_node               = ECB_dev_node(pecb);
    entry                  = package_num;
    num_mmio_secondary_bar = GET_NUM_MMIO_SECONDARY_BAR(dev_node);

    //Read in the counts into temporary buffer
    idx_w = ECB_operations_register_start(pecb, PMU_OPERATION_WRITE);
    FOR_EACH_SCHEDULED_REG_UNC_OPERATION(pecb, id, idx, PMU_OPERATION_READ) {
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

        if (!IS_MMIO_MAP_VALID(dev_node, entry * num_mmio_secondary_bar + ECB_entries_reg_bar_index(pecb, idx))) {
            SEP_DRV_LOG_TRACE_OUT("Early exit (!IS_MMIO_MAP_VALID).");
            continue;
        }

        virtual_addr = virtual_address_table(dev_node, entry * num_mmio_secondary_bar + ECB_entries_reg_bar_index(pecb, idx));

        if (IS_64BIT((U64)(ECB_entries_max_bits(pecb, idx)))) {
            value = PCI_MMIO_Read_U64(virtual_addr, ECB_entries_reg_offset(pecb, idx));
        }
        else {
            value = PCI_MMIO_Read_U32(virtual_addr, ECB_entries_reg_offset(pecb, idx));
        }
        value &= (U64)ECB_entries_max_bits(pecb, idx);

        data = (U64 *)((S8*)param + ECB_entries_counter_event_offset(pecb, idx));
        if (value < LWPMU_DEVICE_prev_value(&devices[id])[package_num][index]) {
            diff = LWPMU_DEVICE_counter_mask(&devices[id]) - LWPMU_DEVICE_prev_value(&devices[id])[package_num][index];
            diff += value;
        }
        else {
            diff = value - LWPMU_DEVICE_prev_value(&devices[id])[package_num][index];
        }
        LWPMU_DEVICE_acc_value(&devices[id])[package_num][cur_grp][index] += diff;
        LWPMU_DEVICE_prev_value(&devices[id])[package_num][index] = value;
        *data = LWPMU_DEVICE_acc_value(&devices[id])[package_num][cur_grp][index];
        index++;
    } END_FOR_EACH_SCHEDULED_REG_UNC_OPERATION;

    SEP_DRV_LOG_TRACE_OUT("");
    return;
}

/* ------------------------------------------------------------------------- */
/*!
 * @fn unc_mmio_single_bar_Read_PMU_Data(param)
 *
 * @param    param    dummy parameter which is not used
 *
 * @return   None     No return needed
 *
 * @brief    Read all the data MSR's into a buffer.  Called by the interrupt handler.
 *
 */
static VOID
unc_mmio_single_bar_Read_PMU_Data (
     PVOID   param,
     U32     dev_idx
)
{
    U32            j                = 0;
    U32            offset_delta     = 0;
    U32            cur_grp          = 0;
    U32            idx_w            = 0;
    U32            event_code       = 0;
    U32            counter          = 0;
    U32            num_events       = 0;
    U32            package_num      = 0;
    U32            entry            = 0;
    U32            dev_node         = 0;
    U32            num_pkgs         = num_packages;
    U32            this_cpu         = 0;
    U64            tmp_value        = 0ULL;
    U64            virtual_addr     = 0;
    U64           *buffer           = (U64 *)param;
    U64           *prev_buffer      = prev_counter_data;
    ECB            pecb;
    DEV_UNC_CONFIG pcfg_unc;
    MMIO_BAR_INFO  mmio_bar_info;

    SEP_DRV_LOG_TRACE_IN("Param: %p.", param);

    this_cpu    = CONTROL_THIS_CPU();
    pcfg_unc    = (DEV_UNC_CONFIG)LWPMU_DEVICE_pcfg(&devices[dev_idx]);
    if (!IS_MASTER(DEV_UNC_CONFIG_device_type(pcfg_unc), this_cpu)) {
        SEP_DRV_LOG_TRACE_OUT("Early exit (!IS_MASTER).");
        return;
    }

    package_num = GET_PACKAGE_NUM(DEV_UNC_CONFIG_device_type(pcfg_unc), this_cpu);
    cur_grp     = LWPMU_DEVICE_cur_group(&devices[(dev_idx)])[package_num];
    pecb        = LWPMU_DEVICE_PMU_register_data(&devices[(dev_idx)])[(cur_grp)];
    if (!pecb) {
        SEP_DRV_LOG_TRACE_OUT("Early exit (!pecb).");
        return;
    }

    dev_node    = ECB_dev_node(pecb);
    entry       = package_num;

    if (DEV_UNC_CONFIG_device_type(pcfg_unc) == DRV_SINGLE_INSTANCE) {
        num_pkgs    = 1;
    }

    num_events = ECB_num_events(pecb);

    idx_w = ECB_operations_register_start(pecb, PMU_OPERATION_WRITE);

    if (!IS_MMIO_MAP_VALID(dev_node, entry)) {
        SEP_DRV_LOG_TRACE_OUT("Early exit (!IS_MMIO_MAP_VALID).");
        return;
    }

    virtual_addr = virtual_address_table(dev_node, entry);

    FOR_EACH_REG_UNC_OPERATION(pecb, dev_idx, idx, PMU_OPERATION_READ) {

        mmio_bar_info = &ECB_mmio_bar_list(pecb, 0);
        if (!mmio_bar_info) {
            SEP_DRV_LOG_TRACE_OUT("Early exit (!mmio_bar_info).");
            break;
        }

        if (ECB_entries_reg_offset(pecb, idx) >= MMIO_BAR_INFO_base_offset_for_mmio(mmio_bar_info)) {
            offset_delta = ECB_entries_reg_offset(pecb, idx) - (U32)MMIO_BAR_INFO_base_offset_for_mmio(mmio_bar_info);
        }
        else {
            offset_delta = ECB_entries_reg_offset(pecb, idx);
        }

        if ((DEV_UNC_CONFIG_device_type(pcfg_unc) == DRV_SINGLE_INSTANCE)  &&
            (GET_NUM_MAP_ENTRIES(dev_node) > 1))   {
            // multiple MMIO mapping per <dev_no, func_no> device, find virtual_addr per mapping.
            entry = ECB_entries_unit_id(pecb, idx);
            virtual_addr = virtual_address_table(dev_node, entry);
        }

        if ((ECB_entries_counter_type(pecb, idx) == PROG_FREERUN_COUNTER) &&
            (ECB_entries_unit_id(pecb, idx) == 0)) {
            PCI_MMIO_Write_U64(virtual_addr, ECB_entries_reg_id(pecb, idx_w), ECB_entries_reg_value(pecb, idx_w));
            control_node.u.bit_field = ECB_entries_reg_value(pecb, idx_w);
            event_code = (U32)control_node.u.bits.event_select;
            idx_w++;
        }

        if ((ECB_entries_event_scope(pecb, idx) == PACKAGE_EVENT) ||
            (ECB_entries_event_scope(pecb, idx) == SYSTEM_EVENT)) {

            if (ECB_entries_event_scope(pecb, idx) == SYSTEM_EVENT) {
                j = ECB_entries_uncore_buffer_offset_in_system(pecb, idx);
            }
            else {
                j = EMON_BUFFER_UNCORE_PACKAGE_EVENT_OFFSET(package_num, EMON_BUFFER_DRIVER_HELPER_num_entries_per_package(emon_buffer_driver_helper),
                                                            ECB_entries_uncore_buffer_offset_in_package(pecb, idx));
            }

            if (IS_64BIT((U64)(ECB_entries_max_bits(pecb, idx)))) {
                if (ECB_entries_counter_type(pecb, idx) == PROG_FREERUN_COUNTER) {
                    do {
                        if (counter > EVENT_COUNTER_MAX_TRY) {
                            break;
                        }
                        tmp_value = PCI_MMIO_Read_U64(virtual_addr, offset_delta);
                        counter++;
                    } while (event_code != (tmp_value >>60));
                }
                tmp_value = PCI_MMIO_Read_U64(virtual_addr, offset_delta);
            }
            else {
                tmp_value = PCI_MMIO_Read_U32(virtual_addr, offset_delta);
            }
            tmp_value &= (U64)ECB_entries_max_bits(pecb, idx);
            if (ECB_entries_counter_type(pecb, idx) == STATIC_COUNTER) {
                buffer[j] = tmp_value;
            }
            else {
                if (tmp_value >= prev_buffer[j]) {
                    buffer[j] = tmp_value - prev_buffer[j];
                }
                else {
                    buffer[j] = tmp_value + (ECB_entries_max_bits(pecb, idx) - prev_buffer[j]);
                }
            }
            SEP_DRV_LOG_TRACE("j=%u, v=%llu", j, buffer[j]);
        }
    } END_FOR_EACH_REG_UNC_OPERATION;

    SEP_DRV_LOG_TRACE_OUT("");
    return;
}

/* ------------------------------------------------------------------------- */
/*!
 * @fn unc_mmio_multiple_bar_Read_PMU_Data(param)
 *
 * @param    param    dummy parameter which is not used
 *
 * @return   None     No return needed
 *
 * @brief    Read all the data MSR's into a buffer.  Called by the interrupt handler.
 *
 */
static VOID
unc_mmio_multiple_bar_Read_PMU_Data (
     PVOID   param,
     U32     dev_idx
)
{
    U32            j                      = 0;
    U32            this_cpu               = 0;
    U32            cur_grp                = 0;
    U32            package_num            = 0;
    U32            entry                  = 0;
    U32            dev_node               = 0;
    U32            num_mmio_secondary_bar = 0;
    U64            tmp_value              = 0ULL;
    U64            virtual_addr           = 0;
    U64           *buffer                 = (U64 *)param;
    U64           *prev_buffer            = prev_counter_data;
    ECB            pecb;
    DEV_UNC_CONFIG pcfg_unc;

    SEP_DRV_LOG_TRACE_IN("Param: %p.", param);

    this_cpu    = CONTROL_THIS_CPU();
    pcfg_unc    = (DEV_UNC_CONFIG)LWPMU_DEVICE_pcfg(&devices[dev_idx]);
    if (!IS_MASTER(DEV_UNC_CONFIG_device_type(pcfg_unc), this_cpu)) {
        SEP_DRV_LOG_TRACE_OUT("Early exit (!IS_MASTER).");
        return;
    }

    package_num = GET_PACKAGE_NUM(DEV_UNC_CONFIG_device_type(pcfg_unc), this_cpu);
    cur_grp     = LWPMU_DEVICE_cur_group(&devices[(dev_idx)])[package_num];
    pecb        = LWPMU_DEVICE_PMU_register_data(&devices[(dev_idx)])[(cur_grp)];
    if (!pecb) {
        SEP_DRV_LOG_TRACE_OUT("Early exit (!pecb).");
        return;
    }

    dev_node    = ECB_dev_node(pecb);
    entry       = package_num;
    num_mmio_secondary_bar = GET_NUM_MMIO_SECONDARY_BAR(dev_node);

    FOR_EACH_SCHEDULED_REG_UNC_OPERATION(pecb, dev_idx, idx, PMU_OPERATION_READ) {

        if (!IS_MMIO_MAP_VALID(dev_node, entry * num_mmio_secondary_bar + ECB_entries_reg_bar_index(pecb, idx))) {
            SEP_DRV_LOG_TRACE_OUT("Early exit (!IS_MMIO_MAP_VALID).");
            continue;
        }

        virtual_addr = virtual_address_table(dev_node, entry * num_mmio_secondary_bar + ECB_entries_reg_bar_index(pecb, idx));

        j = EMON_BUFFER_UNCORE_PACKAGE_EVENT_OFFSET(package_num, EMON_BUFFER_DRIVER_HELPER_num_entries_per_package(emon_buffer_driver_helper),
                                                    ECB_entries_uncore_buffer_offset_in_package(pecb, idx));

        if (IS_64BIT((U64)(ECB_entries_max_bits(pecb, idx)))) {
            tmp_value = PCI_MMIO_Read_U64(virtual_addr, ECB_entries_reg_offset(pecb, idx));
        }
        else {
            tmp_value = PCI_MMIO_Read_U32(virtual_addr, ECB_entries_reg_offset(pecb, idx));
        }
        tmp_value &= (U64)ECB_entries_max_bits(pecb, idx);
        if (tmp_value >= prev_buffer[j]) {
            buffer[j] = tmp_value - prev_buffer[j];
        }
        else {
            buffer[j] = tmp_value + (ECB_entries_max_bits(pecb, idx) - prev_buffer[j]);
        }

        SEP_DRV_LOG_TRACE("j=%u, v=%llu", j, buffer[j]);

    } END_FOR_EACH_SCHEDULED_REG_UNC_OPERATION;

    SEP_DRV_LOG_TRACE_OUT("");
    return;
}

/* ------------------------------------------------------------------------- */
/*!
 * @fn unc_mmio_single_bar_Initialize(param)
 *
 * @param    param    dummy parameter which is not used
 *
 * @return   None     No return needed
 *
 * @brief    Do the mapping of the physical address (to do the invalidates in the TLB)
 *           NOTE: this should never be done with SMP call
 *
 */
static VOID
unc_mmio_single_bar_Initialize (
     PVOID   param
)
{
    U32                        dev_idx           = 0;
    U32                        cur_grp           = 0;
    U32                        dev_node          = 0;
    U32                        i                 = 0;
    U32                        j                 = 0;
    U32                        use_default_busno = 0;
    U32                        entries           = 0;
    U32                        this_cpu          = 0;
    U64                        physical_address  = 0;
    U64                        bar               = 0;
    ECB                        pecb              = NULL;
    MMIO_BAR_INFO              mmio_bar_info;

    SEP_DRV_LOG_TRACE_IN("Param: %p.", param);

    this_cpu     = CONTROL_THIS_CPU();
    dev_idx  = *((U32*)param);
    cur_grp  = LWPMU_DEVICE_cur_group(&devices[(dev_idx)])[0];
    pecb     = LWPMU_DEVICE_PMU_register_data(&devices[dev_idx])[cur_grp];

    if (!pecb) {
        for (j = 0; j < (U32)LWPMU_DEVICE_em_groups_count(&devices[dev_idx]); j++) {
            pecb = LWPMU_DEVICE_PMU_register_data(&devices[dev_idx])[j];
            if (!pecb) {
                continue;
            }
            else {
                break;
            }
        }
    }

    if (!pecb) {
        SEP_DRV_LOG_TRACE_OUT("Early exit (!pecb).");
        return;
    }
    dev_node = ECB_dev_node(pecb);

    if (IS_MMIO_MAP_VALID(dev_node, 0)) {
        SEP_DRV_LOG_INIT_TRACE_OUT("Early exit (device[%d] node %d already mapped).", dev_idx, dev_node);
        return;
    }

    // use busno found from topology scan if available
    // otherwise use the default one
    if (dev_node) {
        entries = GET_NUM_MAP_ENTRIES(dev_node);
        SEP_DRV_LOG_TRACE("# if entries - %u", entries);
        SEP_DRV_LOG_WARNING_TRACE_OUT("PCB node not available in group %u!", cur_grp);
    }
    if (entries == 0) {
        use_default_busno = 1;
        entries = 1;  // this could the client, does not through the scan
        UNC_PCIDEV_num_entries(&(unc_pcidev_map[dev_node])) = 1;
        UNC_PCIDEV_max_entries(&(unc_pcidev_map[dev_node])) = 1;
    }

    if (!UNC_PCIDEV_mmio_map(&(unc_pcidev_map[dev_node]))) {
      // it is better to allocate space in the beginning
        UNC_PCIDEV_mmio_map(&(unc_pcidev_map[dev_node])) =  CONTROL_Allocate_Memory(entries * sizeof(SEP_MMIO_NODE));
        if (UNC_PCIDEV_mmio_map(&(unc_pcidev_map[dev_node])) == NULL) {
            SEP_DRV_LOG_ERROR_TRACE_OUT("Early exit (No Memory).");
            return;
        }
        SEP_DRV_MEMSET(UNC_PCIDEV_mmio_map(&(unc_pcidev_map[dev_node])), 0, entries * sizeof(U64));
    }

    UNC_PCIDEV_num_mmio_main_bar_per_entry(&(unc_pcidev_map[dev_node]))      = 1;
    UNC_PCIDEV_num_mmio_secondary_bar_per_entry(&(unc_pcidev_map[dev_node])) = 1;

    for (i = 0; i < entries; i++) {

        mmio_bar_info = &ECB_mmio_bar_list(pecb, 0);

        if  (!use_default_busno) {
            if (IS_BUS_MAP_VALID(dev_node, i)) {
                MMIO_BAR_INFO_bus_no(mmio_bar_info) = UNC_PCIDEV_busno_entry(&(unc_pcidev_map[dev_node]), i);
            }
        }

        SEP_DRV_LOG_TRACE("b=0x%lx, d=0x%x, f=0x%x, o=0x%llx", MMIO_BAR_INFO_bus_no(mmio_bar_info), MMIO_BAR_INFO_dev_no(mmio_bar_info), MMIO_BAR_INFO_func_no(mmio_bar_info), MMIO_BAR_INFO_main_bar_offset(mmio_bar_info));
        bar = PCI_Read_U64(MMIO_BAR_INFO_bus_no(mmio_bar_info),
                           MMIO_BAR_INFO_dev_no(mmio_bar_info),
                           MMIO_BAR_INFO_func_no(mmio_bar_info),
                           MMIO_BAR_INFO_main_bar_offset(mmio_bar_info));

        bar &= MMIO_BAR_INFO_main_bar_mask(mmio_bar_info);

        physical_address = bar + MMIO_BAR_INFO_base_offset_for_mmio(mmio_bar_info);

        SEP_DRV_LOG_TRACE("pa=0x%llx", physical_address);

        PCI_Map_Memory(&UNC_PCIDEV_mmio_map_entry(&(unc_pcidev_map[dev_node]), i), physical_address, MMIO_BAR_INFO_map_size_for_mmio(mmio_bar_info));

        SEP_DRV_LOG_TRACE("va=0x%llx", virtual_address_table(dev_node, i));
    }

    SEP_DRV_LOG_TRACE_OUT("");
    return;
}

/* ------------------------------------------------------------------------- */
/*!
 * @fn unc_mmio_fpga_Initialize(param)
 *
 * @param    param    dummy parameter which is not used
 *
 * @return   None     No return needed
 *
 * @brief    Do the mapping of the physical address (to do the invalidates in the TLB)
 *           NOTE: this should never be done with SMP call
 *
 */
static VOID
unc_mmio_fpga_Initialize (
     PVOID   param
)
{
#if defined(DRV_EM64T)
    U32              id          = 0;
    U32              j           = 0;
    U32              offset      = 0;
    U32              dev_idx     = 0;
    U32              cur_grp     = 0;
    U32              busno       = 0;
    U32              page_len    = 4096;
    U32              package_num = 0;
    U32              dev_node    = 0;
    U32              entries     = 0;
    U32              bus_list[2] = {0x5e, 0xbe};
    S32              next_offset = -1;
    U64              phys_addr   = 0;
    U64              virt_addr   = 0;
    U64              dfh         = 0;
    ECB              pecb        = NULL;
    SEP_MMIO_NODE    tmp_map     = {0};
    MMIO_BAR_INFO    mmio_bar_info;

    SEP_DRV_LOG_TRACE_IN("Param: %p.", param);

    dev_idx = *((U32*)param);
    cur_grp  = LWPMU_DEVICE_cur_group(&devices[(dev_idx)])[0];
    pecb     = LWPMU_DEVICE_PMU_register_data(&devices[dev_idx])[cur_grp];

    if (!pecb) {
        for (j = 0; j < (U32)LWPMU_DEVICE_em_groups_count(&devices[dev_idx]); j++) {
            pecb = LWPMU_DEVICE_PMU_register_data(&devices[dev_idx])[j];
            if (!pecb) {
                continue;
            }
            else {
                break;
            }
        }
    }

    if (!pecb) {
        SEP_DRV_LOG_TRACE_OUT("Early exit (!pecb).");
        return;
    }

    dev_node = ECB_dev_node(pecb);

    entries = GET_NUM_MAP_ENTRIES(dev_node);
    if (entries == 0) {
        entries = num_packages;
    }

    if (!UNC_PCIDEV_mmio_map(&(unc_pcidev_map[dev_node]))) {
        // it is better to allocate space in the beginning
        UNC_PCIDEV_mmio_map(&(unc_pcidev_map[dev_node])) =  CONTROL_Allocate_Memory(entries * sizeof(SEP_MMIO_NODE));
        if (UNC_PCIDEV_mmio_map(&(unc_pcidev_map[dev_node])) == NULL) {
            SEP_DRV_LOG_ERROR_TRACE_OUT("Early exit (No Memory).");
            return;
        }
        SEP_DRV_MEMSET(UNC_PCIDEV_mmio_map(&(unc_pcidev_map[dev_node])), 0, (entries * sizeof(SEP_MMIO_NODE)));
        UNC_PCIDEV_num_entries(&(unc_pcidev_map[dev_node])) = 0;
        UNC_PCIDEV_max_entries(&(unc_pcidev_map[dev_node])) = entries;
    }
    else {
        if (virtual_address_table(dev_node, 0) != 0) {
            SEP_DRV_LOG_INIT_TRACE_OUT("Early exit (device[%d] node %d already mapped).", dev_idx, dev_node);
            return;
        }
    }

    UNC_PCIDEV_num_mmio_main_bar_per_entry(&(unc_pcidev_map[dev_node]))      = 1;
    UNC_PCIDEV_num_mmio_secondary_bar_per_entry(&(unc_pcidev_map[dev_node])) = 1;

    for (package_num = 0; package_num < num_packages; package_num++) {
        if (package_num < 2) {
            busno = bus_list[package_num];
        }
        else {
            busno = 0;
        }

        mmio_bar_info = &ECB_mmio_bar_list(pecb, 0);

        phys_addr = PCI_Read_U64(busno,
                                 MMIO_BAR_INFO_dev_no(mmio_bar_info),
                                 MMIO_BAR_INFO_func_no(mmio_bar_info),
                                 MMIO_BAR_INFO_main_bar_offset(mmio_bar_info));
        phys_addr &= MMIO_BAR_INFO_main_bar_mask(mmio_bar_info);
        if (package_num == 0) {
            PCI_Map_Memory(&tmp_map, phys_addr, 8 * page_len);
            virt_addr = SEP_MMIO_NODE_virtual_address(&tmp_map);
            while (next_offset != 0) {
                dfh = PCI_MMIO_Read_U64((U64)virt_addr, offset);
                next_offset = (U32)((dfh >> 16) & 0xffffff);
                id = (U32)(dfh & 0xfff);
                if (offset && (id == MMIO_BAR_INFO_feature_id(mmio_bar_info))) {
                    break;
                }
                offset += next_offset;
            }
            PCI_Unmap_Memory(&tmp_map);
        }
        phys_addr += offset;
        PCI_Map_Memory(&UNC_PCIDEV_mmio_map_entry(&(unc_pcidev_map[dev_node]), package_num), phys_addr, 8 * page_len);
        UNC_PCIDEV_num_entries(&(unc_pcidev_map[dev_node]))++;
    }

    SEP_DRV_LOG_TRACE_OUT("");
#endif
    return;
}

/* ------------------------------------------------------------------------- */
/*!
 * @fn unc_mmio_multiple_bar_Initialize(param)
 *
 * @param    param    dummy parameter which is not used
 *
 * @return   None     No return needed
 *
 * @brief    Do the mapping of the physical address (to do the invalidates in the TLB)
 *           NOTE: this should never be done with SMP call
 *
 */
static VOID
unc_mmio_multiple_bar_Initialize (
     PVOID   param
)
{
    U32                        mmio_base              = 0;
    U32                        mem_offset             = 0;
    U32                        mem_bar                = 0;
    U32                        dev_idx                = 0;
    U32                        cur_grp                = 0;
    U32                        dev_node               = 0;
    U32                        i                      = 0;
    U32                        j                      = 0;
    U32                        entries                = 0;
    U32                        this_cpu               = 0;
    U32                        num_mmio_secondary_bar = 0;
    U64                        virtual_address        = 0;
    U64                        physical_address       = 0;
    ECB                        pecb                   = NULL;
    MMIO_BAR_INFO              mmio_bar_info;

    SEP_DRV_LOG_TRACE("Param: %p.", param);

    this_cpu     = CONTROL_THIS_CPU();
    dev_idx  = *((U32*)param);
    cur_grp  = LWPMU_DEVICE_cur_group(&devices[(dev_idx)])[0];
    pecb     = LWPMU_DEVICE_PMU_register_data(&devices[dev_idx])[cur_grp];

    if (!pecb) {
        for (j = 0; j < (U32)LWPMU_DEVICE_em_groups_count(&devices[dev_idx]); j++) {
            pecb = LWPMU_DEVICE_PMU_register_data(&devices[dev_idx])[j];
            if (!pecb) {
                continue;
            }
            else {
                break;
            }
        }
    }

    if (!pecb) {
        SEP_DRV_LOG_TRACE_OUT("Early exit (!pecb).");
        return;
    }
    dev_node = ECB_dev_node(pecb);

    if (IS_MMIO_MAP_VALID(dev_node, 0)) {
        SEP_DRV_LOG_INIT_TRACE_OUT("Early exit (device[%d] node %d already mapped).", dev_idx, dev_node);
        return;
    }

    // use busno found from topology scan if available
    // otherwise use the default one
    entries = GET_NUM_MAP_ENTRIES(dev_node);

    if (!UNC_PCIDEV_mmio_map(&(unc_pcidev_map[dev_node]))) {
      // it is better to allocate space in the beginning
        UNC_PCIDEV_mmio_map(&(unc_pcidev_map[dev_node])) =  CONTROL_Allocate_Memory(entries * ECB_num_mmio_secondary_bar(pecb) * sizeof(SEP_MMIO_NODE));
        if (UNC_PCIDEV_mmio_map(&(unc_pcidev_map[dev_node])) == NULL) {
            SEP_DRV_LOG_ERROR_TRACE_OUT("Early exit (No Memory).");
            return;
        }
        SEP_DRV_MEMSET(UNC_PCIDEV_mmio_map(&(unc_pcidev_map[dev_node])), 0, entries * sizeof(U64));
    }

    UNC_PCIDEV_num_mmio_main_bar_per_entry(&(unc_pcidev_map[dev_node]))      = 1;
    UNC_PCIDEV_num_mmio_secondary_bar_per_entry(&(unc_pcidev_map[dev_node])) = ECB_num_mmio_secondary_bar(pecb);
    num_mmio_secondary_bar                                                   = GET_NUM_MMIO_SECONDARY_BAR(dev_node);

    for (i = 0; i < entries; i++) {
        for (j = 0; j < num_mmio_secondary_bar; j++) {

            mmio_bar_info = &ECB_mmio_bar_list(pecb, j);

            if (IS_BUS_MAP_VALID(dev_node, i)) {
                MMIO_BAR_INFO_bus_no(mmio_bar_info) = UNC_PCIDEV_busno_entry(&(unc_pcidev_map[dev_node]), i);
                SEP_DRV_LOG_TRACE("bus_no:%x", MMIO_BAR_INFO_bus_no(mmio_bar_info));
            }
            else {
                SEP_DRV_LOG_TRACE_OUT("PCI device map not found. Early exit.");
                return;
            }
            mmio_base = PCI_Read_U32(MMIO_BAR_INFO_bus_no(mmio_bar_info),
                                     MMIO_BAR_INFO_dev_no(mmio_bar_info),
                                     MMIO_BAR_INFO_func_no(mmio_bar_info),
                                     MMIO_BAR_INFO_main_bar_offset(mmio_bar_info));
            mem_offset = PCI_Read_U32(MMIO_BAR_INFO_bus_no(mmio_bar_info),
                                      MMIO_BAR_INFO_dev_no(mmio_bar_info),
                                      MMIO_BAR_INFO_func_no(mmio_bar_info),
                                      MMIO_BAR_INFO_secondary_bar_offset(mmio_bar_info));
            mem_bar = (U32)((mmio_base  & (U32)MMIO_BAR_INFO_main_bar_mask(mmio_bar_info))      << MMIO_BAR_INFO_main_bar_shift(mmio_bar_info))       |
                           ((mem_offset & (U32)MMIO_BAR_INFO_secondary_bar_mask(mmio_bar_info)) << MMIO_BAR_INFO_secondary_bar_shift(mmio_bar_info));

            physical_address = mem_bar + MMIO_BAR_INFO_base_offset_for_mmio(mmio_bar_info);

            PCI_Map_Memory(&UNC_PCIDEV_mmio_map_entry(&(unc_pcidev_map[dev_node]),
                          i * num_mmio_secondary_bar + j),
                          physical_address,
                          MMIO_BAR_INFO_map_size_for_mmio(mmio_bar_info));

            virtual_address = SEP_MMIO_NODE_virtual_address(&UNC_PCIDEV_mmio_map_entry(&(unc_pcidev_map[dev_node]),
                                                            i * num_mmio_secondary_bar + j));

            SEP_DRV_LOG_TRACE("i=%u, j=%u, pa=0x%llx, va=0x%llx", i, j, physical_address, virtual_address);
        }
    }

    SEP_DRV_LOG_TRACE_OUT("");
    return;
}

/* ------------------------------------------------------------------------- */
/*!
 * @fn unc_mmio_Destroy(param)
 *
 * @param    param    dummy parameter which is not used
 *
 * @return   None     No return needed
 *
 * @brief    Invalidate the entry in TLB of the physical address
 *           NOTE: this should never be done with SMP call
 *
 */
static VOID
unc_mmio_Destroy (
     PVOID   param
)
{
    U32                    dev_idx                = 0;
    U32                    i                      = 0;
    U32                    j                      = 0;
    U32                    dev_node               = 0;
    U32                    cur_grp                = 0;
    U32                    entries                = 0;
    U32                    num_mmio_secondary_bar = 0;
    ECB                    pecb                   = NULL;

    SEP_DRV_LOG_TRACE_IN("Param: %p.", param);

    dev_idx = *((U32*)param);
    cur_grp  = LWPMU_DEVICE_cur_group(&devices[(dev_idx)])[0];
    pecb     = LWPMU_DEVICE_PMU_register_data(&devices[dev_idx])[cur_grp];

    if (!pecb) {
        for (j = 0; j < (U32)LWPMU_DEVICE_em_groups_count(&devices[dev_idx]); j++) {
            pecb = LWPMU_DEVICE_PMU_register_data(&devices[dev_idx])[j];
            if (!pecb) {
                continue;
            }
            else {
                break;
            }
        }
    }

    if (!pecb) {
        SEP_DRV_LOG_TRACE_OUT("Early exit (!pecb).");
        return;
    }
    dev_node = ECB_dev_node(pecb);

    if (!UNC_PCIDEV_mmio_map(&(unc_pcidev_map[dev_node]))) {
        SEP_DRV_LOG_TRACE_OUT("Early exit (no mapping).");
        return;
    }

    entries                = GET_NUM_MAP_ENTRIES(dev_node);
    num_mmio_secondary_bar = GET_NUM_MMIO_SECONDARY_BAR(dev_node);

    for (i = 0; i < entries; i++) {
        if (num_mmio_secondary_bar > 1) {
            for (j = 0; j < num_mmio_secondary_bar; j++) {
                if (IS_MMIO_MAP_VALID(dev_node, i * num_mmio_secondary_bar + j)) {
                    PCI_Unmap_Memory(&UNC_PCIDEV_mmio_map_entry(&(unc_pcidev_map[dev_node]),
                                     i * num_mmio_secondary_bar + j));
                }
            }
        }
        else {
            if (IS_MMIO_MAP_VALID(dev_node, i)) {
                PCI_Unmap_Memory(&UNC_PCIDEV_mmio_map_entry(&(unc_pcidev_map[dev_node]), i));
            }
        }
    }

    SEP_DRV_LOG_TRACE_OUT("");
    return;
}


/*
 * Initialize the dispatch table
 */
DISPATCH_NODE  unc_mmio_single_bar_dispatch =
{
    unc_mmio_single_bar_Initialize,    // initialize
    unc_mmio_Destroy,                  // destroy
    unc_mmio_single_bar_Write_PMU,     // write
    unc_mmio_single_bar_Disable_PMU,   // freeze
    unc_mmio_single_bar_Enable_PMU,    // restart
    unc_mmio_single_bar_Read_PMU_Data, // read
    NULL,                              // check for overflow
    NULL,                              // swap group
    NULL,                              // read lbrs
    UNC_COMMON_Dummy_Func,             // cleanup
    NULL,                              // hw errata
    NULL,                              // read power
    NULL,                              // check overflow errata
    NULL,                              // read counts
    NULL,                              // check overflow gp errata
    NULL,                              // read_ro
    NULL,                              // platform info
    unc_mmio_single_bar_Trigger_Read,  // trigger read
    NULL,                              // scan for uncore
    NULL                               // read metrics
};

DISPATCH_NODE  unc_mmio_fpga_dispatch =
{
    unc_mmio_fpga_Initialize,          // initialize
    unc_mmio_Destroy,                  // destroy
    unc_mmio_single_bar_Write_PMU,     // write
    unc_mmio_single_bar_Disable_PMU,   // freeze
    unc_mmio_single_bar_Enable_PMU,    // restart
    unc_mmio_single_bar_Read_PMU_Data, // read
    NULL,                              // check for overflow
    NULL,                              // swap group
    NULL,                              // read lbrs
    UNC_COMMON_Dummy_Func,             // cleanup
    NULL,                              // hw errata
    NULL,                              // read power
    NULL,                              // check overflow errata
    NULL,                              // read counts
    NULL,                              // check overflow gp errata
    NULL,                              // read_ro
    NULL,                              // platform info
    unc_mmio_single_bar_Trigger_Read,  // trigger read
    NULL,                              // scan for uncore
    NULL                               // read metrics
};

DISPATCH_NODE  unc_mmio_multiple_bar_dispatch =
{
    unc_mmio_multiple_bar_Initialize,    // initialize
    unc_mmio_Destroy,                    // destroy
    unc_mmio_multiple_bar_Write_PMU,     // write
    unc_mmio_multiple_bar_Disable_PMU,   // freeze
    unc_mmio_multiple_bar_Enable_PMU,    // restart
    unc_mmio_multiple_bar_Read_PMU_Data, // read
    NULL,                                // check for overflow
    NULL,                                // swap group
    NULL,                                // read lbrs
    UNC_COMMON_Dummy_Func,               // cleanup
    NULL,                                // hw errata
    NULL,                                // read power
    NULL,                                // check overflow errata
    NULL,                                // read counts
    NULL,                                // check overflow gp errata
    NULL,                                // read_ro
    NULL,                                // platform info
    unc_mmio_multiple_bar_Trigger_Read,  // trigger read
    NULL,                                // scan for uncore
    NULL                                 // read metrics
};

