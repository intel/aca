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





#ifndef _ECB_ITERATORS_H_
#define _ECB_ITERATORS_H_

#if defined(__cplusplus)
extern "C" {
#endif

//
// Loop macros to walk through the event control block
// Use for access only in the kernel mode
// To Do - Control access from kernel mode by a macro
//
#define FOR_EACH_CCCR_REG(pecb,idx) {                                                  \
    U32        (idx);                                                                  \
    U32        this_cpu__ = CONTROL_THIS_CPU();                                        \
    CPU_STATE  pcpu__  = &pcb[this_cpu__];                                             \
    U32        (dev_idx) = core_to_dev_map[this_cpu__];                                \
    U32        (cur_grp) = CPU_STATE_current_group(pcpu__);                            \
    ECB        (pecb) = LWPMU_DEVICE_PMU_register_data(&devices[dev_idx])[cur_grp];    \
    if ((pecb)) {                                                                      \
        for ((idx) = ECB_cccr_start((pecb));                                           \
             (idx) < ECB_cccr_start((pecb))+ECB_cccr_pop((pecb));                      \
             (idx)++) {                                                                \
            if (ECB_entries_reg_id((pecb),(idx)) == 0) {                               \
                continue;                                                              \
            }

#define END_FOR_EACH_CCCR_REG  }}}

#define FOR_EACH_CCCR_GP_REG(pecb,idx) {                                               \
    U32        (idx);                                                                  \
    U32        this_cpu__ = CONTROL_THIS_CPU();                                        \
    CPU_STATE  pcpu__  = &pcb[this_cpu__];                                             \
    U32        (dev_idx) = core_to_dev_map[this_cpu__];                                \
    U32        (cur_grp) = CPU_STATE_current_group(pcpu__);                            \
    ECB        (pecb) = LWPMU_DEVICE_PMU_register_data(&devices[dev_idx])[cur_grp];    \
    if ((pecb)) {                                                                      \
        for ((idx) = ECB_cccr_start((pecb));                                           \
             (idx) < ECB_cccr_start((pecb))+ECB_cccr_pop((pecb));                      \
             (idx)++) {                                                                \
            if (ECB_entries_is_gp_reg_get((pecb),(idx)) == 0) {                        \
                continue;                                                              \
            }

#define END_FOR_EACH_CCCR_GP_REG  }}}

#define FOR_EACH_ESCR_REG(pecb,idx) {                                                  \
    U32        (idx);                                                                  \
    U32        this_cpu__ = CONTROL_THIS_CPU();                                        \
    CPU_STATE  pcpu__  = &pcb[this_cpu__];                                             \
    U32        (dev_idx) = core_to_dev_map[this_cpu__];                                \
    U32        (cur_grp) = CPU_STATE_current_group(pcpu__);                            \
    ECB        (pecb) = LWPMU_DEVICE_PMU_register_data(&devices[dev_idx])[cur_grp];    \
    if ((pecb)) {                                                                      \
        for ((idx) = ECB_escr_start((pecb));                                           \
             (idx) < ECB_escr_start((pecb))+ECB_escr_pop((pecb));                      \
             (idx)++) {                                                                \
            if (ECB_entries_reg_id((pecb),(idx)) == 0) {                               \
                continue;                                                              \
            }

#define END_FOR_EACH_ESCR_REG  }}}

#define FOR_EACH_DATA_REG(pecb,idx) {                                                  \
    U32        (idx);                                                                  \
    U32        this_cpu__ = CONTROL_THIS_CPU();                                        \
    CPU_STATE  pcpu__  = &pcb[this_cpu__];                                             \
    U32        (dev_idx) = core_to_dev_map[this_cpu__];                                \
    U32        (cur_grp) = CPU_STATE_current_group(pcpu__);                            \
    ECB        (pecb) = LWPMU_DEVICE_PMU_register_data(&devices[dev_idx])[cur_grp];    \
    if ((pecb)) {                                                                      \
        for ((idx) = ECB_data_start((pecb));                                           \
             (idx) < ECB_data_start((pecb))+ECB_data_pop((pecb));                      \
             (idx)++) {                                                                \
            if (ECB_entries_reg_id((pecb),(idx)) == 0) {                               \
                continue;                                                              \
            }

#define END_FOR_EACH_DATA_REG  }}}

#define FOR_EACH_DATA_REG_UNC(pecb,device_idx,idx) {                                      \
    U32        (idx);                                                                     \
    U32        (cpu)     = CONTROL_THIS_CPU();                                            \
    U32        (pkg)     = core_to_package_map[cpu];                                      \
    U32        (cur_grp) = LWPMU_DEVICE_cur_group(&devices[(device_idx)])[(pkg)];         \
    ECB        (pecb) = LWPMU_DEVICE_PMU_register_data(&devices[(device_idx)])[cur_grp];  \
    if ((pecb)) {                                                                         \
      for ((idx) = ECB_data_start((pecb));                                                \
           (idx) < ECB_data_start((pecb))+ECB_data_pop((pecb));                           \
           (idx)++) {                                                                     \
          if (ECB_entries_reg_id((pecb),(idx)) == 0) {                                    \
              continue;                                                                   \
    }

#define END_FOR_EACH_DATA_REG_UNC  }}}

#define FOR_EACH_DATA_REG_UNC_VER2(pecb,i,idx ) {                                         \
    U32        (idx);                                                                     \
    if ((pecb)) {                                                                         \
      for ((idx) = ECB_data_start((pecb));                                                \
           (idx) < ECB_data_start((pecb))+ECB_data_pop((pecb));                           \
           (idx)++) {                                                                     \
          if (ECB_entries_reg_id((pecb),(idx)) == 0) {                                    \
              continue;                                                                   \
    }

#define END_FOR_EACH_DATA_REG_UNC_VER2    } } }

#define FOR_EACH_DATA_GP_REG(pecb,idx) {                                               \
    U32        (idx);                                                                  \
    U32        this_cpu__ = CONTROL_THIS_CPU();                                        \
    CPU_STATE  pcpu__  = &pcb[this_cpu__];                                             \
    U32        (dev_idx) = core_to_dev_map[this_cpu__];                                \
    U32        (cur_grp) = CPU_STATE_current_group(pcpu__);                            \
    ECB        (pecb) = LWPMU_DEVICE_PMU_register_data(&devices[dev_idx])[cur_grp];    \
    if ((pecb)) {                                                                      \
        for ((idx) = ECB_data_start((pecb));                                           \
             (idx) < ECB_data_start((pecb))+ECB_data_pop((pecb));                      \
             (idx)++) {                                                                \
            if (ECB_entries_is_gp_reg_get((pecb),(idx)) == 0) {                        \
                continue;                                                              \
            }

#define END_FOR_EACH_DATA_GP_REG  }}}

#define FOR_EACH_DATA_GENERIC_REG(pecb,idx) {                                          \
    U32        (idx);                                                                  \
    U32        this_cpu__ = CONTROL_THIS_CPU();                                        \
    CPU_STATE  pcpu__  = &pcb[this_cpu__];                                             \
    U32        (dev_idx) = core_to_dev_map[this_cpu__];                                \
    U32        (cur_grp) = CPU_STATE_current_group(pcpu__);                            \
    ECB        (pecb) = LWPMU_DEVICE_PMU_register_data(&devices[dev_idx])[cur_grp];    \
    if ((pecb)) {                                                                      \
        for ((idx) = ECB_data_start((pecb));                                           \
             (idx) < ECB_data_start((pecb))+ECB_data_pop((pecb));                      \
             (idx)++) {                                                                \
            if (ECB_entries_is_generic_reg_get((pecb),(idx)) == 0) {                   \
                continue;                                                              \
            }

#define END_FOR_EACH_DATA_GENERIC_REG  }}}

#define FOR_EACH_REG_ENTRY(pecb,idx) {                                                 \
    U32        (idx);                                                                  \
    U32        this_cpu__ = CONTROL_THIS_CPU();                                        \
    CPU_STATE  pcpu__  = &pcb[this_cpu__];                                             \
    U32        (dev_idx) = core_to_dev_map[this_cpu__];                                \
    U32        (cur_grp) = CPU_STATE_current_group(pcpu__);                            \
    ECB        (pecb) = LWPMU_DEVICE_PMU_register_data(&devices[dev_idx])[cur_grp];    \
    if ((pecb)) {                                                                      \
    for ((idx) = 0; (idx) < ECB_num_entries((pecb)); (idx)++) {                        \
        if (ECB_entries_reg_id((pecb),(idx)) == 0) {                                   \
            continue;                                                                  \
        }

#define END_FOR_EACH_REG_ENTRY  }}}

#define FOR_EACH_REG_ENTRY_UNC(pecb,device_idx,idx) {                                          \
    U32        (idx);                                                                          \
    U32        (cpu)     = CONTROL_THIS_CPU();                                                 \
    U32        (pkg)     = core_to_package_map[cpu];                                           \
    U32        (cur_grp) = LWPMU_DEVICE_cur_group(&devices[(device_idx)])[(pkg)];              \
    ECB        (pecb)    = LWPMU_DEVICE_PMU_register_data(&devices[(device_idx)])[(cur_grp)];  \
    if ((pecb)) {                                                                              \
        for ((idx) = 0; (idx) < ECB_num_entries((pecb)); (idx)++) {                            \
            if (ECB_entries_reg_id((pecb),(idx)) == 0) {                                       \
                continue;                                                                      \
            }

#define END_FOR_EACH_REG_ENTRY_UNC  }}}

#define FOR_EACH_PCI_DATA_REG(pecb,i, device_idx, offset_delta) {                                       \
    U32                 (i)       = 0;                                                                  \
    U32                 (cpu)     = CONTROL_THIS_CPU();                                                 \
    U32                 (pkg)     = core_to_package_map[cpu];                                           \
    U32                 (cur_grp) = LWPMU_DEVICE_cur_group(&devices[(device_idx)])[(pkg)];              \
    ECB                 (pecb)    = LWPMU_DEVICE_PMU_register_data(&devices[(device_idx)])[(cur_grp)];  \
    if ((pecb)) {                                                                                       \
        for ((i) = ECB_data_start((pecb));                                                              \
             (i) < ECB_data_start((pecb))+ECB_data_pop((pecb));                                         \
             (i)++) {                                                                                   \
            if (ECB_entries_reg_offset((pecb),(i)) == 0) {                                              \
                continue;                                                                               \
            }                                                                                           \
            (offset_delta) =  ECB_entries_reg_offset((pecb),(i)) -                                      \
                              DRV_PCI_DEVICE_ENTRY_base_offset_for_mmio(&ECB_pcidev_entry_node(pecb));

#define END_FOR_EACH_PCI_DATA_REG    } } }

#define FOR_EACH_PCI_DATA_REG_VER2(pecb,i, device_idx, offset_delta) {                                  \
    U32                 (i)    = 0;                                                                     \
    if ((pecb)) {                                                                                       \
        for ((i) = ECB_data_start((pecb));                                                              \
             (i) < ECB_data_start((pecb))+ECB_data_pop((pecb));                                         \
             (i)++) {                                                                                   \
            if (ECB_entries_reg_offset((pecb),(i)) == 0) {                                              \
                continue;                                                                               \
            }                                                                                           \
            (offset_delta) =  ECB_entries_reg_offset((pecb),(i)) -                                      \
                              DRV_PCI_DEVICE_ENTRY_base_offset_for_mmio(&ECB_pcidev_entry_node(pecb));

#define END_FOR_EACH_PCI_DATA_REG_VER2    } } }

#define FOR_EACH_PCI_DATA_REG_RAW(pecb,i, device_idx ) {                                                \
    U32                 (i)       = 0;                                                                  \
    U32                 (cpu)     = CONTROL_THIS_CPU();                                                 \
    U32                 (pkg)     = core_to_package_map[cpu];                                           \
    U32                 (cur_grp) = LWPMU_DEVICE_cur_group(&devices[(device_idx)])[(pkg)];              \
    ECB                 (pecb)    = LWPMU_DEVICE_PMU_register_data(&devices[(device_idx)])[(cur_grp)];  \
    if ((pecb)) {                                                                                       \
        for ((i) = ECB_data_start((pecb));                                                              \
             (i) < ECB_data_start((pecb))+ECB_data_pop((pecb));                                         \
             (i)++) {                                                                                   \
            if (ECB_entries_reg_offset((pecb),(i)) == 0) {                                              \
                continue;                                                                               \
            }

#define END_FOR_EACH_PCI_DATA_REG_RAW    } } }

#define FOR_EACH_PCI_CCCR_REG_RAW(pecb,i, device_idx ) {                                            \
    U32              (i)       = 0;                                                                 \
    U32              (cpu)     = CONTROL_THIS_CPU();                                                \
    U32              (pkg)     = core_to_package_map[cpu];                                          \
    U32              (cur_grp) = LWPMU_DEVICE_cur_group(&devices[(device_idx)])[(pkg)];             \
    ECB              (pecb)    = LWPMU_DEVICE_PMU_register_data(&devices[(device_idx)])[(cur_grp)]; \
    if ((pecb)) {                                                                                   \
        for ((i) = ECB_cccr_start((pecb));                                                          \
             (i) < ECB_cccr_start((pecb))+ECB_cccr_pop((pecb));                                     \
             (i)++) {                                                                               \
            if (ECB_entries_reg_offset((pecb),(i)) == 0) {                                          \
                continue;                                                                           \
            }

#define END_FOR_EACH_PCI_CCCR_REG_RAW   } } }

#define FOR_EACH_PCI_REG_RAW(pecb, i, device_idx ) {                                                   \
    U32                 (i)       = 0;                                                                 \
    U32                 (cpu)     = CONTROL_THIS_CPU();                                                \
    U32                 (pkg)     = core_to_package_map[cpu];                                          \
    U32                 (cur_grp) = LWPMU_DEVICE_cur_group(&devices[(device_idx)])[(pkg)];             \
    ECB                 (pecb)    = LWPMU_DEVICE_PMU_register_data(&devices[(device_idx)])[(cur_grp)]; \
    if ((pecb)) {                                                                                      \
        for ((i) = 0;                                                                                  \
             (i) < ECB_num_entries((pecb));                                                            \
             (i)++) {                                                                                  \
            if (ECB_entries_reg_offset((pecb),(i)) == 0) {                                             \
                continue;                                                                              \
            }

#define END_FOR_EACH_PCI_REG_RAW   } } }

#define FOR_EACH_PCI_REG_RAW_GROUP(pecb,i, device_idx, cur_grp ) {                                  \
    U32              (i)       = 0;                                                                 \
    ECB              (pecb)    = LWPMU_DEVICE_PMU_register_data(&devices[(device_idx)])[(cur_grp)]; \
    if ((pecb)) {                                                                                   \
        for ((i) = 0;                                                                               \
             (i) < ECB_num_entries((pecb));                                                         \
             (i)++) {                                                                               \
            if (ECB_entries_reg_offset((pecb),(i)) == 0) {                                          \
                continue;                                                                           \
            }

#define END_FOR_EACH_PCI_REG_RAW_GROUP   } } }

#define CHECK_SAVE_RESTORE_EVENT_INDEX(prev_ei, cur_ei, evt_index)  {                                   \
        if (prev_ei == -1) {                                                                            \
            prev_ei = cur_ei;                                                                           \
        }                                                                                               \
        if (prev_ei < cur_ei) {                                                                         \
            prev_ei = cur_ei;                                                                           \
            evt_index++;                                                                                \
        }                                                                                               \
        else {                                                                                          \
             evt_index = 0;                                                                             \
             prev_ei = cur_ei;                                                                          \
        }}


#define FOR_EACH_REG_ENTRY_UNC_WRITE_MSR(pecb, device_idx, idx) {                              \
    U32        (idx);                                                                          \
    U32        (cpu)     = CONTROL_THIS_CPU();                                                 \
    U32        (pkg)     = core_to_package_map[cpu];                                           \
    U32        (cur_grp) = LWPMU_DEVICE_cur_group(&devices[(device_idx)])[(pkg)];              \
    ECB        (pecb)    = LWPMU_DEVICE_PMU_register_data(&devices[(device_idx)])[(cur_grp)];  \
    if ((pecb)) {                                                                              \
        for ((idx) = 0; (idx) < ECB_num_entries((pecb)); (idx)++) {                            \
            if (ECB_entries_reg_id((pecb),(idx)) == 0) {                                       \
                continue;                                                                      \
            }

#define END_FOR_EACH_REG_ENTRY_UNC  }}}


#define FOR_EACH_REG_UNC_OPERATION(pecb, device_idx, idx, operation) {                         \
    U32        (idx);                                                                          \
    U32        (cpu)     = CONTROL_THIS_CPU();                                                 \
    U32        (pkg)     = core_to_package_map[cpu];                                           \
    U32        (cur_grp) = LWPMU_DEVICE_cur_group(&devices[(device_idx)])[(pkg)];              \
    ECB        (pecb)    = LWPMU_DEVICE_PMU_register_data(&devices[(device_idx)])[(cur_grp)];  \
    if ((pecb)) {                                                                              \
         for ((idx) = ECB_operations_register_start((pecb), (operation));                      \
              (idx) < ECB_operations_register_start((pecb), (operation)) +                     \
                      ECB_operations_register_len((pecb), (operation)); (idx)++) {             \
            if (ECB_entries_reg_id((pecb),(idx)) == 0) {                                       \
                continue;                                                                      \
            }

#define END_FOR_EACH_REG_UNC_OPERATION  }}}

#define FOR_EACH_SCHEDULED_REG_UNC_OPERATION(pecb, device_idx, idx, operation) {               \
    U32        (idx);                                                                          \
    U32        (cpu)     = CONTROL_THIS_CPU();                                                 \
    U32        (pkg)     = core_to_package_map[cpu];                                           \
    U32        (cur_grp) = LWPMU_DEVICE_cur_group(&devices[(device_idx)])[(pkg)];              \
    ECB        (pecb)    = LWPMU_DEVICE_PMU_register_data(&devices[(device_idx)])[(cur_grp)];  \
    if ((pecb)) {                                                                              \
         for ((idx) = ECB_operations_register_start((pecb), (operation));                      \
              (idx) < ECB_operations_register_start((pecb), (operation)) +                     \
                      ECB_operations_register_len((pecb), (operation)); (idx)++) {             \
            if (ECB_entries_scheduled((pecb),(idx)) != TRUE) {                                 \
                continue;                                                                      \
            }

#define END_FOR_EACH_SCHEDULED_REG_UNC_OPERATION  }}}

#define FOR_EACH_NONEVENT_REG(pecb,idx) {                                              \
    U32        (idx);                                                                  \
    U32        this_cpu__ = CONTROL_THIS_CPU();                                        \
    CPU_STATE  pcpu__  = &pcb[this_cpu__];                                             \
    U32        (dev_idx) = core_to_dev_map[this_cpu__];                                \
    U32        (cur_grp) = CPU_STATE_current_group(pcpu__);                            \
    ECB        (pecb) = LWPMU_DEVICE_PMU_register_data(&devices[dev_idx])[cur_grp];    \
    if ((pecb)) {                                                                      \
        for ((idx) = ECB_metric_start((pecb));                                         \
             (idx) < ECB_metric_start((pecb))+ECB_metric_pop((pecb));                  \
             (idx)++) {                                                                \
            if (ECB_entries_reg_id((pecb),(idx)) == 0) {                               \
                continue;                                                              \
            }

#define END_FOR_EACH_NONEVENT_REG  }}}

#define FOR_EACH_REG_CORE_OPERATION(pecb, idx, operation) {                            \
    U32        (idx);                                                                  \
    U32        this_cpu__ = CONTROL_THIS_CPU();                                        \
    CPU_STATE  pcpu__  = &pcb[this_cpu__];                                             \
    U32        cur_grp = CPU_STATE_current_group(pcpu__);                              \
    U32        dev_idx = core_to_dev_map[this_cpu__];                                  \
    ECB        (pecb)  = LWPMU_DEVICE_PMU_register_data(&devices[dev_idx])[cur_grp];   \
    if ((pecb)) {                                                                              \
         for ((idx) = ECB_operations_register_start((pecb), (operation));                      \
              (idx) < ECB_operations_register_start((pecb), (operation)) +                     \
                      ECB_operations_register_len((pecb), (operation)); (idx)++) {             \
            if (ECB_entries_reg_id((pecb),(idx)) == 0) {                                       \
                continue;                                                                      \
            }

#define END_FOR_EACH_REG_CORE_OPERATION  }}}

#define FOR_EACH_REG_CORE_OPERATION_IN_FIRST_NONNULL_GRP(pecb, idx, operation) {              \
    U32              (idx), j;                                                                \
    U32              this_cpu = CONTROL_THIS_CPU();                                           \
    ECB              (pecb) = NULL;                                                           \
    U32              dev_idx = core_to_dev_map[this_cpu];                                     \
    for (j = 0; (S32)j < LWPMU_DEVICE_em_groups_count(&devices[dev_idx]); j++) {              \
        (pecb)     = LWPMU_DEVICE_PMU_register_data(&devices[dev_idx])[j];                    \
        if (!(pecb)) {                                                                        \
            continue;                                                                         \
        }                                                                                     \
        else {                                                                                \
            break;                                                                            \
        }                                                                                     \
    }                                                                                         \
    if ((pecb)) {                                                                             \
         for ((idx) = ECB_operations_register_start((pecb), (operation));                     \
              (idx) < ECB_operations_register_start((pecb), (operation)) +                    \
                      ECB_operations_register_len((pecb), (operation)); (idx)++) {            \
            if (ECB_entries_reg_id((pecb),(idx)) == 0) {                                      \
                continue;                                                                     \
            }

#define END_FOR_EACH_REG_CORE_OPERATION_IN_FIRST_NONNULL_GRP  }}}


#define ECB_SECTION_REG_INDEX(pecb, idx, operation)    (ECB_operations_register_start((pecb), (operation)) + (idx))


#if defined(__cplusplus)
}
#endif

#endif

