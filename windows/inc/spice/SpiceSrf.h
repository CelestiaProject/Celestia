/*

-Header_File SpiceSrf.h ( CSPICE surface definitions )

-Abstract

   Perform CSPICE definitions for surface name-ID mapping.
            
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

   This header defines constants that may be referenced in application
   code that calls CSPICE surface name-ID mapping functions.
   

      Constants
      ==========
   
         Name                  Description
         ----                  ----------
   
         SPICE_SRF_SFNMLN      Maximum length of a surface name,
                               including the terminating null
                               character.
 
         SPICE_SRF_MAXSRF      Maximum number of surfaces that
                               can be accommodated in a surface 
                               list.         

-Literature_References

   None.

-Author_and_Institution

   N.J. Bachman       (JPL)
   
-Restrictions

   None.
      
-Version

   -CSPICE Version 1.0.0, 30-AUG-2016 (NJB)  

       Added macro SPICE_SRF_MAXSRF

    22-JAN-2016 (NJB)  

       Original version.
*/

#ifndef HAVE_SRF

  #define HAVE_SRF
   
      
  /*
  Constants
  */   
  #define SPICE_SRF_SFNMLN          37

  #define SPICE_SRF_MAXSRF          100   
 
#endif

