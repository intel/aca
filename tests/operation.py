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
START                         = 1 # covered
STOP                          = 2 # covered
INIT_PMU                      = 3 # covered
INIT                          = 4 # covered
EM_GROUPS                     = 5 # covered
SET_CPU_MASK                  = 17
PCI_READ                      = 18
PCI_WRITE                     = 19
READ_PCI_CONFIG               = 20
FD_PHYS                       = 21
WRITE_PCI_CONFIG              = 22
INSERT_MARKER                 = 23
GET_NORMALIZED_TSC            = 24 # covered
EM_CONFIG_NEXT                = 25 # covered !!!! Hard Coded
SYS_CONFIG                    = 26
TSC_SKEW_INFO                 = 27 # TODO
NUM_CORES                     = 28 # covered
COLLECT_SYS_CONFIG            = 29 # covered
GET_SYS_CONFIG                = 30 # covered
PAUSE                         = 31
RESUME                        = 32
SET_ASYNC_EVENT               = 33
ASYNC_STOP                    = 34
TERMINATE                     = 35 # covered
READ_MSRS                     = 36
LBR_INFO                      = 37
RESERVE                       = 38 # covered
MARK                          = 39
AWAIT_STOP                    = 40
SEED_NAME                     = 41
KERNEL_CS                     = 42
SET_UID                       = 43
VERSION                       = 51 # covered
CHIPSET_INIT                  = 52
GET_CHIPSET_DEVICE_ID         = 53
SWITCH_GROUP                  = 54
GET_NUM_CORE_CTRS             = 55
PWR_INFO                      = 56
NUM_DESCRIPTOR                = 57 # covered
DESC_NEXT                     = 58 # covered
MARK_OFF                      = 59
CREATE_MARKER                 = 60
GET_DRIVER_STATE              = 61
READ_SWITCH_GROUP             = 62
EM_GROUPS_UNC                 = 63 # covered
EM_CONFIG_NEXT_UNC            = 64 # hard coded
INIT_UNC                      = 65 # covered
RO_INFO                       = 66
READ_MSR                      = 67
WRITE_MSR                     = 68
THREAD_SET_NAME               = 69
GET_PLATFORM_INFO             = 70 # covered
GET_NORMALIZED_TSC_STANDALONE = 71
READ_AND_RESET                = 72
SET_CPU_TOPOLOGY              = 73 # covered
INIT_NUM_DEV                  = 74 # covered
SET_GFX_EVENT                 = 75
GET_NUM_SAMPLES               = 76 # covered
SET_PWR_EVENT                 = 77
SET_DEVICE_NUM_UNITS          = 78 # covered
TIMER_TRIGGER_READ            = 79
GET_INTERVAL_COUNTS           = 80
FLUSH                         = 81
SET_SCAN_UNCORE_TOPOLOGY_INFO = 82
GET_UNCORE_TOPOLOGY           = 83
GET_MARKER_ID                 = 84
GET_SAMPLE_DROP_INFO          = 85 # covered
GET_DRV_SETUP_INFO            = 86 # covered
GET_PLATFORM_TOPOLOGY         = 87
GET_THREAD_COUNT              = 88 # covered
GET_THREAD_INFO               = 89 # covered
GET_DRIVER_LOG                = 90
CONTROL_DRIVER_LOG            = 91
SET_OSID                      = 92
GET_AGENT_MODE                = 93
INIT_DRIVER                   = 94
SET_EMON_BUFFER_DRIVER_HELPER = 95
