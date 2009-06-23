/*

-Header_File SpiceZst.h ( Fortran/C string conversion utilities )

-Abstract

   Define prototypes for CSPICE Fortran/C string conversion utilities.

   Caution: these prototypes are subject to revision without notice.

   These are private routines and are not part of the official CSPICE
   user interface.

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

   None.

-Literature_References

   None.

-Author_and_Institution

   N.J. Bachman       (JPL)
   K.R. Gehringer     (JPL)
   E.D. Wright        (JPL)

-Version

   -CSPICE Version 6.0.0, 10-JUL-2002 (NJB)

      Added prototype for new functions C2F_MapStrArr and 
      C2F_MapFixStrArr.

   -CSPICE Version 5.0.0, 18-MAY-2001 (WLT)

      Added #ifdef's to add namespace specification for C++ compilation.

   -CSPICE Version 4.0.0, 14-FEB-2000 (NJB)

      Added prototype for new function C2F_CreateStrArr_Sig.
    
   -CSPICE Version 3.0.0, 12-JUL-1999 (NJB)
    
      Added prototype for function C2F_CreateFixStrArr.
      Added prototype for function F2C_ConvertTrStrArr.
      Removed reference in comments to C2F_CreateStrArr_Sig, which
      does not exist.

   -CSPICE Version 2.0.1, 06-MAR-1998 (NJB)

      Type SpiceVoid was changed to void.
      
   -CSPICE Version 2.0.1, 09-FEB-1998 (EDW)

      Added prototype for F2C_ConvertStrArr.

   -CSPICE Version 2.0.0, 04-JAN-1998 (NJB)

      Added prototype for F2C_ConvertStr.

   -CSPICE Version 1.0.0, 25-OCT-1997 (NJB) (KRG) (EDW)

-Index_Entries

   protoypes of CSPICE Fortran/C string conversion utilities

*/

#include <stdlib.h>
#include <string.h>
#include "SpiceZdf.h"

#ifndef HAVE_FCSTRINGS_H
#define HAVE_FCSTRINGS_H

#ifdef __cplusplus
namespace Jpl_NAIF_CSpice {
#endif

   SpiceStatus C2F_CreateStr          ( ConstSpiceChar *,
                                        SpiceInt *,
                                        SpiceChar **      );

   void        C2F_CreateStr_Sig      ( ConstSpiceChar *,
                                        SpiceInt *,
                                        SpiceChar **      );

   void        C2F_CreateFixStrArr    ( SpiceInt           nStr,
                                        SpiceInt           cStrDim,
                                        ConstSpiceChar  ** cStrArr,
                                        SpiceInt         * fStrLen,
                                        SpiceChar       ** fStrArr  );
                                        
   SpiceStatus C2F_CreateStrArr       ( SpiceInt,
                                        ConstSpiceChar **,
                                        SpiceInt *,
                                        SpiceChar **      );

   void        C2F_CreateStrArr_Sig   ( SpiceInt           nStr,
                                        ConstSpiceChar  ** cStrArr,
                                        SpiceInt         * fStrLen,
                                        SpiceChar       ** fStrArr );
                                        
   void        C2F_MapFixStrArr       ( ConstSpiceChar  *  caller,
                                        SpiceInt           nStr,
                                        SpiceInt           cStrLen,
                                        const void       * cStrArr,
                                        SpiceInt         * fStrLen,
                                        SpiceChar       ** fStrArr  );

   void        C2F_MapStrArr          ( ConstSpiceChar  *  caller,
                                        SpiceInt           nStr,
                                        SpiceInt           cStrLen,
                                        const void       * cStrArr,
                                        SpiceInt         * fStrLen,
                                        SpiceChar       ** fStrArr  );

   SpiceStatus C2F_StrCpy             ( ConstSpiceChar *,
                                        SpiceInt,
                                        SpiceChar *       );

   void        F_Alloc                ( SpiceInt,
                                        SpiceChar**       );

   void        F2C_ConvertStr         ( SpiceInt,
                                        SpiceChar *       );

   void        F2C_ConvertStrArr      ( SpiceInt      n,
                                        SpiceInt      lenout,
                                        SpiceChar   * cvals  );

   void        F2C_ConvertTrStrArr    ( SpiceInt      n,
                                        SpiceInt      lenout,
                                        SpiceChar   * cvals  );

   SpiceStatus F2C_CreateStr          ( SpiceInt,
                                        ConstSpiceChar *,
                                        SpiceChar **      );

   void        F2C_CreateStr_Sig      ( SpiceInt,
                                        ConstSpiceChar *,
                                        SpiceChar **      );

   SpiceStatus F2C_CreateStrArr       ( SpiceInt,
                                        SpiceInt,
                                        ConstSpiceChar *,
                                        SpiceChar ***     );

   void        F2C_CreateStrArr_Sig   ( SpiceInt,
                                        SpiceInt,
                                        ConstSpiceChar *,
                                        SpiceChar ***     );

   void        F2C_FreeStrArr         ( SpiceChar  **cStrArr );


   SpiceStatus F2C_StrCpy             ( SpiceInt,
                                        ConstSpiceChar *,
                                        SpiceInt,
                                        SpiceChar *       );

   SpiceInt    F_StrLen               ( SpiceInt,
                                        ConstSpiceChar *  );

#ifdef __cplusplus
}
#endif
   
#endif
