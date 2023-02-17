#! /usr/bin/env python3

import sys
import re

class FuseProfile:
    def __init__(self, fuse_profile_file):
        self.cycle_counts = {};
        f = open(fuse_profile_file)
        r = re.compile("0x([0-9A-Fa-f]{4}),([0-9]+)")
        for line in f:
            parsed = r.match(line).groups()
            addr = int(parsed[0], base=16)
            count = int(parsed[1])
            self.cycle_counts[addr] = count

def ms_from_cycles(cycles):
    return (1000*cycles)/3500000;

class FuncData:

    def __init__(self, symbol, addr):
        self.symbol = symbol
        self.addr = addr
        self.count = 0

    def printData(self, cycles_total):
        return "{}: {:4X}-{:4X} {:>10} {:>10.1f} ms {:>5.1f} %"\
            .format(self.symbol, self.addr, self.addr_end, self.count, \
            ms_from_cycles(self.count), 100.0 * self.count/cycles_total)

class MapFile:
    functions = []

    def __init__(self, map_file):
        f = open(map_file)
        r = re.compile("(.+) *= *([0-9A-Fa-f]{4}), ([GL]): (.*)")
        functions_dict = {}
        for line in f:
            if len(line.strip()) > 0:
                parsed = r.match(line).groups()
                (symbol, addr_s, globallocal, module) = parsed
                addr = int(addr_s, base=16)
                # Only global (exported) functions, skip labels
                if globallocal == 'G':
                    functions_dict[symbol] = FuncData(symbol, addr)
        # Remove duplicates - functions are duplicated in the map file
        self.functions = list(functions_dict.values())
        self.functions = sorted(self.functions, key = lambda func: func.addr)
        for i in range(len(self.functions)-1):
            self.functions[i].addr_end = self.functions[i+1].addr-1
        self.functions[-1].addr_end = 0xFFFF;
        other = FuncData("other                           ", 0)
        other.addr_end = self.functions[0].addr-1
        self.functions.append(other)

    def printProfile(self):
        cycles_total = 0
        for func in self.functions:
               cycles_total += func.count
        for func in self.functions:
            print(func.printData(cycles_total))
        print("============================================================================")
        print("total:                                      {:>10} {:>10.1f} ms   100 %"\
            .format(cycles_total, ms_from_cycles(cycles_total)))

    def find_func(self, addr):
        for func in self.functions:
            if (func.addr <= addr) and (addr <= func.addr_end):
                return func
        return None

if len(sys.argv) == 3:
    fuse_profile = FuseProfile(sys.argv[1]);
    map_file = MapFile(sys.argv[2]);

    for addr, count in fuse_profile.cycle_counts.items():
        func = map_file.find_func(addr)
        if func:
            func.count = func.count + count

    map_file.printProfile()
else:
    print("Params: file.profile file.map")
    print("")
    print("Generates a report of CPU cycles spent in each function from the FUSE")
    print("emulator profile file and z88dk .map file.")

