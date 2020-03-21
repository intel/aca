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

import unittest
import time

from config import Config
from communication import Communication
from log import log


class Test(unittest.TestCase):
    def __init__(self, config):
        unittest.TestCase.__init__(self)
        self.config = config
        self.communication = None

    def setUp(self):
        self.communication = Communication(
            self.config.target_ip,
            self.config.target_port,
            self.config.protocol_version,
            log=log
        )

    def tearDown(self):
        self.communication.close()

class TerminateTest(Test):
    def runTest(self):
        self.communication.init()
        self.communication.terminate()

class VersionTest(Test):
    def runTest(self):
        self.communication.init()
        self.communication.version()
        self.communication.terminate()

class SetupInfoTest(Test):
    def runTest(self):
        self.communication.init()
        self.communication.setup_info()
        self.communication.terminate()

class SysConfigSizeTest(Test):
    def runTest(self):
        self.communication.init()
        self.communication.sys_config_size()
        self.communication.terminate()

class SysConfigTest(Test):
    def runTest(self):
        self.communication.init()
        self.communication.sys_config()
        self.communication.terminate()

class PlatformInfoTest(Test):
    def runTest(self):
        self.communication.init()
        self.communication.platform_info()
        self.communication.terminate()

class InitNumDeviceTest(Test):
    def runTest(self):
        self.communication.init()
        self.communication.init_num_devices()
        self.communication.terminate()

class BusyDriverTest(Test):
    def runTest(self):
        self.communication.init()
        self.communication.busy_driver()
        #TODO free before terminate
        self.communication.terminate()

class SetEventConfigTest(Test):
    def runTest(self):
        self.communication.init()
        self.communication.set_event_config()
        self.communication.terminate()

class GetTscTest(Test):
    def runTest(self):
        self.communication.init()
        tsc_list = []
        for _ in range(100):
            tsc_list.append(self.communication.get_tsc())
        self.assertEqual(sorted(tsc_list), tsc_list, "tsc growth is not monotonic")
        self.communication.terminate()

class GetThreadInfoTest(Test):
    def runTest(self):
        self.communication.init()
        self.communication.get_thread_info()
        self.communication.terminate()

class GetNumCoresTest(Test):
    def runTest(self):
        self.communication.init()
        num_cores = self.communication.get_num_cores()
        self.assertEqual(num_cores, self.config.cores_number, "Number of cores is incorrect.")
        self.communication.terminate()

class CollectionTest(Test):
    def __init__(self, config):
        self.enable_uncore = False
        self.collection_time = 5
        Test.__init__(self, config)

    def runTest(self):
        application = None
        self.communication.init()
        self.communication.set_osid()
        self.communication.version()
        self.communication.setup_info()

        i = 0
        while i in range(4):
            self.communication.sys_config()
            self.communication.platform_info()
            i += 1

        self.communication.get_num_cores()
        self.communication.read_msr()

        self.communication.get_tsc()
        self.communication.get_tsc()

        self.communication.control_driver_log()
        self.communication.init_num_devices()
        self.communication.busy_driver()
        self.communication.driver_init_driver()

        self.communication.set_driver_topology()
        self.communication.setup_descriptors()
        self.communication.desc_next()
        self.communication.desc_next()
        self.communication.driver_init()

        self.communication.set_event_config()  # separatly
        self.communication.set_ecb()
        
        self.communication.set_device_num_units()

#uncore
        if self.enable_uncore:
            self.communication.driver_unc_init()
            self.communication.set_unc_event_config()
            self.communication.set_unc_ecb()
            self.communication.set_device_num_units_unc()
#uncore end

        self.communication.get_tsc()
#        self.communication.get_thread_info()
        self.communication.init_pmu()

        time.sleep(3)
        self.communication.driver_start()
        time.sleep(5)
        self.communication.driver_stop()

        if self.enable_uncore:
            self.communication.get_tsc()
            self.communication.get_tsc_skew()
            self.communication.sys_config()
            self.communication.platform_info()
            self.communication.get_num_cores()

        self.communication.get_num_samples()
        self.communication.get_sample_drop_info()
        #TODO free before terminate
        self.communication.terminate()

        #check results
        if not self.enable_uncore:
            self.communication.check_core_data(self.config.testapp, self.config.testapp_hotspot_ip, threshold=5)
        else:
            self.communication.check_uncore_data()

class UncoreCollectionTest(CollectionTest):
    def __init__(self, config):
        CollectionTest.__init__(self, config)
        self.enable_uncore = True
        self.collection_time = 5

    def setUp(self):
        if not self.config.uncore_supported:
            raise unittest.SkipTest('SKU does not support uncore events.')
        CollectionTest.setUp(self)


if __name__ == '__main__':
    test_config = Config()
    test_suite = unittest.TestSuite()

    test_suite.addTest(TerminateTest(test_config))
    test_suite.addTest(VersionTest(test_config))
    test_suite.addTest(SetupInfoTest(test_config))
    test_suite.addTest(SysConfigSizeTest(test_config))
    test_suite.addTest(SysConfigTest(test_config))
    test_suite.addTest(PlatformInfoTest(test_config))
    test_suite.addTest(InitNumDeviceTest(test_config))
    test_suite.addTest(BusyDriverTest(test_config))
    test_suite.addTest(SetEventConfigTest(test_config))
    test_suite.addTest(GetTscTest(test_config))
    # test_suite.addTest(GetThreadInfoTest(test_config)) #status 159. blocking
    test_suite.addTest(GetNumCoresTest(test_config))
    # test_suite.addTest(CollectionTest(test_config))
    # test_suite.addTest(UncoreCollectionTest(test_config))

    runner=unittest.TextTestRunner(verbosity=2)
    runner.run(test_suite)
