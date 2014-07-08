
ZXMAZE3D
========

A simple 3D demo for ZX Spectrum 8-bit computer. 

Walls are rendered as halftone shaded solid color. No monsters yet. 
Achieves frame rates around 10-20 FPS.

Inspired by "3D Monster Maze" for ZX 81 and Wolfenstein 3D's ray-casting
algorithm.

Compiling
---------

   Tested with z88dk v1.10.1 and GNU make (under Linux and under Cygwin).
   
   Make sure that z88dk executables are in the PATH and that the required 
   environment variables are set. Then run make to compile.

Running
-------

   Run the resulting zxmaze3d.tap file with a ZX Spectrum emulator (tested
   in FUSE and EMUZWin). Not yet tested on a  real ZX Spectrum machine at 
   the time of this writing.   
   
   Use keys to move:
     W - forwards
	 S - backwards
	 A - turn left
	 D - turn right
