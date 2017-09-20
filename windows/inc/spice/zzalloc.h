/*

-Abstract

   The memory allocation prototypes and macros for use in CSPICE.

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

-Particulars

   The routines maintain a count of the number of mallocs vs. free,
   signalling an error if any unreleased memory exists at the end
   of an Icy interface call.

   The macro ALLOC_CHECK performs malloc/free test. If used, the macro
   should exists at the end of any routine using these memory management
   routines.

   Prototypes in this file:

      alloc_SpiceBoolean_C_array
      alloc_SpiceDouble_C_array
      alloc_SpiceInt_C_array
      alloc_SpiceMemory
      alloc_SpiceString
      alloc_SpiceString_C_Copy_array
      alloc_SpiceString_C_array
      alloc_SpiceString_Pointer_array
      alloc_count
      free_SpiceMemory
      free_SpiceString_C_array
      zzalloc_count

-Version

   CSPICE 1.3.0 26-AUG-2016 (EDW)

      Added routine alloc_SpiceBoolean_C_array.

   CSPICE 1.0.3 02-MAY-2008 (EDW)

      Added alloc_count prototype.

   CSPICE 1.0.2 10-MAY-2007 (EDW)

      Minor edits to clarify 'size' in alloc_SpiceMemory as
      size_t.

   CSPICE 1.0.1 23-JUN-2005 (EDW)

      Add prototype for alloc_SpiceString_Pointer_array, allocate
      an array of pointers to SpiceChar.

   Icy 1.0.0 December 19, 2003 (EDW)

      Initial release.

*/

#ifndef ZZALLOC_H
#define ZZALLOC_H

   /*
   Allocation call prototypes:
   */
   int           alloc_count                    ();

   SpiceChar  ** alloc_SpiceString_C_array      ( int string_length,
                                                  int string_count   );

   SpiceChar  ** alloc_SpiceString_C_Copy_array ( int array_len ,
                                                  int string_len,
                                                  SpiceChar ** array );

   SpiceDouble * alloc_SpiceDouble_C_array      ( int rows,
                                                  int cols );

   SpiceInt    * alloc_SpiceInt_C_array         ( int rows,
                                                  int cols );

   SpiceBoolean * alloc_SpiceBoolean_C_array    ( int rows,
                                                  int cols );

   SpiceChar   * alloc_SpiceString              ( int length );

   SpiceChar  ** alloc_SpiceString_Pointer_array( int array_len );

   void          free_SpiceString_C_array       ( int dim,
                                                  SpiceChar ** array );

   void        * alloc_SpiceMemory              ( size_t size );

   void          free_SpiceMemory               ( void * ptr );


   /*
   Simple macro to ensure a zero value alloc count at end of routine.
   Note, the need to use this macro exists only in those routines
   allocating/deallocating memory.
   */
#define ALLOC_CHECK  if (  alloc_count() != 0 )                            \
                {                                                          \
                setmsg_c ( "Malloc/Free count not zero at end of routine." \
                           " Malloc count = #.");                          \
                errint_c ( "#", alloc_count()            );                \
                sigerr_c ( "SPICE(MALLOCCOUNT)"        );                  \
                }

#endif

