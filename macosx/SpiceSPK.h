/*

-Header_File SpiceSPK.h ( CSPICE SPK definitions )

-Abstract

   Perform CSPICE definitions to support SPK wrapper interfaces.
            
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

   This header defines types that may be referenced in 
   application code that calls CSPICE SPK functions.

      Typedef
      =======
   
         Name                  Description
         ----                  ----------
   
         SpiceSPK18Subtype     Typedef for enum indicating the 
                               mathematical representation used
                               in an SPK type 18 segment.  Possible
                               values and meanings are:

                                S18TP0:
 
                                  Hermite interpolation, 12-
                                  element packets containing 
                                  
                                     x,  y,  z,  dx/dt,  dy/dt,  dz/dt, 
                                     vx, vy, vz, dvx/dt, dvy/dt, dvz/dt 
                   
                                  where x, y, z represent Cartesian
                                  position components and vx, vy, vz
                                  represent Cartesian velocity
                                  components.  Note well:  vx, vy, and
                                  vz *are not necessarily equal* to the
                                  time derivatives of x, y, and z.
                                  This packet structure mimics that of
                                  the Rosetta/MEX orbit file from which
                                  the data are taken.
 
                                  Position units are kilometers,
                                  velocity units are kilometers per
                                  second, and acceleration units are
                                  kilometers per second per second.


                                S18TP1:
  
                                  Lagrange interpolation, 6-
                                  element packets containing 

                                     x,  y,  z,  dx/dt,  dy/dt,  dz/dt
 
                                  where x, y, z represent Cartesian
                                  position components and  vx, vy, vz
                                  represent Cartesian velocity
                                  components.
 
                                  Position units are kilometers;
                                  velocity units are kilometers per
                                  second.
 
-Literature_References

   None.

-Author_and_Institution

   N.J. Bachman       (JPL)
   
-Restrictions

   None.
      
-Version

   -CSPICE Version 1.0.0, 16-AUG-2002 (NJB)  

*/

#ifndef HAVE_SPICE_SPK_H

   #define HAVE_SPICE_SPK_H
   
   
   
   /*
   SPK type 18 subtype codes:
   */
   
   enum _SpiceSPK18Subtype  { S18TP0, S18TP1 };
   

   typedef enum _SpiceSPK18Subtype SpiceSPK18Subtype;
 
#endif

