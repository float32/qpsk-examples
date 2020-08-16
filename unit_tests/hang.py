#!/usr/bin/env python3

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

from encoder import Encoder, QPSKModulator
import wave
import argparse
import sys

hang_states = [
    'sync',
    'prealign',
    'align',
    'write',
]

parser = argparse.ArgumentParser()
parser.add_argument('hang_state', choices=hang_states)
parser.add_argument('-o', dest='output_file', default='-')
parser.add_argument('-s', dest='sample_rate', type=int, default=48000)
parser.add_argument('-y', dest='symbol_rate', type=int, default=8000)
args = parser.parse_args()

sample_rate = args.sample_rate
symbol_rate = args.symbol_rate

enc = Encoder(symbol_rate, 256, 0)
mod = QPSKModulator(sample_rate, symbol_rate)

# Generate a signal which is interrupted by silence during the specified state.
symbols = []

if args.hang_state == 'sync':
    symbols += enc._encode_blank(0.405)
elif args.hang_state == 'prealign':
    symbols += enc._encode_intro()
    symbols += enc._encode_resync()
elif args.hang_state == 'align':
    symbols += enc._encode_intro()
    symbols += enc._encode_resync()
    for byte in enc._alignment_sequence[:-1]:
        symbols += enc._encode_byte(byte)
elif args.hang_state == 'write':
    symbols += enc._encode_intro()
    data = bytes([n % 256 for n in range(1024)])
    symbols += enc._encode_block(data)

signal = mod.modulate(symbols)
signal.extend([0] * sample_rate)

if args.output_file == '-':
    output_file = sys.stdout.buffer
else:
    output_file = args.output_file

with wave.open(output_file, 'wb') as writer:
    writer.setframerate(sample_rate)
    writer.setsampwidth(2)
    writer.setnchannels(1)
    writer.writeframes(signal.tobytes())
