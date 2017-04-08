#include <stdio.h>

/*
Since we're including static libraries that may have been built using
an older version of Visual Studio, they may have been linked with a
different definition of __iob_func() than the current one. This shim
provides the old-style version of this function for these static
libraries to consume.
*/

FILE ___iob[] = { *stdin, *stdout, *stderr };
extern "C" FILE * __cdecl __iob_func(void) { return ___iob; }
