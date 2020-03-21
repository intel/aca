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

import argparse


class Config(object):
    def __init__(self):
        parser = argparse.ArgumentParser(description="Tests for remote SEP target.")
        parser.add_argument(dest='target_ip', help='IP address for the remote target')
        parser.add_argument(dest='config_type', help='target type defined in config.py')
        parser.add_argument('-p', '--port', dest='target_port', default=9321,
                            help='remote target port', type=int)
        args = parser.parse_args()

        self.target_ip = args.target_ip
        self.target_port = args.target_port
        self.cores_number = None
        self.uncore_supported = False

        try:
            getattr(self, args.config_type)()
        except AttributeError:
            raise Exception("Incorrect config type specified.")

    def ApolloLakePremiumSKU(self):
        self.cores_number = 4
        self.testapp = '/usr/user/test'
        self.testapp_hotspot_ip = 0x804900F
        self.uncore_supported = False
        self.protocol_version = 6

    def Xeon(self):
        self.cores_number = 88
        self.testapp = '/home/vtune/workspace/sampling/ref_tests/apps/one_test/test'
        self.testapp_hotspot_ip = 0x400B0D
        self.uncore_supported = False
        self.protocol_version = 6

