#!/usr/bin/env python3

import itertools

class Encoding:
    def __init__(self, symbol_duration, packet_size, num_packets):
        self.sample_rate = 48000
        self.symbol_duration = symbol_duration
        self.packet_size = packet_size
        self.num_packets = num_packets
        self.page_size = packet_size * num_packets
        self.carrier_frequency = int(self.sample_rate / symbol_duration)

def GenerateEncodings():
    symbol_duration = (6, 8, 12, 16)
    packet_size = (52, 256, 1024)
    num_packets = (1, 4, 13)
    encodings = itertools.product(symbol_duration, packet_size, num_packets)
    return itertools.starmap(Encoding, encodings)
