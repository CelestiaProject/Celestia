/*
 
-Header_File SpiceZpl.h ( CSPICE platform macros )
 
-Abstract
 
   Define macros identifying the host platform for which this
   version of CSPICE is targeted.
 
-Disclaimer
 
   THIS SOFTWARE AND ANY RELATED MATERIALS WERE CREATED BY THE
   CALIFORNIA INSTITUTE OF TECHNOLOGY (CALTECH) UNDER A U.S.
   GOVERNMENT CONTRACT WITH THE NATIONAL AERONAUTICS AND SPACE
   ADMINISTRATION (NASA). THE SOFTWARE IS TECHNOLOGY AND SOFTWARE
   PUBLICLY AVAILABLE UNDER U.S. EXPORT LAWS AND IS PROVIDED "AS-IS"
   TO THE RECIPIENT WITHOUT WARRANTY OF ANY KIND, INCLUDING ANY
   WARRANTIES OF PERFORMANCE OR MERCHANTABILITY OR FITNESS FOR A
   PARTICULAR USE OR PURPOSE (AS SET FORTH IN UNITED STATES UCC
   SECTIONS 2312-2313) OR FOR ANY PURPOSE WHATSOEVER, FOR THE
   SOFTWARE AND RELATED MATERIALS, HOWEVER USED.
 
   IN NO EVENT SHALL CALTECH, ITS JET PROPULSION LABORATORY, OR NASA
   BE LIABLE FOR ANY DAMAGES AND/OR COSTS, INCLUDING, BUT NOT
   LIMITED TO, INCIDENTAL OR CONSEQUENTIAL DAMAGES OF ANY KIND,
   INCLUDING ECONOMIC DAMAGE OR INJURY TO PROPERTY AND LOST PROFITS,
   REGARDLESS OF WHETHER CALTECH, JPL, OR NASA BE ADVISED, HAVE
   REASON TO KNOW, OR, IN FACT, SHALL KNOW OF THE POSSIBILITY.
 
   RECIPIENT BEARS ALL RISK RELATING TO QUALITY AND PERFORMANCE OF
   THE SOFTWARE AND ANY RELATED MATERIALS, AND AGREES TO INDEMNIFY
   CALTECH AND NASA FOR ALL THIRD-PARTY CLAIMS RESULTING FROM THE
   ACTIONS OF RECIPIENT IN THE USE OF THE SOFTWARE.
 
-Required_Reading
 
   None.
 
-Literature_References
 
   None.
 
-Particulars
 
   This header file defines macros that enable CSPICE code to be
   compiled conditionally based on the identity of the host platform.
 
   The macros defined here ARE visible in the macro name space of
   any file that includes SpiceUsr.h.  The names are prefixed with
   the string CSPICE_ to help prevent conflicts with macros defined
   by users' applications.
 
-Author_and_Institution
 
   N.J. Bachman       (JPL)
   B.V. Semenov       (JPL)
   E.D. Wright        (JPL)
 
-Version
 
   -CSPICE Version 2.1.0, 10-MAR-2014 (BVS)
 
      Updated for the:
 
         PC-CYGWIN-64BIT-GCC_C
 
      environment.
 
   -CSPICE Version 2.2.0, 14-MAY-2010 (EDW)(BVS)
 
      Updated for the:
 
         MAC-OSX-64BIT-INTEL_C
         PC-64BIT-MS_C
         SUN-SOLARIS-64BIT-NATIVE_C
         SUN-SOLARIS-INTEL-64BIT-CC_C
         SUN-SOLARIS-INTEL-CC_C
 
      environments.
 
   -CSPICE Version 2.1.0, 15-NOV-2006 (BVS)
 
      Updated for MAC-OSX-INTEL_C environment.
 
   -CSPICE Version 2.0.0, 21-FEB-2006 (NJB)
 
      Updated for PC-LINUX-64BIT-GCC_C environment.
 
   -CSPICE Version 1.3.0, 06-MAR-2005 (NJB)
 
      Updated for SUN-SOLARIS-64BIT-GCC_C environment.
 
   -CSPICE Version 1.2.0, 03-JAN-2005 (BVS)
 
      Updated for PC-CYGWIN_C environment.
 
   -CSPICE Version 1.1.0, 27-JUL-2002 (BVS)
 
      Updated for MAC-OSX-NATIVE_C environment.
 
   -CSPICE Version 1.0.0, 26-FEB-1999 (NJB) (EDW)
 
-Index_Entries
 
   platform ID defines for CSPICE
 
*/
 
 
#ifndef HAVE_PLATFORM_MACROS_H
#define HAVE_PLATFORM_MACROS_H
 
 
   #define   CSPICE_PC_64BIT_MS
 
#endif
 
