
This is Source release of TDA BR4096 4k intro for Assembly 2018,
including the tooling (i.e. laturi)

The intro itself is pretty self explanatory. Basically it has a small
assembly code startup for the GLSL content as well as Komposter player.
Only complications are the fact that certain library calls depend on
the OSX version, anything that depend on dlsymming symbols do not work
propetly on an executable which has been compressed using hash-import
compress process. Thats why certain dependencies are directly fetched
from SkyLight framework instead of CGL.

Komposter player has its effects reduced to the set what is actually
used in the song, other than that it is quite "stock" version.

The shader code itself has been processed through minifier too many
times to be practically readable. There is nothing special to see
there either, it is quite usual raymarcher with some specific tweaks.

(The GL used is still the legacy profile due it being few bytes smaller,
OSX does not let mix core/legacy neither in code nor shader)

The compressor (laturi) is the more useful bit for the prospective intro
makers on OSX. Basically it has functionality for linking, compressing
(gzip + onekpaq) as well optimizing the binary. Since the binary format
changes quite rapidly with each major OSX release, it is almost always
required to patch the tools. This is likely to continue since there is
ongoing war between jail breakers of iOS and Apple. And OSX inherits
all the new security stuff they invent for iOS. Hopefully now with sources
released this process can be done easier (also by anyone). Tuning for 1k/4k
is wildly different so it is better to patch the tools on the fly.

Currently the compressor is compatible with OSX from 10.11 to 10.13,
however 10.12+ recommended (10.11 requires defining compliation flags)

The code quality is as horrible as can be imagined for a hack that has been
evolved over 10 years. It contains probably all the SW development anti
patterns known plus probably few new ones that I accidentally invented.

Although there are insane amount of details that can't be all documented in
a single readme, this should be a working base which anyone with OSX 10.12+
can start experimenting. (The code should compile just by 'make')

Please note that the main part of the code and tools is (C) by me, and have BSD license (see LICENSE)

The Komposter code and integration is done by Firehawk and is covered by the Komposter license.

The song is created by oo. It is provided as an example and it is not covered
by the above licenses (please do not use that in your own productions)

Have fun!
