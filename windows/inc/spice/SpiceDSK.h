/*

-Header_File SpiceDSK.h ( CSPICE DSK-specific definitions )

-Abstract

   Perform CSPICE DSK-specific definitions, including macros and user-
   defined types.
         
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

   This header defines macros, enumerated types, structures, and
   typedefs that may be referenced in application code that calls
   CSPICE DSK functions.
   

   General definitions
   ===================

   Macros
   ======


      Dimensions
      ----------

         Name                  Description
         ----                  -----------

         SPICE_DSK_DSCSIZ      Size of a Fortran DSK descriptor,
                               measured in multiples of the size of a
                               SpiceDouble. A array `descr' containing
                               such a descriptor can be declared
 
                                  SpiceDouble descr[SPICE_DSK_DSCSIZ];
 
                               Such arrays normally should be used only
                               in the implementations of CSPICE
                               wrappers.
   

      Data class values
      -----------------

         Name                  Description
         ----                  -----------

         SPICE_DSK_SVFCLS      (Class 1) indicates a surface
                               that can be represented as a single-valued
                               function of its domain coordinates.
 
                               An example is a surface defined by a
                               function that maps each planetodetic
                               longitude and latitude pair to a unique
                               altitude.

         SPICE_DSK_GENCLS      (Class 2) indicates a general surface.
                               Surfaces that have multiple points for a
                               given pair of domain coordinates---for
                               example, multiple radii for a given
                               latitude and longitude---belong to class
                               2.
 
                             
      Coordinate system values
      ------------------------

         Name                  Description
         ----                  -----------

         SPICE_DSK_LATSYS      Code 1 refers to the planetocentric
                               latitudinal system.
 
                               In this system, the first tangential
                               coordinate is longitude and the second
                               tangential coordinate is latitude. The
                               third coordinate is radius.


         SPICE_DSK_CYLSYS      Code 2 refers to the cylindrical system. 
 
                               In this system, the first tangential
                               coordinate is radius and the second
                               tangential coordinate is longitude. The
                               third, orthogonal coordinate is Z.


         SPICE_DSK_RECSYS      Code 3 refers to the rectangular system.
 
                               In this system, the first tangential
                               coordinate is X and the second
                               tangential coordinate is Y. The third,
                               orthogonal coordinate is Z.


         SPICE_DSK_PDTSYS      Code 4 refers to the
                               planetodetic/geodetic system.

                               In this system, the first tangential
                               coordinate is longitude and the second
                               tangential coordinate is planetodectic
                               latitude. The third, orthogonal
                               coordinate is altitude.


   Structures
   ==========

   
      DSK API structures
      ------------------

         Name                  Description
         ----                  -----------

         SpiceDSKDescr         DSK descriptor.
         
                               The structure members are:
 
                                  surfce:     Surface ID code. Data
                                              type is SpiceInt.
 
                                  center:     Center ID code. Data
                                              type is SpiceInt.
 
                                  dclass:     Data class ID code. Data
                                              type is SpiceInt.
 
                                  dtype:      DSK Data type. Data
                                              type is SpiceInt.
  
                                  frmcde:     Reference frame ID code. Data
                                              type is SpiceInt.

                                  corsys:     Coordinate system ID code. Data
                                              type is SpiceInt.
 
                                  corpar:     Coordinate system parameters.
                                              Data type is SpiceDouble. This
                                              is an array of length 10.
                    
                                  co1min:     Minimum value of first 
                                              coordinate. Data type is 
                                              SpiceDouble.

                                  co1max:     Maximum value of first 
                                              coordinate. Data type is 
                                              SpiceDouble.
                    
                                  co2min:     Minimum value of second 
                                              coordinate. Data type is 
                                              SpiceDouble.

                                  co2max:     Maximum value of second 
                                              coordinate. Data type is 
                                              SpiceDouble.
                    
                                  co3min:     Minimum value of third 
                                              coordinate. Data type is 
                                              SpiceDouble.

                                  co3max:     Maximum value of third 
                                              coordinate. Data type is 
                                              SpiceDouble.

                                  start:      Coverage start time, expressed 
                                              as seconds past J2000 TDB.

                                  stop:       Coverage stop time, expressed 
                                              as seconds past J2000 TDB.
                                  


         ConstSpiceDSKDescr    A constant DSK descriptor.





   Type 2 definitions
   ==================


   Type 2 Macros
   =============

 
      Limits on plate model capacity
      ------------------------------
 
      This section contains parameter descriptions. See declarations
      located near the end of the file for parameter values.

      The maximum number of vertices and plates in a single-segment
      type 2 plate model are provided here. Larger models must be
      distributed across multiple segments, which typically are in
      separate files.

      These values can be used to dimension arrays, or to use as limit
      checks.

      The value of SPICE_DSK02_MAXPLT is determined from
      SPICE_DSK02_MAXVRT via Euler's Formula for simple polyhedra having
      triangular faces.


         Name                  Description
         ----                  -----------

         SPICE_DSK02_MAXVRT    Maximum number of vertices the
                               DSK type 2 software will
                               support in a single segment.


         SPICE_DSK02_MAXPLT    Maximum number of plates the
                               DSK type 2 software will
                               support in a single segment.


         SPICE_DSK02_MAXCGR    Maximum number of elements permitted
                               in the coarse voxel grid.  This parameter
                               is not used directly in CSPICE; rather 
                               it is a convenience parameter that mirrors
                               the parameter MAXCGR declared in the 
                               SPICELIB INCLUDE file

                                  dsk02.inc


         SPICE_DSK02_MAXVXP    Maximum size of voxel-plate pointer array.


         SPICE_DSK02_MXNVLS    Maximum size of voxel-plate association list.


         SPICE_DSK02_MAXCEL    Maximum size of spatial index cell workspace.


         SPICE_DSK02_SPAISZ    Maximum size of array containing 
                               integer component of spatial index.
                               This size is used by MKDSK. Many 
                               applications may be able to use
                               smaller dimensions than the value
                               specified by this parameter.

         SPICE_DSK02_SPADSZ    Size of double precision component
                               of spatial index.


      Integer keyword parameters
      --------------------------

      The following parameters may be passed to dski02_c to identify
      type 2 DSK shape model SpiceInt type data or model parameters to
      be returned.

      
         Name                  Description
         ----                  ----------

         SPICE_DSK02_KWNV      Number of vertices in model.

         SPICE_DSK02_KWNP      Number of plates in model.

         SPICE_DSK02_KWNVXT    Total number of voxels in fine grid.

         SPICE_DSK02_KWVGRX    Voxel grid extent.  This extent is
                               an array of three integers
                               indicating the number of voxels in
                               the X, Y, and Z directions in the
                               fine voxel grid.

         SPICE_DSK02_KWCGSC    Coarse voxel grid scale.  The extent
                               of the fine voxel grid is related to
                               the extent of the coarse voxel grid
                               by this scale factor.

         SPICE_DSK02_KWVXPS    Size of the voxel-to-plate pointer
                               list.

         SPICE_DSK02_KWVXLS    Voxel-plate correspondence list size.

         SPICE_DSK02_KWVTLS    Vertex-plate correspondence list
                               size.

         SPICE_DSK02_KWPLAT    Plate array.  For each plate, this
                               array contains the indices of the
                               plate's three vertices.  The ordering
                               of the array members is:

                                  Plate 1 vertex index 1
                                  Plate 1 vertex index 2
                                  Plate 1 vertex index 3
                                  Plate 2 vertex index 1
                                  ...

                               The vertex indices in this array start
                               at 1 and end at NV, the number of 
                               vertices in the model.

         SPICE_DSK02_KWVXPT    Voxel-plate pointer list. This list
                               contains pointers that map fine
                               voxels to lists of plates that
                               intersect those voxels. Note that
                               only fine voxels belonging to
                               non-empty coarse voxels are in the
                               domain of this mapping.

         SPICE_DSK02_KWVXPL    Voxel-plate correspondence list.
                               This list contains lists of plates
                               that intersect fine voxels. (This
                               list is the data structure into
                               which the voxel-to-plate pointers
                               point.)  This list can contain
                               empty lists.  Plate IDs in this
                               list start at 1 and end at NP,
                               the number of plates in the model.

         SPICE_DSK02_KWVTPT    Vertex-plate pointer list. This list
                               contains pointers that map vertices
                               to lists of plates to which those
                               vertices belong.

                               Note that the size of this list is
                               always NV, the number of vertices.
                               Hence there's no need for a separate
                               keyword for the size of this list.

         SPICE_DSK02_KWVTPL    Vertex-plate correspondence list.
                               This list contains, for each vertex,
                               the indices of the plates to which that
                               vertex belongs. Plate IDs in this list
                               start at 1 and end at NP, the number of
                               plates in the model.

         SPICE_DSK02_KWCGPT    Coarse voxel grid pointers.  This is
                               an array of pointers mapping coarse
                               voxels to lists of pointers in the
                               voxel-plate pointer list.  Each
                               non-empty coarse voxel maps to a
                               list of pointers; every fine voxel
                               contained in a non-empty coarse voxel
                               has its own pointers. Grid elements
                               corresponding to empty coarse voxels
                               contain non-positive values.
         

      Double precision keyword parameters
      -----------------------------------

      The following parameters may be passed to dskd02_c to identify
      type 2 DSK shape model SpiceDouble type data or model parameters to
      be returned.

        
         SPICE_DSK02_KWDSC     Array containing contents of Fortran
                               DSK descriptor of segment. Note
                               that DSK descriptors are not to be
                               confused with DLA descriptors, which
                               contain segment component base address
                               and size information.  The dimension of
                               this array is SPICE_DSK_DSCSIZ.

         SPICE_DSK02_KWVTBD    Vertex bounds. This is an array of
                               six values giving the minimum and
                               maximum values of each component of the
                               vertex set.

         SPICE_DSK02_KWVXOR    Voxel grid origin. This is the location
                               of the voxel grid origin in the
                               body-fixed frame associated with the
                               target body.
 
         SPICE_DSK02_KWVXSZ    Voxel size.  DSK voxels are cubes; the
                               edge length of each cube is given by the
                               voxel size.  This size applies to the
                               fine voxel grid. Units are km.
 
         SPICE_DSK02_KWVERT    Vertex coordinates.




   Type 4 definitions
   ==================
 
      To be added post-N0066.
 
       
   API-specific definitions
   ========================

      Parameters for dskxsi_c:

         SPICE_DSKXSI_DCSIZE      Size of `dc' output array.
         SPICE_DSKXSI_ICSIZE      Size of `ic' output array.

           These sizes may be increased in a future version
           of the CSPICE Toolkit.



-Literature_References

   None.

-Author_and_Institution

   N.J. Bachman       (JPL)
   
-Restrictions

   None.
      
-Version

   -CSPICE Version 1.0.0, 02-NOV-2016 (NJB)  

       Removed type 4 macros.

       21-MAR-2016 (NJB)  

       Changed type 2 keyword parameter names to use the
       substring DSK02 rather than DSK.

       26-FEB-2016 (NJB)  
 
          Added parameter declarations for type 2 spatial index. Added
          parameter declaration for data class 2. Renamed parameter
          SPICE_DSK_MAXCGR to SPICE_DSK02_MAXCGR. Added parameter
          declarations for the API dskxsi_c. Removed include statement
          for SpiceDLA.h. Made miscellaneous updates to comments.

       DSKLIB_C 02-MAY-2014 (NJB)  

          Added include guards for

             SpiceZdf.h
             SpiceDLA.h

          Removed reference to

             dsk_proto.h

          Last update was 13-NOV-2012 (NJB)  

             Updated to support DSK type 4. The SpiceDSKDescr type and
             supporting definitions have been added. The file has been
             reorganized so as to group together data type-specific
             definitions.

       DSKLIB_C Version 3.0.0, 13-MAY-2010 (NJB)  

          Updated for compatibility with new DSK type 2 segment
          design.

       DSKLIB_C Version 2.0.0, 12-FEB-2010 (NJB)  

          Updated to include

             SpiceDLA.h
             f2c_proto.h
             dsk_proto.h

       DSKLIB_C Version 1.0.0, 30-OCT-2006 (NJB)  

*/

#ifndef HAVE_SPICE_DSK_H

   #define HAVE_SPICE_DSK_H
      
   /*
   Prototypes
   */
   #ifndef HAVE_SPICEDEFS_H
      #include "SpiceZdf.h"
   #endif

   
   /*
   General Constants
   */

   
   /*
   Dimension parameters 
   */
 
   /*
   Size of a SPICELIB DSK descriptor (in units of d.p. numbers): 
   */
   #define SPICE_DSK_DSCSIZ                 24                

   /*
   Number of coordinate system parameters in DSK descriptor: 
   */
   #define  SPICE_DSK_NSYPAR                10


   /*
   Index parameters 
   */
   /*
   Fortran DSK descriptor index parameters: 
   */
   #define  SPICE_DSK_SRFIDX                0 
   #define  SPICE_DSK_CTRIDX                1 
   #define  SPICE_DSK_CLSIDX                2 
   #define  SPICE_DSK_TYPIDX                3 
   #define  SPICE_DSK_FRMIDX                4 
   #define  SPICE_DSK_SYSIDX                5 
   #define  SPICE_DSK_PARIDX                6 

   /*
   The offset between the indices immediately above and below
   is given by the parameter SPICE_DSK_NSYPAR. Literal values 
   are used below for convenience of the reader. 
   */
   #define  SPICE_DSK_MN1IDX                16
   #define  SPICE_DSK_MX1IDX                17
   #define  SPICE_DSK_MN2IDX                18
   #define  SPICE_DSK_MX2IDX                19
   #define  SPICE_DSK_MN3IDX                20
   #define  SPICE_DSK_MX3IDX                21
   #define  SPICE_DSK_BTMIDX                22
   #define  SPICE_DSK_ETMIDX                23

 


   /*
   Data class parameters 
   */

   /*
   Single-valued surface data class
   */
   #define SPICE_DSK_SVFCLS                 1

   /*
   General surface data class 
   */
   #define SPICE_DSK_GENCLS                 2


   /*
   Coordinate system parameters 
   */

   /*
   Latitudinal coordinate system
   */
   #define SPICE_DSK_LATSYS                 1

   /*
   Cylindrical coordinate system
   */
   #define SPICE_DSK_CYLSYS                 2

   /*
   Rectangular coordinate system
   */
   #define SPICE_DSK_RECSYS                 3

   /*
   Planetodetic coordinate system
   */
   #define SPICE_DSK_PDTSYS                 4


   /*
   Structures 
   */

   /*
   DSK segment descriptor:
   */   
   struct _SpiceDSKDescr 
   
      {  SpiceInt         surfce;
         SpiceInt         center;
         SpiceInt         dclass;
         SpiceInt         dtype; 
         SpiceInt         frmcde;
         SpiceInt         corsys;
         SpiceDouble      corpar [SPICE_DSK_NSYPAR];
         SpiceDouble      co1min;
         SpiceDouble      co1max;
         SpiceDouble      co2min;
         SpiceDouble      co2max;
         SpiceDouble      co3min;
         SpiceDouble      co3max;
         SpiceDouble      start;
         SpiceDouble      stop;        };
   
   typedef struct _SpiceDSKDescr  SpiceDSKDescr;
   
   /*
   Constant DSK segment descriptor:
   */   
   typedef const SpiceDSKDescr    ConstSpiceDSKDescr;




   /*

   Type 2 definitions 
   ==================

   */

   /*
   Dimension parameters 
   */

   /*
   Maximum vertex count for single segment: 
   */
   #define SPICE_DSK02_MAXVRT               16000002

   /*
   Maximum plate count for single segment: 
   */
   #define SPICE_DSK02_MAXPLT               ( 2 * (SPICE_DSK02_MAXVRT - 2 ) )

   /*
   The maximum allowed number of vertices, not taking into
   account shared vertices.

   Note that this value is not sufficient to create a vertex-plate
   mapping for a model of maximum plate count.
   */
   #define SPICE_DSK02_MAXNPV               ( 3 * (SPICE_DSK02_MAXPLT/2) + 1 )

   /*
   Maximum number of fine voxels. 
   */
   #define SPICE_DSK02_MAXVOX               100000000

   /*
   Maximum size of the coarse voxel grid array (in units of 
   integers):
   */
   #define SPICE_DSK02_MAXCGR               100000

   /*
   Maximum allowed number of vertex or plate 
   neighbors a vertex may have.  
   */
   #define SPICE_DSK02_MAXEDG               120


   /*
   DSK type 2 spatial index parameters
   ===================================

      DSK type 2 spatial index integer component
      ------------------------------------------

         +-----------------+
         | VGREXT          |  (voxel grid extents, 3 integers)
         +-----------------+
         | CGRSCL          |  (coarse voxel grid scale, 1 integer)
         +-----------------+
         | VOXNPT          |  (size of voxel-plate pointer list)
         +-----------------+
         | VOXNPL          |  (size of voxel-plate list)
         +-----------------+
         | VTXNPL          |  (size of vertex-plate list)
         +-----------------+
         | CGRPTR          |  (coarse grid occupancy pointers)
         +-----------------+
         | VOXPTR          |  (voxel-plate pointer array)
         +-----------------+
         | VOXPLT          |  (voxel-plate list)
         +-----------------+
         | VTXPTR          |  (vertex-plate pointer array)
         +-----------------+
         | VTXPLT          |  (vertex-plate list)
         +-----------------+       
   */

   /*
   Index parameters 
   */

   /*
   Grid extent index: 
   */
   #define SPICE_DSK02_SIVGRX               0

   /*
   Coarse grid scale index: 
   */
   #define SPICE_DSK02_SICGSC             ( SPICE_DSK02_SIVGRX + 3 )

   /*
   Voxel pointer count index:
   */
   #define SPICE_DSK02_SIVXNP             ( SPICE_DSK02_SICGSC + 1 )

   /*
   Voxel-plate list count index: 
   */ 
   #define SPICE_DSK02_SIVXNL             ( SPICE_DSK02_SIVXNP + 1 )

   /*
   Vertex-plate list count index: 
   */
   #define SPICE_DSK02_SIVTNL              ( SPICE_DSK02_SIVXNL + 1 )

   /*
   Coarse grid pointer array index:
   */
   #define SPICE_DSK02_SICGRD              ( SPICE_DSK02_SIVTNL + 1 )


   /*
   Spatial index integer component dimensions 
   */
 
   /*
   Size of fixed-size portion of integer component:
   */
   #define SPICE_DSK02_IXIFIX             ( SPICE_DSK02_MAXCGR + 7 )


   /*

   DSK type 2 spatial index double precision component
   ---------------------------------------------------

      +-----------------+
      | Vertex bounds   |  6 values (min/max for each component)
      +-----------------+
      | Voxel origin    |  3 elements
      +-----------------+
      | Voxel size      |  1 element
      +-----------------+

   */


   /*
   Spatial index double precision indices
   */

   /*
   Vertex bounds index: 
   */
   #define SPICE_DSK02_SIVTBD               0

   /*
   Voxel grid origin index: 
   */
   #define SPICE_DSK02_SIVXOR             ( SPICE_DSK02_SIVTBD + 6 )

   /*
   Voxel size index: 
   */
   #define SPICE_DSK02_SIVXSZ             ( SPICE_DSK02_SIVXOR + 3 )


   /*
   Spatial index double precision component dimensions 
   */
 
   /*
   Size of fixed-size portion of double precision component:
   */
   #define SPICE_DSK02_IXDFIX               10

   /*
   Size of double precision component. This is a convenience
   parameter chosen to have a name consisent with the 
   integer spatial index size. 
   */
   #define SPICE_DSK02_SPADSZ               SPICE_DSK02_IXDFIX

   /*
   The limits below are used to define a suggested maximum
   size for the integer component of the spatial index. 
   */

   /*
   Maximum number of entries in voxel-plate pointer array:
   */
   #define SPICE_DSK02_MAXVXP             ( SPICE_DSK02_MAXPLT /2 )
  
   /*
   Maximum cell size: 
   */
   #define SPICE_DSK02_MAXCEL               60000000

   /*
   Maximum number of entries in voxel-plate list:
   */
   #define SPICE_DSK02_MXNVLS               SPICE_DSK02_MAXCEL +    \
                                          ( SPICE_DSK02_MAXVXP / 2 )

   /*
   Spatial index integer component size: 
   */
   #define SPICE_DSK02_SPAISZ             ( SPICE_DSK02_IXIFIX +   \
                                            SPICE_DSK02_MAXVXP +   \
                                            SPICE_DSK02_MXNVLS +   \
                                            SPICE_DSK02_MAXVRT +   \
                                            SPICE_DSK02_MAXNPV       )




   /*
   Keyword parameters for SpiceInt data items:
   */

   /*
   Index parameters 
   */
   #define  SPICE_DSK02_KWNV                1
   #define  SPICE_DSK02_KWNP               (SPICE_DSK02_KWNV   + 1)
   #define  SPICE_DSK02_KWNVXT             (SPICE_DSK02_KWNP   + 1)
   #define  SPICE_DSK02_KWVGRX             (SPICE_DSK02_KWNVXT + 1)
   #define  SPICE_DSK02_KWCGSC             (SPICE_DSK02_KWVGRX + 1)
   #define  SPICE_DSK02_KWVXPS             (SPICE_DSK02_KWCGSC + 1)
   #define  SPICE_DSK02_KWVXLS             (SPICE_DSK02_KWVXPS + 1) 
   #define  SPICE_DSK02_KWVTLS             (SPICE_DSK02_KWVXLS + 1)
   #define  SPICE_DSK02_KWPLAT             (SPICE_DSK02_KWVTLS + 1)
   #define  SPICE_DSK02_KWVXPT             (SPICE_DSK02_KWPLAT + 1)
   #define  SPICE_DSK02_KWVXPL             (SPICE_DSK02_KWVXPT + 1)
   #define  SPICE_DSK02_KWVTPT             (SPICE_DSK02_KWVXPL + 1)
   #define  SPICE_DSK02_KWVTPL             (SPICE_DSK02_KWVTPT + 1)
   #define  SPICE_DSK02_KWCGPT             (SPICE_DSK02_KWVTPL + 1)


   /*
   Keyword parameters for SpiceDouble data items:
   */
   #define  SPICE_DSK02_KWDSC              (SPICE_DSK02_KWCGPT + 1)
   #define  SPICE_DSK02_KWVTBD             (SPICE_DSK02_KWDSC  + 1)
   #define  SPICE_DSK02_KWVXOR             (SPICE_DSK02_KWVTBD + 1)
   #define  SPICE_DSK02_KWVXSZ             (SPICE_DSK02_KWVXOR + 1)
   #define  SPICE_DSK02_KWVERT             (SPICE_DSK02_KWVXSZ + 1)
 

   /*

   Type 4 definitions 
   ==================

   These definitions should be treated as "SPICE private." They
   may change in a future version of the SPICE Toolkit. They
   should not be referenced by user applications. 

   To be added post-N0066.

   */

 

   /*
   API-specific definitions
   ========================

   Parameters for dskxsi_c:
   */
   #define  SPICE_DSKXSI_DCSIZE             1
   #define  SPICE_DSKXSI_ICSIZE             1


#endif

