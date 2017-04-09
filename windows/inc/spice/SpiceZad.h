/*

-Header_File SpiceZad.h ( CSPICE adapter definitions )

-Abstract

   Perform CSPICE declarations to support passed-in function
   adapters used in wrapper interfaces.
            
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

   This header file contains declarations used by the CSPICE
   passed-in function adapter ("PFA") system. This system enables
   CSPICE wrapper functions to support passed-in function
   arguments whose prototypes are C-style, even when these 
   functions are to be called from f2c'd Fortran routines
   expecting f2c-style interfaces.

   This header declares:

     - The prototype for the passed-in function argument 
       pointer storage and fetch routines

          zzadsave_c
          zzadget_c

     - Prototypes for CSPICE adapter functions. Each passed-in       
       function argument in a CSPICE wrapper has a corresponding
       adapter function. The adapter functions have interfaces
       that match those of their f2c'd counterparts; this allows
       the adapters to be called by f2c'd SPICELIB code. The 
       adapters look up saved function pointers for routines
       passed in by the wrapper's caller and call these functions.

     - Values for the enumerated type SpicePassedInFunc. These
       values are used to map function pointers to the
       functions they represent, enabling adapters to call
       the correct passed-in functions.

Literature_References

   None.

-Author_and_Institution

   N.J. Bachman       (JPL)
   
-Restrictions

   None.
      
-Version

   -CSPICE Version 2.2.0, 29-NOV-2011 (EDW)  

      Updated to support the user defined boolean function capability.

   -CSPICE Version 2.1.0, 21-DEC-2009 (EDW)  

      Updated to support the user defined scalar function capability.

   -CSPICE Version 2.0.0, 29-JAN-2009 (NJB)  

      Now conditionally includes SpiceZfc.h.
   
      Updated to reflect new calling sequence of f2c'd
      routine gfrefn_. Some header updates were made
      as well.

   -CSPICE Version 1.0.0, 29-MAR-2008 (NJB)  

*/


/*
   This file has dependencies defined in SpiceZfc.h. Include that
   file if it hasn't already been included.
*/
#ifndef HAVE_SPICEF2C_H
   #include "SpiceZfc.h"
#endif



#ifndef HAVE_SPICE_ZAD_H

   #define HAVE_SPICE_ZAD_H
   


   /*
   Prototypes for GF adapters:
   */

   logical  zzadbail_c ( void );


   int      zzadstep_c ( doublereal   * et,
                         doublereal   * step );


   int      zzadrefn_c ( doublereal   * t1,
                         doublereal   * t2,
                         logical      * s1,
                         logical      * s2,
                         doublereal   * t    );


   int      zzadrepf_c ( void );


   int      zzadrepi_c ( doublereal   * cnfine,
                         char         * srcpre,
                         char         * srcsuf,
                         ftnlen         srcprelen,
                         ftnlen         srcsuflen );


   int      zzadrepu_c ( doublereal   * ivbeg,
                         doublereal   * ivend,
                         doublereal   * et      );


   int      zzadfunc_c ( doublereal   * et,
                         doublereal   * value );


   int      zzadqdec_c (  U_fp          udfunc,
                          doublereal  * et,
                          logical     * xbool );

   /*
   Define the enumerated type 

      SpicePassedInFunc

   for names of passed-in functions. Using this type gives
   us compile-time checking and avoids string comparisons.
   */
   enum _SpicePassedInFunc  { 
                               UDBAIL,
                               UDREFN,
                               UDREPF,
                               UDREPI,
                               UDREPU,
                               UDSTEP,
                               UDFUNC,
                               UDQDEC,
                            };
   
   typedef enum _SpicePassedInFunc SpicePassedInFunc;
   
   /*
   SPICE_N_PASSED_IN_FUNC is the count of SpicePassedInFunc values.
   */
   #define SPICE_N_PASSED_IN_FUNC     8


   /*
   CSPICE wrappers supporting passed-in function arguments call
   the adapter setup interface function once per each such argument;
   these calls save the function pointers for later use within the
   f2c'd code that calls passed-in functions. The saved pointers
   will be used in calls by the adapter functions whose prototypes
   are declared above.

   Prototypes for adapter setup interface: 
   */
   void    zzadsave_c ( SpicePassedInFunc    functionID,
                        void               * functionPtr );

   void *  zzadget_c  ( SpicePassedInFunc    functionID  );
 
 
#endif

/*
End of header file SpiceZad.h 
*/

