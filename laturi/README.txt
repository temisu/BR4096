
Laturi by TS/The Digital Artists
--------------------------------

Laturi is a linker and compressor intended to be used for Mac OS X
1k, 4k and perhaps 64K intros.

Compatibility:

Following executables can be created from corresponding objects
- 10.6 to 10.8 32bit
- 10.6 to 10.8 64bit (experimental)

Due to the fact that Apple moved around and re-divided some libraries
in 10.7 and put there a better ASLR, binaries created in different
major versions of OS X do not work in other systems, although Lion
and Mountain Lion are known to be mostly compatible.

There exists options for different boot-stubs and hash-lengths as well
as call translation options. It is usually difficult to say which works
beforehand so experimenting with different options is preferred


Working principle:

Objects that are specified from commandline are read, linked into
one absolute linear segment starting from 0x0 (including bootstrap),
text-segments in order of command line appearance, then data segments
in same order. Finally BSS-segments are defined together with heap.
Top of the heap is stack. Entrypoint is '_main', no argc/argv/envp provided.

This one output segment has rwx-rights, thus allowing you make
self-modidyfying code if you like or defining variables together with your
code.

Please note that this royally messes null-pointers: they are readable _and_
writable. Find bugs in your code by using std. linker.

Bootstrap includes code which resolves external symbols from libraries
which are replaced by hashes. (Due to the inner workings of OS X typically
only top-level libraries need to specified, rest of them will get loaded by
dependencies f.e. GLUT also loads OpenGL, Cocoa loads almost everything)

The next optional step is to feed the output to compressor. The compresor
is content-adaptive deflate using simple shell script to unpack and execute
the resulting binary. the more properly you divide similar data-items into
their own sections, the more compressor can churn and probably improve
compression performance. laturi does not automatically reorganize your
sections, reordering them from commandline can be useful.


Usage: laturi [options]

Options:
	-i,--input file 
		Input file(s) to be linked and optionally compressed
	-o,--output file
		Output file to be generated
	-l,--library library
		Link with library
	-f,--framework framework
		Link with framework
	-H,--hash-length length
		Hash length for function and library references
		[default: 3, valid values 2-4]
	-m,--heap-size size
		Target program heap size incl. code, stack on top of heap
		[default: minimum 64k, rounded to next largest 2^n]
	-t,--tgt-trans call-translate-type
		Translate relative calls to indirect and/or absolute calls
		0 - no call translation
		1 - translate relative calls to external libraries to
		indirect
		2 - translate relative calls to absolute calls
		3 - use both translations
		[default: no translation]
		[note: 64-bit mode only support rel-to-abs conversion]
	-d,--debug create gdb-debuggable binary
		[default: no debug]
	   --tgt-noclean
		Use smaller, messy start-stub
		[default: clean-version]
	   --tgt-nopack
		Do not compress, only link
		[default: compression enabled]
	-v,--verbose
		Be verbose
	-h,--help
		This page

please see examples!


I would like to thank these people for sharing their ideas/code in the net:

Firehawk/TDA -- Partner in crime for 1K/4K stuff. His code has benchmarked
                laturi more than anything else
Marq/Fit     -- Thanks for the shell script unpacker idea: shortened it from
                56 bytes to 50 bytes though by abusing gzip headers. Although
                now we have better stuff :)
jix/Titan    -- You found some more creative ways to abuse Mach-O headers. I
                copied them, unfortunately they are very much 10.5 ;(

Have fun!
