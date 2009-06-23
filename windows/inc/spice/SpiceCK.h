/*

-Header_File SpiceCK.h ( CSPICE CK definitions )

-Abstract

   Perform CSPICE definitions to support CK wrapper interfaces.
            
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
   application code that calls CSPICE CK functions.

      Typedef
      =======
   
         Name                  Description
         ----                  ----------
   
         SpiceCK05Subtype      Typedef for enum indicating the 
                               mathematical representation used
                               in an CK type 05 segment.  Possible
                               values and meanings are:

                               C05TP0:
 
                                  Hermite interpolation, 8-
                                  element packets containing 
                                  
                                     q0,      q1,      q2,     q3, 
                                     dq0/dt,  dq1/dt,  dq2/dt  dq3/dt
                   
                                  where q0, q1, q2, q3 represent 
                                  quaternion components and dq0/dt, 
                                  dq1/dt, dq2/dt, dq3/dt represent 
                                  quaternion time derivative components.  
 
                                  Quaternions are unitless.  Quaternion
                                  time derivatives have units of 
                                  1/second.


                               C05TP1:
  
                                  Lagrange interpolation, 4-
                                  element packets containing 

                                     q0,     q1,     q2,     q3, 
 
                                  where q0, q1, q2, q3 represent 
                                  quaternion components.  Quaternion
                                  derivatives are obtained by
                                  differentiating interpolating
                                  polynomials.


                               C05TP2:
  
                                  Hermite interpolation, 14-
                                  element packets containing 

                                     q0,      q1,      q2,     q3,  
                                     dq0/dt,  dq1/dt,  dq2/dt  dq3/dt,
                                     av0,     av1,     av2,
                                     dav0/dt, dav1/dt, dav2/dt
                   
                                  where q0, q1, q2, q3 represent 
                                  quaternion components and dq0/dt, 
                                  dq1/dt, dq2/dt, dq3/dt represent 
                                  quaternion time derivative components,
                                  av0, av1, av2 represent angular
                                  velocity components, and 
                                  dav0/dt, dav1/dt, dav2/dt represent 
                                  angular acceleration components.
 

                               C05TP3:
  
                                  Lagrange interpolation, 7-
                                  element packets containing 

                                     q0,     q1,     q2,     q3, 
                                     av0,    av1,    av2
 
                                  where q0, q1, q2, q3 represent 
                                  quaternion components and         
                                  av0, av1, av2 represent angular
                                  velocity components.



Literature_References

   None.

-Author_and_Institution

   N.J. Bachman       (JPL)
   
-Restrictions

   None.
      
-Version

   -CSPICE Version 1.0.0, 20-AUG-2002 (NJB)  

*/

#ifndef HAVE_SPICE_CK_H

   #define HAVE_SPICE_CK_H
   
   
   
   /*
   CK type 05 subtype codes:
   */
   
   enum _SpiceCK05Subtype  { C05TP0, C05TP1, C05TP2, C05TP3 };
   

   typedef enum _SpiceCK05Subtype SpiceCK05Subtype;
 
#endif

