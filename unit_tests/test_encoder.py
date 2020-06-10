#!/usr/bin/env python3
#
# MIT License
#
# Copyright 2020 Tyler Coy
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

import unittest
from intelhex import IntelHex
import tempfile
import subprocess
import random
import zlib
import io
import codecs

class TestQPSKEncoder(unittest.TestCase):

    GOLDEN_CRC32 = 0xb3692966

    @classmethod
    def setUp(cls):
        random.seed(420)
        cls._bin_data = bytes([random.randint(0, 0xFF) for i in range(10000)])
        ihex = IntelHex()
        ihex.frombytes(cls._bin_data)
        hex_data = io.StringIO()
        ihex.write_hex_file(hex_data, write_start_addr=False)
        cls._hex_data = codecs.encode(hex_data.getvalue(), encoding='ascii')
        cls._args = ['python3', 'encoder.py',
            '-s', '48000',
            '-y', '6000',
            '-b', '1K',
            '-f', '1K:40',
            '-a', '0',
            '-w', '10',
            '-p', '256']

    def test_bin(self):
        with tempfile.NamedTemporaryFile() as binfile:
            binfile.write(self._bin_data)
            with tempfile.NamedTemporaryFile() as wavfile:
                result = subprocess.run(self._args +
                    ['-t', 'bin', '-i', binfile.name, '-o', wavfile.name],
                    capture_output=True)
                try:
                    result.check_returncode()
                except:
                    print(result.stderr.decode('utf-8'))
                wav_data = wavfile.read()
                crc = zlib.crc32(wav_data)
                self.assertEqual(self.GOLDEN_CRC32, crc)

    def test_hex(self):
        with tempfile.NamedTemporaryFile() as hexfile:
            hexfile.write(self._hex_data)
            with tempfile.NamedTemporaryFile() as wavfile:
                result = subprocess.run(self._args +
                    ['-t', 'hex', '-i', hexfile.name, '-o', wavfile.name],
                    capture_output=True)
                try:
                    result.check_returncode()
                except:
                    print(result.stderr.decode('utf-8'))
                wav_data = wavfile.read()
                crc = zlib.crc32(wav_data)
                self.assertEqual(self.GOLDEN_CRC32, crc)

    def test_bin_stdio(self):
        result = subprocess.run(self._args + ['-t', 'bin'],
            capture_output=True, input=self._bin_data)
        try:
            result.check_returncode()
        except:
            print(result.stderr.decode('utf-8'))
        wav_data = result.stdout
        crc = zlib.crc32(wav_data)
        self.assertEqual(self.GOLDEN_CRC32, crc)

    def test_hex_stdio(self):
        result = subprocess.run(self._args + ['-t', 'hex'],
            capture_output=True, input=self._hex_data)
        try:
            result.check_returncode()
        except:
            print(result.stderr.decode('utf-8'))
        wav_data = result.stdout
        crc = zlib.crc32(wav_data)
        self.assertEqual(self.GOLDEN_CRC32, crc)

if __name__ == '__main__':
    unittest.main()
