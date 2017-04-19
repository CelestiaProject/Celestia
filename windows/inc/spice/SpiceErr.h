/*

-Header_File SpiceErr.h ( CSPICE error handling definitions )

-Abstract

   Perform CSPICE definitions for error handling APIs.
            
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

   This header defines constants that may be referenced in 
   application code that calls CSPICE error handling functions.
   

      CONSTANTS
      ==========
   
         Name                  Description
         ----                  ----------
   
         SPICE_ERROR_LMSGLN    Maximum length of a long error message,
                               including the null terminator.

         SPICE_ERROR_SMSGLN    Maximum length of a short error message,
                               including the null terminator.

         SPICE_ERROR_XMSGLN    Maximum length of a short error
                               explanation message, including the null
                               terminator.

         SPICE_ERROR_MODLEN    Maximum length of a module name
                               appearing in the traceback message,
                               including the null terminator.
  
         SPICE_ERROR_MAXMOD    Maximum count of module names
                               appearing in the traceback message.

         SPICE_ERROR_TRCLEN    Maximum length of a traceback message,
                               including the null terminator.
         
         
-Literature_References

   None.

-Author_and_Institution

   N.J. Bachman       (JPL)
   
-Restrictions

   None.
      
-Version

   -CSPICE Version 1.0.0, 05-NOV-2013 (NJB)  

*/

#ifndef HAVE_SPICE_ERROR_HANDLING

   #define HAVE_SPICE_ERROR_HANDLING
   
   
   /*
   Local constants 
   */
   #define ARROWLEN                    5

   /*
   Public constants 
   */

   /*
   Long error message length, which is equal to

      ( 23 * 80 ) + 1

   */
   #define SPICE_ERROR_LMSGLN         1841

   /*
   Short error message length:
   */
   #define SPICE_ERROR_SMSGLN         26

   /*
   Short error message explanation length:
   */
   #define SPICE_ERROR_XMSGLN         81

   /*
   Module name length for traceback entries:
   */
   #define SPICE_ERROR_MODLEN         33

   /*
   Maximum module count for traceback string: 
   */
   #define SPICE_ERROR_MAXMOD         100

   /*
   Maximum length of traceback string returned
   by qcktrc_c.
   */
   #define SPICE_ERROR_TRCLEN  (   (     SPICE_ERROR_MAXMOD          \
                                     * ( SPICE_ERROR_MODLEN-1 ) )    \
                                 + (     ARROWLEN                    \
                                     * ( SPICE_ERROR_MAXMOD-1 ) )    \
                                 + 1                                )
#endif

