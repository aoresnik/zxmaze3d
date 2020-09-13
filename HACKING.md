# ZXMAZE3D Hacking

(So that I don't forget):

* Formatting standard is to indent by 4 spaces, "align { with }" style.

* The map is hardcoded in **zxmaze3d.c**. By default, the program expects 16x8
blocks map, but could probably be easily set to 16x16. It could in principle
work even with larger world map with some modifications, provided that there's
no line of visibility longer than 32 blocks.

* The code is quite optimized, but could probably be made around 10-20% faster.
Low level assembly (for example in **fixed-math.c** and **span.c**) is pretty
much maxed out - but there is a lot of C code in critical path and *z88dk* is
not known for geneating the fastest code (it was used in preference to *sdcc*
because it has better "out of the box" support for ZX Spectrum platform).

* Profiling of the code is possible with the Fuse emulator under Linux. It can
generate a profile which contains cycle counts for each address. The included
Python script [tools/parse-fuse-profile.py](tools/parse-fuse-profile.py) can
combine this with the map file produced by z88dk compiler to generate a report
of cycle counts and percentage of time spent in each function.

* The idea was to make a fast algorithm at the expense of visual quality.
Texture mapping would slow it down quite a bit and/or consume a lot more
memory. The projection is a simple cylindrical projection (correct projection
would require more multiply and divide operations, which are extremely slow on
Z80, since it doesn't implement them in hardware). Some approximations
are used instead of exact calculations.

* There are some Java programs in the **Zxmaze3dGenPrecalc** directory. These
were used to generate precalculated sin/cos/tg/ctg/sqr tables and are not
needed for compilation (because the files that were generated are included in
the source).

* If much new code is added, Makefile needs to be changed (otherwise the
program can crash without warning): the starting address at `-zorg=34816` must
be moved lower, for example up to `-zorg=24576` to get 10k more available. As
it is now, the code is around 24k in debug mode, whereas there's 26k space
available (between the `-zorg` address and 60k point where some look-up tables
are put at runtime).

* Program start (`-zorg`) was set to 34k to squeeze out a bit more speed: the
memory range 32k-64k is a bit faster than the memory range at 16k-32k. The
space between 32k-34k is used for stack.
