
import sys
import re

# Params: file.profile file.map
# 
# Generates a report of CPU cycles spent in each function from the FUSE 
# emulator profile file and z88dk .map file.

class FuseProfile:
	def __init__(self, fuse_profile_file):
		self.cycle_counts = {};
		f = file(fuse_profile_file)
		r = re.compile("0x([0-9A-Fa-f]{4}),([0-9]+)")
		for line in f:
			print line
			parsed = r.match(line).groups()
			print parsed
			addr = int(parsed[0], base=16)
			count = int(parsed[1])
			print addr, count
			self.cycle_counts[addr] = count

def ms_from_cycles(cycles):
	return (1000*cycles)/3500000;

class FuncData:
  
	def __init__(self, symbol, addr):
		self.symbol = symbol
		self.addr = addr
		self.count = 0
		
	def printData(self, cycles_total):
		return "{}: {:4X}-{:4X} {:>10} {:>10} ms {:>5.1f} %".format(self.symbol, self.addr, self.addr_end, self.count, ms_from_cycles(self.count), 100.0 * self.count/cycles_total)

class MapFile:
	functions = []
  
	def __init__(self, map_file):
		f = file(map_file)
		r = re.compile("(.+) *= *([0-9A-Fa-f]{4}), ([GL]): (.*)")
		for line in f:
			if len(line.strip()) > 0:
				print line
				parsed = r.match(line).groups()
				print parsed
				(symbol, addr_s, globallocal, module) = parsed
				addr = int(addr_s, base=16)
				print symbol, addr, globallocal, module
				# Only global (exported) functions, skip labels
				if globallocal == 'G':
					self.functions.append(FuncData(symbol, addr))
		self.functions = sorted(self.functions, lambda x,y: cmp(x.addr, y.addr))
		for i in range(len(self.functions)-1):
			self.functions[i].addr_end = self.functions[i+1].addr-1
		self.functions[-1].addr_end = 0xFFFF;
		other = FuncData("other", 0)
		other.addr_end = self.functions[0].addr-1
		self.functions.insert(0, other)
			
	def printProfile(self):
	        cycles_total = 0
		for func in self.functions:
			cycles_total += func.count
		for func in self.functions:
			print func.printData(cycles_total)
		print "total: ", cycles_total, " ", ms_from_cycles(cycles_total), " ms"
			
	def find_func(self, addr):
		for func in self.functions:
			if (func.addr <= addr) and (addr <= func.addr_end):
				return func
		return None


fuse_profile = FuseProfile(sys.argv[1]);

map_file = MapFile(sys.argv[2]);

for addr, count in fuse_profile.cycle_counts.items():
	 print count
	 func = map_file.find_func(addr)
	 if func:
		func.count = func.count + count

map_file.printProfile()

