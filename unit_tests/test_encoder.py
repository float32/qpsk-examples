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

class TestQPSKEncoder(unittest.TestCase):

    GOLDEN_CRC32 = 0xb3692966

    @classmethod
    def setUp(cls):
        random.seed(420)
        cls._data = bytes([random.randint(0, 0xFF) for i in range(10000)])

    def test_bin(self):
        with tempfile.NamedTemporaryFile() as binfile:
            binfile.write(self._data)

            with tempfile.NamedTemporaryFile() as wavfile:
                result = subprocess.run(['python3', 'encoder.py',
                    '-t', 'bin',
                    '-s', '48000',
                    '-c', '6000',
                    '-f', '1024',
                    '-p', '256',
                    '-w', '0.05',
                    '-i', binfile.name,
                    '-o', wavfile.name])
                wav_data = wavfile.read()
                crc = zlib.crc32(wav_data)

                self.assertEqual(self.GOLDEN_CRC32, crc)

    def test_hex(self):
        with tempfile.NamedTemporaryFile() as hexfile:
            ihex = IntelHex()
            ihex.frombytes(self._data)
            ihex.write_hex_file(hexfile.name, False)

            with tempfile.NamedTemporaryFile() as wavfile:
                result = subprocess.run(['python3', 'encoder.py',
                    '-t', 'hex',
                    '-s', '48000',
                    '-c', '6000',
                    '-f', '1024',
                    '-p', '256',
                    '-w', '0.05',
                    '-i', hexfile.name,
                    '-o', wavfile.name])
                wav_data = wavfile.read()
                crc = zlib.crc32(wav_data)

                self.assertEqual(self.GOLDEN_CRC32, crc)

if __name__ == '__main__':
    unittest.main()
