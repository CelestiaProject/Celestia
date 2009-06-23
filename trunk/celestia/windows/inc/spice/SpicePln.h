/*

-Header_File SpicePln.h ( CSPICE Plane definitions )

-Abstract

   Perform CSPICE definitions for the SpicePlane data type.
            
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

   This header defines structures and typedefs that may be referenced in 
   application code that calls CSPICE Plane functions.
   

      Structures
      ==========
   
         Name                  Description
         ----                  ----------
   
         SpicePlane            Structure representing a plane in 3-
                               dimensional space.
         
                               The members are:
 
                                  normal:     Vector normal to plane.

                                  constant:   Constant of plane equation
                                       
                                              Plane =  
                                              
                                              {X: <normal,X> = constant}



         ConstSpicePlane       A const SpicePlane.
         
         
-Literature_References

   None.

-Author_and_Institution

   N.J. Bachman       (JPL)
   
-Restrictions

   None.
      
-Version

   -CSPICE Version 1.0.0, 04-MAR-1999 (NJB)  

*/

#ifndef HAVE_SPICE_PLANES

   #define HAVE_SPICE_PLANES
   
   
   
   /*
   Plane structure:
   */
   
   struct _SpicePlane 
   
      { SpiceDouble      normal   [3];
        SpiceDouble      constant;     };
          
   typedef struct _SpicePlane  SpicePlane;

   typedef const SpicePlane    ConstSpicePlane;
 
#endif

