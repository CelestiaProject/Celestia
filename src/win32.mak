# win32.mak
#
# Copyright (C) 2007, Chris Laurel <claurel@gmail.com>
#
# Compiler settings for building under MS Visual C++
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.


##### Default to debug configuration

!IF "$(CFG)" == ""
CFG=Debug
!MESSAGE No configuration specified. Defaulting to debug.
!ENDIF


##### Compile flag settings for all build configurations

COMPILE_OPTS=/nologo /W3 /EHsc /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D WINVER=0x0400 /D _WIN32_WINNT=0x0400 /D _CRT_SECURE_NO_DEPRECATE /D _SCL_SECURE_NO_WARNINGS

!IF "$(CFG)" == "Release"
OUTDIR=.\Release
INTDIR=.\Release
LIBDIR=Release
COMPILE_OPTS = $(COMPILE_OPTS) /MT
!ELSE
OUTDIR=.\Debug
INTDIR=.\Debug
LIBDIR=Debug
COMPILE_OPTS = $(COMPILE_OPTS) /MTd
!ENDIF

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 


##### Configuration-specific compile flag settings

!IF "$(CFG)" == "Release"

CPP=cl.exe
CPPFLAGS=$(COMPILE_OPTS) /O2 /D "NDEBUG" /Fp"$(INTDIR)\celestia.pch" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c $(EXTRADEFS) $(INCLUDEDIRS)

!ELSE

CPP=cl.exe
CPPFLAGS=$(COMPILE_OPTS) /Gm /ZI /Od /D "_DEBUG" /Fp"$(INTDIR)\celestia.pch" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /RTC1 /c $(EXTRADEFS) $(INCLUDEDIRS)

!ENDIF


##### Build rules

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPPFLAGS) $<
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPPFLAGS) $<
<<
