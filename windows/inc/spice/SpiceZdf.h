/*

-Header_File SpiceZdf.h ( CSPICE definitions )

-Abstract

   Define CSPICE data types via typedefs; also define some user-visible
   enumerated types.
      
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
   
   CSPICE data types
   =================
   
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
 
      SpiceBoolean -> int  
      SpiceChar    -> char
      SpiceDouble  -> double
      SpiceInt     -> int or long
      ConstX       -> const X  (X = any of the above types)
 
   The type SpiceInt is a special case: the corresponding type is picked
   so as to be half the size of a double. On most currently supported
   platforms, type double occupies 8 bytes and type long occupies 4 
   bytes.  Other platforms may require a SpiceInt to map to type int.
   The Alpha/Digital Unix platform is an example of the latter case.
   
   While other data types may be used internally in CSPICE, no other
   types appear in the API.
      
      
   CSPICE enumerated types
   =======================
   
   These are provided to enhance readability of the code.
      
      Type name                Value set
      ---------                ---------
      
      _Spicestatus             { SPICEFAILURE = -1,   SPICESUCCESS = 0 }
      
      
-Literature_References

   None.

-Author_and_Institution

   N.J. Bachman       (JPL)
   B.V. Semenov       (JPL)
   E.D. Wright        (JPL)
   
-Restrictions

   None.
      
-Version

   -CSPICE Version 6.2.0, 10-MAR-2014 (BVS)

       Updated for:
       
          PC-CYGWIN-64BIT-GCC_C
          
       environment. Added the corresponding tag:
       
          CSPICE_PC_CYGWIN_64BIT_GCC

       tag to the #ifdefs set.
        
   -CSPICE Version 6.1.0, 14-MAY-2010 (EDW)(BVS)

       Updated for:
       
          MAC-OSX-64BIT-INTEL_C
          SUN-SOLARIS-64BIT-NATIVE_C
          SUN-SOLARIS-INTEL-64BIT-CC_C
          
       environments. Added the corresponding tags:
       
          CSPICE_MAC_OSX_INTEL_64BIT_GCC
          CSPICE_SUN_SOLARIS_64BIT_NATIVE
          CSPICE_SUN_SOLARIS_INTEL_64BIT_CC

       tag to the #ifdefs set.
        
   -CSPICE Version 6.0.0, 21-FEB-2006 (NJB)

       Updated to support the PC Linux 64 bit mode/gcc platform.

   -CSPICE Version 5.0.0, 27-JAN-2003 (NJB)

       Updated to support the Sun Solaris 64 bit mode/gcc platform.

   -CSPICE Version 4.0.0 27-JUL-2002 (NJB) 
   
      Added definition of SpiceDataType.
   
   -CSPICE Version 3.0.0 18-SEP-1999 (NJB) 
   
      SpiceBoolean implementation changed from enumerated type to 
      typedef mapping to int.
   
   -CSPICE Version 2.0.0 29-JAN-1999 (NJB) 
   
      Made definition of SpiceInt and ConstSpiceInt platform 
      dependent to accommodate the Alpha/Digital Unix platform.
      
      Removed definitions of SpiceVoid and ConstSpiceVoid.
   
   -CSPICE Version 1.0.0 25-OCT-1997 (KRG) (NJB) (EDW)
*/

   #ifndef HAVE_SPICEDEFS_H
   #define HAVE_SPICEDEFS_H
   
   /*
   Include platform definitions, if they haven't been executed already.
   */
   #ifndef HAVE_PLATFORM_MACROS_H
      #include "SpiceZpl.h"
   #endif
   
   /*
   Basic data types. These are defined to be compatible with the
   types used by f2c, and so they follow the Fortran notion of what
   these things are. See the f2c documentation for the details
   about the choices for the sizes of these types.
   */
   typedef char           SpiceChar;
   typedef double         SpiceDouble;
   typedef float          SpiceFloat;
   

 
   #if (    defined(CSPICE_ALPHA_DIGITAL_UNIX    )      \
         || defined(CSPICE_SUN_SOLARIS_64BIT_NATIVE)    \
         || defined(CSPICE_SUN_SOLARIS_64BIT_GCC )      \
         || defined(CSPICE_MAC_OSX_INTEL_64BIT_GCC )    \
         || defined(CSPICE_SUN_SOLARIS_INTEL_64BIT_CC ) \
         || defined(CSPICE_PC_CYGWIN_64BIT_GCC )        \
         || defined(CSPICE_PC_LINUX_64BIT_GCC    )  )
   
      typedef int         SpiceInt;
   #else
      typedef long        SpiceInt;
   #endif

   
   typedef const char     ConstSpiceChar;
   typedef const double   ConstSpiceDouble;
   typedef const float    ConstSpiceFloat;


   #if (    defined(CSPICE_ALPHA_DIGITAL_UNIX    )      \
         || defined(CSPICE_SUN_SOLARIS_64BIT_NATIVE)    \
         || defined(CSPICE_SUN_SOLARIS_64BIT_GCC )      \
         || defined(CSPICE_MAC_OSX_INTEL_64BIT_GCC )    \
         || defined(CSPICE_SUN_SOLARIS_INTEL_64BIT_CC ) \
         || defined(CSPICE_PC_CYGWIN_64BIT_GCC )        \
         || defined(CSPICE_PC_LINUX_64BIT_GCC    )  )
   
      typedef const int   ConstSpiceInt;
   #else
      typedef const long  ConstSpiceInt;
   #endif
   
   
   /*
   More basic data types. These give mnemonics for some other data
   types in C that are not used in Fortran written by NAIF or
   supported by ANSI Fortran 77. These are for use in C functions
   but should not be passed to any C SPICE wrappers, ``*_c.c''
   since they are not Fortran compatible.
   */
   typedef long           SpiceLong;
   typedef short          SpiceShort;

   /*
   Unsigned data types
   */
   typedef unsigned char  SpiceUChar;
   typedef unsigned int   SpiceUInt;
   typedef unsigned long  SpiceULong;
   typedef unsigned short SpiceUShort;

   /*
   Signed data types
   */
   typedef signed char    SpiceSChar;

   /*
   Other basic types
   */
   typedef int            SpiceBoolean;
   typedef const int      ConstSpiceBoolean;
   
   #define SPICETRUE      1
   #define SPICEFALSE     0
      
   
   enum _Spicestatus { SPICEFAILURE = -1, SPICESUCCESS = 0 };
   
   typedef enum _Spicestatus SpiceStatus;


   enum _SpiceDataType { SPICE_CHR  = 0, 
                         SPICE_DP   = 1, 
                         SPICE_INT  = 2,
                         SPICE_TIME = 3,
                         SPICE_BOOL = 4 };
    

   typedef enum _SpiceDataType SpiceDataType;


#endif
