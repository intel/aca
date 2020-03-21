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

import ctypes
import operator

import operation

from structures import structures
from channel import Channel, ChannelList, ChannelType


def create_channels(protocol_version, log):
    class Base(object):
        def __init__(self, log):
            self._log = log
            self._log.debug('COMMUNICATION CHANNELS - Initialisation')
            self._init_channels()

        def _init_channels(self):
            raise NotImplementedError()

        def close(self):
            raise NotImplementedError()

    class v3(Base):
        MAX_CORE_CHANNELS=16

        def _init_channels(self):
            self.control_channel = Channel(index=0, log=self._log)

            self.cpu_data_channels = ChannelList(length=self.MAX_CORE_CHANNELS, log=self._log)
            self.module_data_channel = Channel(index=self.MAX_CORE_CHANNELS, log=self._log)
            self.uncore_data_channel = Channel(index=self.MAX_CORE_CHANNELS + 1, log=self._log)

            self.data_channels = ChannelList(length=self.MAX_CORE_CHANNELS + 2, log=self._log)
            self.data_channels.includes(*self.cpu_data_channels.reserved)
            self.data_channels.includes(self.module_data_channel)
            self.data_channels.includes(self.uncore_data_channel)

        def close(self):
            self._log.debug('COMMUNICATION CHANNELS - Closing')
            for channel in self.data_channels:
                channel.close()
            self.control_channel.close()
            self._init_channels()

    class v6(Base):
        MAX_CORE_CHANNELS=100

        def _init_channels(self):
            self.control_channel = Channel(index=0, log=self._log)

            self.cpu_data_channels = ChannelList(length=self.MAX_CORE_CHANNELS, log=self._log)

            self.module_data_channel = None
            self.uncore_data_channel = None

            self.data_channels = ChannelList(length=self.MAX_CORE_CHANNELS + 2, log=self._log)
            self.data_channels.includes(*self.cpu_data_channels.reserved)

        def close(self):
            self._log.debug('COMMUNICATION CHANNELS - Closing')
            for channel in self.data_channels:
                channel.close()
            self.control_channel.close()
            self._init_channels()

    return {3: v3, 6: v6}[protocol_version](log)


class CommunicationException(Exception): pass

class Communication(object):
    def __init__(self, ip, port, protocol_version, log):
        self.log = log
        self._ip = ip
        self._port = port
        self._protocol_version = protocol_version

        self.log.debug('COMMUNICATION - Initialisation - {ip}:{port}'.format(**locals()))
        self.struct = structures(self._protocol_version)
        self.channels = create_channels(self._protocol_version, self.log)
        self.num_cpus = None

    def check_status(self, status):
        if status.status != 0:
            raise CommunicationException("ERROR: Incorrect 1st control message status - {0.status}".format(status))

    def check_protocol(self, message):
        if message.proto_version != self._protocol_version:
            raise CommunicationException("ERROR: protocol version id is different request={}, response={}".format(
                            self._protocol_version, message.proto_version))

    def init(self):
        self.log.info('COMMUNICATION Establishing connection to remote target')
        self.log.info('COMMUNICATION Creation of control communication channel')
        self.channels.control_channel.create(ChannelType.CONTROL)
        self.channels.control_channel.connect(self._ip, self._port, attempts=10)

        self.log.info('COMMUNICATION Sending handshake message to remote target')
        init_msg = self.struct.FirstCommunicationMsg()
        self.channels.control_channel.send_structure(init_msg)

        status_msg = self.channels.control_channel.receive_structure(self.struct.TargetStatusMsg)
        self.check_status(status_msg)
        self.check_protocol(status_msg)
        self.num_cpus = status_msg.remote_hardware_info.num_cpus

        self.log.info('COMMUNICATION Creation of data communication channels')
        self.channels.cpu_data_channels.create(
            global_indexes=range(status_msg.remote_hardware_info.num_cpus), channel_type=ChannelType.CORE)

        self.tmp = Channel(index=self.channels.data_channels.length - 2, log=self.log)
        self.tmp.create(ChannelType.NONE)
        self.channels.data_channels.includes(self.tmp)

        self.tmp = Channel(index=self.channels.data_channels.length - 1, log=self.log)
        self.tmp.create(ChannelType.NONE)
        self.channels.data_channels.includes(self.tmp)
        
        self.channels.data_channels.connect(self._ip, self._port)

        self.log.info('COMMUNICATION Send handshake to data communication channels')
        for channel in self.channels.data_channels:
            channel.send_structure(init_msg)
            data_msg = channel.receive_structure(self.struct.FirstDataMsg)
            self.check_protocol(data_msg)
            if data_msg.data_type == ChannelType.MODULE:
                channel.set_type(ChannelType.MODULE)
                self.channels.module_data_channel = channel
            if data_msg.data_type == ChannelType.UNCORE:
                channel.set_type(ChannelType.UNCORE)
                self.channels.uncore_data_channel = channel

    def run_operation(self, cmd_id, send_data="", rcv_data_size=0):
        control_message = self.struct.ControlMsg(
            command_id=cmd_id,
            from_target_data_size=rcv_data_size,
            to_target_data_size = len(send_data)
        )
        self.channels.control_channel.send_structure(control_message)
        if send_data:
            self.channels.control_channel.send(send_data)

        received_msg = self.channels.control_channel.receive_structure(self.struct.ControlMsg)
        received_msg = self.struct.ControlMsg.from_buffer(received_msg)

        if received_msg.command_id != control_message.command_id:
            raise CommunicationException("ERROR: Got incorret echo response from target")

        if (control_message.from_target_data_size == 0):
            return
        if (received_msg.status == 159 and (received_msg.command_id == 88 or received_msg.command_id == 85)):
            return
        return self.channels.control_channel.receive(control_message.from_target_data_size)

    def terminate(self):
        self.log.debug('COMMAND: TERMINATE')
        self.run_operation(cmd_id=operation.TERMINATE)

    def version(self):
        self.log.debug('COMMAND: VERSION')
        received_data = self.run_operation(cmd_id=operation.VERSION,
            rcv_data_size = ctypes.sizeof(self.struct.DriverVersionInfo))
        structure = self.struct.DriverVersionInfo.from_buffer(received_data)
        self.log.debug(structure.to_string())
        return structure

    def setup_info(self):
        self.log.debug('COMMAND: GET_DRV_SETUP_INFO')
        received_data = self.run_operation(cmd_id=operation.GET_DRV_SETUP_INFO,
            rcv_data_size = ctypes.sizeof(self.struct.SetupInfo))
        structure = self.struct.SetupInfo.from_buffer(received_data)
        self.log.debug(structure.to_string())
        return structure

    def sys_config_size(self):
        self.log.debug('COMMAND: COLLECT_SYS_CONFIG')
        received_data = self.run_operation(cmd_id=operation.COLLECT_SYS_CONFIG,
            rcv_data_size = ctypes.sizeof(ctypes.c_uint))
        buf_size = int(ctypes.c_uint.from_buffer(received_data).value)
        self.log.debug('Sys config size: {}'.format(buf_size))
        return buf_size

    def sys_config(self):
        self.log.debug('Collecting system configuration')
        buf_size = self.sys_config_size()
        if buf_size > 0:
            self.log.debug('COMMAND: GET_SYS_CONFIG')
            received_data = self.run_operation(cmd_id=operation.GET_SYS_CONFIG,
                rcv_data_size = buf_size)

    def platform_info(self):
        self.log.debug('Collecting platform configuration')
        self.log.debug('COMMAND: GET_PLATFORM_INFO')
        received_data = self.run_operation(cmd_id=operation.GET_PLATFORM_INFO,
            rcv_data_size = ctypes.sizeof(self.struct.PlatformInfo))
        structure = self.struct.PlatformInfo.from_buffer(received_data)
        self.log.debug(structure.to_string())
        return structure

    def init_num_devices(self):
        self.log.debug('Init num of devices')
        harcoded_num = ctypes.c_uint(4) # just core and imc uncore
        self.log.debug('COMMAND: INIT_NUM_DEV')
        received_data = self.run_operation(cmd_id=operation.INIT_NUM_DEV,
            send_data = bytearray(harcoded_num))

    def busy_driver(self):
        self.log.debug('COMMAND: RESERVE')
        received_data = self.run_operation(cmd_id=operation.RESERVE,
            rcv_data_size = ctypes.sizeof(ctypes.c_uint))
        busy = bool(ctypes.c_uint.from_buffer(received_data).value)
        self.log.debug('Driver busy status: {}'.format(busy))
        return busy

    def read_msr(self):
        self.log.debug('COMMAND: READ_MSR')
        value = ctypes.c_uint(702)
        self.run_operation(cmd_id=operation.READ_MSR, send_data = bytearray(value))

    def set_osid(self):
        self.log.debug('COMMAND: SET_OSID')
        self.run_operation(cmd_id=operation.SET_OSID)

    def control_driver_log(self):
        self.log.debug('COMMAND: CONTROL_DRIVER_LOG')
        config = self.struct.DriverControlLog()
        config.command = 1

        self.log.debug(config.to_string())
        received_data = self.run_operation(cmd_id=operation.CONTROL_DRIVER_LOG,
            send_data = bytearray(config))

    def driver_init_driver(self):
        self.log.debug('COMMAND: INIT_DRIVER')
        # hardcoded configuration
        config = self.struct.DrvConfig()
        config.size = 120
        config.version = 1
        config.num_events = 1
        config.sead_name = 'fake'
        config.seed_name_len = 5
#        config.collection_config = 17408
        config.read_pstate_msrs = 1
        config.ds_area_available = 1
        config.unc_timer_interval = 10
        config.unc_em_factor = 100
        config.p_state_trigger_index = -1
        self.log.debug(config.to_string())
        received_data = self.run_operation(cmd_id=operation.INIT_DRIVER,
            send_data = bytearray(config))

    def driver_init(self):
        self.log.debug('COMMAND: INIT')
        # hardcoded configuration
        config = self.struct.DevConfig()
        config.size             = 0
        config.version          = 0
        #config.dispatch_id      = 7
        config.dispatch_id      = 2
        config.results_offset   = 48
        config.max_gp_counters  = 4

        self.log.debug(config.to_string())
        received_data = self.run_operation(cmd_id=operation.INIT,
            send_data = bytearray(config))

    def driver_unc_init(self):
        self.log.debug('COMMAND: INIT_UNC')
#        # hardcoded configuration
        config = self.struct.DevUncConfig()
        config.dispatch_id = 120
        config.results_offset = 32 #ctypes.sizeof(self.struct.SampleRecordPC)
        config.device_type = 1
        received_data = self.run_operation(cmd_id=operation.INIT_UNC,
            send_data = bytearray(config))

    def set_driver_topology(self):
        if self.num_cpus is None:
            raise CommunicationException("ERROR: Cpu number is undefined")

        self.log.debug('COMMAND: SET_CPU_TOPOLOGY')
        # hardcoded configuration
        topology = (self.struct.DrvTopology * self.num_cpus)()
        #cpu 0 sm = 1 cm = 1 tm = 1 cpu_module_num 0 cpu_module_master = 1 system_master = 1
        #cpu 1 sm = 0 cm = 1 tm = 1 cpu_module_num 7 cpu_module_master = 1 system_master = 0
        #cpu 2 sm = 0 cm = 1 tm = 1 cpu_module_num 4 cpu_module_master = 1 system_master = 0
        #cpu 3 sm = 0 cm = 1 tm = 1 cpu_module_num 6 cpu_module_master = 1 system_master = 0
        #cpu 4 sm = 0 cm = 1 tm = 1 cpu_module_num 1 cpu_module_master = 1 system_master = 0
        #cpu 5 sm = 0 cm = 1 tm = 1 cpu_module_num 3 cpu_module_master = 1 system_master = 0
        #cpu 6 sm = 0 cm = 1 tm = 1 cpu_module_num 5 cpu_module_master = 1 system_master = 0
        #cpu 7 sm = 0 cm = 1 tm = 1 cpu_module_num 2 cpu_module_master = 1 system_master = 0

#        sep5_0: [DEBUG] DRV_OPERATION_SET_CPU_TOPOLOGY
#        sep5_0: [DEBUG] cpu 0 sm = 1 cm = 1 tm = 1
#        sep5_0: [DEBUG] cpu 1 sm = 0 cm = 1 tm = 1
#        sep5_0: [DEBUG] cpu 2 sm = 1 cm = 1 tm = 1
#        sep5_0: [DEBUG] cpu 3 sm = 0 cm = 1 tm = 1

        #cpu_module_nums = {0:0, 1:2, 2:4, 3:6, 4:5, 5:1, 6:3, 7:7}
        for idx in range(self.num_cpus):
            topology[idx].cpu_number        = idx
            topology[idx].cpu_package_num   = 0
            topology[idx].cpu_core_num      = 0
            topology[idx].cpu_hw_thread_num = 0
            topology[idx].socket_master     = 0
            if idx == 0:
                topology[idx].socket_master = 1
            topology[idx].core_master       = 1
            topology[idx].thr_master        = 1
            topology[idx].cpu_module_num    = idx #cpu_module_nums[idx]
            topology[idx].cpu_module_master = 1
            topology[idx].cpu_num_modules   = 2
            topology[idx].arch_perfmon_ver  = 4
            topology[idx].num_gp_counters   = 4
            topology[idx].num_fixed_counters= 3     

            self.log.debug(topology[idx].to_string())


        received_data = self.run_operation(cmd_id=operation.SET_CPU_TOPOLOGY,
            send_data = bytearray(topology))

    def set_event_config(self):
        self.log.debug('COMMAND: EM_GROUPS')
        # hardcoded configuration
        event_config = self.struct.EventConfig()
        event_config.num_groups         = 1
        event_config.em_mode            = -1
        event_config.em_factor          = -1
        event_config.em_event_num       = -1
        event_config.sample_size        = ctypes.sizeof(self.struct.SampleRecordPC) # 48
        event_config.max_gp_events      = 4
        event_config.max_fixed_counters = 3

        self.log.debug(event_config.to_string())

        received_data = self.run_operation(cmd_id=operation.EM_GROUPS,
            send_data = bytearray(event_config))


    def set_unc_event_config(self):
        self.log.debug('COMMAND: EM_GROUPS_UNC')
        # hardcoded configuration
        event_config = self.struct.EventConfig()
        event_config.num_groups         = 1
        event_config.em_mode            = -1
        event_config.em_factor          = -1
        event_config.em_event_num       = -1
        event_config.sample_size        = 0
        event_config.max_gp_events      = 4
        event_config.max_fixed_counters = 3
        event_config.num_groups_unc     = 1

        self.log.debug(event_config.to_string())

        received_data = self.run_operation(cmd_id=operation.EM_GROUPS_UNC,
            send_data = bytearray(event_config))

    def set_ecb(self):
        self.log.debug('COMMAND: EM_CONFIG_NEXT')
        # hardcoded configuration
        ecb = self.struct.Ecb()
        ecb.version            = 2
        ecb.num_entries        = 33
        ecb.num_events         = 3
        ecb.cccr_start         = 0
        ecb.cccr_pop           = 13
        ecb.escr_start         = 24
        ecb.escr_pop           = 9
        ecb.data_start         = 13
        ecb.data_pop           = 11
        ecb.size_of_allocation = 4720
#        ecb.size_of_allocation = 9072
        ecb.device_type        = 1

        self.log.debug(ecb.to_string())

        def bin_ecb_v3():
            with open("bin_ecb.config_3_0_CPU_CLK_UNHALTED.REF_TSC", 'rb') as dump:
                    received_data = self.run_operation(cmd_id=operation.EM_CONFIG_NEXT,
                        send_data = dump.read())
            return dump

        def bin_ecb_v6():
            with open("bin_ecb.config_6_0_CPU_CLK_UNHALTED.REF_TSC", 'rb') as dump:
                    received_data = self.run_operation(cmd_id=operation.EM_CONFIG_NEXT,
                        send_data = dump.read())
            return dump

        bin_ecb_switcher = {
                3: bin_ecb_v3,
                6: bin_ecb_v6
            }

        bin_ecb_switcher[self._protocol_version]()

    def set_unc_ecb(self):
        self.log.debug('COMMAND: EM_CONFIG_NEXT_UNC')
        # hardcoded configuration

        def bin_unc_ecb_v3():
            with open("bin_ecb.config_3_0_UNC_IMC_DRAM_RW_SLICE0_UNC_IMC_DRAM_RW_SLICE1", 'rb') as dump:
                    received_data = self.run_operation(cmd_id=operation.EM_CONFIG_NEXT_UNC,
                        send_data = dump.read())
            return dump

        def bin_unc_ecb_v6():
            with open("bin_ecb.config_6_0_UNC_IMC_DRAM_RW_SLICE0_UNC_IMC_DRAM_RW_SLICE1", 'rb') as dump:
                    received_data = self.run_operation(cmd_id=operation.EM_CONFIG_NEXT_UNC,
                        send_data = dump.read())
            return dump

        bin_unc_ecb_switcher = {
                3: bin_unc_ecb_v3,
                6: bin_unc_ecb_v6
            }

        bin_unc_ecb_switcher[self._protocol_version]()


    def set_device_num_units(self):
        self.log.debug('COMMAND: SET_DEVICE_NUM_UNITS')
        num_units = ctypes.c_uint(4)
        received_data = self.run_operation(cmd_id=operation.SET_DEVICE_NUM_UNITS,
            send_data = bytearray(num_units))

    def set_device_num_units_unc(self):
        self.log.debug('COMMAND: SET_DEVICE_NUM_UNITS')
        num_units = ctypes.c_uint(1)
        received_data = self.run_operation(cmd_id=operation.SET_DEVICE_NUM_UNITS,
            send_data = bytearray(num_units))

    def unc_desc_next(self):
        self.log.debug('COMMAND: DESC_NEXT')
        # hardcoded configuration
        desc = self.struct.EventDesc()
        desc.sample_size       = 56 #HARDCODE

#        desc.sample_size = ctypes.sizeof(self.struct.UncoreSampleRecordPC)

        desc.uncore_ebc_offset = ctypes.sizeof(self.struct.UncoreSampleRecordPC)

        self.log.debug(desc.to_string())

        received_data = self.run_operation(cmd_id=operation.DESC_NEXT,
            send_data = bytearray(desc))

    def desc_next(self):
        self.log.debug('COMMAND: DESC_NEXT')
        # hardcoded configuration
        desc = self.struct.EventDesc()
        desc.sample_size = ctypes.sizeof(self.struct.SampleRecordPC)
        self.log.debug(desc.to_string())

        received_data = self.run_operation(cmd_id=operation.DESC_NEXT,
            send_data = bytearray(desc))

    def setup_descriptors(self):
        self.log.debug('COMMAND: NUM_DESCRIPTOR')
        harcoded_num = ctypes.c_uint(2) #HARDCODE
        received_data = self.run_operation(cmd_id=operation.NUM_DESCRIPTOR,
            send_data = bytearray(harcoded_num))

    def get_tsc(self):
        self.log.debug('COMMAND: GET_NORMALIZED_TSC')
        received_data = self.run_operation(cmd_id=operation.GET_NORMALIZED_TSC,
            rcv_data_size = ctypes.sizeof(ctypes.c_ulonglong))
        tsc = int(ctypes.c_ulonglong.from_buffer(received_data).value)
        self.log.debug('Tsc: {}'.format(tsc))
        return tsc

    def get_tsc_skew(self):
        self.log.debug('COMMAND: TSC_SKEW_INFO')
        received_data = self.run_operation(cmd_id=operation.TSC_SKEW_INFO,
            rcv_data_size = 32)
        
        self.log.debug('Tsc: {}'.format(received_data))
        return received_data

    def get_thread_info(self):
        self.log.debug('COMMAND: GET_THREAD_COUNT')
        received_data = self.run_operation(cmd_id=operation.GET_THREAD_COUNT,
            rcv_data_size = ctypes.sizeof(ctypes.c_uint))
#        num_threads = int(ctypes.c_uint.from_buffer(received_data).value)
        num_threads = 0
        self.log.debug('Num of threads: {}'.format(num_threads))

        if num_threads > 0:
            self.log.debug('COMMAND: GET_THREAD_INFO')
            received_data = self.run_operation(cmd_id=operation.GET_THREAD_INFO,
                rcv_data_size = ctypes.sizeof(self.struct.TaskInfo) * num_threads)
            task_list = (self.struct.TaskInfo * num_threads).from_buffer(received_data)
            for item in task_list:
                self.log.debug(item.to_string())

    def init_pmu(self):
        if self.num_cpus is None:
            raise CommunicationException("ERROR: Cpu number is undefined")

        self.log.debug('COMMAND: INIT_PMU')
        # hardcoded configuration
        max_total_counters = 7
        #max_total_counters = 1
        s = self.num_cpus * max_total_counters * ctypes.sizeof(ctypes.c_ulonglong)
        buffer_size = ctypes.c_uint(self.num_cpus * max_total_counters * ctypes.sizeof(ctypes.c_ulonglong))
        self.log.debug('buffer size: {}'.format(buffer_size))
        received_data = self.run_operation(cmd_id=operation.INIT_PMU,
            send_data = bytearray(buffer_size))

    def get_num_cores(self):
        self.log.debug('COMMAND: NUM_CORES')
        received_data = self.run_operation(cmd_id=operation.NUM_CORES,
            rcv_data_size = ctypes.sizeof(ctypes.c_uint))
        num_cores = int(ctypes.c_uint.from_buffer(received_data).value)
        self.log.debug('Num cores: {}'.format(num_cores))
        return num_cores

    def get_num_samples(self):
        self.log.debug('COMMAND: GET_NUM_SAMPLES')
        received_data = self.run_operation(cmd_id=operation.GET_NUM_SAMPLES,
            rcv_data_size = ctypes.sizeof(ctypes.c_ulonglong))
        count_samples = int(ctypes.c_ulonglong.from_buffer(received_data).value)
        self.log.debug('Count samples: {}'.format(count_samples))
        return count_samples

    def get_sample_drop_info(self):
        self.log.debug('COMMAND: GET_SAMPLE_DROP_INFO')
        received_data = self.run_operation(cmd_id=operation.GET_SAMPLE_DROP_INFO,
            rcv_data_size = ctypes.sizeof(self.struct.SampleDropInfo))
        if received_data is None:
            return
        structure = self.struct.SampleDropInfo.from_buffer(received_data)
        self.log.debug(structure.to_string())
        return structure

    def driver_start(self):
        self.log.debug('COMMAND: START')
        for channel in self.channels.data_channels:
            channel.start_receive_thread(to_file=True)
        self.run_operation(cmd_id=operation.START)

    def driver_stop(self):
        self.log.debug('COMMAND: STOP')
        self.run_operation(cmd_id=operation.STOP)
        for channel in self.channels.data_channels:
            channel.stop_receive_thread()

    def check_core_data(self, module_of_interest, hotspot_instruction_adrress, threshold=100):
        total_by_iip = {}
        total_by_pid = {}
        total_samples = 0
        samples_on_cpus = 0
        for channel in self.channels.cpu_data_channels:
            data = channel.data_from_file()
            self.log.debug('Data: {}'.format(data))
            samples_number = len(data) / ctypes.sizeof(self.struct.SampleRecordPC)
            self.log.debug('Count of samples: {}'.format(samples_number))
            if samples_number:
                samples_on_cpus += 1
                samples_array = self.struct.SampleRecordPC * int(samples_number)
                array = samples_array.from_buffer(data)
                for sample in array:
                    total_samples += 1
#                    self.log.debug(sample.to_string())
                    iip = int(sample.iip)
                    pid = int(sample.pidRecIndex)

                    if iip not in total_by_iip.keys():
                        total_by_iip[iip] = 1
                    else:
                        total_by_iip[iip] += 1

                    if pid not in total_by_pid.keys():
                        total_by_pid[pid] = 1
                    else:
                        total_by_pid[pid] += 1

        if samples_on_cpus == 0:
            raise CommunicationException("ERROR: There are no samples")
        elif samples_on_cpus == 1:
            raise CommunicationException("ERROR: All Samples on one cpu only")

        number_samples_on_hotspot = 0
        for ip, samples_number in total_by_iip.items():
            if threshold >= abs(hotspot_instruction_adrress - ip):
                number_samples_on_hotspot += samples_number

        if float(number_samples_on_hotspot) / float(total_samples) <= 0.95:
            raise CommunicationException("ERROR: Number of samples on expected hotspot is low.")

        modules = self.check_module_data()
        if not module_of_interest in modules.keys():
            raise CommunicationException("ERROR: There are no {} module in module map".format(module_of_interest))
        load_adresses = modules[module_of_interest]
        samples_on_module_of_interest = 0
        for iip in total_by_iip.keys():
            for load_adress in load_adresses:
                if iip >= load_adress[0] and iip <= load_adress[1]:
                    samples_on_module_of_interest += total_by_iip[iip]
        self.log.debug('Samples by iip: {}'.format(sorted(total_by_iip.items(), key=operator.itemgetter(1), reverse=True)))
        self.log.debug('Samples by pid: {}'.format(sorted(total_by_pid.items(), key=operator.itemgetter(1), reverse=True)))
        self.log.debug('Samples by {} module of interest: {} from {}'.format(module_of_interest, samples_on_module_of_interest, total_samples))

        if samples_on_module_of_interest < 0.9 * total_samples:
            raise CommunicationException("ERROR: Too small samples are on {} module of interset".format(module_of_interest))


    def check_module_data(self):
        data = self.channels.module_data_channel.data_from_file()
        counter = 0
        current_point = 0
        record_size = ctypes.sizeof(self.struct.ModuleRecord)
        modules = {}
        while current_point < len(data):
            struct_data = data[current_point : current_point + record_size]
            record = self.struct.ModuleRecord.from_buffer(struct_data)
            module_name = data[current_point + record_size : current_point + record_size + record.pathLength - 1]
#            self.log.debug('Module record #{} in bytes [{}, {}]'.format(counter, current_point, current_point + record.recLength))
            module_name = str(module_name).strip()
#            self.log.debug('Module name {}'.format(module_name))
#            self.log.debug(record.to_string())
            current_point += record.recLength
            counter += 1
            if not module_name in modules.keys():
                modules[module_name] = []
            modules[module_name].append([record.loadAddr64, record.loadAddr64 + record.length64])

        return modules


    def check_uncore_data(self):
        data = self.channels.uncore_data_channel.data_from_file()
        counter = 0
        current_point = 0
        record_size = ctypes.sizeof(self.struct.UncoreSampleRecordPC)
        rec_length = 48 #HARDCODE
        prev_count = 0
        diffs = []
        while current_point < len(data):
            struct_data = data[current_point : current_point + record_size]
            record = self.struct.UncoreSampleRecordPC.from_buffer(struct_data)
            self.log.debug('Uncore record #{} in bytes [{}, {}]'.format(counter, current_point, current_point + rec_length))

            value_data_len = (rec_length - record_size) / ctypes.sizeof(ctypes.c_ulonglong)
            samples_array = ctypes.c_ulonglong * value_data_len
            value_data = data[current_point + record_size : current_point + rec_length]
            array = samples_array.from_buffer(value_data)
            values_info = []
            for value in array:
                values_info.append(int(value))
            if prev_count == 0:
                prev_count = sum(values_info[1:])
            diff = sum(values_info[1:]) - prev_count
            diffs.append(diff)
            self.log.debug('Uncore data {} - Sum: {} - Diff: {}'.format(values_info, sum(values_info[1:]), diff))
            prev_count = sum(values_info[1:])
            self.log.debug(record.to_string())
            current_point += rec_length
            counter += 1
        average_diff = sum(diffs[1:])/(counter - 1)
        if average_diff < 50000:
            raise CommunicationException("ERROR: The uncore DRAM bandwidth counters has too small value")
        good_diff_counts = sum( (diff > 0.85 * average_diff and diff < 1.15 * average_diff) for diff in diffs[1:])
        if good_diff_counts < 0.9 * (counter - 1):
            raise CommunicationException("ERROR: The uncore DRAM bandwidth values are incorrect")
        self.log.debug('Average Diff: {}'.format(average_diff))
        self.log.debug('Correct Diff Count: {} ({}%)'.format(good_diff_counts, good_diff_counts * 100 / (counter - 1)))


    def close(self):
        self.log.info('COMMUNICATION Closing')
        self.channels.close()
