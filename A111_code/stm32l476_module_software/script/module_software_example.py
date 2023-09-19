#!/usr/bin/python3
#######################################
# Copyright (c) Acconeer AB, 2020-2022
# All rights reserved
# This file is subject to the terms and
# conditions defined in the file
# 'LICENSES/license_acconeer.txt',
# which is part of this source code
# package.
#######################################
"""
This is a simple example how to communicate with the module software
over the UART interface.
"""
import argparse
import struct
import sys
import time

import serial


class ModuleError(Exception):
    """
    One of the error bits was set in the module
    """


class ModuleCommunication:
    """
    Simple class to communicate with the module software
    """
    def __init__(self, port, rtscts):
        self._port = serial.Serial(port, 115200, rtscts=rtscts,
                                   exclusive=True, timeout=2)

    def read_packet_type(self, packet_type):
        """
        Read any packet of packet_type. Any packages received with
        another type is discarded.
        """
        while True:
            header, payload = self._read_packet()
            if header[3] == packet_type:
                break
        return header, payload

    def _read_packet(self):
        header = self._port.read(4)
        length = int.from_bytes(header[1:3], byteorder='little')

        data = self._port.read(length + 1)
        assert data[-1] == 0xCD
        payload = data[:-1]
        return header, payload

    def register_write(self, addr, value):
        """
        Write a register
        """
        data = bytearray()
        data.extend(b'\xcc\x05\x00\xf9')
        data.append(addr)
        data.extend(value.to_bytes(4, byteorder='little', signed=False))
        data.append(0xcd)
        self._port.write(data)
        _header, payload = self.read_packet_type(0xF5)
        assert payload[0] == addr

    def register_read(self, addr):
        """
        Read a register
        """
        data = bytearray()
        data.extend(b'\xcc\x01\x00\xf8')
        data.append(addr)
        data.append(0xcd)
        self._port.write(data)
        _header, payload = self.read_packet_type(0xF6)
        assert payload[0] == addr
        return int.from_bytes(payload[1:5], byteorder='little', signed=False)

    def buffer_read(self, offset):
        """
        Read the buffer
        """
        data = bytearray()
        data.extend(b'\xcc\x03\x00\xfa\xe8')
        data.extend(offset.to_bytes(2, byteorder='little', signed=False))
        data.append(0xcd)
        self._port.write(data)

        _header, payload = self.read_packet_type(0xF7)
        assert payload[0] == 0xE8
        return payload[1:]

    def read_stream(self):
        """
        Read a stream of data
        """
        _header, payload = self.read_packet_type(0xFE)
        return payload

    @staticmethod
    def _check_error(status):
        ERROR_MASK = 0xFFFF0000
        if status & ERROR_MASK != 0:
            ModuleError(f"Error in module, status: 0x{status:08X}")

    @staticmethod
    def _check_timeout(start, max_time):
        if (time.monotonic() - start) > max_time:
            raise TimeoutError()

    def _wait_status_set(self, wanted_bits, max_time):
        """
        Wait for wanted_bits bits to be set in status register
        """
        start = time.monotonic()

        while True:
            status = self.register_read(0x6)
            self._check_timeout(start, max_time)
            self._check_error(status)

            if status & wanted_bits == wanted_bits:
                return
            time.sleep(0.1)

    def wait_start(self):
        """
        Poll status register until created and activated
        """
        ACTIVATED_AND_CREATED = 0x3
        self._wait_status_set(ACTIVATED_AND_CREATED, 3)

    def wait_for_data(self, max_time):
        """
        Poll status register until data is ready
        """
        DATA_READY = 0x00000100
        self._wait_status_set(DATA_READY, max_time)


def _polling_mode_distance(com, duration):
    # Wait for it to start
    com.wait_start()
    print('Sensor activated')

    # Read out distance start
    dist_start = com.register_read(0x81)
    print(f'dist_start={dist_start / 1000} m')

    dist_length = com.register_read(0x82)
    print(f'dist_length={dist_length / 1000} m')

    start = time.monotonic()
    while time.monotonic() - start < duration:
        com.register_write(3, 4)
        # Wait for data read
        com.wait_for_data(2)
        dist_count = com.register_read(0xB0)
        print('                                               ', end='\r')
        print(f'Detected {dist_count} peaks:', end='')
        for count in range(dist_count):
            dist_distance = com.register_read(0xB1 + 2 * count)
            dist_amplitude = com.register_read(0xB2 + 2 * count)
            print(f' dist_{count}_distance={dist_distance / 1000} m', end='')
            print(f' dist_{count}_amplitude={dist_amplitude}', end='')
        print('', end='', flush=True)
        time.sleep(0.3)


def _polling_mode_presence(com, duration):
    # Wait for it to start
    com.wait_start()
    print('Sensor activated')

    start = time.monotonic()
    while time.monotonic() - start < duration:
        com.register_write(3, 4)
        # Wait for data read
        com.wait_for_data(2)
        presence = com.register_read(0xB0)
        score = com.register_read(0xB1)
        distance = com.register_read(0xB2) / 1000
        print(f'Presence: {"True" if presence else "False"} score={score} distance={distance} m')
        time.sleep(0.3)


def _decode_streaming_buffer(stream):
    assert stream[0] == 0xFD

    offset = 3

    result_info = {}

    while stream[offset] != 0xFE:
        address = stream[offset]
        offset += 1
        value = int.from_bytes(stream[offset:offset + 4], byteorder='little')
        offset += 4
        result_info[address] = value

    buffer = stream[offset + 3:]
    return result_info, buffer


def _streaming_mode_presence(com, duration):
    start = time.monotonic()
    while time.monotonic() - start < duration:
        stream = com.read_stream()
        _result_info, buffer = _decode_streaming_buffer(stream)

        (presence, score, distance) = struct.unpack("<bff", buffer)

        print(f'Presence: {"True" if presence else "False"} score={score} distance={distance} m')


def _streaming_mode_distance(com, duration):
    start = time.monotonic()
    while time.monotonic() - start < duration:
        stream = com.read_stream()
        result_info, buffer = _decode_streaming_buffer(stream)

        dist_count = result_info[0xB0]
        print('                                               ', end='\r')
        print(f'Detected {dist_count} peaks:', end='')
        for count in range(dist_count):
            (dist_amplitude, dist_distance) = struct.unpack("<Hf", buffer[count * 6:count * 6 + 6])
            print(f' dist_{count}_distance={dist_distance} m', end='')
            print(f' dist_{count}_amplitude={dist_amplitude}', end='')
        print('', end='', flush=True)


def module_software_test(port, flowcontrol, mode, streaming, duration):
    """
    A simple example demonstrating how to use the distance detector
    """
    print(f'Communicating with module software on port {port}')
    com = ModuleCommunication(port, flowcontrol)

    # Make sure that module is stopped
    com.register_write(0x03, 0)

    # Give some time to stop (status register could be polled too)
    time.sleep(0.5)

    # Clear any errors and status
    com.register_write(0x3, 4)

    # Read product ID
    product_identification = com.register_read(0x10)
    print(f'product_identification=0x{product_identification:08X}')

    version = com.buffer_read(0)
    print(f'Software version: {version}')

    if mode == 'presence':
        # Set mode to 'presence'
        com.register_write(0x2, 0x400)
    elif mode == 'distance':
        # Set mode to 'distance'
        com.register_write(0x2, 0x200)

    # Update rate 1 Hz
    com.register_write(0x23, 1000)

    if streaming:
        # Enable UART streaming mode
        com.register_write(5, 1)
    else:
        # Disable UART streaming mode
        com.register_write(5, 0)

    # Activate and start
    com.register_write(3, 3)

    if mode == 'presence':
        if streaming:
            _streaming_mode_presence(com, duration)
        else:
            _polling_mode_presence(com, duration)
    elif mode == 'distance':
        if streaming:
            _streaming_mode_distance(com, duration)
        else:
            _polling_mode_distance(com, duration)

    print()
    print('End of example')
    com.register_write(0x03, 0)


def main():
    """
    Main entry function
    """
    parser = argparse.ArgumentParser(description='Test UART communication')
    parser.add_argument('--port', default="/dev/ttyUSB0",
                        help='Port to use, e.g. COM1 or /dev/ttyUSB0')
    parser.add_argument('--no-rtscts', action='store_true',
                        help='XM132 and XM122 use rtscts, XM112 does not')
    parser.add_argument('--duration', default=30,
                        help='Duration of the example', type=int)
    parser.add_argument('--streaming', action='store_true',
                        help='Use UART streaming protocol')
    parser.add_argument('--mode', choices=['presence', 'distance'],
                        help='Mode to use', default="distance")

    args = parser.parse_args()
    module_software_test(args.port, not args.no_rtscts, args.mode, args.streaming, args.duration)


if __name__ == "__main__":
    sys.exit(main())
