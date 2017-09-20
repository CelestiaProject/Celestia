/*

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

*/

/*
 CSPICE private macro file.

-Particulars

   Current list of macros (spelling counts)

     BLANK
     C2F_MAP_CELL
     C2F_MAP_CELL2
     C2F_MAP_CELL3
     CELLINIT
     CELLINIT2
     CELLINIT3
     CELLISSETCHK
     CELLISSETCHK2
     CELLISSETCHK2_VAL
     CELLISSETCHK3
     CELLISSETCHK3_VAL
     CELLISSETCHK_VAL
     CELLMATCH2
     CELLMATCH2_VAL
     CELLMATCH3
     CELLMATCH3_VAL
     CELLTYPECHK
     CELLTYPECHK2
     CELLTYPECHK2_VAL
     CELLTYPECHK3
     CELLTYPECHK3_VAL
     CELLTYPECHK_VAL
     CHKFSTR
     CHKFSTR_VAL
     CHKOSTR
     CHKOSTR_VAL
     CHKPTR
     Constants
     Even
     F2C_MAP_CELL
     Index values
     MOVED
     MOVEI
     MaxAbs
     MaxVal
     MinAbs
     MinVal
     Odd
     SpiceError
     TolOrFail

-Restrictions

   This is a private macro file for use within CSPICE.
   Do not use or alter any entry.  Or else!

-Author_and_Institution
 
   N.J. Bachman    (JPL)
   E.D. Wright     (JPL) 

-Version
 
   -CSPICE Version 5.0.0, 07-FEB-2017   (NJB)

      Updated MaxAbs and MinAbs macros to cast their input arguments
      to type double.

   -CSPICE Version 4.3.0, 18-SEP-2013   (NJB)

      Bug fix: missing comma was added to argument list
      in body of macro CELLTYPECHK3_VAL.

   -CSPICE Version 4.2.0, 16-FEB-2005   (NJB)

      Bug fix:  in the macro C2F_MAP_CELL, error checking has been
      added after the sequence of calls to ssizec_ and scardc_.
      If either of these routines signals an error, the dynamically
      allocated memory for the "Fortran cell" is freed.

   -CSPICE Version 4.1.0, 06-DEC-2002   (NJB)
   
      Bug fix:  added previous missing, bracketing parentheses to 
      references to input cell pointer argument in macro 
      CELLINIT.

      Changed CELLINIT macro so it no longer initializes to zero
      length all strings in data array of a character cell.  Instead,
      strings are terminated with a null in their final element.  

   -CSPICE Version 4.0.0, 22-AUG-2002   (NJB)
   
       Added macro definitions to support CSPICE cells and sets:

          C2F_MAP_CELL
          C2F_MAP_CELL2
          C2F_MAP_CELL3
          CELLINIT
          CELLINIT2
          CELLINIT3
          CELLISSETCHK
          CELLISSETCHK2
          CELLISSETCHK2_VAL
          CELLISSETCHK3
          CELLISSETCHK3_VAL
          CELLISSETCHK_VAL
          CELLMATCH2
          CELLMATCH2_VAL
          CELLMATCH3
          CELLMATCH3_VAL
          CELLTYPECHK
          CELLTYPECHK2
          CELLTYPECHK2_VAL
          CELLTYPECHK3
          CELLTYPECHK3_VAL
          CELLTYPECHK_VAL
          F2C_MAP_CELL

   -CSPICE Version 3.0.0, 09-JAN-1998   (NJB)
   
       Added output string check macros CHKOSTR and CHKOSTR_VAL.  
       Removed variable name arguments from macros
       
          CHKPTR
          CHKPTR_VAL
          CHKFSTR
          CHKRSTR_VAL
          
       The strings containing names of the checked variables are now 
       generated from the variables themselves via the # operator.
       
   -CSPICE Version 2.0.0, 03-DEC-1997   (NJB)
   
       Added pointer check macro CHKPTR and Fortran string check macro 
       CHKFSTR.
       
   -CSPICE Version 1.0.0, 25-OCT-1997   (EDW)
*/



#include <math.h>
#include <string.h>
#include "SpiceZdf.h"


#define MOVED( arrfrm, ndim, arrto )                \
                                                    \
        ( memmove ( (void*)               (arrto) , \
                    (void*)               (arrfrm), \
                    sizeof (SpiceDouble) * (ndim)  ) )





#define MOVEI( arrfrm, ndim, arrto )               \
                                                   \
        ( memmove ( (void*)            (arrto) ,   \
                    (void*)            (arrfrm),   \
                    sizeof (SpiceInt) * (ndim)    ) )





/* 
Define a tolerance test for those pesky double precision reals.
True if the difference is less than the tolerance, false otherwise.
The tolerance refers to a percentage. x, y and tol should be declared
double.  All values are assumed to be non-zero.  Okay?
*/

#define TolOrFail( x, y, tol )            \
                                          \
        ( fabs( x-y ) < ( tol * fabs(x) ) )





/*
Simple error output through standard SPICE error system .  Set the error
message and the type
*/

#define SpiceError( errmsg, errtype )   \
                                        \
        {                               \
        setmsg_c ( errmsg  );           \
        sigerr_c ( errtype );           \
        }






/*
Return a value which is the maximum/minimum of the absolute values of
two values.
*/

#define MaxAbs(a,b)                                                     \
                                                                        \
           ( fabs((double)(a)) >= fabs((double)(b)) ?                   \
                                  fabs((double)(a)) : fabs((double)(b)) )

#define MinAbs(a,b)                                                     \
                                                                        \
           ( fabs((double)(a)) <  fabs((double)(b)) ?                   \
                                  fabs((double)(a)) : fabs((double)(b)) )


/*
Return a value which is the maximum/minimum value of two values.
*/

#define MaxVal(A,B) ( (A) >= (B) ? (A) : (B) )
#define MinVal(A,B) ( (A) <  (B) ? (A) : (B) )





/*
Determine whether a value is even or odd
*/
#define Even( x ) ( ( (x) & 1 ) == 0 )
#define Odd ( x ) ( ( (x) & 1 ) != 0 )





/*
Array indexes for vectors.
*/

#define  SpiceX         0
#define  SpiceY         1
#define  SpiceZ         2
#define  SpiceVx        3
#define  SpiceVy        4
#define  SpiceVz        5




/*
Physical constants and dates.
*/

#define  B1900     2415020.31352
#define  J1900     2415020.0
#define  JYEAR     31557600.0
#define  TYEAR     31556925.9747
#define  J1950     2433282.5
#define  SPD       86400.0
#define  B1950     2433282.42345905
#define  J2100     2488070.0
#define  CLIGHT    299792.458
#define  J2000     2451545.0





/*
Common literal values.
*/

#define  NULLCHAR  ( (SpiceChar   )   0 )
#define  NULLCPTR  ( (SpiceChar * )   0 )
#define  BLANK     ( (SpiceChar   ) ' ' )



/*
Macro CHKPTR is used for checking for a null pointer.  CHKPTR uses
the constants 

   CHK_STANDARD
   CHK_DISCOVER
   CHK_REMAIN

to control tracing behavior.  Values  and meanings are:

   CHK_STANDARD          Standard tracing.  If an error
                         is found, signal it, check out
                         and return.

   CHK_DISCOVER          Discovery check-in.  If an
                         error is found, check in, signal
                         the error, check out, and return.

   CHK_REMAIN            If an error is found, signal it.
                         Do not check out or return.  This
                         would allow the caller to clean up
                         before returning, if necessary.
                         In such cases the caller must test
                         failed_c() after the macro call.
                         
CHKPTR should be used in void functions.  In non-void functions,
use CHKPTR_VAL, which is defined below.

*/

#define  CHK_STANDARD   1
#define  CHK_DISCOVER   2
#define  CHK_REMAIN     3

#define  CHKPTR( errHandling, modname, pointer )                     \
                                                                     \
         if ( (void *)(pointer) == (void *)0 )                       \
         {                                                           \
            if ( (errHandling) == CHK_DISCOVER  )                    \
            {                                                        \
               chkin_c ( modname );                                  \
            }                                                        \
                                                                     \
            setmsg_c ( "Pointer \"#\" is null; a non-null "          \
                       "pointer is required."             );         \
            errch_c  ( "#", (#pointer)                    );         \
            sigerr_c ( "SPICE(NULLPOINTER)"               );         \
                                                                     \
            if (    ( (errHandling) == CHK_DISCOVER  )               \
                 || ( (errHandling) == CHK_STANDARD  )   )           \
            {                                                        \
               chkout_c ( modname );                                 \
               return;                                               \
            }                                                        \
         }


#define  CHKPTR_VAL( errHandling, modname, pointer, retval )         \
                                                                     \
         if ( (void *)(pointer) == (void *)0 )                       \
         {                                                           \
            if ( (errHandling) == CHK_DISCOVER  )                    \
            {                                                        \
               chkin_c ( modname );                                  \
            }                                                        \
                                                                     \
            setmsg_c ( "Pointer \"#\" is null; a non-null "          \
                       "pointer is required."             );         \
            errch_c  ( "#", (#pointer)                    );         \
            sigerr_c ( "SPICE(NULLPOINTER)"               );         \
                                                                     \
            if (    ( (errHandling) == CHK_DISCOVER  )               \
                 || ( (errHandling) == CHK_STANDARD  )   )           \
            {                                                        \
               chkout_c ( modname );                                 \
               return   ( retval  );                                 \
            }                                                        \
         }


/*
Macro CHKFSTR checks strings that are to be passed to Fortran or
f2c'd Fortran routines.  Such strings must have non-zero length,
and their pointers must be non-null.

CHKFSTR should be used in void functions.  In non-void functions,
use CHKFSTR_VAL, which is defined below.
*/

#define  CHKFSTR( errHandling, modname, string )                     \
                                                                     \
         CHKPTR ( errHandling, modname, string );                    \
                                                                     \
         if (     (  (void *)string    !=  (void *)0  )              \
              &&  (   strlen(string)   ==  0          )  )           \
         {                                                           \
            if ( (errHandling) == CHK_DISCOVER  )                    \
            {                                                        \
               chkin_c ( modname );                                  \
            }                                                        \
                                                                     \
            setmsg_c ( "String \"#\" has length zero." );            \
            errch_c  ( "#", (#string)                  );            \
            sigerr_c ( "SPICE(EMPTYSTRING)"            );            \
                                                                     \
            if (    ( (errHandling) == CHK_DISCOVER  )               \
                 || ( (errHandling) == CHK_STANDARD  )   )           \
            {                                                        \
               chkout_c ( modname );                                 \
               return;                                               \
            }                                                        \
         }

#define  CHKFSTR_VAL( errHandling, modname, string, retval )         \
                                                                     \
         CHKPTR_VAL( errHandling, modname, string, retval);          \
                                                                     \
         if (     (  (void *)string    !=  (void *)0  )              \
              &&  (   strlen(string)   ==  0          )  )           \
         {                                                           \
            if ( (errHandling) == CHK_DISCOVER  )                    \
            {                                                        \
               chkin_c ( modname );                                  \
            }                                                        \
                                                                     \
            setmsg_c ( "String \"#\" has length zero." );            \
            errch_c  ( "#", (#string)                  );            \
            sigerr_c ( "SPICE(EMPTYSTRING)"            );            \
                                                                     \
            if (    ( (errHandling) == CHK_DISCOVER  )               \
                 || ( (errHandling) == CHK_STANDARD  )   )           \
            {                                                        \
               chkout_c ( modname );                                 \
               return    ( retval );                                 \
            }                                                        \
         }


/*
Macro CHKOSTR checks output string pointers and the associated
string length values supplied as input arguments.  Output string
pointers must be non-null, and the string lengths must be at
least 2, so Fortran routine can write at least one character to
the output string, and so a null terminator can be appended.
CHKOSTR should be used in void functions.  In non-void functions,
use CHKOSTR_VAL, which is defined below.
*/

#define  CHKOSTR( errHandling, modname, string, length )             \
                                                                     \
         CHKPTR ( errHandling, modname, string );                    \
                                                                     \
         if (     (  (void *)string    !=  (void *)0  )              \
              &&  (   length            <  2          )  )           \
         {                                                           \
            if ( (errHandling) == CHK_DISCOVER  )                    \
            {                                                        \
               chkin_c ( modname );                                  \
            }                                                        \
                                                                     \
            setmsg_c ( "String \"#\" has length #; must be >= 2." ); \
            errch_c  ( "#", (#string)                  );            \
            errint_c ( "#", (length)                   );            \
            sigerr_c ( "SPICE(STRINGTOOSHORT)"         );            \
                                                                     \
            if (    ( (errHandling) == CHK_DISCOVER  )               \
                 || ( (errHandling) == CHK_STANDARD  )   )           \
            {                                                        \
               chkout_c ( modname );                                 \
               return;                                               \
            }                                                        \
         }


#define  CHKOSTR_VAL( errHandling, modname, string, length, retval ) \
                                                                     \
         CHKPTR_VAL( errHandling, modname, string, retval );         \
                                                                     \
         if (     (  (void *)string    !=  (void *)0  )              \
              &&  (   length            <  2          )  )           \
         {                                                           \
            if ( (errHandling) == CHK_DISCOVER  )                    \
            {                                                        \
               chkin_c ( modname );                                  \
            }                                                        \
                                                                     \
            setmsg_c ( "String \"#\" has length #; must be >= 2." ); \
            errch_c  ( "#", (#string)                  );            \
            errint_c ( "#", (length)                   );            \
            sigerr_c ( "SPICE(STRINGTOOSHORT)"         );            \
                                                                     \
            if (    ( (errHandling) == CHK_DISCOVER  )               \
                 || ( (errHandling) == CHK_STANDARD  )   )           \
            {                                                        \
               chkout_c ( modname );                                 \
               return   ( retval  );                                 \
            }                                                        \
         }


   /*
   Definitions for Cells and Sets 
   */


   /*
   Cell initialization macros 
   */
   #define CELLINIT( cellPtr )                                       \
                                                                     \
      if ( !( (cellPtr)->init )  )                                   \
      {                                                              \
         if (  (cellPtr)->dtype  ==  SPICE_CHR  )                    \
         {                                                           \
            /*                                                       \
            Make sure all elements of the data array, including      \
            the control area, start off null-terminated.  We place   \
            the null character in the final element of each string,  \
            so as to avoid wiping out data that may have been        \
            assigned to the data array prior to initialization.      \
            */                                                       \
            SpiceChar  * sPtr;                                       \
            SpiceInt     i;                                          \
            SpiceInt     nmax;                                       \
                                                                     \
            nmax  = SPICE_CELL_CTRLSZ + (cellPtr)->size;             \
                                                                     \
            for ( i = 1;  i <= nmax;  i++ )                          \
            {                                                        \
               sPtr  =          (SpiceChar *)((cellPtr)->base)       \
                         +  i * (cellPtr)->length                    \
                         -  1;                                       \
                                                                     \
               *sPtr = NULLCHAR;                                     \
            }                                                        \
         }                                                           \
         else                                                        \
         {                                                           \
            zzsynccl_c ( C2F, (cellPtr) );                           \
         }                                                           \
                                                                     \
         (cellPtr)->init = SPICETRUE;                                \
      }                                                             


   #define CELLINIT2( cellPtr1, cellPtr2 )                           \
                                                                     \
      CELLINIT ( cellPtr1 );                                         \
      CELLINIT ( cellPtr2 );


   #define CELLINIT3( cellPtr1, cellPtr2, cellPtr3 )                 \
                                                                     \
      CELLINIT ( cellPtr1 );                                         \
      CELLINIT ( cellPtr2 );                                         \
      CELLINIT ( cellPtr3 );


   /*
   Data type checking macros: 
   */
   #define CELLTYPECHK( errHandling, modname, dType, cellPtr1 )      \
                                                                     \
         if ( (cellPtr1)->dtype != (dType) )                         \
         {                                                           \
            SpiceChar  * typstr[3] =                                 \
            {                                                        \
              "character", "double precision", "integer"             \
            };                                                       \
                                                                     \
            if ( (errHandling) == CHK_DISCOVER  )                    \
            {                                                        \
               chkin_c ( modname );                                  \
            }                                                        \
                                                                     \
            setmsg_c ( "Data type of # is #; expected type "         \
                       "is #."                                );     \
            errch_c  ( "#", (#cellPtr1)                       );     \
            errch_c  ( "#", typstr[ (cellPtr1)->dtype ]       );     \
            errch_c  ( "#", typstr[  dType            ]       );     \
            sigerr_c ( "SPICE(TYPEMISMATCH)"                  );     \
                                                                     \
            if (    ( (errHandling) == CHK_DISCOVER  )               \
                 || ( (errHandling) == CHK_STANDARD  )   )           \
            {                                                        \
               chkout_c ( modname );                                 \
               return;                                               \
            }                                                        \
         }


   #define CELLTYPECHK_VAL( errHandling,  modname,                   \
                            dType,        cellPtr1,  retval )        \
                                                                     \
         if ( (cellPtr1)->dtype != (dType) )                         \
         {                                                           \
            SpiceChar  * typstr[3] =                                 \
            {                                                        \
              "character", "double precision", "integer"             \
            };                                                       \
                                                                     \
            if ( (errHandling) == CHK_DISCOVER  )                    \
            {                                                        \
               chkin_c ( modname );                                  \
            }                                                        \
                                                                     \
            setmsg_c ( "Data type of # is #; expected type "         \
                       "is #."                                );     \
            errch_c  ( "#", (#cellPtr1)                       );     \
            errch_c  ( "#", typstr[ (cellPtr1)->dtype ]       );     \
            errch_c  ( "#", typstr[  dType            ]       );     \
            sigerr_c ( "SPICE(TYPEMISMATCH)"                  );     \
                                                                     \
            if (    ( (errHandling) == CHK_DISCOVER  )               \
                 || ( (errHandling) == CHK_STANDARD  )   )           \
            {                                                        \
               chkout_c ( modname );                                 \
               return (retval);                                      \
            }                                                        \
         }


   #define CELLTYPECHK2( errHandling,  modname,  dtype,              \
                         cellPtr1,     cellPtr2         )            \
                                                                     \
       CELLTYPECHK( errHandling, modname, dtype, cellPtr1 );         \
       CELLTYPECHK( errHandling, modname, dtype, cellPtr2 );



   #define CELLTYPECHK2_VAL( errHandling,  modname,  dtype,          \
                             cellPtr1,     cellPtr2, retval )        \
                                                                     \
       CELLTYPECHK_VAL( errHandling, modname, dtype, cellPtr1,       \
                        retval                                 );    \
       CELLTYPECHK_VAL( errHandling, modname, dtype, cellPtr2,       \
                        retval                                 );



   #define CELLTYPECHK3( errHandling,  modname,   dtype,             \
                         cellPtr1,     cellPtr2,  cellPtr3  )        \
                                                                     \
       CELLTYPECHK( errHandling, modname, dtype, cellPtr1 );         \
       CELLTYPECHK( errHandling, modname, dtype, cellPtr2 );         \
       CELLTYPECHK( errHandling, modname, dtype, cellPtr3 );


   #define CELLTYPECHK3_VAL( errHandling, modname,  dtype,           \
                             cellPtr1,    cellPtr2, cellPtr3,        \
                             retval                            )     \
                                                                     \
       CELLTYPECHK_VAL( errHandling, modname, dtype, cellPtr1,       \
                        retval                                 );    \
       CELLTYPECHK_VAL( errHandling, modname, dtype, cellPtr2,       \
                        retval                                 );    \
       CELLTYPECHK_VAL( errHandling, modname, dtype, cellPtr3,       \
                        retval                                 );



   #define CELLMATCH2( errHandling, modname, cellPtr1, cellPtr2 )    \
                                                                     \
         if ( (cellPtr1)->dtype != (cellPtr2)->dtype )               \
         {                                                           \
            SpiceChar  * typstr[3] =                                 \
            {                                                        \
              "character", "double precision", "integer"             \
            };                                                       \
                                                                     \
            if ( (errHandling) == CHK_DISCOVER  )                    \
            {                                                        \
               chkin_c ( modname );                                  \
            }                                                        \
                                                                     \
            setmsg_c ( "Data type of # is #; data type of # "        \
                       "is #, but types must match."          );     \
            errch_c  ( "#", (#cellPtr1)                       );     \
            errch_c  ( "#", typstr[ (cellPtr1)->dtype ]       );     \
            errch_c  ( "#", (#cellPtr2)                       );     \
            errch_c  ( "#", typstr[ (cellPtr2)->dtype ]       );     \
            sigerr_c ( "SPICE(TYPEMISMATCH)"                  );     \
                                                                     \
            if (    ( (errHandling) == CHK_DISCOVER  )               \
                 || ( (errHandling) == CHK_STANDARD  )   )           \
            {                                                        \
               chkout_c ( modname );                                 \
               return;                                               \
            }                                                        \
         }

   #define CELLMATCH2_VAL( errHandling, modname,                     \
                           cellPtr1,    cellPtr2, retval )           \
                                                                     \
         if ( (cellPtr1)->dtype != (cellPtr2)->dtype )               \
         {                                                           \
            SpiceChar  * typstr[3] =                                 \
            {                                                        \
              "character", "double precision", "integer"             \
            };                                                       \
                                                                     \
            if ( (errHandling) == CHK_DISCOVER  )                    \
            {                                                        \
               chkin_c ( modname );                                  \
            }                                                        \
                                                                     \
            setmsg_c ( "Data type of # is #; data type of # "        \
                       "is #, but types must match."          );     \
            errch_c  ( "#", (#cellPtr1)                       );     \
            errch_c  ( "#", typstr [ (cellPtr1)->dtype ]      );     \
            errch_c  ( "#", (#cellPtr2)                       );     \
            errch_c  ( "#", typstr [ (cellPtr2)->dtype ]      );     \
            sigerr_c ( "SPICE(TYPEMISMATCH)"                  );     \
                                                                     \
            if (    ( (errHandling) == CHK_DISCOVER  )               \
                 || ( (errHandling) == CHK_STANDARD  )   )           \
            {                                                        \
               chkout_c ( modname );                                 \
               return   ( retval  );                                 \
            }                                                        \
         }


   #define CELLMATCH3( errHandling, modname,                         \
                       cellPtr1,    cellPtr2,  cellPtr3 )            \
                                                                     \
       CELLMATCH2 ( errHandling, modname, cellPtr1, cellPtr2 );      \
       CELLMATCH2 ( errHandling, modname, cellPtr2, cellPtr3 );



      
   #define CELLMATCH3_VAL( errHandling, modname,  cellPtr1,          \
                           cellPtr2,    cellPtr3, retval    )        \
                                                                     \
       CELLMATCH2_VAL ( errHandling, modname,                        \
                        cellPtr1,    cellPtr2, retval );             \
                                                                     \
       CELLMATCH2_VAL ( errHandling, modname,                        \
                        cellPtr2,    cellPtr3, retval );

   /*
   Set checking macros: 
   */
   #define CELLISSETCHK( errHandling, modname, cellPtr1 )            \
                                                                     \
         if ( !(cellPtr1)->isSet )                                   \
         {                                                           \
            if ( (errHandling) == CHK_DISCOVER  )                    \
            {                                                        \
               chkin_c ( modname );                                  \
            }                                                        \
                                                                     \
            setmsg_c ( "Cell # must be sorted and have unique "      \
                       "values in order to be a CSPICE set. "        \
                       "The isSet flag in this cell is SPICEFALSE, " \
                       "indicating the cell may have been modified " \
                       "by a routine that doesn't preserve these "   \
                       "properties."                          );     \
            errch_c  ( "#", (#cellPtr1)                       );     \
            sigerr_c ( "SPICE(NOTASET)"                       );     \
                                                                     \
            if (    ( (errHandling) == CHK_DISCOVER  )               \
                 || ( (errHandling) == CHK_STANDARD  )   )           \
            {                                                        \
               chkout_c ( modname );                                 \
               return;                                               \
            }                                                        \
         }                                                           
                                                                     

   #define CELLISSETCHK_VAL( errHandling,  modname,                  \
                             cellPtr1,     retval  )                 \
                                                                     \
         if ( !(cellPtr1)->isSet )                                   \
         {                                                           \
            if ( (errHandling) == CHK_DISCOVER  )                    \
            {                                                        \
               chkin_c ( modname );                                  \
            }                                                        \
                                                                     \
            setmsg_c ( "Cell # must be sorted and have unique "      \
                       "values in order to be a CSPICE set. "        \
                       "The isSet flag in this cell is SPICEFALSE, " \
                       "indicating the cell may have been modified " \
                       "by a routine that doesn't preserve these "   \
                       "properties."                          );     \
            errch_c  ( "#", (#cellPtr1)                       );     \
            sigerr_c ( "SPICE(NOTASET)"                       );     \
                                                                     \
            if (    ( (errHandling) == CHK_DISCOVER  )               \
                 || ( (errHandling) == CHK_STANDARD  )   )           \
            {                                                        \
               chkout_c ( modname );                                 \
               return (retval);                                      \
            }                                                        \
         }


   #define CELLISSETCHK2( errHandling,  modname,                     \
                          cellPtr1,     cellPtr2 )                   \
                                                                     \
       CELLISSETCHK( errHandling, modname, cellPtr1 );               \
       CELLISSETCHK( errHandling, modname, cellPtr2 );



   #define CELLISSETCHK2_VAL( errHandling,  modname,                 \
                              cellPtr1,     cellPtr2, retval )       \
                                                                     \
       CELLISSETCHK_VAL( errHandling, modname, cellPtr1, retval );   \
       CELLISSETCHK_VAL( errHandling, modname, cellPtr2, retval );   \
 


   #define CELLISSETCHK3( errHandling,  modname,                     \
                          cellPtr1,     cellPtr2,  cellPtr3  )       \
                                                                     \
       CELLISSETCHK ( errHandling, modname, cellPtr1 );              \
       CELLISSETCHK ( errHandling, modname, cellPtr2 );              \
       CELLISSETCHK ( errHandling, modname, cellPtr3 );


   #define CELLISSETCHK3_VAL( errHandling, modname,  cellPtr1,       \
                              cellPtr2,    cellPtr3, retval    )     \
                                                                     \
       CELLISSETCHK_VAL ( errHandling, modname, cellPtr1, retval );  \
       CELLISSETCHK_VAL ( errHandling, modname, cellPtr2, retval );  \
       CELLISSETCHK_VAL ( errHandling, modname, cellPtr3, retval );


   /*
   C-to-Fortran and Fortran-to-C character cell translation macros:
   */

   /*
   Macros that map one or more character C cells to dynamically 
   allocated Fortran-style character cells:
   */
   #define C2F_MAP_CELL( caller, CCell, fCell, fLen )                \
                                                                     \
      {                                                              \
         /*                                                          \
         fCell and fLen are to be passed by reference, as if this    \
         macro were a function.                                      \    
                                                                     \
                                                                     \
         Caution: dynamically allocates array fCell, which is to be  \
         freed by caller!                                            \
         */                                                          \
         SpiceInt                ndim;                               \
         SpiceInt                lenvals;                            \
                                                                     \
                                                                     \
         ndim     =  (CCell)->size  +  SPICE_CELL_CTRLSZ;            \
         lenvals  =  (CCell)->length;                                \
                                                                     \
         C2F_MapFixStrArr ( (caller),      ndim,    lenvals,         \
                            (CCell)->base, (fLen),  (fCell)  );      \
                                                                     \
         if ( !failed_c() )                                          \
         {                                                           \
            /*                                                       \
            Explicitly set the control area info in the Fortran cell.\
            */                                                       \
            ssizec_ ( ( integer * ) &((CCell)->size),                \
                      ( char    * ) *(fCell),                        \
                      ( ftnlen    ) *(fLen)           );             \
                                                                     \
            scardc_ ( ( integer * ) &((CCell)->card),                \
                      ( char    * ) *(fCell),                        \
                      ( ftnlen    ) *(fLen)           );             \
                                                                     \
            if ( failed_c() )                                        \
            {                                                        \
               /*                                                    \
               Setting size or cardinality of the Fortran cell       \
               can fail, for example if the cell's string length     \
               is too short.                                         \
               */                                                    \
               free ( *(fCell) );                                    \
            }                                                        \
         }                                                           \
      }


   #define C2F_MAP_CELL2( caller, CCell1, fCell1, fLen1,             \
                                  CCell2, fCell2, fLen2 )            \
                                                                     \
      {                                                              \
         C2F_MAP_CELL( caller, CCell1, fCell1, fLen1 );              \
                                                                     \
         if ( !failed_c() )                                          \
         {                                                           \
            C2F_MAP_CELL( caller, CCell2, fCell2, fLen2 );           \
                                                                     \
            if ( failed_c() )                                        \
            {                                                        \
               free ( *(fCell1) );                                   \
            }                                                        \
         }                                                           \
      }


   #define C2F_MAP_CELL3( caller, CCell1, fCell1, fLen1,             \
                                  CCell2, fCell2, fLen2,             \
                                  CCell3, fCell3, fLen3 )            \
                                                                     \
      {                                                              \
         C2F_MAP_CELL2( caller, CCell1, fCell1, fLen1,               \
                                CCell2, fCell2, fLen2  );            \
                                                                     \
         if ( !failed_c() )                                          \
         {                                                           \
            C2F_MAP_CELL( caller, CCell3, fCell3, fLen3 );           \
                                                                     \
            if ( failed_c() )                                        \
            {                                                        \
               free ( *(fCell1) );                                   \
               free ( *(fCell2) );                                   \
            }                                                        \
         }                                                           \
      }



   /*
   Macro that maps a Fortran-style character cell to a C cell
   (Note: this macro frees the Fortran cell): 
   */

   #define F2C_MAP_CELL( fCell, fLen, CCell )                        \
                                                                     \
      {                                                              \
         SpiceInt                card;                               \
         SpiceInt                lenvals;                            \
         SpiceInt                ndim;                               \
         SpiceInt                nBytes;                             \
         SpiceInt                size;                               \
         void                  * array;                              \
                                                                     \
         ndim     =  (CCell)->size  +  SPICE_CELL_CTRLSZ;            \
         lenvals  =  (CCell)->length;                                \
         array    =  (CCell)->base;                                  \
                                                                     \
         /*                                                          \
         Capture the size and cardinality of the Fortran cell.       \
         */                                                          \
         if ( !failed_c() )                                          \
         {                                                           \
            size = sizec_ (  ( char      * ) (fCell),                \
                             ( ftnlen      ) fLen     );             \
                                                                     \
            card = cardc_ (  ( char      * ) (fCell),                \
                             ( ftnlen      ) fLen     );             \
         }                                                           \
                                                                     \
                                                                     \
         /*                                                          \
         Copy the Fortran array into the output array.               \
         */                                                          \
                                                                     \
         nBytes = ndim * fLen * sizeof(SpiceChar);                   \
         memmove ( array, fCell, nBytes );                           \
         /*                                                          \
         Convert the output array from Fortran to C style.           \
         */                                                          \
         F2C_ConvertTrStrArr ( ndim, lenvals, (SpiceChar *)array );  \
                                                                     \
         /*                                                          \
         Sync the size and cardinality of the C cell.                \
         */                                                          \
         if ( !failed_c() )                                          \
         {                                                           \
            (CCell)->size = size;                                    \
            (CCell)->card = card;                                    \
         }                                                           \
      }


   
/*
   End of header SpiceZmc.h 
*/
