
/*

-Header_File SpiceDtl.h ( CSPICE DSK tolerance definitions )

-Abstract

   Define CSPICE DSK tolerance and margin macros.
         
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

   DSK
   
-Particulars

   This file contains declarations of tolerance and margin values
   used by the DSK subsystem.

   The values declared in this file are accessible at run time
   through the routines

      dskgtl_c  {DSK, get tolerance value}
      dskstl_c  {DSK, set tolerance value}

-Literature_References

   None.

-Author_and_Institution

   N.J. Bachman       (JPL)
   
-Restrictions

   It is recommended that the default values defined in this file be
   changed only by expert SPICE users.     
      
-Version

   -CSPICE Version 1.0.0, 27-FEB-2016 (NJB)  

*/


#ifndef HAVE_SPICE_DSKTOL_H

   #define HAVE_SPICE_DSKTOL_H


/*
   Parameter declarations
   ======================

   DSK type 2 plate expansion factor
   ---------------------------------

   The factor SPICE_DSK_XFRACT is used to slightly expand plates
   read from DSK type 2 segments in order to perform ray-plate
   intercept computations.

   This expansion is performed to prevent rays from passing through
   a target object without any intersection being detected. Such
   "false miss" conditions can occur due to round-off errors.

   Plate expansion is done by computing the difference vectors
   between a plate's vertices and the plate's centroid, scaling
   those differences by (1 + SPICE_DSK_XFRACT), then producing new
   vertices by adding the scaled differences to the centroid. This
   process doesn't affect the stored DSK data.

   Plate expansion is also performed when surface points are mapped
   to plates on which they lie, as is done for illumination angle
   computations.

   This parameter is user-adjustable.
   */
   #define  SPICE_DSK_XFRACT     ( 1.0e-10 )

   /*
   The keyword for setting or retrieving this factor is
   */
   #define  SPICE_DSK_KEYXFR      1


   /*
   Greedy segment selection factor
   -------------------------------

   The factor SGREED is used to slightly expand DSK segment
   boundaries in order to select segments to consider for
   ray-surface intercept computations. The effect of this factor is
   to make the multi-segment intercept algorithm consider all
   segments that are sufficiently close to the ray of interest, even
   if the ray misses those segments.

   This expansion is performed to prevent rays from passing through
   a target object without any intersection being detected. Such
   "false miss" conditions can occur due to round-off errors.

   The exact way this parameter is used is dependent on the
   coordinate system of the segment to which it applies, and the DSK
   software implementation. This parameter may be changed in a
   future version of SPICE.
   */
   #define  SPICE_DSK_SGREED     ( 1.0e-8 )    

   /*
   The keyword for setting or retrieving this factor is
   */
   #define  SPICE_DSK_KEYSGR     ( SPICE_DSK_KEYXFR + 1 )


   /*
   Segment pad margin
   ------------------

   The segment pad margin is a scale factor used to determine when a
   point resulting from a ray-surface intercept computation, if
   outside the segment's boundaries, is close enough to the segment
   to be considered a valid result.

   This margin is required in order to make DSK segment padding
   (surface data extending slightly beyond the segment's coordinate
   boundaries) usable: if a ray intersects the pad surface outside
   the segment boundaries; the pad is useless if the intercept is
   automatically rejected.

   However, an excessively large value for this parameter is
   detrimental, since a ray-surface intercept solution found "in" a
   segment will supersede solutions in segments farther from the ray's
   vertex. Solutions found outside of a segment thus can mask solutions
   that are closer to the ray's vertex by as much as the value of this
   margin, when applied to a segment's boundary dimensions.
   */
   #define  SPICE_DSK_SGPADM        ( 1.0e-10 )


   /*
   The keyword for setting or retrieving this factor is
   */
   #define  SPICE_DSK_KEYSPM        ( SPICE_DSK_KEYSGR + 1 )


   /*
   Surface-point membership margin
   -------------------------------

   The surface-point membership margin limits the distance
   between a point and a surface to which the point is
   considered to belong. The margin is a scale factor applied
   to the size of the segment containing the surface.

   This margin is used to map surface points to outward
   normal vectors at those points.

   If this margin is set to an excessively small value,
   routines that make use of the surface-point mapping won't
   work properly.
   */
   #define  SPICE_DSK_PTMEMM        ( 1.0e-7 )

   /*    
   The keyword for setting or retrieving this factor is
   */
   #define  SPICE_DSK_KEYPTM        ( SPICE_DSK_KEYSPM + 1 )


   /*
   Angular rounding margin
   -----------------------

   This margin specifies an amount by which angular values
   may deviate from their proper ranges without a SPICE error
   condition being signaled.

   For example, if an input latitude exceeds pi/2 radians by a
   positive amount less than this margin, the value is treated as
   though it were pi/2 radians.

   Units are radians.
   */
   #define  SPICE_DSK_ANGMRG        ( 1.0e-12 )

   /*
   This parameter is not user-adjustable.

   The keyword for retrieving this parameter is
   */
   #define  SPICE_DSK_KEYAMG        ( SPICE_DSK_KEYPTM + 1 )


   /*
   Longitude alias margin
   ----------------------

   This margin specifies an amount by which a longitude
   value can be outside a given longitude range without
   being considered eligible for transformation by
   addition or subtraction of 2*pi radians.

   A longitude value, when compared to the endpoints of
   a longitude interval, will be considered to be equal
   to an endpoint if the value is outside the interval
   differs from that endpoint by a magnitude less than
   the alias margin.


   Units are radians.
   */
   #define  SPICE_DSK_LONALI        ( 1.0e-12 )

   /*
   This parameter is not user-adjustable.

   The keyword for retrieving this parameter is
   */
   #define  SPICE_DSK_KEYLAL        ( SPICE_DSK_KEYAMG + 1 )



#endif

/*
End of header file SpiceDtl.h
*/

