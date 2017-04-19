/*

-Header_File SpiceOccult.h ( CSPICE Occultation specific definitions )

-Abstract
 
   Perform CSPICE occultation specific definitions.

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

-Keywords

   OCCULTATION
   GEOMETRY
   ELLIPSOID

-Exceptions

   None

-Files

   None

-Particulars

   The following integer codes indicate the geometric relationship
   of the three bodies.

   The meaning of the sign of each code is given below.

               Code sign          Meaning
               ---------          ------------------------------
                  > 0             The second ellipsoid is
                                  partially or fully occulted
                                  by the first.

                  < 0             The first ellipsoid is
                                  partially of fully
                                  occulted by the second.

                  = 0             No occultation.

   The meanings of the codes are given below. The variable names
   indicate the type of occultation and which target is in the back.
   For example, SPICE_OCCULT_TOTAL1 represents a total occultation in which
   the first target is in the back (or occulted by) the second target.

         Name                 Code     Meaning
         ------               -----    ------------------------------
         SPICE_OCCULT_TOTAL1   -3      Total occultation of first
                                       target by second.

         SPICE_OCCULT_ANNLR1   -2      Annular occultation of first
                                       target by second.  The second
                                       target does not block the limb
                                       of the first.

         SPICE_OCCULT_PARTL1   -1      Partial occultation of first
                                       target by second target.

         SPICE_OCCULT_NOOCC     0      No occultation or transit:  both
                                       objects are completely visible
                                       to the observer.

         SPICE_OCCULT_PARTL2    1      Partial occultation of second
                                       target by first target.

         SPICE_OCCULT_ANNLR2    2      Annular occultation of second
                                       target by first.

         SPICE_OCCULT_TOTAL2    3      Total occultation of second
                                       target by first.

-Examples
      
   None 

-Restrictions

   None.

-Literature_References

   None.

-Author_and_Institution

   S.C. Krening    (JPL)
   N.J. Bachman    (JPL)

-Version

   -CSPICE Version 1.0.0, 23-FEB-2012 (SCK)

*/


#ifndef HAVE_SPICE_OCCULT_H

   #define HAVE_SPICE_OCCULT_H
   
   /*
   See the Particulars section above for parameter descriptions.
   */

   /*
   Occultation parameters
   */

   #define SPICE_OCCULT_TOTAL1   -3
   #define SPICE_OCCULT_ANNLR1   -2
   #define SPICE_OCCULT_PARTL1   -1
   #define SPICE_OCCULT_NOOCC     0
   #define SPICE_OCCULT_PARTL2    1
   #define SPICE_OCCULT_ANNLR2    2
   #define SPICE_OCCULT_TOTAL2    3


#endif
