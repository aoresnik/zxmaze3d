
# ZXMAZE3D

A simple 3D demo for ZX Spectrum 8-bit computer (48k model or better). 

Walls are rendered as halftone shaded solid color. No monsters yet. 
Achieves frame rates around 10-20 FPS. The goal was as high frame rate as possible while producing something resembling a 3D environment (there are texture-mapped fully-blown 3D games for ZX Spectrum, but they are slow and tipically work only on 128k model).

![screenshot](doc/animated-sample.gif?raw=true)

The world is very simple:

![map](doc/map.png?raw=true)

NOTE: This program was developed for a computer from 1982, which was low end even then, so don't expect much.

## Usage

Run the zxmaze3d.z80  binary (see [download page](http://andrejo.github.io/zxmaze3d/)) with a ZX Spectrum emulator (tested in FUSE and EMUZWin). Not yet tested on a  real ZX Spectrum machine at the time of this writing.   
   
Use standard WSAD keys to move around (no other interactions yet):
* W - forwards
* S - backwards
* A - turn left
* D - turn right

Inspired by "3D Monster Maze" for ZX 81, Wolfenstein 3D's ray-casting
algorithm and Doom's sector-based algorithm.

## Compiling from source

Tested with z88dk v1.10.1 and GNU make (under Linux and under Cygwin).
   
Make sure that z88dk executables are in the PATH and that the environment variables required by z88dk are set. Then run make to compile. 

The resulting file to be run is zxmaze3d.tap (converted to more commonly supported .z80 format on [download page](http://andrejo.github.io/zxmaze3d/)).
