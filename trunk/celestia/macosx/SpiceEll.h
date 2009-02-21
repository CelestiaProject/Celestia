/*

-Header_File SpiceEll.h ( CSPICE Ellipse definitions )

-Abstract

   Perform CSPICE definitions for the SpiceEllipse data type.
            
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
   application code that calls CSPICE Ellipse functions.
   

      Structures
      ==========
   
         Name                  Description
         ----                  ----------
   
         SpiceEllipse          Structure representing an ellipse in 3-
                               dimensional space.
         
                               The members are:
 
                                  center:     Vector defining ellipse's
                                              center.

                                  semiMajor:  Vector defining ellipse's
                                              semi-major axis.
                                       
                                  semiMinor:  Vector defining ellipse's
                                              semi-minor axis.
                                       
                               The ellipse is the set of points
                               
                                 {X:  X =                  center 
                                            + cos(theta) * semiMajor
                                            + sin(theta) * semiMinor,
                                            
                                  theta in [0, 2*Pi) }


         ConstSpiceEllipse     A const SpiceEllipse.
         
         
-Literature_References

   None.

-Author_and_Institution

   N.J. Bachman       (JPL)
   
-Restrictions

   None.
      
-Version

   -CSPICE Version 1.0.0, 04-MAR-1999 (NJB)  

*/

#ifndef HAVE_SPICE_ELLIPSES

   #define HAVE_SPICE_ELLIPSES
   
   
   
   /*
   Ellipse structure:
   */
   
   struct _SpiceEllipse 
   
      { SpiceDouble      center    [3];
        SpiceDouble      semiMajor [3];     
        SpiceDouble      semiMinor [3];  };
          
   typedef struct _SpiceEllipse  SpiceEllipse;

   typedef const SpiceEllipse    ConstSpiceEllipse;
 
#endif

