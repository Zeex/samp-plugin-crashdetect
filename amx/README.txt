This is an altered version of the original AMX from the Pawn toolkit version 3.0.3667.

To re-generate this code from the original source:

1) download Pawn 3.0.3667 source and extract it

   http://www.compuphase.com/pawn/pawn-3.0.3367.zip

2) copy amx.patch to pawn-3.0.3667/SOURCE/AMX and apply it (in order):

   cd pawn-3.0.3667/SOURCE/AMX
   patch -p0 -i compile-fixes.patch
   patch -p0 -i mingw.patch
   patch -p0 -i dbg-info.patch
   patch -p0 -i rte-reporting.patch
