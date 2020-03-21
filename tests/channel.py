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

import time
import socket
import errno
import ctypes

from threading import Thread


class ChannelType(object):
    class __type(object):
        def __init__(self, name, value):
            self.name = name
            self.__value = value

        def __str__(self):
            return str(self.__value)

        def __eq__(self, second):
            return self.__value == second

    NONE    = __type('NONE', -2)
    CONTROL = __type('CONTROL', -1)
    CORE    = __type('CORE', 0)
    MODULE  = __type('MODULE', 1)
    UNCORE  = __type('UNCORE', 2)


class ChannelException(Exception): pass

class Channel(object):
    def __init__(self, index, log):
        self.global_index = index
        self.__log = log
        self.__is_file_busy = False
        self.__file_name = None
        self.__created = False
        self.__channel_type = None
        self.__socket = None
        self.__connected = False
        self.__listener = None
        self.__clean()

    def __clean(self):
        self.__socket = None
        self.__listener = None
        self.__is_file_busy = False
        self.__channel_type = ChannelType.NONE

    def __check_socket(self):
        if not self.__socket:
            raise ChannelException("ERROR: Channel.create() wasn't called")

    def __send_data(self, data):
        ret = self.__socket.sendall(data)
        if ret is None:
            return True

    def __receive_data(self, length):
        if self.__is_file_busy:
            raise ChannelException("ERROR: Receving data to buffer while the file receiving")

        data = bytearray()
        while len(data) < length:
            try:
                packet = self.__socket.recv(length - len(data))
            except IOError as err:
                raise ChannelException("ERROR: Data transfering failed. With exception {}".format(err))

            if not packet:
                break
            data += bytearray(packet)
        return data

    def __close_socket(self):
        if self.__socket is not None:
            try:
#                self.__socket.shutdown(socket.SHUT_RDWR)
                self.__socket.close()
            except IOError:
                if self.__connected == False:
                    self.__connected = True
                else:
                    raise ChannelException("ERROR: Cannot close channel")

    @property
    def info(self):
        return "CHANNEL {}#{}".format(self.__channel_type.name, self.global_index)

    @property
    def type(self):
        return self.__channel_type

    def set_type(self, type):
        self.__channel_type = type

    def is_created(self):
        return self.__created

    def create(self, channel_type):
        self.__created = True
        self.__channel_type = channel_type
        self.__log.debug('{} - Creation. "data_{}.{}.bin"'.format(self.info, self.__channel_type.name, self.global_index))
        self.__file_name = "data_{}.{}.bin".format(self.__channel_type.name, self.global_index)
        self.__socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    def connect(self, ip, port, attempts=1):
        self.__check_socket()
        for _ in range(attempts):
            try:
                self.__socket.connect((ip, port))
                break
            except socket.error as error:
                if error.errno != errno.ECONNREFUSED:
                    raise
                time.sleep(0.1)
        else:
            raise ChannelException("ERROR: Cannot connect to target socket")
        self.__connected = True

    def send(self, data):
        self.__check_socket()
        self.__log.debug('{0} - Sending message: {data}'.format(self.info, **locals()))
        self.__send_data(data)

    def send_structure(self, structure):
        self.__check_socket()
        self.__log.debug('{} - Sending structure: {}'.format(self.info, structure.to_string()))
        self.__send_data(structure)

    def receive(self, length):
        self.__check_socket()
        data = self.__receive_data(length)
        self.__log.debug('{0} - Received data length of {length}: {data}'.format(self.info, **locals()))
        return data

    def receive_structure(self, structure_type):
        data = self.__receive_data(length=ctypes.sizeof(structure_type))
        structure = structure_type.from_buffer(data)
        self.__log.debug('{0} - Received structure: {1}'.format(self.info, structure.to_string()))
        return structure

    def start_receive_thread(self, to_file=False):
        def listen_to_file():
            self.__is_file_busy = True
            with open(self.__file_name, 'wb') as file_obj:
                try:
                    packet = self.__socket.recv(1024)
                    while packet:
                        file_obj.write(packet)
                        packet = self.__socket.recv(1024)
                except IOError as err:
                    pass
                    #print("ERROR: Data transfering failed on {}. With exception {}".format(self.info, err))
#                    raise ChannelException("ERROR: Data transfering failed on {}. With exception {}".format(self.info, err))

            self.__is_file_busy = False

        if to_file:
            self.__listener = Thread(target=listen_to_file)
        else:
            NotImplementedError("Possible to write only to file now")
        self.__listener.start()

    def stop_receive_thread(self):
        if self.__listener:
            self.__listener.join()
            self.__listener = None

    def data_from_file(self):
        with open(self.__file_name, 'rb') as file_obj:
            return bytearray(file_obj.read())

    def close(self):
        self.__log.debug('{} - Closing'.format(self.info))
        self.stop_receive_thread()
        self.__close_socket()
        self.__clean()


class ChannelList(object):
    def __init__(self, length, log):
        self._log = log
        self._length = length
        self._channels = [Channel(global_index, log=self._log) for global_index in range(length)]
        self._module_channel = None
        self._uncore_channel = None

    def __iter__(self):
        return iter([channel for channel in self._channels if channel.is_created()])

    @property
    def reserved(self):
        return iter(self._channels)

    @property
    def length(self):
        return self._length

    def create(self, global_indexes, channel_type):
        for global_index in global_indexes:
            if global_index >= self._length:
                raise ChannelException("ERROR: Not possible to create data channel. Max count is reached")
            self._channels[global_index].create(channel_type)

    def connect(self, ip, port, attempts=1):
        for channel in self:
            channel.connect(ip, port, attempts)

    def includes(self, *channels):
        for channel in channels:
            self._channels[channel.global_index] = channel

    def close(self):
        for channel in self._channels:
            if channel.is_initialized():
               channel.close()
