
N_INTENSITIES = 17

bayer_4x4 = \
	[[1, 9, 3, 11],
	 [13, 5, 15, 7],
	 [4, 12, 2, 10],
	 [16, 8, 14, 6]]
	 
ht_bits = [0 for i in xrange(N_INTENSITIES * 8)]

def ht_is_filled(x, y, intensity):
	"""Returns 1 if halftone pixel is black, 0 if white"""
	return bayer_4x4[x&3][y&3] > intensity

# for each possible intensity, build 8x8 halftone bitmap	
for i in xrange(N_INTENSITIES):
	for y in xrange(8):
		b = 0;
		for x in xrange(8):
			if ht_is_filled(x, y, i):
				b = (b | (1 << x))
		ht_bits[8 * i + y] = b

# dump bitmaps as C array
for i in xrange(N_INTENSITIES):
	for j in xrange(8):
		print '0x{0:02x},'.format(ht_bits[i * 8 + j]),
	print ''
