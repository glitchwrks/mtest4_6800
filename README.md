# MTEST4

`MTEST4` is a memory test program originally written by Martin Eberhard to find a difficult memory issue in his MITS Altair 680 -- you can read about that [here, on Herb Johnson's site](https://www.retrotechnology.com/restore/altair680.html). 

`MTEST4` tests a range of memory specified by the user on the console. It can relocate itself to anywhere in memory outside of page 0 (`0x0000` to `0x00FF`). It is capable of testing anywhere in memory outside of where `MTEST4` itself is currently located. ***Do note that `MTEST4` cannot currently test all of page 0 memory on the Altair 680!*** `MTEST4` currently uses some of the monitor's I/O routines, which use variables located in page 0.

### Test Features

From Martin Eberhard's original notes:

	The memory test algorithm is designed to ferret out a variety of memory problems. It is based on a test pattern that contains two walking bit patterns - one with the bit high, and the other with the bit low. It also contains various high-frequency patterns like 55 and AA. These pattern elements are scrambled to maximize the number of times each memory bit changes value during the test. The entire pattern is deliberately 29 bytes  long -- a prime number. The pattern is written repeatedly  through the test memory range, then read back and checked. Then the pattern is incremented for another pass through memory. This is repeated until every memory location in the test range has been tried with each byte in the test pattern. This should catch all address-line shorts, data line shorts, coupling between nearby cells, stuck bits, and even catch some longer-term memory loss faults.

`MTEST4` was used to test and debug our [Altair 680 Universal RAM board](https://www.tindie.com/products/glitchwrks/glitch-works-altair-680-universal-64k-ram-board/). Its use exposed a few edge cases we likely wouldn't have found without it! Additionally, it is very fast for how thorough it is. 

### Technical Notes

Currently, `MTEST4` uses some monitor routines in the MITS Altair 680 ROM monitor, and requires this monitor to be present at the usual location (`0xFF00` to `0xFFFF`).

The relocatable nature of `MTEST4` makes the code bigger and less performant than it would otherwise be. Some of the subroutines have odd ordering to make them reachable by `BSR` instructions, thus allowing relocation. The source is commented to reflect this.

The mover program, which relocates `MTEST4` from its initial load address to the user's desired run address, does check for valid data after moving. If there's an error in this process (bad/missing memory at the specified run address), it will abort to the ROM monitor.

### Building

This project requires the [Glitch Works](http://www.glitchwrks.com/) modified version of the `A68` assembler to be compiled and available on your `$PATH`. `A68` can be found [in our GitHub repository](https://github.com/glitchwrks/a68/).

Once `A68` is available on your path, just type `make` to assemble. Default output format is [Motorola S-Record](https://en.wikipedia.org/wiki/SREC_(file_format)), which is loadable by many ROM monitors found on Motorola 6800 systems.

### Running `MTEST4` on the Altair 680

Load the file MTEST4.obj into an Altair 680b using the PROM Monitor's 'L' command. Then jump to it at 0100 by typing 'J 0100'. Follow the directions - MTEST will first ask you where to put the code - pick an address outside where you want to test, and MTEST will relocate itself there, assuming there is actually memory at the address you specified. (If not, MTEST will abort to the PROM Monitor.)

Next, tell MTEST the address range of the memory to test. MTEST will run through 30 passes over the range. Each pass takes about 10 seconds for each 16Kbytes of tested memory. MTEST will print the pass number at the end of each pass. (Note that pass 0 only fills memory - no testing occurs.) Every time MTEST finds an error, it will report the address, what it wrote, and what it read.

### TO-DO

 * Replace Altair 680 ROM monitor's `INCH` routine so we can relocate to page 0 (Note that `INCH` use `ECHO` in page 0)
 * Check for overlap of address range to test with `MTEST` itself

### Revision History

The following enumerates revisions to the monitor before tracking in this particular Git repository:

 * 2011-10-23 M. Eberhard  Created
 * 2011-10-27 M. Eberhard  Don't use page 0 memory, so we can test it
 * 2011-10-27 M. Eberhard  Make code relocatable and relocating
 * 2011-11-03 M. Eberhard  comp. tsts, and check memory immediately
 * 2011-11-03 M. Eberhard  combine fill and test passes for speed
 * 2023-12-18 J. Chapman   Ported to modified WCC3 A68 cross-assembler
 * 2023-12-24 J. Chapman   General cleanup, no program changes

`MTEST4` was ported to assemble with a modified version of Warren C. Colley III's `A68` cross-assembler ([Glitch Works modified version](https://github.com/glitchwrks/a68/), [Herb Johnson's version](https://www.retrotechnology.com/restore/a68.html)). MFE's original version was written to assemble with PseudoCode's A68.com, a DOS-based cross assembler for the MC6800. Porting to `A68` removed the workarounds for bugs in the PseudoCode assembler -- e.g. not correctly using direct addressing mode for some instructions.
