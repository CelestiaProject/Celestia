/*

-Header_File SpiceDLA.h ( CSPICE DLA-specific definitions )

-Abstract

   Perform CSPICE DLA-specific definitions, including macros and user-
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
   CSPICE DLA functions.
   

   Macros
   ======
   
      Dimensions
      ----------
      
         Name                  Description
         ----                  -----------
         SPICE_DLA_DSCSIZ      Size of a SPICELIB DLA descriptor,
                               measured in multiples of the size of a
                               SpiceInt. A DLA descriptor `DLADescr'
                               can be declared

                                  SpiceInt DLADescr [SPICE_DLA_DSCSIZ];
 
                               This parameter is provided for
                               compatibility with f2c'd SPICELIB
                               interfaces; CSPICE wrappers should pass
                               DLA descriptors via pointers to the
                               SpiceDLADescr structure defined below.


      Fortran Structure Offsets
      -------------------------

         These parameters are provided to support CSPICE wrapper
         implementation.


         Name                  Description
         ----                  ----------
         SPICE_DLA_BWDIDX      Backward pointer index in a Fortran DLA
                               descriptor.
 
         SPICE_DLA_FWDIDX      Forward pointer index in a Fortran DLA
                               descriptor.
 
         SPICE_DLA_IBSIDX      Integer base address index in a Fortran
                               DLA descriptor.
 
         SPICE_DLA_ISZIDX      Integer component size index in a
                               Fortran DLA descriptor.
 
         SPICE_DLA_DBSIDX      D.p. base address index in a Fortran DLA
                               descriptor.
 
         SPICE_DLA_DSZIDX      D.p. component size index in a Fortran
                               DLA descriptor.
 
         SPICE_DLA_CBSIDX      Character base address index in a
                               Fortran DLA descriptor.
 
         SPICE_DLA_CSZIDX      Character component size index in a
                               Fortran DLA descriptor.



   Structures
   ==========
   
      DLA API structures
      ------------------

         Name                  Description
         ----                  -----------
   
         SpiceDLADescr         DLA descriptor.
         
                               Note:  the "base addresses" described
                               below are the *predecessors* of the
                               first addresses occupied by the
                               respective components of each data type.

                               The structure members are:
 
                                  bwdptr:     backward pointer.  Data
                                              type is SpiceInt.
 
                                  fwdptr:     forward pointer.  Data
                                              type is SpiceInt.
 
                                  ibase:      base DAS address of the
                                              integer component of a
                                              DLA segment.
 
                                  isize:      number of elements in the
                                              integer component of a
                                              DLA segment.
 
                                  dbase:      base DAS address of
                                              double precision
                                              component of a DLA
                                              segment.
 
                                  dsize:      number of elements in the
                                              double precision
                                              component of a DLA
                                              segment.
 
                                  cbase:      base DAS address of
                                              character component of a
                                              DLA segment.
 
                                  csize:      number of elements in the
                                              character component of a
                                              DLA segment.
                   


         ConstSpiceDLADescr   A constant DLA descriptor.


-Literature_References

   None.

-Author_and_Institution

   N.J. Bachman       (JPL)
   
-Restrictions

   None.
      
-Version

   -CSPICE Version 1.1.0, 21-JAN-2016 (NJB)  

       Updated to remove 

          f2c_proto.h
          dla_proto.h

       The prototypes declared in these headers have been moved
       to the files

          SpiceZfc.h
          SpiceZpr.h

   -DSKLIB_C Version 1.0.1, 12-FEB-2010 (NJB)  

       Updated to include 

          f2c_proto.h
          dla_proto.h


   -DSKLIB_C Version 1.0.1, 09-FEB-2007 (NJB)  

       Comments were corrected:  names of structure members bwdptr and 
       fwdptr in the comments now match the names used in the structure
       definition.

   -DSKLIB_C Version 1.0.0, 26-OCT-2006 (NJB)  

*/

#ifndef HAVE_SPICE_DLA_H

   #define HAVE_SPICE_DLA_H
   

   
   /*
   Constants
   */


   /*
   Fortran DLA descriptor dimension:
   */
   #define  SPICE_DLA_DSCSIZ                8

   /*
   Fortran DLA descriptor index parameters: 
   */
   #define  SPICE_DLA_BWDIDX                0 
   #define  SPICE_DLA_FWDIDX                1 
   #define  SPICE_DLA_IBSIDX                2 
   #define  SPICE_DLA_ISZIDX                3 
   #define  SPICE_DLA_DBSIDX                4 
   #define  SPICE_DLA_DSZIDX                5 
   #define  SPICE_DLA_CBSIDX                6 
   #define  SPICE_DLA_CSZIDX                7



   /*
   Structures
   */
 
   /*
   DLA segment descriptor:
   */   
   struct _SpiceDLADescr 
   
      {  SpiceInt         bwdptr;
         SpiceInt         fwdptr;
         SpiceInt         ibase;
         SpiceInt         isize; 
         SpiceInt         dbase;
         SpiceInt         dsize;
         SpiceInt         cbase;
         SpiceInt         csize;   };
   
   typedef struct _SpiceDLADescr  SpiceDLADescr;
   
   /*
   Constant DLA segment descriptor:
   */   
   typedef const SpiceDLADescr    ConstSpiceDLADescr;


#endif

