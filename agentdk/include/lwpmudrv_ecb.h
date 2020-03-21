/****
    Copyright (C) 2019-2020 Intel Corporation.  All Rights Reserved.

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






****/

#ifndef _LWPMUDRV_ECB_UTILS_H_
#define _LWPMUDRV_ECB_UTILS_H_

#if defined(DRV_OS_WINDOWS)
#pragma warning (disable:4200)
#endif

#if defined(__cplusplus)
extern "C" {
#endif

// control register types
#define CCCR                1   // counter configuration control register
#define ESCR                2   // event selection control register
#define DATA                4   // collected as snapshot of current value
#define DATA_RO_DELTA       8   // read-only counter collected as current-previous
#define DATA_RO_SS          16  // read-only counter collected as snapshot of current value
#define METRICS             32  // hardware metrics

// event multiplexing modes
#define EM_DISABLED                -1
#define EM_TIMER_BASED              0
#define EM_EVENT_BASED_PROFILING    1
#define EM_TRIGGER_BASED            2

// ***************************************************************************

/*!\struct EVENT_DESC_NODE
 * \var    sample_size                   - size of buffer in bytes to hold the sample + extras
 * \var    max_gp_events                 - max number of General Purpose events per EM group
 * \var    pebs_offset                   - offset in the sample to locate the pebs capture information
 * \var    lbr_offset                    - offset in the sample to locate the lbr information
 * \var    lbr_num_regs                  - offset in the sample to locate the number of lbr register information
 * \var    latency_offset_in_sample      - offset in the sample to locate the latency information
 * \var    latency_size_in_sample        - size of latency records in the sample
 * \var    latency_size_from_pebs_record - size of the latency data from pebs record in the sample
 * \var    latency_offset_in_pebs_record - offset in the sample to locate the latency information
 *                                         in pebs record
 * \var    power_offset_in_sample        - offset in the sample to locate the power information
 * \var    ebc_offset                    - offset in the sample to locate the ebc count information
 * \var    uncore_ebc_offset             - offset in the sample to locate the uncore ebc count information
 *
 * \var    ro_offset                     - offset of RO data in the sample
 * \var    ro_count                      - total number of RO entries (including all of IEAR/DEAR/BTB/IPEAR)
 * \var    iear_offset                   - offset into RO data at which IEAR entries begin
 * \var    dear_offset                   - offset into RO data at which DEAR entries begin
 * \var    btb_offset                    - offset into RO data at which BTB entries begin (these use the same PMDs)
 * \var    ipear_offset                  - offset into RO data at which IPEAR entries begin (these use the same PMDs)
 * \var    iear_count                    - number of IEAR entries
 * \var    dear_count                    - number of DEAR entries
 * \var    btb_count                     - number of BTB entries
 * \var    ipear_count                   - number of IPEAR entries
 *
 * \var    pwr_offset                    - offset in the sample to locate the pwr count information
 * \var    p_state_offset                - offset in the sample to locate the p_state information (APERF/MPERF)
 *
 * \brief  Data structure to describe the events and the mode
 *
 */

typedef struct EVENT_DESC_NODE_S  EVENT_DESC_NODE;
typedef        EVENT_DESC_NODE   *EVENT_DESC;

struct EVENT_DESC_NODE_S {
    U32     sample_size;
    U32     pebs_offset;
    U32     pebs_size;
    U32     lbr_offset;
    U32     lbr_num_regs;
    U32     latency_offset_in_sample;
    U32     latency_size_in_sample;
    U32     latency_size_from_pebs_record;
    U32     latency_offset_in_pebs_record;
    U32     power_offset_in_sample;
    U32     ebc_offset;
    U32     uncore_ebc_offset;
    U32     eventing_ip_offset;
    U32     hle_offset;
    U32     pwr_offset;
    U32     callstack_offset;
    U32     callstack_size;
    U32     p_state_offset;
    U32     pebs_tsc_offset;
    U32     perfmetrics_offset;
    U32     perfmetrics_size;
    /* ----------ADAPTIVE PEBS FIELDS --------- */
    U16     applicable_counters_offset;
    U16     gpr_info_offset;
    U16     gpr_info_size;
    U16     xmm_info_offset;
    U16     xmm_info_size;
    U16     lbr_info_size;
    /*------------------------------------------*/
    U32     reserved2;
    U64     reserved3;
};

//
// Accessor macros for EVENT_DESC node
//
#define EVENT_DESC_sample_size(ec)                        (ec)->sample_size
#define EVENT_DESC_pebs_offset(ec)                        (ec)->pebs_offset
#define EVENT_DESC_pebs_size(ec)                          (ec)->pebs_size
#define EVENT_DESC_lbr_offset(ec)                         (ec)->lbr_offset
#define EVENT_DESC_lbr_num_regs(ec)                       (ec)->lbr_num_regs
#define EVENT_DESC_latency_offset_in_sample(ec)           (ec)->latency_offset_in_sample
#define EVENT_DESC_latency_size_from_pebs_record(ec)      (ec)->latency_size_from_pebs_record
#define EVENT_DESC_latency_offset_in_pebs_record(ec)      (ec)->latency_offset_in_pebs_record
#define EVENT_DESC_latency_size_in_sample(ec)             (ec)->latency_size_in_sample
#define EVENT_DESC_power_offset_in_sample(ec)             (ec)->power_offset_in_sample
#define EVENT_DESC_ebc_offset(ec)                         (ec)->ebc_offset
#define EVENT_DESC_uncore_ebc_offset(ec)                  (ec)->uncore_ebc_offset
#define EVENT_DESC_eventing_ip_offset(ec)                 (ec)->eventing_ip_offset
#define EVENT_DESC_hle_offset(ec)                         (ec)->hle_offset
#define EVENT_DESC_pwr_offset(ec)                         (ec)->pwr_offset
#define EVENT_DESC_callstack_offset(ec)                   (ec)->callstack_offset
#define EVENT_DESC_callstack_size(ec)                     (ec)->callstack_size
#define EVENT_DESC_perfmetrics_offset(ec)                 (ec)->perfmetrics_offset
#define EVENT_DESC_perfmetrics_size(ec)                   (ec)->perfmetrics_size
#define EVENT_DESC_p_state_offset(ec)                     (ec)->p_state_offset
#define EVENT_DESC_pebs_tsc_offset(ec)                    (ec)->pebs_tsc_offset
#define EVENT_DESC_applicable_counters_offset(ec)         (ec)->applicable_counters_offset
#define EVENT_DESC_gpr_info_offset(ec)                    (ec)->gpr_info_offset
#define EVENT_DESC_gpr_info_size(ec)                      (ec)->gpr_info_size
#define EVENT_DESC_xmm_info_offset(ec)                    (ec)->xmm_info_offset
#define EVENT_DESC_xmm_info_size(ec)                      (ec)->xmm_info_size
#define EVENT_DESC_lbr_info_size(ec)                      (ec)->lbr_info_size

// ***************************************************************************

/*!\struct EVENT_CONFIG_NODE
 * \var    num_groups      -  The number of groups being programmed
 * \var    em_mode         -  Is EM valid?  If so how?
 * \var    em_time_slice   -  EM valid?  time slice in milliseconds
 * \var    sample_size     -  size of buffer in bytes to hold the sample + extras
 * \var    max_gp_events   -  Max number of General Purpose events per EM group
 * \var    pebs_offset     -  offset in the sample to locate the pebs capture information
 * \var    lbr_offset      -  offset in the sample to locate the lbr information
 * \var    lbr_num_regs    -  offset in the sample to locate the lbr information
 * \var    latency_offset_in_sample      -  offset in the sample to locate the latency information
 * \var    latency_size_in_sample        -  size of latency records in the sample
 * \var    latency_size_from_pebs_record -  offset in the sample to locate the latency
 *                                          size from pebs record
 * \var    latency_offset_in_pebs_record -  offset in the sample to locate the latency information
 *                                          in pebs record
 * \var    power_offset_in_sample        -  offset in the sample to locate the power information
 * \var    ebc_offset                    -  offset in the sample to locate the ebc count information
 *
 * \var    pwr_offset                    -  offset in the sample to locate the pwr count information
 * \var    p_state_offset                -  offset in the sample to locate the p_state information (APERF/MPERF)
 *
 * \brief  Data structure to describe the events and the mode
 *
 */

typedef struct EVENT_CONFIG_NODE_S  EVENT_CONFIG_NODE;
typedef        EVENT_CONFIG_NODE   *EVENT_CONFIG;

struct EVENT_CONFIG_NODE_S {
    U32     num_groups;
    S32     em_mode;
    S32     em_factor;
    S32     em_event_num;
    U32     sample_size;
    U32     max_gp_events;
    U32     max_fixed_counters;
    U32     max_ro_counters;    // maximum read-only counters
    U32     pebs_offset;
    U32     pebs_size;
    U32     lbr_offset;
    U32     lbr_num_regs;
    U32     latency_offset_in_sample;
    U32     latency_size_in_sample;
    U32     latency_size_from_pebs_record;
    U32     latency_offset_in_pebs_record;
    U32     power_offset_in_sample;
    U32     ebc_offset;
    U32     num_groups_unc;
    U32     ebc_offset_unc;
    U32     sample_size_unc;
    U32     eventing_ip_offset;
    U32     hle_offset;
    U32     pwr_offset;
    U32     callstack_offset;
    U32     callstack_size;
    U32     p_state_offset;
    U32     pebs_tsc_offset;
    U64     reserved1;
    U64     reserved2;
    U64     reserved3;
    U64     reserved4;
};

//
// Accessor macros for EVENT_CONFIG node
//
#define EVENT_CONFIG_num_groups(ec)                         (ec)->num_groups
#define EVENT_CONFIG_mode(ec)                               (ec)->em_mode
#define EVENT_CONFIG_em_factor(ec)                          (ec)->em_factor
#define EVENT_CONFIG_em_event_num(ec)                       (ec)->em_event_num
#define EVENT_CONFIG_sample_size(ec)                        (ec)->sample_size
#define EVENT_CONFIG_max_gp_events(ec)                      (ec)->max_gp_events
#define EVENT_CONFIG_max_fixed_counters(ec)                 (ec)->max_fixed_counters
#define EVENT_CONFIG_max_ro_counters(ec)                    (ec)->max_ro_counters
#define EVENT_CONFIG_pebs_offset(ec)                        (ec)->pebs_offset
#define EVENT_CONFIG_pebs_size(ec)                          (ec)->pebs_size
#define EVENT_CONFIG_lbr_offset(ec)                         (ec)->lbr_offset
#define EVENT_CONFIG_lbr_num_regs(ec)                       (ec)->lbr_num_regs
#define EVENT_CONFIG_latency_offset_in_sample(ec)           (ec)->latency_offset_in_sample
#define EVENT_CONFIG_latency_size_from_pebs_record(ec)      (ec)->latency_size_from_pebs_record
#define EVENT_CONFIG_latency_offset_in_pebs_record(ec)      (ec)->latency_offset_in_pebs_record
#define EVENT_CONFIG_latency_size_in_sample(ec)             (ec)->latency_size_in_sample
#define EVENT_CONFIG_power_offset_in_sample(ec)             (ec)->power_offset_in_sample
#define EVENT_CONFIG_ebc_offset(ec)                         (ec)->ebc_offset
#define EVENT_CONFIG_num_groups_unc(ec)                     (ec)->num_groups_unc
#define EVENT_CONFIG_ebc_offset_unc(ec)                     (ec)->ebc_offset_unc
#define EVENT_CONFIG_sample_size_unc(ec)                    (ec)->sample_size_unc
#define EVENT_CONFIG_eventing_ip_offset(ec)                 (ec)->eventing_ip_offset
#define EVENT_CONFIG_hle_offset(ec)                         (ec)->hle_offset
#define EVENT_CONFIG_pwr_offset(ec)                         (ec)->pwr_offset
#define EVENT_CONFIG_callstack_offset(ec)                   (ec)->callstack_offset
#define EVENT_CONFIG_callstack_size(ec)                     (ec)->callstack_size
#define EVENT_CONFIG_p_state_offset(ec)                     (ec)->p_state_offset
#define EVENT_CONFIG_pebs_tsc_offset(ec)                    (ec)->pebs_tsc_offset

typedef enum {
    UNC_MUX = 1,
    UNC_COUNTER
} UNC_SA_PROG_TYPE;

typedef enum {
    UNC_PCICFG = 1,
    UNC_MMIO,
    UNC_STOP,
    UNC_MEMORY,
    UNC_STATUS
} UNC_SA_CONFIG_TYPE;

typedef enum {
    UNC_MCHBAR = 1,
    UNC_DMIBAR,
    UNC_PCIEXBAR,
    UNC_GTTMMADR,
    UNC_GDXCBAR,
    UNC_CHAPADR,
    UNC_SOCPCI,
    UNC_NPKBAR
} UNC_SA_BAR_TYPE;

typedef enum {
    UNC_OP_READ =  1,
    UNC_OP_WRITE,
    UNC_OP_RMW
} UNC_SA_OPERATION;


typedef enum {
    STATIC_COUNTER = 1,
    FREERUN_COUNTER,
    PROG_FREERUN_COUNTER,
    PROGRAMMABLE_COUNTER
} COUNTER_TYPES;

typedef enum {
    PACKAGE_EVENT = 1,
    MODULE_EVENT,
    THREAD_EVENT,
    SYSTEM_EVENT
} EVENT_SCOPE_TYPES;

typedef enum {
    DEVICE_CORE          = 1,         // CORE DEVICE
    DEVICE_HETERO,
    DEVICE_UNC_CBO       = 10,        // UNCORE DEVICES START
    DEVICE_UNC_HA,
    DEVICE_UNC_IMC,
    DEVICE_UNC_IRP,
    DEVICE_UNC_NCU,
    DEVICE_UNC_PCU,
    DEVICE_UNC_POWER,
    DEVICE_UNC_QPI,
    DEVICE_UNC_R2PCIE,
    DEVICE_UNC_R3QPI,
    DEVICE_UNC_SBOX,
    DEVICE_UNC_GT,
    DEVICE_UNC_UBOX,
    DEVICE_UNC_WBOX,
    DEVICE_UNC_COREI7,
    DEVICE_UNC_CHA,
    DEVICE_UNC_EDC,
    DEVICE_UNC_IIO,
    DEVICE_UNC_M2M,
    DEVICE_UNC_EDRAM,
    DEVICE_UNC_FPGA_CACHE,
    DEVICE_UNC_FPGA_FAB,
    DEVICE_UNC_FPGA_THERMAL,
    DEVICE_UNC_FPGA_POWER,
    DEVICE_UNC_FPGA_GB,
    DEVICE_UNC_MDF,
    DEVICE_UNC_RDT,
    DEVICE_UNC_TELEMETRY  = 150,      // TELEMETRY DEVICE
    DEVICE_UNC_CHAP       = 200,      // CHIPSET DEVICES START
    DEVICE_UNC_GMCH,
    DEVICE_UNC_GFX,
    DEVICE_UNC_SOCPERF    = 300,      // UNCORE VISA DEVICES START
    DEVICE_UNC_HFI_RXE    = 400,      // STL HFI
    DEVICE_UNC_HFI_TXE,
} DEVICE_TYPES;

typedef enum {
    LBR_ENTRY_TOS = 0,
    LBR_ENTRY_FROM_IP,
    LBR_ENTRY_TO_IP,
    LBR_ENTRY_INFO
} LBR_ENTRY_TYPE;


// ***************************************************************************

/*!\struct EVENT_REG_ID_NODE
 * \var    reg_id      -  MSR index to r/w
 * \var    pci_id     PCI based register and its details to operate on
 */
typedef struct EVENT_REG_ID_NODE_S   EVENT_REG_ID_NODE;
typedef        EVENT_REG_ID_NODE    *EVENT_REG_ID;

 struct EVENT_REG_ID_NODE_S {
   U32           reg_id;
   U32           pci_bus_no;
   U32           pci_dev_no;
   U32           pci_func_no;
   U32           data_size;
   U32           bar_index;  // Points to the index (MMIO_INDEX_LIST)
                             // of bar memory map list to be used in mmio_bar_list of ECB
   U32           reserved1;
   U32           reserved2;
   U64           reserved3;
} ;



// ***************************************************************************

typedef enum {
    PMU_REG_RW_READ = 1,
    PMU_REG_RW_WRITE,
    PMU_REG_RW_READ_WRITE,
    PMU_REG_RW_READ_MASK_WRITE,
    PMU_REG_RW_READ_VALIDATE_MASK,
    PMU_REG_RW_READ_MERGE_READ,
} PMU_REG_RW_TYPES;

typedef enum {
    PMU_REG_PROG_MSR = 1,
    PMU_REG_PROG_PCI,
    PMU_REG_PROG_MMIO,
} PMU_REG_PROG_TYPES;

typedef enum {
    PMU_REG_GLOBAL_CTRL = 1,
    PMU_REG_UNIT_CTRL,
    PMU_REG_UNIT_STATUS,
    PMU_REG_DATA,
    PMU_REG_EVENT_SELECT,
    PMU_REG_FILTER,
    PMU_REG_FIXED_CTRL,
} PMU_REG_TYPES;

/*!\struct EVENT_REG_NODE
 * \var    reg_type             - register type
 * \var    event_id_index       - event ID index
 * \var    event_reg_id         - register ID/pci register details
 * \var    desc_id              - desc ID
 * \var    flags                - flags
 * \var    reg_value            - register value
 * \var    max_bits             - max bits
 * \var    scheduled            - boolean to specify if this event node has been scheduled already
 * \var    bus_no               - PCI bus number
 * \var    dev_no               - PCI device number
 * \var    func_no              - PCI function number
 * \var    counter_type         - Event counter type - static/freerun
 * \var    event_scope          - Event scope - package/module/thread
 * \var    reg_prog_type        - Register Programming type
 * \var    reg_rw_type          - Register Read/Write type
 * \var    reg_order            - Register order in the programming sequence
 * \var
 * \brief  Data structure to describe the event registers
 *
 */

typedef struct EVENT_REG_NODE_S  EVENT_REG_NODE;
typedef        EVENT_REG_NODE   *EVENT_REG;

struct EVENT_REG_NODE_S {
    U8                   reg_type;
    U8                   unit_id;
    U16                  event_id_index;
    U16                  counter_event_offset;
    U16                  reserved1;
    EVENT_REG_ID_NODE    event_reg_id;
    U64                  reg_value;
    U16                  desc_id;
    U16                  flags;
    U32                  reserved2;
    U64                  max_bits;
    U8                   scheduled;
    S8                   secondary_pci_offset_shift;
    U16                  secondary_pci_offset_offset; // offset of the offset...
    U32                  counter_type;
    U32                  event_scope;
    U8                   reg_prog_type;
    U8                   reg_rw_type;
    U8                   reg_order;
    U8                   bit_position;
    U64                  secondary_pci_offset_mask;
    U32                  core_event_id;
    U32                  uncore_buffer_offset_in_package;
    U32                  uncore_buffer_offset_in_system;
    U32                  aux_reg_id_to_read;
    U64                  aux_read_mask;
    U64                  reserved3;
    U64                  reserved4;
};

//
// Accessor macros for EVENT_REG node
// Note: the flags field is not directly addressible to prevent hackery
//
#define EVENT_REG_reg_type(x,i)                         (x)[(i)].reg_type
#define EVENT_REG_event_id_index(x,i)                   (x)[(i)].event_id_index
#define EVENT_REG_unit_id(x,i)                          (x)[(i)].unit_id
#define EVENT_REG_counter_event_offset(x,i)             (x)[(i)].counter_event_offset
#define EVENT_REG_reg_id(x,i)                           (x)[(i)].event_reg_id.reg_id
#define EVENT_REG_bus_no(x,i)                           (x)[(i)].event_reg_id.pci_bus_no
#define EVENT_REG_dev_no(x,i)                           (x)[(i)].event_reg_id.pci_dev_no
#define EVENT_REG_func_no(x,i)                          (x)[(i)].event_reg_id.pci_func_no
#define EVENT_REG_offset(x,i)                           (x)[(i)].event_reg_id.reg_id   // points to the reg_id
#define EVENT_REG_data_size(x,i)                        (x)[(i)].event_reg_id.data_size
#define EVENT_REG_bar_index(x,i)                        (x)[(i)].event_reg_id.bar_index
#define EVENT_REG_desc_id(x,i)                          (x)[(i)].desc_id
#define EVENT_REG_flags(x,i)                            (x)[(i)].flags
#define EVENT_REG_reg_value(x,i)                        (x)[(i)].reg_value
#define EVENT_REG_max_bits(x,i)                         (x)[(i)].max_bits
#define EVENT_REG_scheduled(x,i)                        (x)[(i)].scheduled
#define EVENT_REG_secondary_pci_offset_shift(x,i)       (x)[(i)].secondary_pci_offset_shift
#define EVENT_REG_secondary_pci_offset_offset(x,i)      (x)[(i)].secondary_pci_offset_offset
#define EVENT_REG_secondary_pci_offset_mask(x,i)        (x)[(i)].secondary_pci_offset_mask

#define EVENT_REG_counter_type(x,i)                     (x)[(i)].counter_type
#define EVENT_REG_event_scope(x,i)                      (x)[(i)].event_scope
#define EVENT_REG_reg_prog_type(x,i)                    (x)[(i)].reg_prog_type
#define EVENT_REG_reg_rw_type(x,i)                      (x)[(i)].reg_rw_type
#define EVENT_REG_reg_order(x,i)                        (x)[(i)].reg_order
#define EVENT_REG_bit_position(x,i)                     (x)[(i)].bit_position

#define EVENT_REG_core_event_id(x,i)                    (x)[(i)].core_event_id
#define EVENT_REG_uncore_buffer_offset_in_package(x,i)  (x)[(i)].uncore_buffer_offset_in_package
#define EVENT_REG_uncore_buffer_offset_in_system(x,i)   (x)[(i)].uncore_buffer_offset_in_system
#define EVENT_REG_aux_reg_id_to_read(x,i)               (x)[(i)].aux_reg_id_to_read
#define EVENT_REG_aux_read_mask(x,i)                    (x)[(i)].aux_read_mask
#define EVENT_REG_aux_shift_index(x,i)                  (x)[(i)].bit_position       // Alias

//
// Config bits
//
#define EVENT_REG_precise_bit               0x00000001
#define EVENT_REG_global_bit                0x00000002
#define EVENT_REG_uncore_bit                0x00000004
#define EVENT_REG_uncore_q_rst_bit          0x00000008
#define EVENT_REG_latency_bit               0x00000010
#define EVENT_REG_is_gp_reg_bit             0x00000020
#define EVENT_REG_clean_up_bit              0x00000040
#define EVENT_REG_em_trigger_bit            0x00000080
#define EVENT_REG_lbr_value_bit             0x00000100
#define EVENT_REG_fixed_reg_bit             0x00000200
#define EVENT_REG_unc_evt_intr_read_bit     0x00000400
#define EVENT_REG_multi_pkg_evt_bit         0x00001000
#define EVENT_REG_branch_evt_bit            0x00002000
#define EVENT_REG_ebc_sampling_evt_bit      0x00004000
#define EVENT_REG_collect_on_ctx_sw         0x00008000
//
// Accessor macros for config bits
//
#define EVENT_REG_precise_get(x,i)          ((x)[(i)].flags &   EVENT_REG_precise_bit)
#define EVENT_REG_precise_set(x,i)          ((x)[(i)].flags |=  EVENT_REG_precise_bit)
#define EVENT_REG_precise_clear(x,i)        ((x)[(i)].flags &= ~EVENT_REG_precise_bit)

#define EVENT_REG_global_get(x,i)           ((x)[(i)].flags &   EVENT_REG_global_bit)
#define EVENT_REG_global_set(x,i)           ((x)[(i)].flags |=  EVENT_REG_global_bit)
#define EVENT_REG_global_clear(x,i)         ((x)[(i)].flags &= ~EVENT_REG_global_bit)

#define EVENT_REG_uncore_get(x,i)           ((x)[(i)].flags &   EVENT_REG_uncore_bit)
#define EVENT_REG_uncore_set(x,i)           ((x)[(i)].flags |=  EVENT_REG_uncore_bit)
#define EVENT_REG_uncore_clear(x,i)         ((x)[(i)].flags &= ~EVENT_REG_uncore_bit)

#define EVENT_REG_uncore_q_rst_get(x,i)     ((x)[(i)].flags &   EVENT_REG_uncore_q_rst_bit)
#define EVENT_REG_uncore_q_rst_set(x,i)     ((x)[(i)].flags |=  EVENT_REG_uncore_q_rst_bit)
#define EVENT_REG_uncore_q_rst_clear(x,i)   ((x)[(i)].flags &= ~EVENT_REG_uncore_q_rst_bit)

#define EVENT_REG_latency_get(x,i)          ((x)[(i)].flags &   EVENT_REG_latency_bit)
#define EVENT_REG_latency_set(x,i)          ((x)[(i)].flags |=  EVENT_REG_latency_bit)
#define EVENT_REG_latency_clear(x,i)        ((x)[(i)].flags &= ~EVENT_REG_latency_bit)

#define EVENT_REG_is_gp_reg_get(x,i)        ((x)[(i)].flags &   EVENT_REG_is_gp_reg_bit)
#define EVENT_REG_is_gp_reg_set(x,i)        ((x)[(i)].flags |=  EVENT_REG_is_gp_reg_bit)
#define EVENT_REG_is_gp_reg_clear(x,i)      ((x)[(i)].flags &= ~EVENT_REG_is_gp_reg_bit)

#define EVENT_REG_lbr_value_get(x,i)        ((x)[(i)].flags &   EVENT_REG_lbr_value_bit)
#define EVENT_REG_lbr_value_set(x,i)        ((x)[(i)].flags |=  EVENT_REG_lbr_value_bit)
#define EVENT_REG_lbr_value_clear(x,i)      ((x)[(i)].flags &= ~EVENT_REG_lbr_value_bit)

#define EVENT_REG_fixed_reg_get(x,i)        ((x)[(i)].flags &   EVENT_REG_fixed_reg_bit)
#define EVENT_REG_fixed_reg_set(x,i)        ((x)[(i)].flags |=  EVENT_REG_fixed_reg_bit)
#define EVENT_REG_fixed_reg_clear(x,i)      ((x)[(i)].flags &= ~EVENT_REG_fixed_reg_bit)

#define EVENT_REG_multi_pkg_evt_bit_get(x,i)   ((x)[(i)].flags &   EVENT_REG_multi_pkg_evt_bit)
#define EVENT_REG_multi_pkg_evt_bit_set(x,i)   ((x)[(i)].flags |=  EVENT_REG_multi_pkg_evt_bit)
#define EVENT_REG_multi_pkg_evt_bit_clear(x,i) ((x)[(i)].flags &= ~EVENT_REG_multi_pkg_evt_bit)

#define EVENT_REG_clean_up_get(x,i)         ((x)[(i)].flags &   EVENT_REG_clean_up_bit)
#define EVENT_REG_clean_up_set(x,i)         ((x)[(i)].flags |=  EVENT_REG_clean_up_bit)
#define EVENT_REG_clean_up_clear(x,i)       ((x)[(i)].flags &= ~EVENT_REG_clean_up_bit)

#define EVENT_REG_em_trigger_get(x,i)       ((x)[(i)].flags &   EVENT_REG_em_trigger_bit)
#define EVENT_REG_em_trigger_set(x,i)       ((x)[(i)].flags |=  EVENT_REG_em_trigger_bit)
#define EVENT_REG_em_trigger_clear(x,i)     ((x)[(i)].flags &= ~EVENT_REG_em_trigger_bit)

#define EVENT_REG_branch_evt_get(x,i)       ((x)[(i)].flags &   EVENT_REG_branch_evt_bit)
#define EVENT_REG_branch_evt_set(x,i)       ((x)[(i)].flags |=  EVENT_REG_branch_evt_bit)
#define EVENT_REG_branch_evt_clear(x,i)     ((x)[(i)].flags &= ~EVENT_REG_branch_evt_bit)

#define EVENT_REG_ebc_sampling_evt_get(x,i)   ((x)[(i)].flags &   EVENT_REG_ebc_sampling_evt_bit)
#define EVENT_REG_ebc_sampling_evt_set(x,i)   ((x)[(i)].flags |=  EVENT_REG_ebc_sampling_evt_bit)
#define EVENT_REG_ebc_sampling_evt_clear(x,i) ((x)[(i)].flags &= ~EVENT_REG_ebc_sampling_evt_bit)

#define EVENT_REG_collect_on_ctx_sw_get(x,i)   ((x)[(i)].flags &   EVENT_REG_collect_on_ctx_sw)
#define EVENT_REG_collect_on_ctx_sw_set(x,i)   ((x)[(i)].flags |=  EVENT_REG_collect_on_ctx_sw)
#define EVENT_REG_collect_on_ctx_sw_clear(x,i) ((x)[(i)].flags &= ~EVENT_REG_collect_on_ctx_sw)

#define EVENT_REG_unc_evt_intr_read_get(x,i)   ((x)[(i)].flags &  EVENT_REG_unc_evt_intr_read_bit)
#define EVENT_REG_unc_evt_intr_read_set(x,i)   ((x)[(i)].flags |=  EVENT_REG_unc_evt_intr_read_bit)
#define EVENT_REG_unc_evt_intr_read_clear(x,i) ((x)[(i)].flags &= ~EVENT_REG_unc_evt_intr_read_bit)

// ***************************************************************************

/*!\struct DRV_PCI_DEVICE_ENTRY_NODE_S
 * \var    bus_no          -  PCI bus no to read
 * \var    dev_no          -  PCI device no to read
 * \var    func_no            PCI device no to read
 * \var    bar_offset         BASE Address Register offset of the PCI based PMU
 * \var    bit_offset         Bit offset of the same
 * \var    size               size of read/write
 * \var    bar_address        the actual BAR present
 * \var    enable_offset      Offset info to enable/disable
 * \var    enabled            Status of enable/disable
 * \brief  Data structure to describe the PCI Device
 *
 */

typedef struct DRV_PCI_DEVICE_ENTRY_NODE_S  DRV_PCI_DEVICE_ENTRY_NODE;
typedef        DRV_PCI_DEVICE_ENTRY_NODE   *DRV_PCI_DEVICE_ENTRY;

struct DRV_PCI_DEVICE_ENTRY_NODE_S {
    U32        bus_no;
    U32        dev_no;
    U32        func_no;
    U32        bar_offset;
    U64        bar_mask;
    U32        bit_offset;
    U32        size;
    U64        bar_address;
    U32        enable_offset;
    U32        enabled;
    U32        base_offset_for_mmio;
    U32        operation;
    U32        bar_name;
    U32        prog_type;
    U32        config_type;
    S8         bar_shift;     // positive shifts right, negative shifts left
    U8         reserved0;
    U16        reserved1;
    U64        value;
    U64        mask;
    U64        virtual_address;
    U32        port_id;
    U32        op_code;
    U32        device_id;
    U16        bar_num;
    U16        feature_id;
    U64        reserved2;
    U64        reserved3;
    U64        reserved4;
};

//
// Accessor macros for DRV_PCI_DEVICE_NODE node
//
#define DRV_PCI_DEVICE_ENTRY_bus_no(x)                (x)->bus_no
#define DRV_PCI_DEVICE_ENTRY_dev_no(x)                (x)->dev_no
#define DRV_PCI_DEVICE_ENTRY_func_no(x)               (x)->func_no
#define DRV_PCI_DEVICE_ENTRY_bar_offset(x)            (x)->bar_offset
#define DRV_PCI_DEVICE_ENTRY_bar_mask(x)              (x)->bar_mask
#define DRV_PCI_DEVICE_ENTRY_bit_offset(x)            (x)->bit_offset
#define DRV_PCI_DEVICE_ENTRY_size(x)                  (x)->size
#define DRV_PCI_DEVICE_ENTRY_bar_address(x)           (x)->bar_address
#define DRV_PCI_DEVICE_ENTRY_enable_offset(x)         (x)->enable_offset
#define DRV_PCI_DEVICE_ENTRY_enable(x)                (x)->enabled
#define DRV_PCI_DEVICE_ENTRY_base_offset_for_mmio(x)  (x)->base_offset_for_mmio
#define DRV_PCI_DEVICE_ENTRY_operation(x)             (x)->operation
#define DRV_PCI_DEVICE_ENTRY_bar_name(x)              (x)->bar_name
#define DRV_PCI_DEVICE_ENTRY_prog_type(x)             (x)->prog_type
#define DRV_PCI_DEVICE_ENTRY_config_type(x)           (x)->config_type
#define DRV_PCI_DEVICE_ENTRY_bar_shift(x)             (x)->bar_shift
#define DRV_PCI_DEVICE_ENTRY_value(x)                 (x)->value
#define DRV_PCI_DEVICE_ENTRY_mask(x)                  (x)->mask
#define DRV_PCI_DEVICE_ENTRY_virtual_address(x)       (x)->virtual_address
#define DRV_PCI_DEVICE_ENTRY_port_id(x)               (x)->port_id
#define DRV_PCI_DEVICE_ENTRY_op_code(x)               (x)->op_code
#define DRV_PCI_DEVICE_ENTRY_device_id(x)             (x)->device_id
#define DRV_PCI_DEVICE_ENTRY_bar_num(x)               (x)->bar_num
#define DRV_PCI_DEVICE_ENTRY_feature_id(x)            (x)->feature_id

// ***************************************************************************
typedef enum {
    PMU_OPERATION_INITIALIZE = 0,
    PMU_OPERATION_WRITE,
    PMU_OPERATION_ENABLE,
    PMU_OPERATION_DISABLE,
    PMU_OPERATION_READ,
    PMU_OPERATION_CLEANUP,
    PMU_OPERATION_READ_LBRS,
    PMU_OPERATION_GLOBAL_REGS,
    PMU_OPERATION_CTRL_GP,
    PMU_OPERATION_DATA_FIXED,
    PMU_OPERATION_DATA_GP,
    PMU_OPERATION_OCR,
    PMU_OPERATION_HW_ERRATA,
    PMU_OPERATION_CHECK_OVERFLOW_GP_ERRATA,
    PMU_OPERATION_CHECK_OVERFLOW_ERRATA,
    PMU_OPERATION_ALL_REG,
    PMU_OPERATION_DATA_ALL,
    PMU_OPERATION_GLOBAL_STATUS,
    PMU_OPERATION_METRICS,
} PMU_OPERATION_TYPES;
#define MAX_OPERATION_TYPES   32

/*!\struct PMU_OPERATIONS_NODE
 * \var    operation_type -    Type of operation from enumeration PMU_OPERATION_TYPES
 * \var    register_start -    Start index of the registers for a specific operation
 * \var    register_len   -    Number of registers for a specific operation
 *
 * \brief
 * Structure for defining start and end indices in the ECB entries array for
 * each type of operation performed in the driver
 * initialize, write, read, enable, disable, etc.
 */
typedef struct PMU_OPERATIONS_NODE_S  PMU_OPERATIONS_NODE;
typedef        PMU_OPERATIONS_NODE   *PMU_OPERATIONS;
struct PMU_OPERATIONS_NODE_S {
    U32 operation_type;
    U32 register_start;
    U32 register_len;
    U32 reserved1;
    U32 reserved2;
    U32 reserved3;
};
#define PMU_OPERATIONS_operation_type(x)             (x)->operation_type
#define PMU_OPERATIONS_register_start(x)             (x)->register_start
#define PMU_OPERATIONS_register_len(x)               (x)->register_len
#define PMU_OPER_operation_type(x,i)                 (x)[(i)].operation_type
#define PMU_OPER_register_start(x,i)                 (x)[(i)].register_start
#define PMU_OPER_register_len(x,i)                   (x)[(i)].register_len

typedef enum {
    ECB_MMIO_BAR1    = 1,
    ECB_MMIO_BAR2    = 2,
    ECB_MMIO_BAR3    = 3,
    ECB_MMIO_BAR4    = 4,
    ECB_MMIO_BAR5    = 5,
    ECB_MMIO_BAR6    = 6,
    ECB_MMIO_BAR7    = 7,
    ECB_MMIO_BAR8    = 8,
} MMIO_INDEX_LIST;
#define MAX_MMIO_BARS   8


/*!\struct MMIO_BAR_INFO_NODE
 */
typedef struct MMIO_BAR_INFO_NODE_S   MMIO_BAR_INFO_NODE;
typedef        MMIO_BAR_INFO_NODE    *MMIO_BAR_INFO;

 struct MMIO_BAR_INFO_NODE_S {
   U32           bus_no;
   U32           dev_no;
   U32           func_no;
   U32           addr_size;
   U64           base_offset_for_mmio;
   U32           map_size_for_mmio;
   U32           main_bar_offset;
   U64           main_bar_mask;
   U8            main_bar_shift;
   U8            reserved1;
   U16           reserved2;
   U32           reserved3;
   U32           reserved4;
   U32           secondary_bar_offset;
   U64           secondary_bar_mask;
   U8            secondary_bar_shift;
   U8            reserved5;
   U16           reserved6;
   U32           reserved7;
   U16           feature_id;
   U16           reserved8;
   U32           reserved9;
   U64           reserved10;
} ;

#define MMIO_BAR_INFO_bus_no(x)                 (x)->bus_no
#define MMIO_BAR_INFO_dev_no(x)                 (x)->dev_no
#define MMIO_BAR_INFO_func_no(x)                (x)->func_no
#define MMIO_BAR_INFO_addr_size(x)              (x)->addr_size
#define MMIO_BAR_INFO_base_offset_for_mmio(x)   (x)->base_offset_for_mmio
#define MMIO_BAR_INFO_map_size_for_mmio(x)      (x)->map_size_for_mmio
#define MMIO_BAR_INFO_main_bar_offset(x)        (x)->main_bar_offset
#define MMIO_BAR_INFO_main_bar_mask(x)          (x)->main_bar_mask
#define MMIO_BAR_INFO_main_bar_shift(x)         (x)->main_bar_shift
#define MMIO_BAR_INFO_secondary_bar_offset(x)   (x)->secondary_bar_offset
#define MMIO_BAR_INFO_secondary_bar_mask(x)     (x)->secondary_bar_mask
#define MMIO_BAR_INFO_secondary_bar_shift(x)    (x)->secondary_bar_shift
#define MMIO_BAR_INFO_physical_address(x)       (x)->physical_address
#define MMIO_BAR_INFO_virtual_address(x)        (x)->virtual_address
#define MMIO_BAR_INFO_feature_id(x)             (x)->feature_id

/*!\struct ECB_NODE_S
 * \var    num_entries -       Total number of entries in "entries".
 * \var    group_id    -       Group ID.
 * \var    num_events  -       Number of events in this group.
 * \var    cccr_start  -       Starting index of counter configuration control registers in "entries".
 * \var    cccr_pop    -       Number of counter configuration control registers in "entries".
 * \var    escr_start  -       Starting index of event selection control registers in "entries".
 * \var    escr_pop    -       Number of event selection control registers in "entries".
 * \var    data_start  -       Starting index of data registers in "entries".
 * \var    data_pop    -       Number of data registers in "entries".
 * \var    pcidev_entry_node   PCI device details for one device
 * \var    entries     - .     All the register nodes required for programming
 *
 * \brief
 */

typedef struct ECB_NODE_S  ECB_NODE;
typedef        ECB_NODE   *ECB;

struct ECB_NODE_S {
    U8                           version;
    U8                           reserved1;
    U16                          reserved2;
    U32                          num_entries;
    U32                          group_id;
    U32                          num_events;
    U32                          cccr_start;
    U32                          cccr_pop;
    U32                          escr_start;
    U32                          escr_pop;
    U32                          data_start;
    U32                          data_pop;
    U16                          flags;
    U8                           pmu_timer_interval;
    U8                           reserved3;
    U32                          size_of_allocation;
    U32                          group_offset;
    U32                          reserved4;
    DRV_PCI_DEVICE_ENTRY_NODE    pcidev_entry_node;
    U32                          num_pci_devices;
    U32                          pcidev_list_offset;
    DRV_PCI_DEVICE_ENTRY         pcidev_entry_list;
#if defined(DRV_IA32)
    U32                          pointer_padding; //add padding for the DRV_PCI_DEVICE_ENTRY pointer
                                                  //before this field for 32-bit mode
#endif
    U32                          device_type;
    U32                          dev_node;
    PMU_OPERATIONS_NODE          operations[MAX_OPERATION_TYPES];
    U32                          descriptor_id;
    U32                          reserved5;
    U32                          metric_start;
    U32                          metric_pop;
    MMIO_BAR_INFO_NODE           mmio_bar_list[MAX_MMIO_BARS];
    U32                          num_mmio_secondary_bar;
    U32                          group_id_offset_in_trigger_evt_desc;
    U64                          reserved7;
    U64                          reserved8;
    EVENT_REG_NODE               entries[];
};

//
// Accessor macros for ECB node
//
#define ECB_version(x)                                (x)->version
#define ECB_num_entries(x)                            (x)->num_entries
#define ECB_group_id(x)                               (x)->group_id
#define ECB_num_events(x)                             (x)->num_events
#define ECB_cccr_start(x)                             (x)->cccr_start
#define ECB_cccr_pop(x)                               (x)->cccr_pop
#define ECB_escr_start(x)                             (x)->escr_start
#define ECB_escr_pop(x)                               (x)->escr_pop
#define ECB_data_start(x)                             (x)->data_start
#define ECB_data_pop(x)                               (x)->data_pop
#define ECB_metric_start(x)                           (x)->metric_start
#define ECB_metric_pop(x)                             (x)->metric_pop
#define ECB_pcidev_entry_node(x)                      (x)->pcidev_entry_node
#define ECB_num_pci_devices(x)                        (x)->num_pci_devices
#define ECB_pcidev_list_offset(x)                     (x)->pcidev_list_offset
#define ECB_pcidev_entry_list(x)                      (x)->pcidev_entry_list
#define ECB_flags(x)                                  (x)->flags
#define ECB_pmu_timer_interval(x)                     (x)->pmu_timer_interval
#define ECB_size_of_allocation(x)                     (x)->size_of_allocation
#define ECB_group_offset(x)                           (x)->group_offset
#define ECB_device_type(x)                            (x)->device_type
#define ECB_dev_node(x)                               (x)->dev_node
#define ECB_operations(x)                             (x)->operations
#define ECB_descriptor_id(x)                          (x)->descriptor_id
#define ECB_entries(x)                                (x)->entries
#define ECB_num_mmio_secondary_bar(x)                 (x)->num_mmio_secondary_bar
#define ECB_mmio_bar_list(x, i)                       (x)->mmio_bar_list[i]
#define ECB_group_id_offset_in_trigger_evt_desc(x)    (x)->group_id_offset_in_trigger_evt_desc

// for flag bit field
#define ECB_direct2core_bit                0x0001
#define ECB_bl_bypass_bit                  0x0002
#define ECB_pci_id_offset_bit              0x0003
#define ECB_pcu_ccst_debug                 0x0004

#define ECB_VERSION                        2

#define ECB_CONSTRUCT(x,num_entries,group_id,cccr_start,escr_start,data_start, size_of_allocation)    \
                                           ECB_num_entries((x)) = (num_entries);  \
                                           ECB_group_id((x)) = (group_id);        \
                                           ECB_cccr_start((x)) = (cccr_start);    \
                                           ECB_cccr_pop((x)) = 0;                 \
                                           ECB_escr_start((x)) = (escr_start);    \
                                           ECB_escr_pop((x)) = 0;                 \
                                           ECB_data_start((x)) = (data_start);    \
                                           ECB_data_pop((x)) = 0;                 \
                                           ECB_metric_start((x)) = 0;             \
                                           ECB_metric_pop((x)) = 0;               \
                                           ECB_num_pci_devices((x)) = 0;          \
                                           ECB_version((x)) = ECB_VERSION;        \
                                           ECB_size_of_allocation((x)) = (size_of_allocation);

#define ECB_CONSTRUCT2(x, num_entries, group_id, size_of_allocation)    \
                                           ECB_num_entries((x)) = (num_entries);  \
                                           ECB_group_id((x)) = (group_id);        \
                                           ECB_num_pci_devices((x)) = 0;          \
                                           ECB_version((x)) = ECB_VERSION;        \
                                           ECB_size_of_allocation((x)) = (size_of_allocation);

#define ECB_CONSTRUCT1(x,num_entries,group_id,cccr_start,escr_start,data_start,num_pci_devices, size_of_allocation)    \
                                           ECB_num_entries((x)) = (num_entries);  \
                                           ECB_group_id((x)) = (group_id);        \
                                           ECB_cccr_start((x)) = (cccr_start);    \
                                           ECB_cccr_pop((x)) = 0;                 \
                                           ECB_escr_start((x)) = (escr_start);    \
                                           ECB_escr_pop((x)) = 0;                 \
                                           ECB_data_start((x)) = (data_start);    \
                                           ECB_data_pop((x)) = 0;                 \
                                           ECB_metric_start((x)) = 0;             \
                                           ECB_metric_pop((x)) = 0;               \
                                           ECB_num_pci_devices((x)) = (num_pci_devices);  \
                                           ECB_version((x)) = ECB_VERSION;        \
                                           ECB_size_of_allocation((x)) = (size_of_allocation);

//
// Accessor macros for ECB node entries
//
#define ECB_entries_reg_type(x,i)                    EVENT_REG_reg_type((ECB_entries(x)),(i))
#define ECB_entries_event_id_index(x,i)              EVENT_REG_event_id_index((ECB_entries(x)),(i))
#define ECB_entries_unit_id(x,i)                     EVENT_REG_unit_id((ECB_entries(x)),(i))
#define ECB_entries_counter_event_offset(x,i)        EVENT_REG_counter_event_offset((ECB_entries(x)),(i))
#define ECB_entries_reg_id(x,i)                      EVENT_REG_reg_id((ECB_entries(x)),(i))
#define ECB_entries_reg_prog_type(x,i)               EVENT_REG_reg_prog_type((ECB_entries(x)),(i))
#define ECB_entries_reg_offset(x,i)                  EVENT_REG_offset((ECB_entries(x)),(i))
#define ECB_entries_reg_data_size(x,i)               EVENT_REG_data_size((ECB_entries(x)),(i))
#define ECB_entries_reg_bar_index(x,i)               EVENT_REG_bar_index((ECB_entries(x)),(i))
#define ECB_entries_desc_id(x,i)                     EVENT_REG_desc_id((ECB_entries(x)),i)
#define ECB_entries_flags(x,i)                       EVENT_REG_flags((ECB_entries(x)),i)
#define ECB_entries_reg_order(x,i)                   EVENT_REG_reg_order((ECB_entries(x)),i)
#define ECB_entries_reg_value(x,i)                   EVENT_REG_reg_value((ECB_entries(x)),(i))
#define ECB_entries_max_bits(x,i)                    EVENT_REG_max_bits((ECB_entries(x)),(i))
#define ECB_entries_scheduled(x,i)                   EVENT_REG_scheduled((ECB_entries(x)),(i))
#define ECB_entries_counter_event_offset(x,i)        EVENT_REG_counter_event_offset((ECB_entries(x)),(i))
#define ECB_entries_bit_position(x,i)                EVENT_REG_bit_position((ECB_entries(x)),(i))
// PCI config-specific fields
#define ECB_entries_bus_no(x,i)                      EVENT_REG_bus_no((ECB_entries(x)),(i))
#define ECB_entries_dev_no(x,i)                      EVENT_REG_dev_no((ECB_entries(x)),(i))
#define ECB_entries_func_no(x,i)                     EVENT_REG_func_no((ECB_entries(x)),(i))
#define ECB_entries_counter_type(x,i)                EVENT_REG_counter_type((ECB_entries(x)),(i))
#define ECB_entries_event_scope(x,i)                 EVENT_REG_event_scope((ECB_entries(x)),(i))
#define ECB_entries_precise_get(x,i)                 EVENT_REG_precise_get((ECB_entries(x)),(i))
#define ECB_entries_global_get(x,i)                  EVENT_REG_global_get((ECB_entries(x)),(i))
#define ECB_entries_uncore_get(x,i)                  EVENT_REG_uncore_get((ECB_entries(x)),(i))
#define ECB_entries_uncore_q_rst_get(x,i)            EVENT_REG_uncore_q_rst_get((ECB_entries(x)),(i))
#define ECB_entries_is_gp_reg_get(x,i)               EVENT_REG_is_gp_reg_get((ECB_entries(x)),(i))
#define ECB_entries_lbr_value_get(x,i)               EVENT_REG_lbr_value_get((ECB_entries(x)),(i))
#define ECB_entries_fixed_reg_get(x,i)               EVENT_REG_fixed_reg_get((ECB_entries(x)),(i))
#define ECB_entries_is_multi_pkg_bit_set(x,i)        EVENT_REG_multi_pkg_evt_bit_get((ECB_entries(x)),(i))
#define ECB_entries_clean_up_get(x,i)                EVENT_REG_clean_up_get((ECB_entries(x)),(i))
#define ECB_entries_em_trigger_get(x,i)              EVENT_REG_em_trigger_get((ECB_entries(x)),(i))
#define ECB_entries_branch_evt_get(x,i)              EVENT_REG_branch_evt_get((ECB_entries(x)),(i))
#define ECB_entries_ebc_sampling_evt_get(x,i)        EVENT_REG_ebc_sampling_evt_get((ECB_entries(x)),(i))
#define ECB_entries_unc_evt_intr_read_get(x,i)       EVENT_REG_unc_evt_intr_read_get((ECB_entries(x)),(i))
#define ECB_entries_reg_rw_type(x,i)                 EVENT_REG_reg_rw_type((ECB_entries(x)),(i))
#define ECB_entries_collect_on_ctx_sw_get(x,i)       EVENT_REG_collect_on_ctx_sw_get((ECB_entries(x)),(i))
#define ECB_entries_secondary_pci_offset_offset(x,i) EVENT_REG_secondary_pci_offset_offset((ECB_entries(x)),(i))
#define ECB_entries_secondary_pci_offset_shift(x,i)  EVENT_REG_secondary_pci_offset_shift((ECB_entries(x)),(i))
#define ECB_entries_secondary_pci_offset_mask(x,i)   EVENT_REG_secondary_pci_offset_mask((ECB_entries(x)),(i))
#define ECB_operations_operation_type(x,i)           PMU_OPER_operation_type((ECB_operations(x)),(i))
#define ECB_operations_register_start(x,i)           PMU_OPER_register_start((ECB_operations(x)),(i))
#define ECB_operations_register_len(x,i)             PMU_OPER_register_len((ECB_operations(x)),(i))

#define ECB_entries_core_event_id(x,i)                      EVENT_REG_core_event_id((ECB_entries(x)),(i))
#define ECB_entries_uncore_buffer_offset_in_package(x,i)    EVENT_REG_uncore_buffer_offset_in_package((ECB_entries(x)),(i))
#define ECB_entries_uncore_buffer_offset_in_system(x,i)     EVENT_REG_uncore_buffer_offset_in_system((ECB_entries(x)),(i))

#define ECB_entries_aux_reg_id_to_read(x,i)             EVENT_REG_aux_reg_id_to_read((ECB_entries(x)),(i))
#define ECB_entries_aux_read_mask(x,i)                  EVENT_REG_aux_read_mask((ECB_entries(x)),(i))
#define ECB_entries_aux_shift_index(x,i)                EVENT_REG_aux_shift_index((ECB_entries(x)),(i))
#define ECB_SET_OPERATIONS(x, operation_type, start, len)                                \
                     ECB_operations_operation_type(x, operation_type) = operation_type;  \
                     ECB_operations_register_start(x, operation_type) = start;           \
                     ECB_operations_register_len(x, operation_type)   = len;             \

#define ECB_mmio_bar_list_bus_no(x,i)                MMIO_BAR_bus_no(ECB_mmio_bar_list(x),(i))
#define ECB_mmio_bar_list_dev_no(x,i)                MMIO_BAR_dev_no(ECB_mmio_bar_list(x),(i))
#define ECB_mmio_bar_list_func_no(x,i)               MMIO_BAR_func_no(ECB_mmio_bar_list(x),(i))
#define ECB_mmio_bar_list_offset(x,i)                MMIO_BAR_offset(ECB_mmio_bar_list(x),(i))
#define ECB_mmio_bar_list_addr_size(x,i)             MMIO_BAR_addr_size(ECB_mmio_bar_list(x),(i))
#define ECB_mmio_bar_list_map_size(x,i)              MMIO_BAR_map_size(ECB_mmio_bar_list(x),(i))
#define ECB_mmio_bar_list_bar_shift(x,i)             MMIO_BAR_bar_shift(ECB_mmio_bar_list(x),(i))
#define ECB_mmio_bar_list_bar_mask(x,i)              MMIO_BAR_bar_mask(ECB_mmio_bar_list(x),(i))
#define ECB_mmio_bar_list_base_mmio_offset(x,i)      MMIO_BAR_base_mmio_offset(ECB_mmio_bar_list(x),(i))
#define ECB_mmio_bar_list_physical_address(x,i)      MMIO_BAR_physical_address(ECB_mmio_bar_list(x),(i))
#define ECB_mmio_bar_list_virtual_address(x,i)       MMIO_BAR_virtual_address(ECB_mmio_bar_list(x),(i))

// ***************************************************************************

/*!\struct  LBR_ENTRY_NODE_S
 * \var     etype       TOS = 0; FROM = 1; TO = 2
 * \var     type_index
 * \var     reg_id
 */

typedef struct LBR_ENTRY_NODE_S  LBR_ENTRY_NODE;
typedef        LBR_ENTRY_NODE   *LBR_ENTRY;

struct LBR_ENTRY_NODE_S {
    U16    etype;
    U16    type_index;
    U32    reg_id;
};

//
// Accessor macros for LBR entries
//
#define LBR_ENTRY_NODE_etype(lentry)          (lentry).etype
#define LBR_ENTRY_NODE_type_index(lentry)     (lentry).type_index
#define LBR_ENTRY_NODE_reg_id(lentry)         (lentry).reg_id

// ***************************************************************************

/*!\struct LBR_NODE_S
 * \var    num_entries     -  The number of entries
 * \var    entries         -  The entries in the list
 *
 * \brief  Data structure to describe the LBR registers that need to be read
 *
 */

typedef struct LBR_NODE_S  LBR_NODE;
typedef        LBR_NODE   *LBR;

struct LBR_NODE_S {
    U32               size;
    U32               num_entries;
    LBR_ENTRY_NODE    entries[];
};

//
// Accessor macros for LBR node
//
#define LBR_size(lbr)                      (lbr)->size
#define LBR_num_entries(lbr)               (lbr)->num_entries
#define LBR_entries_etype(lbr,idx)         (lbr)->entries[idx].etype
#define LBR_entries_type_index(lbr,idx)    (lbr)->entries[idx].type_index
#define LBR_entries_reg_id(lbr,idx)        (lbr)->entries[idx].reg_id

// ***************************************************************************

/*!\struct  PWR_ENTRY_NODE_S
 * \var     etype       none as yet
 * \var     type_index
 * \var     reg_id
 */

typedef struct PWR_ENTRY_NODE_S  PWR_ENTRY_NODE;
typedef        PWR_ENTRY_NODE   *PWR_ENTRY;

struct PWR_ENTRY_NODE_S {
    U16    etype;
    U16    type_index;
    U32    reg_id;
};

//
// Accessor macros for PWR entries
//
#define PWR_ENTRY_NODE_etype(lentry)          (lentry).etype
#define PWR_ENTRY_NODE_type_index(lentry)     (lentry).type_index
#define PWR_ENTRY_NODE_reg_id(lentry)         (lentry).reg_id

// ***************************************************************************

/*!\struct PWR_NODE_S
 * \var    num_entries     -  The number of entries
 * \var    entries         -  The entries in the list
 *
 * \brief  Data structure to describe the PWR registers that need to be read
 *
 */

typedef struct PWR_NODE_S  PWR_NODE;
typedef        PWR_NODE   *PWR;

struct PWR_NODE_S {
    U32               size;
    U32               num_entries;
    PWR_ENTRY_NODE    entries[];
};

//
// Accessor macros for PWR node
//
#define PWR_size(lbr)                      (lbr)->size
#define PWR_num_entries(lbr)               (lbr)->num_entries
#define PWR_entries_etype(lbr,idx)         (lbr)->entries[idx].etype
#define PWR_entries_type_index(lbr,idx)    (lbr)->entries[idx].type_index
#define PWR_entries_reg_id(lbr,idx)        (lbr)->entries[idx].reg_id

// ***************************************************************************

/*!\struct  RO_ENTRY_NODE_S
 * \var     type       - DEAR, IEAR, BTB.
 */

typedef struct RO_ENTRY_NODE_S  RO_ENTRY_NODE;
typedef        RO_ENTRY_NODE   *RO_ENTRY;

struct RO_ENTRY_NODE_S {
    U32    reg_id;
};

//
// Accessor macros for RO entries
//
#define RO_ENTRY_NODE_reg_id(lentry)       (lentry).reg_id

// ***************************************************************************

/*!\struct RO_NODE_S
 * \var    size            - The total size including header and entries.
 * \var    num_entries     - The number of entries.
 * \var    entries         - The entries in the list.
 *
 * \brief  Data structure to describe the RO registers that need to be read.
 *
 */

typedef struct RO_NODE_S  RO_NODE;
typedef        RO_NODE   *RO;

struct RO_NODE_S {
    U32              size;
    U32              num_entries;
    RO_ENTRY_NODE    entries[];
};

//
// Accessor macros for RO node
//
#define RO_size(ro)                      (ro)->size
#define RO_num_entries(ro)               (ro)->num_entries
#define RO_entries_reg_id(ro,idx)        (ro)->entries[idx].reg_id

#if defined(__cplusplus)
}
#endif

#endif

