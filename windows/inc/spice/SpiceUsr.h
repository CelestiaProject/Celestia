/*

-Header_File SpiceUsr.h ( CSPICE user interface definitions )

-Abstract

   Perform CSPICE user interface declarations, including type
   definitions and function prototype declarations.
      
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
   
-Particulars

   This file is an umbrella header that includes all header files 
   required to support the CSPICE application programming interface
   (API).  Users' application code that calls CSPICE need include only
   this single header file.  This file includes function prototypes for 
   the entire set of CSPICE routines.  Typedef statements used to create
   SPICE data types are also included.
   
   
   About SPICE data types
   ======================
   
   To assist with long-term maintainability of CSPICE, NAIF has elected
   to use typedefs to represent data types occurring in argument lists
   and as return values of CSPICE functions. These are:
 
      SpiceBoolean
      SpiceChar
      SpiceDouble
      SpiceInt
      ConstSpiceBoolean
      ConstSpiceChar
      ConstSpiceDouble
      ConstSpiceInt
 
   The SPICE typedefs map in an arguably natural way to ANSI C types:
 
      SpiceBoolean -> enum { SPICEFALSE = 0, SPICETRUE = 1 }
      SpiceChar    -> char
      SpiceDouble  -> double
      SpiceInt     -> int or long
      ConstX       -> const X  (X = any of the above types)
 
   The type SpiceInt is a special case: the corresponding type is picked
   so as to be half the size of a double. On all currently supported
   platforms, type double occupies 8 bytes and type int occupies 4 
   bytes.  Other platforms may require a SpiceInt to map to type long.
 
   While other data types may be used internally in CSPICE, no other
   types appear in the API.
      
   
   About CSPICE function prototypes
   ================================ 
   
   Because CSPICE function prototypes enable substantial compile-time
   error checking, we recommend that user applications always reference
   them.  Including the header file SpiceUsr.h in any module that calls
   CSPICE will automatically make the prototypes available.
      
      
   About CSPICE C style
   ====================
   
   CSPICE is written in ANSI C.  No attempt has been made to support K&R
   conventions or restrictions.
   
   
   About C++ compatibility
   =======================
   
   The preprocessor directive -D__cplusplus should be used when 
   compiling C++ source code that includes this header file.  This 
   directive will suppress mangling of CSPICE names, permitting linkage
   to a CSPICE object library built from object modules produced by
   an ANSI C compiler.
   
-Literature_References

   None.

-Author_and_Institution

   N.J. Bachman       (JPL)
   S.C. Krening       (JPL)
   E.D. Wright        (JPL)
   
-Restrictions

   The #include statements contained in this file are not part of 
   the CSPICE API.  The set of files included may change without notice.
   Users should not include these files directly in their own
   application code.
   
-Version

   -CSPICE Version 6.0.0, 07-FEB-2010 (NJB) 

      Now includes SpiceOsc.h.

    27-FEB-2016 (NJB) 

      Updated to include header files

        SpiceDLA.h
        SpiceDSK.h
        SpiceSrf.h
        SpiceDtl.h

   -CSPICE Version 5.0.0, 11-MAY-2012 (NJB) (SCK) 

      Updated to include header files

        SpiceErr.h
        SpiceFrm.h
        SpiceOccult.h

   -CSPICE Version 4.0.0, 30-SEP-2008 (NJB) 

      Updated to include header file

        SpiceGF.h

   -CSPICE Version 3.0.0, 19-AUG-2002 (NJB) 

      Updated to include header files

        SpiceCel.h
        SpiceCK.h
        SpiceSPK.h
 
   -CSPICE Version 3.0.0, 17-FEB-1999 (NJB)  

      Updated to support suppression of name mangling when included in 
      C++ source code.  Also now interface macros to intercept function
      calls and perform automatic type casting.
      
      Now includes platform macro definition header file.
      
      References to types SpiceVoid and ConstSpiceVoid were removed.
   
   -CSPICE Version 2.0.0, 06-MAY-1998 (NJB) (EDW)

*/

#ifdef __cplusplus
   extern "C" { 
#endif
           

#ifndef HAVE_SPICE_USER

   #define HAVE_SPICE_USER
   

   /*
   Include CSPICE platform macro definitions.
   */
   #include "SpiceZpl.h"

   /*
   Include CSPICE data type definitions.
   */
   #include "SpiceZdf.h"   
         
   /*
   Include the CSPICE error handling interface definitions.
   */
   #include "SpiceErr.h"

   /*
   Include the CSPICE EK interface definitions.
   */
   #include "SpiceEK.h"
     
   /*
   Include the CSPICE frame subsystem API definitions.
   */
   #include "SpiceFrm.h"

   /*
   Include the CSPICE Cell interface definitions.
   */
   #include "SpiceCel.h"
   
   /*
   Include the CSPICE CK interface definitions.
   */
   #include "SpiceCK.h"
   
   /*
   Include the CSPICE SPK interface definitions.
   */
   #include "SpiceSPK.h"

   /*
   Include the CSPICE GF interface definitions.
   */
   #include "SpiceGF.h"

   /*
   Include the CSPICE occultation definitions.
   */
   #include "SpiceOccult.h"

   /*
   Include the CSPICE DLA definitions.
   */
   #include "SpiceDLA.h"

   /*
   Include the CSPICE DSK definitions.
   */
   #include "SpiceDSK.h"
  
   /*
   Include the CSPICE DSK tolerance definitions.
   */
   #include "SpiceDtl.h"
  
   /*
   Include the CSPICE surface definitions.
   */
   #include "SpiceSrf.h"

   /*
   Include oscltx_c definitions.
   */
   #include "SpiceOsc.h"
  
   /*
   Include CSPICE prototypes.
   */
   #include "SpiceZpr.h"

   /*
   Define the CSPICE function interface macros.
   */
   #include "SpiceZim.h"

 

#endif


#ifdef __cplusplus
   }
#endif

