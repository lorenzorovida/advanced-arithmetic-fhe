#So apparently is annoying to handle 257 bits in C++, so...

import sys

def int_to_bits_lsb(x, width=None):
    if width is None:
        width = max(1, x.bit_length())
    return [(x >> i) & 1 for i in range(width)]


bits = int(sys.argv[1])
b = int(sys.argv[2])

reciprocal = (1 << (bits + b.bit_length())) // b

#The MSB is always 1
arr = int_to_bits_lsb(reciprocal)[:bits]

print(arr)