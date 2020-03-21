#
#    Copyright (C) 2019-2020 Intel Corporation.  All Rights Reserved.
#
#Permission is hereby granted, free of charge, to any person obtaining a copy
#of this software and associated documentation files (the "Software"), to deal
#in the Software without restriction, including without limitation the rights
#to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
#copies of the Software, and to permit persons to whom the Software is
#furnished to do so, subject to the following conditions:
#
#The above copyright notice and this permission notice shall be included in all
#copies or substantial portions of the Software.
#
#THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
#AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
#OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
#SOFTWARE.
#
#
#
#
#
#
#

import os
import ctypes

from collections import OrderedDict
from copy import deepcopy

# U64 ctypes.c_ulonglong
# U32 ctypes.c_uint
# U16 ctypes.c_ushort
# U8  ctypes.c_ubyte
# DRV_BOOL ctypes.c_uint
# S8 ctypes.c_char
# S32 ctypes.c_int
# S8* ctypes.c_char_p

class _Structure(ctypes.Structure):
    _full_name_ = 'unknown'
    _defaults_ = []

    def __init__(self, **kwargs):
        values = deepcopy(dict(self._defaults_))
        values.update(kwargs)
        ctypes.Structure.__init__(self, **values)

    def to_string(self):
        def get_field_value(field):
            return getattr(self, field)

        def value_to_str(field, value):
            is_structure = isinstance(value, ctypes.Structure)
            is_array = isinstance(value, ctypes.Array)
            string = ""
            if is_structure:
                value_lines = value.to_string().splitlines()
                string += "  {} : {}{}".format(field, value_lines[0], os.linesep)
                for line in value_lines[1:]:
                    string += "  {}{}".format(line, os.linesep)
            elif is_array:
                value_lines = []
                value_lines.append("Array of {} elements".format(len(value)))
                for count, item in enumerate(value):
                    try:
                        element_lines = item.to_string().splitlines()
                        value_lines.append("  [{}] - {}".format(count, element_lines[0]))
                        for line in element_lines[1:]:
                            value_lines.append("    {}".format(line))
                    except AttributeError:
                        value_lines.append("  {} : {}".format(field, value))

                string += "  {} : {}{}".format(field, value_lines[0], os.linesep)
                for line in value_lines[1:]:
                    string += "  {}{}".format(line, os.linesep)
            else:
                string += "  {} : {}{}".format(field, value, os.linesep)

            return string

        body = ""
        for field in OrderedDict((_field[0], _field[1]) for _field in self._fields_).keys():
            value = get_field_value(field)
            body += value_to_str(field, value)

        header = self._full_name_ + os.linesep
        return header + body



MODE_UNKNOWN                             = 99
MODE_64BIT                               = 3
MODE_32BIT                               = 2
MODE_16BIT                               = 1
MODE_V86                                 = 0
SM_RTC                                   = 2020
SM_VTD                                   = 2021
SM_NMI                                   = 2022
SM_EBS                                   = 2023
SM_EBC                                   = 2024
INTERRUPT_RTC                            = 0x1
INTERRUPT_VTD                            = 0x2
INTERRUPT_NMI                            = 0x4
INTERRUPT_EBS                            = 0x8
DEV_CORE                                 = 0x01
DEV_UNC                                  = 0x02
EFLAGS_VM                                = 0x00020000
EFLAGS_IOPL0                             = 0
EFLAGS_IOPL1                             = 0x00001000
EFLAGS_IOPL2                             = 0x00002000
EFLAGS_IOPL3                             = 0x00003000
MAX_PCI_BUSNO                            = 256
MAX_DEVICES                              = 30
MAX_REGS                                 = 64
MAX_EMON_GROUPS                          = 1000
MAX_PCI_DEVNO                            = 32
MAX_PCI_FUNCNO                           = 8
MAX_PCI_DEVUNIT                          = 16
MAX_TURBO_VALUES                         = 32
REG_BIT_MASK                             = 0xFFFFFFFFFFFFFFFF

class FirstCommunicationMsg(object): # CONTROL_FIRST_MSG_NODE_S
    class v3(_Structure):
        _full_name_ = 'FirstCommunicationMsg_v3'
        _fields_ = [
            ('proto_version',       ctypes.c_uint),
            ('per_cpu_buffer_size', ctypes.c_uint),
            ('reserved1',           ctypes.c_ulonglong),
            ('reserved2',           ctypes.c_ulonglong),
        ]
        _defaults_ = [
            ('proto_version', 3),
        ]
    class v6(_Structure):
        _full_name_ = 'FirstCommunicationMsg_v6'
        _fields_ = [
            ('msg_size',            ctypes.c_uint),
            ('proto_version',       ctypes.c_uint),
            ('per_cpu_buffer_size', ctypes.c_uint),
            ('reserved1',           ctypes.c_uint),
            ('reserved2',           ctypes.c_ulonglong),
        ]
        _defaults_ = [
            ('proto_version', 6),
            ('msg_size',      24),
        ]


class FirstDataMsg(object): # DATA_FIRST_MSG_NODE_S
    class v3(_Structure):
        _full_name_ = 'FirstDataMsg_v3'
        _fields_ = [
            ('proto_version', ctypes.c_uint),
            ('data_type',     ctypes.c_ushort),
            ('data_id',       ctypes.c_ushort),
        ]
        _defaults_ = [
            ('proto_version', 3),
        ]
    class v6(_Structure):
        _full_name_ = 'FirstDataMsg_v6'
        _fields_ = [
            ('proto_version', ctypes.c_uint),
            ('data_type',     ctypes.c_ushort),
            ('data_id',       ctypes.c_ushort),
        ]
        _defaults_ = [
            ('proto_version', 6),
        ]


class ControlMsg(object): # CONTROL_MSG_HEADER_NODE_S
    class v3(_Structure):
        _full_name_ = 'ControlMsg_v3'
        _fields_ = [
            ('proto_version',         ctypes.c_uint),
            ('command_id',            ctypes.c_uint),
            ('to_target_data_size',   ctypes.c_ulonglong),
            ('from_target_data_size', ctypes.c_ulonglong),
        ]
        _defaults_ = [
            ('proto_version', 3),
        ]
    class v6(_Structure):
        _full_name_ = 'ControlMsg_v6'
        _fields_ = [
            ('header_size',           ctypes.c_uint),
            ('proto_version',         ctypes.c_uint),
            ('command_id',            ctypes.c_uint),
            ('status',                ctypes.c_int),
            ('to_target_data_size',   ctypes.c_ulonglong),
            ('from_target_data_size', ctypes.c_ulonglong),
#            ('device_type',           ctypes.c_uint),
#            ('reserved1',             ctypes.c_uint),
            ('reserved1',             ctypes.c_ulonglong),
            ('reserved2',             ctypes.c_ulonglong),
        ]
        _defaults_ = [
            ('proto_version', 6),
            ('header_size',   48),
#            ('device_type',   1),
        ]

class RemoteOsInfo(object): # REMOTE_OS_INFO_NODE
    class v3(_Structure):
        _full_name_ = 'RemoteOsInfo_v3'
        _fields_ = [
            ('os_family', ctypes.c_uint),
            ('reserved1', ctypes.c_uint),
            ('sysname',   ctypes.c_char * 64),
            ('release',   ctypes.c_char * 64),
            ('version',   ctypes.c_char * 64),
        ]


class RemoteHardwareInfo(object): # REMOTE_HARDWARE_INFO_NODE
    class v3(_Structure):
        _full_name_ = 'RemoteHardwareInfo_v3'
        _fields_ = [
            ('num_cpus',  ctypes.c_uint),
            ('family',    ctypes.c_uint),
            ('model',     ctypes.c_uint),
            ('stepping',  ctypes.c_uint),
            ('tsc_freq',  ctypes.c_ulonglong),
            ('reserved2', ctypes.c_ulonglong),
            ('reserved3', ctypes.c_ulonglong),
        ]


class DriverVersionInfo(object): # SEP_VERSION_NODE_S
    class v3(_Structure):
        _full_name_ = 'DriverVersionInfo_v3'
        _fields_ = [
            ('major',  ctypes.c_ubyte),
            ('minor',  ctypes.c_ubyte),
            ('api',    ctypes.c_ubyte),
            ('update', ctypes.c_ubyte),
#          ('sep_version', ctypes.uint),
#            ('major',  ctypes.c_ubyte),
#            ('minor',  ctypes.c_ubyte),
#            ('api',    ctypes.c_ubyte),
#            ('update', ctypes.c_ubyte),
        ]


class RemoteSwitch(object): # REMOTE_SWITCH_NODE
    class v3(_Structure):
        _full_name_ = 'RemoteSwitch_v3'
        _fields_ = [
            ('auto_mode',           ctypes.c_uint, 1),
            ('adv_hotspot',         ctypes.c_uint, 1),
            ('lbr_callstack',       ctypes.c_uint, 2),
            ('full_pebs',           ctypes.c_uint, 1),
            ('uncore_supported',    ctypes.c_uint, 1),
            ('agent_mode',          ctypes.c_uint, 2),
            ('sched_switch_enable', ctypes.c_uint, 1),
            ('data_transfer_mode',  ctypes.c_uint, 1),
            ('reserved1',           ctypes.c_uint, 22),
            ('reserved2',           ctypes.c_uint),
        ]

class TargetStatusMsg(object): # TARGET_STATUS_MSG_NODE_S
    class v3(_Structure):
        _full_name_ = 'TargetStatusMsg_v3'
        _fields_ = [
            ('status',               ctypes.c_uint),
            ('proto_version',        ctypes.c_uint),
            ('remote_os_info',       RemoteOsInfo.v3),
            ('remote_switch',        RemoteSwitch.v3),
            ('remote_hardware_info', RemoteHardwareInfo.v3)
        ]
        _defaults_ = [
            ('proto_version', 3),
        ]
    class v6(_Structure):
        _full_name_ = 'TargetStatusMsg_v6'
        _fields_ = [
            ('msg_size',               ctypes.c_uint),
            ('proto_version',          ctypes.c_uint),
            ('status',                 ctypes.c_int),
            ('reserved1',              ctypes.c_uint),
            ('reserved2',              ctypes.c_ulonglong),
            ('os_info_offset',         ctypes.c_uint),
            ('os_info_size',           ctypes.c_uint),
            ('collect_switch_offset',  ctypes.c_uint),
            ('collect_switch_size',    ctypes.c_uint),
            ('hardware_info_offset',   ctypes.c_uint),
            ('hardware_info_size',     ctypes.c_uint),
            ('remote_os_info',       RemoteOsInfo.v3),
            ('remote_switch',        RemoteSwitch.v3),
            ('remote_hardware_info', RemoteHardwareInfo.v3)
        ]
        _defaults_ = [
            ('proto_version', 6),
        ]


MAX_NUM_OS_ALLOWED                       = 6
TARGET_IP_NAMELEN                        = 64

#class TargetStatusMsg(object): # TARGET_INFO_NODE_S
#    class v3(_Structure):
#        _full_name_ = 'TARGET_INFO_NODE_S_v3'
#        _fields_ = [
#            ('num_of_agents',                          ctypes.c_uint),
#            ('reserved',                               ctypes.c_uint),
#            ('os_id',                                  ctypes.c_uint * MAX_NUM_OS_ALLOWED),
#            ('ip_address',                             ctypes.c_char * MAX_NUM_OS_ALLOWED * TARGET_IP_NAMELEN),
#            ('os_info',                                RemoteOsInfo.v3 * MAX_NUM_OS_ALLOWED),
#            ('hardware_info',                          RemoteHardwareInfo.v3 * MAX_NUM_OS_ALLOWED),
#            ('remote_switch',                          RemoteSwitch.v3 * MAX_NUM_OS_ALLOWED),
#        ]
#        _defaults_ = [
#            ('proto_version', 3),
#        ]

class DrvConfig(object): # DRV_CONFIG_NODE_S
    class v3(_Structure):
        _full_name_ = 'DrvConfig_v3'
        _fields_ = [
            ('size',                                   ctypes.c_uint),
            ('version',                                ctypes.c_ushort),
            ('reserved1',                              ctypes.c_ushort),
            ('num_events',                             ctypes.c_uint),
            ('num_chipset_events',                     ctypes.c_uint),
            ('chipset_offset',                         ctypes.c_uint),
            ('seed_name_len',                          ctypes.c_int),
            ('seed_name',                              ctypes.c_char_p),
#            ('dummy1',                                 ctypes.c_ulonglong),
            ('cpu_mask',                               ctypes.c_char_p),
#            ('dummy2',                                 ctypes.c_ulonglong),
#            ('collection_config',                      ctypes.c_ulonglong),
            ('start_paused',                           ctypes.c_ulonglong,  1),
            ('counting_mode',                          ctypes.c_ulonglong,  1),
            ('enable_chipset',                         ctypes.c_ulonglong,  1),
            ('enable_gfx',                             ctypes.c_ulonglong,  1),
            ('enable_pwr',                             ctypes.c_ulonglong,  1),
            ('emon_mode',                              ctypes.c_ulonglong,  1),
            ('debug_inject',                           ctypes.c_ulonglong,  1),
            ('virt_phys_translation',                  ctypes.c_ulonglong,  1),
            ('enable_p_state',                         ctypes.c_ulonglong,  1),
            ('enable_cp_mode',                         ctypes.c_ulonglong,  1),
            ('read_pstate_msrs',                       ctypes.c_ulonglong,  1),
            ('use_pcl',                                ctypes.c_ulonglong,  1),
            ('enable_ebc',                             ctypes.c_ulonglong,  1),
            ('enable_tbc',                             ctypes.c_ulonglong,  1),
            ('ds_area_available',                      ctypes.c_ulonglong,  1),
            ('per_cpu_tsc',                            ctypes.c_ulonglong,  1),
            ('reserved_field1',                        ctypes.c_ulonglong,  48),
            ('target_pid',                             ctypes.c_ulonglong),
            ('os_of_interest',                         ctypes.c_uint),
            ('unc_timer_interval',                     ctypes.c_ushort),
            ('unc_em_factor',                          ctypes.c_ushort),
            ('p_state_trigger_index',                  ctypes.c_int),
            ('multi_pebs_enabled',                     ctypes.c_uint),
            ('reserved2',                              ctypes.c_uint),
            ('reserved3',                              ctypes.c_uint),
            ('reserved4',                              ctypes.c_ulonglong),
            ('reserved5',                              ctypes.c_ulonglong),
            ('reserved6',                              ctypes.c_ulonglong),
        ]
    class v6(_Structure):
        _full_name_ = 'DrvConfig_v6'
        _fields_ = [
            ('size',                                   ctypes.c_uint),
            ('version',                                ctypes.c_ushort),
            ('reserved1',                              ctypes.c_ushort),
            ('num_events',                             ctypes.c_uint),
            ('num_chipset_events',                     ctypes.c_uint),
            ('chipset_offset',                         ctypes.c_uint),
            ('seed_name_len',                          ctypes.c_int),
            ('seed_name',                              ctypes.c_char_p),
#            ('dummy1',                                 ctypes.c_ulonglong),
            ('cpu_mask',                               ctypes.c_char_p),
#            ('dummy2',                                 ctypes.c_ulonglong),
#            ('collection_config',                      ctypes.c_ulonglong),
            ('start_paused',                           ctypes.c_ulonglong,  1),
            ('counting_mode',                          ctypes.c_ulonglong,  1),
            ('enable_chipset',                         ctypes.c_ulonglong,  1),
            ('enable_gfx',                             ctypes.c_ulonglong,  1),
            ('enable_pwr',                             ctypes.c_ulonglong,  1),
            ('emon_mode',                              ctypes.c_ulonglong,  1),
            ('debug_inject',                           ctypes.c_ulonglong,  1),
            ('virt_phys_translation',                  ctypes.c_ulonglong,  1),
            ('enable_p_state',                         ctypes.c_ulonglong,  1),
            ('enable_cp_mode',                         ctypes.c_ulonglong,  1),
            ('read_pstate_msrs',                       ctypes.c_ulonglong,  1),
            ('use_pcl',                                ctypes.c_ulonglong,  1),
            ('enable_ebc',                             ctypes.c_ulonglong,  1),
            ('enable_tbc',                             ctypes.c_ulonglong,  1),
            ('ds_area_available',                      ctypes.c_ulonglong,  1),
            ('per_cpu_tsc',                            ctypes.c_ulonglong,  1),
            ('reserved_field1',                        ctypes.c_ulonglong,  48),
            ('target_pid',                             ctypes.c_ulonglong),
            ('os_of_interest',                         ctypes.c_uint),
            ('unc_timer_interval',                     ctypes.c_ushort),
            ('unc_em_factor',                          ctypes.c_ushort),
            ('p_state_trigger_index',                  ctypes.c_int),
            ('multi_pebs_enabled',                     ctypes.c_uint),
            ('emon_timer_interval',                    ctypes.c_int),
            ('reserved3',                              ctypes.c_uint),
            ('reserved4',                              ctypes.c_ulonglong),
            ('reserved5',                              ctypes.c_ulonglong),
            ('reserved6',                              ctypes.c_ulonglong),
        ]

DRV_CONFIG_VERSION                       = 1

class DevConfig(object): # DEV_CONFIG_NODE_S
    class v3(_Structure):
        _full_name_ = 'DevConfig_v3'
        _fields_ = [
            ('size',                                   ctypes.c_ushort),
            ('version',                                ctypes.c_ushort),
            ('dispatch_id',                            ctypes.c_uint),
            ('pebs_mode',                              ctypes.c_uint),
            ('pebs_record_num',                        ctypes.c_uint),
            ('results_offset',                         ctypes.c_uint),
            ('max_gp_counters',                        ctypes.c_uint),
            ('device_type',                            ctypes.c_uint),
            ('core_type',                              ctypes.c_uint),
#            ('enable_bit_fields',                      ctypes.c_ulonglong),
            ('pebs_capture',                           ctypes.c_ulonglong,  1),
            ('collect_lbrs',                           ctypes.c_ulonglong,  1),
            ('collect_callstacks',                     ctypes.c_ulonglong,  1),
            ('collect_kernel_callstacks',              ctypes.c_ulonglong,  1),
            ('latency_capture',                        ctypes.c_ulonglong,  1),
            ('power_capture',                          ctypes.c_ulonglong,  1),
            ('htoff_mode',                             ctypes.c_ulonglong,  1),
            ('eventing_ip_capture',                    ctypes.c_ulonglong,  1),
            ('hle_capture',                            ctypes.c_ulonglong,  1),
            ('precise_ip_lbrs',                        ctypes.c_ulonglong,  1),
            ('store_lbrs',                             ctypes.c_ulonglong,  1),
            ('tsc_capture',                            ctypes.c_ulonglong,  1),
            ('enable_perf_metrics',                    ctypes.c_ulonglong,  1),
            ('enable_adaptive_pebs',                   ctypes.c_ulonglong,  1),
            ('apebs_collect_mem_info',                 ctypes.c_ulonglong,  1),
            ('apebs_collect_gpr',                      ctypes.c_ulonglong,  1),
            ('apebs_collect_xmm',                      ctypes.c_ulonglong,  1),
            ('apebs_collect_lbrs',                     ctypes.c_ulonglong,  1),
            ('collect_fixed_counter_pebs',             ctypes.c_ulonglong,  1),
            ('collect_os_callstacks',                  ctypes.c_ulonglong,  1),
            ('reserved_field1',                        ctypes.c_ulonglong,  44),
            ('emon_unc_offset',                        ctypes.c_uint * MAX_EMON_GROUPS),
            ('ebc_group_id_offset',                    ctypes.c_uint),
            ('num_perf_metrics',                       ctypes.c_ubyte),
            ('apebs_num_lbr_entries',                  ctypes.c_ubyte),
            ('emon_perf_metrics_offset',               ctypes.c_ushort),
            ('device_scope',                           ctypes.c_uint),
            ('reserved1',                              ctypes.c_uint),
            ('reserved2',                              ctypes.c_ulonglong),
            ('reserved3',                              ctypes.c_ulonglong),
            ('reserved4',                              ctypes.c_ulonglong),
        ]

class DevUncConfig(object): # DEV_UNC_CONFIG_NODE_S
    class v3(_Structure):
        _full_name_ = 'DevUncConfig_v3'
        _fields_ = [
            ('size',                                   ctypes.c_ushort),
            ('version',                                ctypes.c_ushort),
            ('dispatch_id',                            ctypes.c_uint),
            ('results_offset',                         ctypes.c_uint),
            ('device_type',                            ctypes.c_uint),
            ('device_scope',                           ctypes.c_uint),
            ('reserved1',                              ctypes.c_uint),
            ('emon_unc_offset',                        ctypes.c_uint * MAX_EMON_GROUPS),
            ('reserved2',                              ctypes.c_ulonglong),
            ('reserved3',                              ctypes.c_ulonglong),
            ('reserved4',                              ctypes.c_ulonglong),
        ]

MARKER_DEFAULT_TYPE                      = "Default_Marker"
MARKER_DEFAULT_ID                        = 0
MAX_MARKER_LENGTH                        = 136
MARK_ID                                  = 4
MARK_DATA                                = 2
THREAD_INFO                              = 8
DEBUG_CTL_LBR                            = 0x0000001
DEBUG_CTL_BTF                            = 0x0000002
DEBUG_CTL_TR                             = 0x0000040
DEBUG_CTL_BTS                            = 0x0000080
DEBUG_CTL_BTINT                          = 0x0000100
DEBUG_CTL_BT_OFF_OS                      = 0x0000200
DEBUG_CTL_BTS_OFF_USR                    = 0x0000400
DEBUG_CTL_FRZ_LBR_ON_PMI                 = 0x0000800
DEBUG_CTL_FRZ_PMON_ON_PMI                = 0x0001000
DEBUG_CTL_ENABLE_UNCORE_PMI_BIT          = 0x0002000

#class DriverVersionInfo(object): # DRV_CONFIG_NODE_S
#    class v3(_Structure):
#        _full_name_ = 'DriverVersionInfo_v3'
#        _fields_ = [
##            ('sep_version',                            ctypes.c_uint),
#            ('major',                                  ctypes.c_int,        8),
#            ('minor',                                  ctypes.c_int,        8),
#            ('api',                                    ctypes.c_int,        8),
#            ('update',                                 ctypes.c_int,        8),
#        ]


class DrvTopology(object): # DRV_CONFIG_NODE_S
    class v3(_Structure):
        _full_name_ = 'DrvTopologyInfo_v3'
        _fields_ = [
            ('cpu_number',                             ctypes.c_uint),
            ('cpu_package_num',                        ctypes.c_ushort),
            ('cpu_core_num',                           ctypes.c_ushort),
            ('cpu_hw_thread_num',                      ctypes.c_ushort),
            ('reserved1',                              ctypes.c_ushort),
            ('socket_master',                          ctypes.c_int),
            ('core_master',                            ctypes.c_int),
            ('thr_master',                             ctypes.c_int),
            ('cpu_module_num',                         ctypes.c_uint),
            ('cpu_module_master',                      ctypes.c_uint),
            ('cpu_num_modules',                        ctypes.c_uint),
            ('cpu_core_type',                          ctypes.c_uint),
            ('arch_perfmon_ver',                       ctypes.c_uint),
            ('num_gp_counters',                        ctypes.c_uint),
            ('num_fixed_counters',                     ctypes.c_uint),
            ('reserved2',                              ctypes.c_uint),
            ('reserved3',                              ctypes.c_ulonglong),
            ('reserved4',                              ctypes.c_ulonglong),
        ]

VALUE_TO_BE_DISCOVERED                   = 0

class DimmInfo(object): # DRV_CONFIG_NODE_S
    class v3(_Structure):
        _full_name_ = 'DimmInfo_v3'
        _fields_ = [
            ('platform_id',                            ctypes.c_uint),
            ('channel_num',                            ctypes.c_uint),
            ('rank_num',                               ctypes.c_uint),
            ('value',                                  ctypes.c_uint),
            ('mc_num',                                 ctypes.c_ubyte),
            ('dimm_valid',                             ctypes.c_ubyte),
            ('valid_value',                            ctypes.c_ubyte),
            ('rank_value',                             ctypes.c_ubyte),
            ('density_value',                          ctypes.c_ubyte),
            ('width_value',                            ctypes.c_ubyte),
            ('socket_num',                             ctypes.c_ushort),
            ('reserved1',                              ctypes.c_ulonglong),
            ('reserved2',                              ctypes.c_ulonglong),
        ]

MAX_PACKAGES                             = 16
MAX_CHANNELS                             = 8
MAX_RANKS                                = 3

DRV_LOG_CONTROL_MAX_DATA_SIZE                = 256

class DriverControlLog(object): # DRV_LOG_CONTROL_NODE_S
    class v3(_Structure):
        _full_name_ = 'DriverControlLog_v3'
        _fields_ = [
            ('command',                                 ctypes.c_uint),
            ('reserved1',                               ctypes.c_uint),
            ('data',                                    ctypes.c_ubyte * DRV_LOG_CONTROL_MAX_DATA_SIZE),
            ('reserved2',                               ctypes.c_ulonglong),
            ('reserved3',                               ctypes.c_ulonglong),
            ('reserved4',                               ctypes.c_ulonglong),
            ('reserved5',                               ctypes.c_ulonglong),
        ]

class PlatformInfo(object): # DRV_PLATFORM_INFO_NODE_S
    class v3(_Structure):
        _full_name_ = 'PlatformInfo_v3'
        _fields_ = [
            ('info',                                   ctypes.c_ulonglong),
            ('ddr_freq_index',                         ctypes.c_ulonglong),
            ('misc_valid',                             ctypes.c_ubyte),
            ('reserved1',                              ctypes.c_ubyte),
            ('reserved2',                              ctypes.c_ushort),
            ('vmm_timer_freq',                         ctypes.c_uint),
            ('misc_info',                              ctypes.c_ulonglong),
            ('ufs_freq',                               ctypes.c_ulonglong),
            ('dimm_info',                              DimmInfo.v3 * (MAX_PACKAGES * MAX_CHANNELS * MAX_RANKS)),
            ('energy_multiplier',                      ctypes.c_ulonglong),
            ('reserved3',                              ctypes.c_ulonglong),
            ('reserved4',                              ctypes.c_ulonglong),
            ('reserved5',                              ctypes.c_ulonglong),
            ('reserved6',                              ctypes.c_ulonglong),
        ]

class PlatformFreqInfo(object): # DRV_CONFIG_NODE_S
    class v3(_Structure):
        _full_name_ = 'PlatformFreqInfo_v3'
        _fields_ = [
            ('table_size',                             ctypes.c_uint),
            ('reserved1',                              ctypes.c_ulonglong),
            ('reserved2',                              ctypes.c_ulonglong),
            ('reserved3',                              ctypes.c_ulonglong),
            ('reserved4',                              ctypes.c_ulonglong),
        ]


class DeviceInfo(object): # DRV_CONFIG_NODE_S
    class v3(_Structure):
        _full_name_ = 'DeviceInfo_v3'
        _fields_ = [
            ('dll_name',                               ctypes.c_char_p),
            ('cpu_name',                               ctypes.c_char_p),
            ('pmu_name',                               ctypes.c_char_p),
            ('plat_type',                              ctypes.c_uint),
            ('plat_sub_type',                          ctypes.c_uint),
            ('dispatch_id',                            ctypes.c_int),
            ('num_of_groups',                          ctypes.c_uint),
            ('size_of_alloc',                          ctypes.c_uint),
            ('num_events',                             ctypes.c_uint),
            ('event_id_index',                         ctypes.c_uint),
            ('num_counters',                           ctypes.c_uint),
            ('group_index',                            ctypes.c_uint),
            ('num_packages',                           ctypes.c_uint),
            ('num_units',                              ctypes.c_uint),
            ('device_type',                            ctypes.c_uint),
            ('core_type',                              ctypes.c_uint),
            ('pmu_clone_id',                           ctypes.c_uint),
            ('device_scope',                           ctypes.c_uint),
            ('reserved1',                              ctypes.c_uint),
            ('reserved2',                              ctypes.c_ulonglong),
            ('reserved3',                              ctypes.c_ulonglong),
        ]

MAX_EVENT_NAME_LENGTH                    = 256

class DeviceInfoData(object): # DRV_CONFIG_NODE_S
    class v3(_Structure):
        _full_name_ = 'DeviceInfoData_v3'
        _fields_ = [
            ('num_elements',                           ctypes.c_uint),
            ('num_allocated',                          ctypes.c_uint),
            ('reserved1',                              ctypes.c_ulonglong),
            ('reserved2',                              ctypes.c_ulonglong),
            ('reserved3',                              ctypes.c_ulonglong),
            ('reserved4',                              ctypes.c_ulonglong),
        ]


class PcifuncInfo(object): # DRV_CONFIG_NODE_S
    class v3(_Structure):
        _full_name_ = 'PcifuncInfo_v3'
        _fields_ = [
            ('valid',                                  ctypes.c_uint),
            ('num_entries',                            ctypes.c_uint),
            ('deviceId',                               ctypes.c_ulonglong),
            ('reserved1',                              ctypes.c_ulonglong),
            ('reserved2',                              ctypes.c_ulonglong),
        ]


class PcidevInfo(object): # DRV_CONFIG_NODE_S
    class v3(_Structure):
        _full_name_ = 'PcidevInfo_v3'
        _fields_ = [
            ('valid',                                  ctypes.c_uint),
            ('dispatch_id',                            ctypes.c_uint),
            ('reserved1',                              ctypes.c_ulonglong),
            ('reserved2',                              ctypes.c_ulonglong),
        ]


class UncorePcidev(object): # DRV_CONFIG_NODE_S
    class v3(_Structure):
        _full_name_ = 'UncorePcidev_v3'
        _fields_ = [
            ('dispatch_id',                            ctypes.c_uint),
            ('scan',                                   ctypes.c_uint),
            ('num_uncore_units',                       ctypes.c_uint),
            ('num_deviceid_entries',                   ctypes.c_uint),
            ('dimm_device1',                           ctypes.c_ubyte),
            ('dimm_device2',                           ctypes.c_ubyte),
            ('reserved1',                              ctypes.c_ushort),
            ('reserved2',                              ctypes.c_uint),
            ('reserved3',                              ctypes.c_ulonglong),
            ('reserved4',                              ctypes.c_ulonglong),
            ('deviceid_list',                          ctypes.c_uint * MAX_PCI_DEVNO),
        ]


class UncoreTopologyInfo(object): # DRV_CONFIG_NODE_S
    class v3(_Structure):
        _full_name_ = 'UncoreTopologyInfo_v3'
        _fields_ = [
            ('device',                                 UncorePcidev.v3 * MAX_DEVICES),
        ]


class PlatformTopologyReg(object): # DRV_CONFIG_NODE_S
    class v3(_Structure):
        _full_name_ = 'PlatformTopologyReg_v3'
        _fields_ = [
            ('bus',                                    ctypes.c_uint),
            ('device',                                 ctypes.c_uint),
            ('function',                               ctypes.c_uint),
            ('reg_id',                                 ctypes.c_uint),
            ('reg_mask',                               ctypes.c_ulonglong),
            ('reg_value',                              ctypes.c_ulonglong * MAX_PACKAGES),
            ('reg_type',                               ctypes.c_ubyte),
            ('device_valid',                           ctypes.c_ubyte),
            ('reserved1',                              ctypes.c_ushort),
            ('reserved2',                              ctypes.c_uint),
            ('reserved3',                              ctypes.c_ulonglong),
            ('reserved4',                              ctypes.c_ulonglong),
        ]


class PlatformTopologyDiscovery(object): # DRV_CONFIG_NODE_S
    class v3(_Structure):
        _full_name_ = 'PlatformTopologyDiscovery_v3'
        _fields_ = [
            ('device_index',                           ctypes.c_uint),
            ('device_id',                              ctypes.c_uint),
            ('num_registers',                          ctypes.c_uint),
            ('scope',                                  ctypes.c_ubyte),
            ('prog_valid',                             ctypes.c_ubyte),
            ('reserved2',                              ctypes.c_ushort),
            ('reserved3',                              ctypes.c_ulonglong),
            ('reserved4',                              ctypes.c_ulonglong),
            ('reserved5',                              ctypes.c_ulonglong),
            ('topology_regs',                          PlatformTopologyReg.v3 * MAX_REGS),
        ]


class PlatformTopologyProg(object): # DRV_CONFIG_NODE_S
    class v3(_Structure):
        _full_name_ = 'PlatformTopologyProg_v3'
        _fields_ = [
            ('num_devices',                            ctypes.c_uint),
        ]


class FpgaGbDiscovery(object): # DRV_CONFIG_NODE_S
    class v3(_Structure):
        _full_name_ = 'FpgaGbDiscovery_v3'
        _fields_ = [
            ('bar_num',                                ctypes.c_ushort),
            ('feature_id',                             ctypes.c_ushort),
            ('device_id',                              ctypes.c_uint),
            ('afu_id_l',                               ctypes.c_ulonglong),
            ('afu_id_h',                               ctypes.c_ulonglong),
            ('feature_offset',                         ctypes.c_uint),
            ('feature_len',                            ctypes.c_uint),
            ('scan',                                   ctypes.c_ubyte),
            ('valid',                                  ctypes.c_ubyte),
            ('reserved1',                              ctypes.c_ushort),
            ('reserved2',                              ctypes.c_uint),
        ]


class FpgaGbDev(object): # DRV_CONFIG_NODE_S
    class v3(_Structure):
        _full_name_ = 'FpgaGbDev_v3'
        _fields_ = [
            ('num_devices',                            ctypes.c_uint),
            ('fpga_gb_device',                         FpgaGbDiscovery.v3 * MAX_DEVICES),
        ]


class SIDEBAND_INFO_NODE_S(object): # DRV_CONFIG_NODE_S
    class v3(_Structure):
        _full_name_ = 'SIDEBAND_INFO_NODE_S_v3'
        _fields_ = [
            ('tid',                                    ctypes.c_uint),
            ('pid',                                    ctypes.c_uint),
            ('tsc',                                    ctypes.c_ulonglong),
        ]


class SampleDrop(object): # DRV_CONFIG_NODE_S
    class v3(_Structure):
        _full_name_ = 'SampleDrop_v3'
        _fields_ = [
            ('os_id',                                  ctypes.c_uint),
            ('cpu_id',                                 ctypes.c_uint),
            ('sampled',                                ctypes.c_uint),
            ('dropped',                                ctypes.c_uint),
        ]

MAX_SAMPLE_DROP_NODES                    = 20

class SampleDropInfo(object): # DRV_CONFIG_NODE_S
    class v3(_Structure):
        _full_name_ = 'SampleDropInfo_v3'
        _fields_ = [
            ('size',                                   ctypes.c_uint),
            ('drop_info',                              SampleDrop.v3 * MAX_SAMPLE_DROP_NODES),
        ]
        
class CodeDescriptor(object): # CodeDescriptor_s
    class v3(_Structure):
        _full_name_ = "CodeDescriptor_v3"
        _fields_ = [
            ("lowWord",      ctypes.c_uint), # limitLow && baseLow
#            ("limitLow",    ctypes.c_ushort),
#            ("baseLow",     ctypes.c_ushort),
 #           ("highWord",    ctypes.c_uint),
            ("baseMid",      ctypes.c_uint, 8),
            ("accessed",     ctypes.c_uint, 1),
            ("readable",     ctypes.c_uint, 1),
            ("conforming",   ctypes.c_uint, 1),
            ("oneOne",       ctypes.c_uint, 2),
            ("dpl",          ctypes.c_uint, 2),
            ("pres",         ctypes.c_uint, 1),
            ("limitHi",      ctypes.c_uint, 4),
            ("sys",          ctypes.c_uint, 1),
            ("reserved_0",   ctypes.c_uint, 1),
            ("default_size", ctypes.c_uint, 1),
            ("granularity",  ctypes.c_uint, 1),
            ("baseHi",       ctypes.c_uint, 8),
    ]


class SampleRecordPC(object): # SampleRecordPC_s
    class v3(_Structure):
        _full_name_ = "SampleRecordPC_v3"
        _fields_ = [
            ("descriptor_id",  ctypes.c_uint),
            ("osid",           ctypes.c_uint),
            ("iip",            ctypes.c_ulonglong),
            ("ipsr",           ctypes.c_ulonglong),
#            ("eip",           ctypes.c_uint),
#            ("eflags",        ctypes.c_uint),
#            ("csd",                               CodeDescriptor.v3),
            ("cs",             ctypes.c_ushort),
#            ("cpuAndOS",      ctypes.c_ushort),
            ("cpuNum",         ctypes.c_ushort, 12),
            ("notVmid0",       ctypes.c_ushort, 1),
            ("codeMode",       ctypes.c_ushort, 2),
            ("uncore_valid",   ctypes.c_ushort, 1),
            ("tid",            ctypes.c_uint),
            ("pidRecIndex",    ctypes.c_uint),
#            ("bitFields2",    ctypes.c_uint),
            ("mrIndex",        ctypes.c_uint, 20),
            ("eventIndex",     ctypes.c_uint, 8),
            ("tidIsRaw",       ctypes.c_uint, 1),
            ("IA64PC",         ctypes.c_uint, 1),
            ("pidRecIndexRaw", ctypes.c_uint, 1),
            ("mrIndexNone",    ctypes.c_uint, 1),
            ("tsc",            ctypes.c_ulonglong),
    ]
        
class ModuleRecord(object): # ModuleRecord_s
    class v3(_Structure):
        _full_name_ = "ModuleRecord_v3"
        _fields_ = [
            ("recLength",               ctypes.c_ushort),
            ("segmentType",             ctypes.c_ushort, 2),
            ("loadEvent",               ctypes.c_ushort, 1),
            ("processed",               ctypes.c_ushort, 1),
            ("reserved0",               ctypes.c_ushort, 12),
            ("selector",                ctypes.c_ushort),
            ("segmentNameLength",       ctypes.c_ushort),
            ("segmentNumber",           ctypes.c_uint),
            ("exe",                     ctypes.c_uint, 1),
            ("globalModule",            ctypes.c_uint, 1),
            ("bogusWin95",              ctypes.c_uint, 1),
            ("pidRecIndexRaw",          ctypes.c_uint, 1),
            ("sampleFound",             ctypes.c_uint, 1),
            ("tscUsed",                 ctypes.c_uint, 1),
            ("duplicate",               ctypes.c_uint, 1),
            ("globalModuleTB5",         ctypes.c_uint, 1),
            ("segmentNameSet",          ctypes.c_uint, 1),
            ("firstModuleRecInProcess", ctypes.c_uint, 1),
            ("source",                  ctypes.c_uint, 1),
            ("unknownLoadAddress",      ctypes.c_uint, 1),
            ("reserved1",               ctypes.c_uint, 20),
            ("length64",                ctypes.c_ulonglong),
            ("loadAddr64",              ctypes.c_ulonglong),
            ("pidRecIndex",             ctypes.c_uint),
            ("osid",                    ctypes.c_uint),
            ("unloadTsc",               ctypes.c_ulonglong),
            ("path",                    ctypes.c_uint),
            ("pathLength",              ctypes.c_ushort),
            ("filenameOffset",          ctypes.c_ushort),
            ("segmentName",             ctypes.c_uint),
            ("page_offset_high",        ctypes.c_uint),
            ("tsc",                     ctypes.c_ulonglong),
            ("parent_pid",              ctypes.c_uint),
            ("page_offset_low",         ctypes.c_uint),
    ]


class UncoreSampleRecordPC(object): # UncoreSampleRecordPC_s
    class v3(_Structure):
        _full_name_ = "UncoreSampleRecordPC_v3"
        _fields_ = [
            ("descriptor_id", ctypes.c_uint),
            ("osid",          ctypes.c_uint),
            ("cpuNum",        ctypes.c_ushort),
            ("pkgNum",        ctypes.c_ushort),
            ("uncore_valid",  ctypes.c_uint, 1),
            ("reserved1",     ctypes.c_uint, 31),
            ("reserved2",     ctypes.c_ulonglong),
            ("tsc",           ctypes.c_ulonglong),
    ]


KVM_SIGNATURE                            = "KVMKVMKVM\0\0\0"
XEN_SIGNATURE                            = "XenVMMXenVMM"
VMWARE_SIGNATURE                         = "VMwareVMware"
DRV_VMM_UNKNOWN                          = 0
DRV_VMM_MOBILEVISOR                      = 1
DRV_VMM_KVM                              = 2
DRV_VMM_XEN                              = 3
DRV_VMM_HYPERV                           = 4
DRV_VMM_VMWARE                           = 5
DRV_VMM_ACRN                             = 6

class SetupInfo(object): # DRV_CONFIG_NODE_S
    class v3(_Structure):
        _full_name_ = 'SetupInfo_v3'
        _fields_ = [
#            ('modes',                                  ctypes.c_ulonglong),
            ('nmi_mode',                               ctypes.c_ulonglong,  1),
            ('vmm_mode',                               ctypes.c_ulonglong,  1),
            ('vmm_vendor',                             ctypes.c_ulonglong,  8),
            ('vmm_guest_vm',                           ctypes.c_ulonglong,  1),
            ('pebs_accessible',                        ctypes.c_ulonglong,  1),
            ('cpu_hotplug_mode',                       ctypes.c_ulonglong,  1),
            ('matrix_inaccessible',                    ctypes.c_ulonglong,  1),
            ('page_table_isolation',                   ctypes.c_ulonglong,  2),
            ('pebs_ignored_by_pti',                    ctypes.c_ulonglong,  1),
            ('reserved1',                              ctypes.c_ulonglong,  47),
            ('reserved2',                              ctypes.c_ulonglong),
            ('reserved3',                              ctypes.c_ulonglong),
            ('reserved4',                              ctypes.c_ulonglong),
        ]
    class v6(_Structure):
        _full_name_ = 'SetupInfo_v6'
        _fields_ = [
#            ('modes',                                  ctypes.c_ulonglong),
            ('nmi_mode',                               ctypes.c_ulonglong,  1),
            ('vmm_mode',                               ctypes.c_ulonglong,  1),
            ('vmm_vendor',                             ctypes.c_ulonglong,  8),
            ('vmm_guest_vm',                           ctypes.c_ulonglong,  1),
            ('pebs_accessible',                        ctypes.c_ulonglong,  1),
            ('cpu_hotplug_mode',                       ctypes.c_ulonglong,  1),
            ('matrix_inaccessible',                    ctypes.c_ulonglong,  1),
            ('page_table_isolation',                   ctypes.c_ulonglong,  2),
            ('pebs_ignored_by_pti',                    ctypes.c_ulonglong,  1),
            ('core_event_mux_unavailable',             ctypes.c_ulonglong,  1),
            ('reserved1',                              ctypes.c_ulonglong,  46),
            ('reserved2',                              ctypes.c_ulonglong),
            ('reserved3',                              ctypes.c_ulonglong),
            ('reserved4',                              ctypes.c_ulonglong),
        ]

DRV_SETUP_INFO_PTI_DISABLED              = 0
DRV_SETUP_INFO_PTI_KPTI                  = 1
DRV_SETUP_INFO_PTI_KAISER                = 2
DRV_SETUP_INFO_PTI_VA_SHADOW             = 3
DRV_SETUP_INFO_PTI_UNKNOWN               = 4

class TaskInfo(object): # DRV_CONFIG_NODE_S
    class v3(_Structure):
        _full_name_ = 'task_info_node_s_v3'
        _fields_ = [
            ('id',                                     ctypes.c_ulonglong),
            ('name',                                   ctypes.c_char * 32),
            ('address_space_id',                       ctypes.c_ulonglong),
        ]


class REMOTE_SWITCH_NODE_S(object): # DRV_CONFIG_NODE_S
    class v3(_Structure):
        _full_name_ = 'REMOTE_SWITCH_NODE_S_v3'
        _fields_ = [
            ('auto_mode',                              ctypes.c_uint,       1),
            ('adv_hotspot',                            ctypes.c_uint,       1),
            ('lbr_callstack',                          ctypes.c_uint,       2),
            ('full_pebs',                              ctypes.c_uint,       1),
            ('uncore_supported',                       ctypes.c_uint,       1),
            ('agent_mode',                             ctypes.c_uint,       2),
            ('sched_switch_enabled',                   ctypes.c_uint,       1),
            ('data_transfer_mode',                     ctypes.c_uint,       1),
            ('reserved1',                              ctypes.c_uint,       22),
            ('reserved2',                              ctypes.c_uint),
        ]

OSINFOLEN                                = 64

class REMOTE_OS_INFO_NODE_S(object): # DRV_CONFIG_NODE_S
    class v3(_Structure):
        _full_name_ = 'REMOTE_OS_INFO_NODE_S_v3'
        _fields_ = [
            ('os_family',                              ctypes.c_uint),
            ('reserved1',                              ctypes.c_uint),
            ('sysname',                                ctypes.c_char * OSINFOLEN),
            ('release',                                ctypes.c_char * OSINFOLEN),
            ('version',                                ctypes.c_char * OSINFOLEN),
        ]


class REMOTE_HARDWARE_INFO_NODE_S(object): # DRV_CONFIG_NODE_S
    class v3(_Structure):
        _full_name_ = 'REMOTE_HARDWARE_INFO_NODE_S_v3'
        _fields_ = [
            ('num_cpus',                               ctypes.c_uint),
            ('family',                                 ctypes.c_uint),
            ('model',                                  ctypes.c_uint),
            ('stepping',                               ctypes.c_uint),
            ('tsc_freq',                               ctypes.c_ulonglong),
            ('reserved2',                              ctypes.c_ulonglong),
            ('reserved3',                              ctypes.c_ulonglong),
        ]

class CPU_MAP_TRACE_NODE_S(object): # DRV_CONFIG_NODE_S
    class v3(_Structure):
        _full_name_ = 'CPU_MAP_TRACE_NODE_S_v3'
        _fields_ = [
            ('tsc',                                    ctypes.c_ulonglong),
            ('os_id',                                  ctypes.c_uint),
            ('vcpu_id',                                ctypes.c_uint),
            ('pcpu_id',                                ctypes.c_uint),
            ('is_static',                              ctypes.c_ubyte,      1),
            ('initial',                                ctypes.c_ubyte,      1),
            ('reserved1',                              ctypes.c_ubyte,      6),
            ('reserved2',                              ctypes.c_ubyte),
            ('reserved3',                              ctypes.c_ushort),
            ('reserved4',                              ctypes.c_ulonglong),
        ]


MAX_NUM_VCPU = 64
MAX_NUM_VM = 16

class CPU_MAP_TRACE_LIST_NODE_S(object):  
    class v3(_Structure):
        _full_name_ = 'CPU_MAP_TRACE_LIST_NODE_S_v3'
        _fields_ = [
            ('osid',                                   ctypes.c_uint),
            ('num_entries',                            ctypes.c_ubyte),
            ('reserved1',                              ctypes.c_ubyte),
            ('reserved2',                              ctypes.c_ushort),
            ('entries',                                CPU_MAP_TRACE_NODE_S.v3 * MAX_NUM_VCPU),
        ]

        
class VM_OSID_MAP_NODE_S(object):  
    class v(_Structure):
        _full_name_ = 'VM_OSID_MAP_NODE_S_v6'
        _fields_ = [
            ('num_vms',                                ctypes.c_uint),
            ('reserved1',                              ctypes.c_uint),          
            ('osid',                                   ctypes.c_uint * MAX_NUM_VM),
        ]


class VM_SWITCH_TRACE_NODE_S(object): # DRV_CONFIG_NODE_S
    class v6(_Structure):
        _full_name_ = 'VM_SWITCH_TRACE_NODE_S_v6'
        _fields_ = [
            ('tsc',                                    ctypes.c_ulonglong),
            ('from_os_id',                             ctypes.c_uint),
            ('to_os_id',                               ctypes.c_uint),
            ('reason',                                 ctypes.c_ulonglong),
            ('reserved1',                              ctypes.c_ulonglong),
            ('reserved2',                              ctypes.c_ulonglong),
        ]


class EMON_BUFFER_DRIVER_HELPER_NODE_S(object): # DRV_CONFIG_NODE_S
    class v3(_Structure):
        _full_name_ = 'EMON_BUFFER_DRIVER_HELPER_NODE_S_v3'
        _fields_ = [
            ('num_entries_per_package',                ctypes.c_uint),
            ('num_cpu',                                ctypes.c_uint),
            ('power_num_package_events',               ctypes.c_uint),
            ('power_num_module_events',                ctypes.c_uint),
            ('power_num_thread_events',                ctypes.c_uint),
            ('power_device_offset_in_package',         ctypes.c_uint),
            ('core_num_events',                        ctypes.c_uint),
            ('core_index_to_thread_offset_map',        ctypes.c_uint),
        ]

DRV_MAX_NB_LOG_CATEGORIES                = 256
DRV_NB_LOG_CATEGORIES                    = 14
DRV_LOG_CATEGORY_LOAD                    = 0
DRV_LOG_CATEGORY_INIT                    = 1
DRV_LOG_CATEGORY_DETECTION               = 2
DRV_LOG_CATEGORY_ERROR                   = 3
DRV_LOG_CATEGORY_STATE_CHANGE            = 4
DRV_LOG_CATEGORY_MARK                    = 5
DRV_LOG_CATEGORY_DEBUG                   = 6
DRV_LOG_CATEGORY_FLOW                    = 7
DRV_LOG_CATEGORY_ALLOC                   = 8
DRV_LOG_CATEGORY_INTERRUPT               = 9
DRV_LOG_CATEGORY_TRACE                   = 10
DRV_LOG_CATEGORY_REGISTER                = 11
DRV_LOG_CATEGORY_NOTIFICATION            = 12
DRV_LOG_CATEGORY_WARNING                 = 13
LOG_VERBOSITY_UNSET                      = 0xFF
LOG_VERBOSITY_DEFAULT                    = 0xFE
LOG_VERBOSITY_NONE                       = 0
LOG_CHANNEL_MEMLOG                       = 0x1
LOG_CHANNEL_AUXMEMLOG                    = 0x2
LOG_CHANNEL_PRINTK                       = 0x4
LOG_CHANNEL_TRACEK                       = 0x8
#LOG_CHANNEL_MASK                         = LOG_CATEGORY_VERBOSITY_EVERYWHERE
LOG_CONTEXT_REGULAR                      = 0x10
LOG_CONTEXT_INTERRUPT                    = 0x20
LOG_CONTEXT_NOTIFICATION                 = 0x40
#LOG_CONTEXT_MASK                         = LOG_CONTEXT_A
LOG_CONTEXT_SHIFT                        = 4
DRV_LOG_NOTHING                          = 0
DRV_LOG_FLOW_IN                          = 1
DRV_LOG_FLOW_OUT                         = 2
DRV_LOG_MESSAGE_LENGTH                   = 64
DRV_LOG_FUNCTION_NAME_LENGTH             = 32

class DRV_LOG_ENTRY_NODE_S(object): # DRV_CONFIG_NODE_S
    class v3(_Structure):
        _full_name_ = 'DRV_LOG_ENTRY_NODE_S_v3'
        _fields_ = [
            ('function_name',                          ctypes.c_char * DRV_LOG_FUNCTION_NAME_LENGTH),
            ('message',                                ctypes.c_char * DRV_LOG_MESSAGE_LENGTH),
            ('temporal_tag',                           ctypes.c_ushort),
            ('integrity_tag',                          ctypes.c_ushort),
            ('category',                               ctypes.c_ubyte),
            ('secondary_info',                         ctypes.c_ubyte),
            ('processor_id',                           ctypes.c_ushort),
            ('tsc',                                    ctypes.c_ulonglong),
            ('nb_active_interrupts',                   ctypes.c_ushort),
            ('active_drv_operation',                   ctypes.c_ubyte),
            ('driver_state',                           ctypes.c_ubyte),
            ('line_number',                            ctypes.c_ushort),
            ('nb_active_notifications',                ctypes.c_ushort),
            ('reserved',                               ctypes.c_ulonglong),
        ]

DRV_LOG_SIGNATURE_SIZE                   = 8
DRV_LOG_SIGNATURE_0                      = 'S'
DRV_LOG_SIGNATURE_1                      = 'e'
DRV_LOG_SIGNATURE_2                      = 'P'
DRV_LOG_SIGNATURE_3                      = 'd'
DRV_LOG_SIGNATURE_4                      = 'R'
DRV_LOG_SIGNATURE_5                      = 'v'
DRV_LOG_SIGNATURE_6                      = '5'
DRV_LOG_SIGNATURE_7                      = '\0'
DRV_LOG_VERSION                          = 1
DRV_LOG_FILLER_BYTE                      = 1
DRV_LOG_DRIVER_VERSION_SIZE              = 64

class DRV_LOG_BUFFER_NODE_S(object): # DRV_CONFIG_NODE_S
    class v3(_Structure):
        _full_name_ = 'DRV_LOG_BUFFER_NODE_S_v3'
        _fields_ = [
            ('header_signature',                       ctypes.c_char * DRV_LOG_SIGNATURE_SIZE),
            ('log_size',                               ctypes.c_uint),
            ('max_nb_pri_entries',                     ctypes.c_uint),
            ('max_nb_aux_entries',                     ctypes.c_uint),
            ('reserved1',                              ctypes.c_uint),
            ('init_time',                              ctypes.c_ulonglong),
            ('disambiguator',                          ctypes.c_uint),
            ('log_version',                            ctypes.c_uint),
            ('pri_entry_index',                        ctypes.c_uint),
            ('aux_entry_index',                        ctypes.c_uint),
            ('driver_version',                         ctypes.c_char * DRV_LOG_DRIVER_VERSION_SIZE),
            ('driver_state',                           ctypes.c_ubyte),
            ('active_drv_operation',                   ctypes.c_ubyte),
            ('reserved2',                              ctypes.c_ushort),
            ('nb_drv_operations',                      ctypes.c_uint),
            ('nb_interrupts',                          ctypes.c_uint),
            ('nb_active_interrupts',                   ctypes.c_ushort),
            ('nb_active_notifications',                ctypes.c_ushort),
            ('nb_notifications',                       ctypes.c_uint),
            ('nb_driver_state_transitions',            ctypes.c_uint),
            ('contiguous_physical_memory',             ctypes.c_ubyte),
            ('reserved3',                              ctypes.c_ubyte),
            ('reserved4',                              ctypes.c_ushort),
            ('reserved5',                              ctypes.c_uint),
            ('verbosities',                            ctypes.c_ubyte * DRV_MAX_NB_LOG_CATEGORIES),
            ('footer_signature',                       ctypes.c_char * DRV_LOG_SIGNATURE_SIZE),
        ]

DRV_LOG_CONTROL_MAX_DATA_SIZE            = DRV_MAX_NB_LOG_CATEGORIES

class DRV_LOG_CONTROL_NODE_S(object): # DRV_CONFIG_NODE_S
    class v3(_Structure):
        _full_name_ = 'DRV_LOG_CONTROL_NODE_S_v3'
        _fields_ = [
            ('command',                                ctypes.c_uint),
            ('reserved1',                              ctypes.c_uint),
            ('data',                                   ctypes.c_ubyte * DRV_LOG_CONTROL_MAX_DATA_SIZE),
            ('reserved2',                              ctypes.c_ulonglong),
            ('reserved3',                              ctypes.c_ulonglong),
            ('reserved4',                              ctypes.c_ulonglong),
            ('reserved5',                              ctypes.c_ulonglong),
        ]

DRV_LOG_CONTROL_COMMAND_NONE             = 0
DRV_LOG_CONTROL_COMMAND_ADJUST_VERBOSITY = 1
DRV_LOG_CONTROL_COMMAND_MARK             = 2
DRV_LOG_CONTROL_COMMAND_QUERY_SIZE       = 3
DRV_LOG_CONTROL_COMMAND_BENCHMARK        = 4

CCCR                                     = 1
ESCR                                     = 2
DATA                                     = 4
DATA_RO_DELTA                            = 8
DATA_RO_SS                               = 16
METRICS                                  = 32
EM_DISABLED                              = -1
EM_TIMER_BASED                           = 0
EM_EVENT_BASED_PROFILING                 = 1
EM_TRIGGER_BASED                         = 2

class EventDesc(object): # EVENT_DESC_NODE_S
    class v3(_Structure):
        _full_name_ = 'EventDesc_v3'
        _fields_ = [
            ('sample_size',                            ctypes.c_uint),
            ('pebs_offset',                            ctypes.c_uint),
            ('pebs_size',                              ctypes.c_uint),
            ('lbr_offset',                             ctypes.c_uint),
            ('lbr_num_regs',                           ctypes.c_uint),
            ('latency_offset_in_sample',               ctypes.c_uint),
            ('latency_size_in_sample',                 ctypes.c_uint),
            ('latency_size_from_pebs_record',          ctypes.c_uint),
            ('latency_offset_in_pebs_record',          ctypes.c_uint),
            ('power_offset_in_sample',                 ctypes.c_uint),
            ('ebc_offset',                             ctypes.c_uint),
            ('uncore_ebc_offset',                      ctypes.c_uint),
            ('eventing_ip_offset',                     ctypes.c_uint),
            ('hle_offset',                             ctypes.c_uint),
            ('pwr_offset',                             ctypes.c_uint),
            ('callstack_offset',                       ctypes.c_uint),
            ('callstack_size',                         ctypes.c_uint),
            ('p_state_offset',                         ctypes.c_uint),
            ('pebs_tsc_offset',                        ctypes.c_uint),
            ('perfmetrics_offset',                     ctypes.c_uint),
            ('perfmetrics_size',                       ctypes.c_uint),
            ('applicable_counters_offset',             ctypes.c_ushort),
            ('gpr_info_offset',                        ctypes.c_ushort),
            ('gpr_info_size',                          ctypes.c_ushort),
            ('xmm_info_offset',                        ctypes.c_ushort),
            ('xmm_info_size',                          ctypes.c_ushort),
            ('lbr_info_size',                          ctypes.c_ushort),
            ('reserved2',                              ctypes.c_uint),
            ('reserved3',                              ctypes.c_ulonglong),
        ]


class EventConfig(object): # EVENT_CONFIG_NODE_S
    class v3(_Structure):
        _full_name_ = 'EventConfig_v3'
        _fields_ = [
            ('num_groups',                             ctypes.c_uint),
            ('em_mode',                                ctypes.c_int),
            ('em_factor',                              ctypes.c_int),
            ('em_event_num',                           ctypes.c_int),
            ('sample_size',                            ctypes.c_uint),
            ('max_gp_events',                          ctypes.c_uint),
            ('max_fixed_counters',                     ctypes.c_uint),
            ('max_ro_counters',                        ctypes.c_uint),
            ('pebs_offset',                            ctypes.c_uint),
            ('pebs_size',                              ctypes.c_uint),
            ('lbr_offset',                             ctypes.c_uint),
            ('lbr_num_regs',                           ctypes.c_uint),
            ('latency_offset_in_sample',               ctypes.c_uint),
            ('latency_size_in_sample',                 ctypes.c_uint),
            ('latency_size_from_pebs_record',          ctypes.c_uint),
            ('latency_offset_in_pebs_record',          ctypes.c_uint),
            ('power_offset_in_sample',                 ctypes.c_uint),
            ('ebc_offset',                             ctypes.c_uint),
            ('num_groups_unc',                         ctypes.c_uint),
            ('ebc_offset_unc',                         ctypes.c_uint),
            ('sample_size_unc',                        ctypes.c_uint),
            ('eventing_ip_offset',                     ctypes.c_uint),
            ('hle_offset',                             ctypes.c_uint),
            ('pwr_offset',                             ctypes.c_uint),
            ('callstack_offset',                       ctypes.c_uint),
            ('callstack_size',                         ctypes.c_uint),
            ('p_state_offset',                         ctypes.c_uint),
            ('pebs_tsc_offset',                        ctypes.c_uint),
            ('reserved1',                              ctypes.c_ulonglong),
            ('reserved2',                              ctypes.c_ulonglong),
            ('reserved3',                              ctypes.c_ulonglong),
            ('reserved4',                              ctypes.c_ulonglong),
        ]


class EventRegId(object): # EVENT_REG_ID_NODE_S
    class v3(_Structure):
        _full_name_ = 'EventRegId_v3'
        _fields_ = [
            ('reg_id',                                 ctypes.c_uint),
            ('pci_bus_no',                             ctypes.c_uint),
            ('pci_dev_no',                             ctypes.c_uint),
            ('pci_func_no',                            ctypes.c_uint),
            ('data_size',                              ctypes.c_uint),
            ('bar_index',                              ctypes.c_uint),
            ('reserved1',                              ctypes.c_uint),
            ('reserved2',                              ctypes.c_uint),
            ('reserved3',                              ctypes.c_ulonglong),
        ]


class EventReg(object): # EVENT_REG_NODE_S
    class v3(_Structure):
        _full_name_ = 'EventReg_v3'
        _fields_ = [
            ('reg_type',                               ctypes.c_ubyte),
            ('unit_id',                                ctypes.c_ubyte),
            ('event_id_index',                         ctypes.c_ushort),
            ('counter_event_offset',                   ctypes.c_ushort),
            ('reserved1',                              ctypes.c_ushort),
            ('event_reg_id',                           EventRegId.v3), # EVENT_REG_ID_NODE
            ('reg_value',                              ctypes.c_ulonglong),
            ('desc_id',                                ctypes.c_ushort),
            ('flags',                                  ctypes.c_ushort),
            ('reserved2',                              ctypes.c_uint),
            ('max_bits',                               ctypes.c_ulonglong),
            ('scheduled',                              ctypes.c_ubyte),
            ('secondary_pci_offset_shift',             ctypes.c_char),
            ('secondary_pci_offset_offset',            ctypes.c_ushort),
            ('counter_type',                           ctypes.c_uint),
            ('event_scope',                            ctypes.c_uint),
            ('reg_prog_type',                          ctypes.c_ubyte),
            ('reg_rw_type',                            ctypes.c_ubyte),
            ('reg_order',                              ctypes.c_ubyte),
            ('bit_position',                           ctypes.c_ubyte),
            ('secondary_pci_offset_mask',              ctypes.c_ulonglong),
            ('core_event_id',                          ctypes.c_uint),
            ('uncore_buffer_offset_in_package',        ctypes.c_uint),
            ('uncore_buffer_offset_in_system',         ctypes.c_uint),
            ('reserved3',                              ctypes.c_uint),
            ('reserved4',                              ctypes.c_ulonglong),
            ('reserved5',                              ctypes.c_ulonglong),
            ('reserved6',                              ctypes.c_ulonglong),
        ]

EVENT_REG_precise_bit                    = 0x00000001
EVENT_REG_global_bit                     = 0x00000002
EVENT_REG_uncore_bit                     = 0x00000004
EVENT_REG_uncore_q_rst_bit               = 0x00000008
EVENT_REG_latency_bit                    = 0x00000010
EVENT_REG_is_gp_reg_bit                  = 0x00000020
EVENT_REG_clean_up_bit                   = 0x00000040
EVENT_REG_em_trigger_bit                 = 0x00000080
EVENT_REG_lbr_value_bit                  = 0x00000100
EVENT_REG_fixed_reg_bit                  = 0x00000200
EVENT_REG_multi_pkg_evt_bit              = 0x00001000
EVENT_REG_branch_evt_bit                 = 0x00002000

class PciDeviceEntry(object): # DRV_PCI_DEVICE_ENTRY_NODE_S
    class v3(_Structure):
        _full_name_ = 'PciDeviceEntry_v3'
        _fields_ = [
            ('bus_no',                                 ctypes.c_uint),
            ('dev_no',                                 ctypes.c_uint),
            ('func_no',                                ctypes.c_uint),
            ('bar_offset',                             ctypes.c_uint),
            ('bar_mask',                               ctypes.c_ulonglong),
            ('bit_offset',                             ctypes.c_uint),
            ('size',                                   ctypes.c_uint),
            ('bar_address',                            ctypes.c_ulonglong),
            ('enable_offset',                          ctypes.c_uint),
            ('enabled',                                ctypes.c_uint),
            ('base_offset_for_mmio',                   ctypes.c_uint),
            ('operation',                              ctypes.c_uint),
            ('bar_name',                               ctypes.c_uint),
            ('prog_type',                              ctypes.c_uint),
            ('config_type',                            ctypes.c_uint),
            ('bar_shift',                              ctypes.c_char),
            ('reserved0',                              ctypes.c_ubyte),
            ('reserved1',                              ctypes.c_ushort),
            ('value',                                  ctypes.c_ulonglong),
            ('mask',                                   ctypes.c_ulonglong),
            ('virtual_address',                        ctypes.c_ulonglong),
            ('port_id',                                ctypes.c_uint),
            ('op_code',                                ctypes.c_uint),
            ('device_id',                              ctypes.c_uint),
            ('bar_num',                                ctypes.c_ushort),
            ('feature_id',                             ctypes.c_ushort),
            ('reserved2',                              ctypes.c_ulonglong),
            ('reserved3',                              ctypes.c_ulonglong),
            ('reserved4',                              ctypes.c_ulonglong),
        ]

MAX_OPERATION_TYPES                      = 32

class PmuOperation(object): # PMU_OPERATIONS_NODE_S
    class v3(_Structure):
        _full_name_ = 'PmuOperation_v3'
        _fields_ = [
            ('operation_type',                         ctypes.c_uint),
            ('register_start',                         ctypes.c_uint),
            ('register_len',                           ctypes.c_uint),
            ('reserved1',                              ctypes.c_uint),
            ('reserved2',                              ctypes.c_uint),
            ('reserved3',                              ctypes.c_uint),
        ]

MAX_MMIO_BARS                            = 8

class MmioBar(object): # MMIO_BAR_INFO_NODE_S
    class v3(_Structure):
        _full_name_ = 'MMIO_BAR_INFO_NODE_S_v3'
        _fields_ = [
            ('bus_no',                                 ctypes.c_uint),
            ('dev_no',                                 ctypes.c_uint),
            ('func_no',                                ctypes.c_uint),
            ('offset',                                 ctypes.c_uint),
            ('addr_size',                              ctypes.c_uint),
            ('map_size',                               ctypes.c_uint),
            ('bar_shift',                              ctypes.c_char),
            ('reserved1',                              ctypes.c_ubyte),
            ('reserved2',                              ctypes.c_ushort),
            ('reserved3',                              ctypes.c_uint),
            ('reserved4',                              ctypes.c_uint),
            ('reserved5',                              ctypes.c_uint),
            ('bar_mask',                               ctypes.c_ulonglong),
            ('base_mmio_offset',                       ctypes.c_ulonglong),
            ('physical_address',                       ctypes.c_ulonglong),
            ('virtual_address',                        ctypes.c_ulonglong),
            ('reserved6',                              ctypes.c_ulonglong),
            ('reserved7',                              ctypes.c_ulonglong),
        ]
    class v6(_Structure):
        _full_name_ = 'MmioBar_v6'
        _fields_ = [
            ('bus_no',                                 ctypes.c_uint),
            ('dev_no',                                 ctypes.c_uint),
            ('func_no',                                ctypes.c_uint),
            ('addr_size',                              ctypes.c_uint),
            ('base_offset_for_mmio',                   ctypes.c_ulonglong),
            ('map_size_for_mmio',                      ctypes.c_uint),
            ('main_bar_offset',                        ctypes.c_uint),
            ('main_bar_mask',                          ctypes.c_ulonglong),
            ('main_bar_shift',                         ctypes.c_ubyte),
            ('reserved1',                              ctypes.c_ubyte),
            ('reserved2',                              ctypes.c_ushort),
            ('reserved3',                              ctypes.c_uint),
            ('reserved4',                              ctypes.c_uint),
            ('secondary_bar_offset',                   ctypes.c_uint),
            ('secondary_bar_mask',                     ctypes.c_ulonglong),
            ('secondary_bar_shift',                    ctypes.c_ubyte),
            ('reserved5',                              ctypes.c_ubyte),
            ('reserved6',                              ctypes.c_ushort),
            ('reserved7',                              ctypes.c_uint),
            ('feature_id',                             ctypes.c_ushort),
            ('reserved8',                              ctypes.c_ushort),
            ('reserved9',                              ctypes.c_uint),
            ('reserved10',                             ctypes.c_ulonglong),
        ]


class Ecb(object): # ECB_NODE_S
    class v3(_Structure):
        _full_name_ = 'Ecb_v3'
        _fields_ = [
            ('version',                                ctypes.c_ubyte),
            ('reserved1',                              ctypes.c_ubyte),
            ('reserved2',                              ctypes.c_ushort),
            ('num_entries',                            ctypes.c_uint),
            ('group_id',                               ctypes.c_uint),
            ('num_events',                             ctypes.c_uint),
            ('cccr_start',                             ctypes.c_uint),
            ('cccr_pop',                               ctypes.c_uint),
            ('escr_start',                             ctypes.c_uint),
            ('escr_pop',                               ctypes.c_uint),
            ('data_start',                             ctypes.c_uint),
            ('data_pop',                               ctypes.c_uint),
            ('flags',                                  ctypes.c_ushort),
            ('pmu_timer_interval',                     ctypes.c_ubyte),
            ('reserved3',                              ctypes.c_ubyte),
            ('size_of_allocation',                     ctypes.c_uint),
            ('group_offset',                           ctypes.c_uint),
            ('reserved4',                              ctypes.c_uint),
            ('pcidev_entry_node',                      PciDeviceEntry.v3), # DRV_PCI_DEVICE_ENTRY_NODE
            ('num_pci_devices',                        ctypes.c_uint),
            ('pcidev_list_offset',                     ctypes.c_uint),
            ('pcidev_entry_list',                      ctypes.POINTER(PciDeviceEntry.v3)), # DRV_PCI_DEVICE_ENTRY
            ('device_type',                            ctypes.c_uint),
            ('dev_node',                               ctypes.c_uint),
            ('operations',                             PmuOperation.v3*MAX_OPERATION_TYPES), # PMU_OPERATIONS_NODE
            ('descriptor_id',                          ctypes.c_uint),
            ('reserved5',                              ctypes.c_uint),
            ('metric_start',                           ctypes.c_uint),
            ('metric_pop',                             ctypes.c_uint),
            ('mmio_bar_list',                          MmioBar.v3*MAX_MMIO_BARS), # MMIO_BAR_INFO_NODE
            ('reserved6',                              ctypes.c_ulonglong),
            ('reserved7',                              ctypes.c_ulonglong),
            ('reserved8',                              ctypes.c_ulonglong),
            ('entries',                                ctypes.POINTER(EventReg.v3)), # EVENT_REG_NODE
        ]
    class v6(_Structure):
        _full_name_ = 'Ecb_v6'
        _fields_ = [
            ('version',                                ctypes.c_ubyte),
            ('reserved1',                              ctypes.c_ubyte),
            ('reserved2',                              ctypes.c_ushort),
            ('num_entries',                            ctypes.c_uint),
            ('group_id',                               ctypes.c_uint),
            ('num_events',                             ctypes.c_uint),
            ('cccr_start',                             ctypes.c_uint),
            ('cccr_pop',                               ctypes.c_uint),
            ('escr_start',                             ctypes.c_uint),
            ('escr_pop',                               ctypes.c_uint),
            ('data_start',                             ctypes.c_uint),
            ('data_pop',                               ctypes.c_uint),
            ('flags',                                  ctypes.c_ushort),
            ('pmu_timer_interval',                     ctypes.c_ubyte),
            ('reserved3',                              ctypes.c_ubyte),
            ('size_of_allocation',                     ctypes.c_uint),
            ('group_offset',                           ctypes.c_uint),
            ('reserved4',                              ctypes.c_uint),
            ('pcidev_entry_node',                      PciDeviceEntry.v3), # DRV_PCI_DEVICE_ENTRY_NODE
            ('num_pci_devices',                        ctypes.c_uint),
            ('pcidev_list_offset',                     ctypes.c_uint),
            ('pcidev_entry_list',                      ctypes.POINTER(PciDeviceEntry.v3)), # DRV_PCI_DEVICE_ENTRY
            ('device_type',                            ctypes.c_uint),
            ('dev_node',                               ctypes.c_uint),
            ('operations',                             PmuOperation.v3*MAX_OPERATION_TYPES), # PMU_OPERATIONS_NODE
            ('descriptor_id',                          ctypes.c_uint),
            ('reserved5',                              ctypes.c_uint),
            ('metric_start',                           ctypes.c_uint),
            ('metric_pop',                             ctypes.c_uint),
            ('mmio_bar_list',                          MmioBar.v6*MAX_MMIO_BARS), # MMIO_BAR_INFO_NODE
            ('num_mmio_secondary_bar',                 ctypes.c_uint),
            ('reserved6',                              ctypes.c_uint),
            ('reserved7',                              ctypes.c_ulonglong),
            ('reserved8',                              ctypes.c_ulonglong),
            ('entries',                                ctypes.POINTER(EventReg.v3)), # EVENT_REG_NODE
        ]

ECB_direct2core_bit                      = 0x0001
ECB_bl_bypass_bit                        = 0x0002
ECB_pci_id_offset_bit                    = 0x0003
ECB_pcu_ccst_debug                       = 0x0004
ECB_VERSION                              = 2

class LBR_ENTRY_NODE_S(object): # LBR_ENTRY_NODE_S
    class v3(_Structure):
        _full_name_ = 'LBR_ENTRY_NODE_S_v3'
        _fields_ = [
            ('etype',                                  ctypes.c_ushort),
            ('type_index',                             ctypes.c_ushort),
            ('reg_id',                                 ctypes.c_uint),
        ]


class LBR_NODE_S(object): # LBR_NODE_S
    class v3(_Structure):
        _full_name_ = 'LBR_NODE_S_v3'
        _fields_ = [
            ('size',                                   ctypes.c_uint),
            ('num_entries',                            ctypes.c_uint),
            ('entries',                                LBR_ENTRY_NODE_S.v3),
        ]


class PWR_ENTRY_NODE_S(object): # PWR_ENTRY_NODE_S
    class v3(_Structure):
        _full_name_ = 'PWR_ENTRY_NODE_S_v3'
        _fields_ = [
            ('etype',                                  ctypes.c_ushort),
            ('type_index',                             ctypes.c_ushort),
            ('reg_id',                                 ctypes.c_uint),
        ]


class PWR_NODE_S(object): # PWR_NODE_S
    class v3(_Structure):
        _full_name_ = 'PWR_NODE_S_v3'
        _fields_ = [
            ('size',                                   ctypes.c_uint),
            ('num_entries',                            ctypes.c_uint),
            ('entries',                                PWR_ENTRY_NODE_S.v3),
        ]


class RO_ENTRY_NODE_S(object): # RO_ENTRY_NODE_S
    class v3(_Structure):
        _full_name_ = 'RO_ENTRY_NODE_S_v3'
        _fields_ = [
            ('reg_id',                                 ctypes.c_uint),
        ]


class RO_NODE_S(object): # RO_NODE_S
    class v3(_Structure):
        _full_name_ = 'RO_NODE_S_v3'
        _fields_ = [
            ('size',                                   ctypes.c_uint),
            ('num_entries',                            ctypes.c_uint),
            ('entries',                                RO_ENTRY_NODE_S.v3),
        ]

class PciId(object): # PCI_ID_NODE_S
    class v3(_Structure):
        _full_name_ = 'PciId_v3'
        _fields_ = [
            ('offset',    ctypes.c_uint),
            ('data_size', ctypes.c_uint),
        ]

def structures(version):
    class v3(object):
        FirstCommunicationMsg = FirstCommunicationMsg.v3
        FirstDataMsg          = FirstDataMsg.v3
        ControlMsg            = ControlMsg.v3
        RemoteOsInfo          = RemoteOsInfo.v3
        RemoteHardwareInfo    = RemoteHardwareInfo.v3
        DriverVersionInfo     = DriverVersionInfo.v3
        RemoteSwitch          = RemoteSwitch.v3
        TargetStatusMsg       = TargetStatusMsg.v3
        SetupInfo             = SetupInfo.v3
        PlatformInfo          = PlatformInfo.v3
        DrvConfig             = DrvConfig.v3
        DevUncConfig          = DevUncConfig.v3
        DrvTopology           = DrvTopology.v3
        DevConfig             = DevConfig.v3
        EventConfig           = EventConfig.v3
        Ecb                   = Ecb.v3
        PciDeviceEntry        = PciDeviceEntry.v3
        PmuOperation          = PmuOperation.v3
        EventRegId            = EventRegId.v3
        EventReg              = EventReg.v3
        PciId                 = PciId.v3
        EventDesc             = EventDesc.v3
        TaskInfo              = TaskInfo.v3
        SampleDropInfo        = SampleDropInfo.v3
        SampleRecordPC        = SampleRecordPC.v3
        ModuleRecord          = ModuleRecord.v3
        UncoreSampleRecordPC  = UncoreSampleRecordPC.v3
        DriverControlLog      = DriverControlLog.v3
    class v6(object):
        FirstCommunicationMsg = FirstCommunicationMsg.v6
        FirstDataMsg          = FirstDataMsg.v6
        ControlMsg            = ControlMsg.v6
        RemoteOsInfo          = RemoteOsInfo.v3
        RemoteHardwareInfo    = RemoteHardwareInfo.v3
        DriverVersionInfo     = DriverVersionInfo.v3
        RemoteSwitch          = RemoteSwitch.v3
        TargetStatusMsg       = TargetStatusMsg.v6
        SetupInfo             = SetupInfo.v6
        PlatformInfo          = PlatformInfo.v3
        DrvConfig             = DrvConfig.v6
        DevUncConfig          = DevUncConfig.v3
        DrvTopology           = DrvTopology.v3
        DevConfig             = DevConfig.v3
        EventConfig           = EventConfig.v3
        Ecb                   = Ecb.v6
        PciDeviceEntry        = PciDeviceEntry.v3
        PmuOperation          = PmuOperation.v3
        EventRegId            = EventRegId.v3
        EventReg              = EventReg.v3
        PciId                 = PciId.v3
        EventDesc             = EventDesc.v3
        TaskInfo              = TaskInfo.v3
        SampleDropInfo        = SampleDropInfo.v3
        SampleRecordPC        = SampleRecordPC.v3
        ModuleRecord          = ModuleRecord.v3
        UncoreSampleRecordPC  = UncoreSampleRecordPC.v3
        DriverControlLog      = DriverControlLog.v3

    return { 3: v3, 6: v6 }[version]
