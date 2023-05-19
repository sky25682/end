#!/usr/bin/env python3

import sys
import struct

def align_512(data):
    if len(data) % 512 != 0:
        data = data + b"\x00" * (512 - (len(data)%512))
    return data

if len(sys.argv) < 3:
    print("[ERROR] ImageMaker BootLoader.bin Kernel32.bin")
    sys.exit(-1)

disk_fd = open("Disk.img", "wb")
BootLoader_fd = open(sys.argv[1], "rb")
Kernel32_fd = open(sys.argv[2], "rb")
Kernel64_fd = open(sys.argv[3], "rb")


BootLoader_data = align_512(BootLoader_fd.read())
Kernel32_data = align_512(Kernel32_fd.read())
Kernel64_data = align_512(Kernel64_fd.read())

Disk_data = BootLoader_data + Kernel32_data + Kernel64_data

Kernel32_Sector_Count = int(len(Kernel32_data)/512)
Kernel64_Sector_Count = int(len(Kernel64_data)/512)
total_sector_count = Kernel32_Sector_Count + Kernel64_Sector_Count


#change TOTALSECTORCOUNT
Disk_data = Disk_data[:5] + struct.pack("<H", total_sector_count) + struct.pack("<H", Kernel32_Sector_Count) + Disk_data[9:]

disk_fd.write(Disk_data)
