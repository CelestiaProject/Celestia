/*

-Header_File SpiceZim.h ( CSPICE interface macros )

-Abstract

   Define interface macros to be called in place of CSPICE
   user-interface-level functions.  These macros are generally used
   to compensate for compiler deficiencies.

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

-Literature_References

   None.

-Particulars

   This header file defines interface macros to be called in place of
   CSPICE user-interface-level functions.  Currently, the sole purpose
   of these macros is to implement automatic type casting under some
   environments that generate compile-time warnings without the casts.
   The typical case that causes a problem is a function argument list
   containing an input formal argument of type

      const double [3][3]

   Under some compilers, a non-const actual argument supplied in a call
   to such a function will generate a spurious warning due to the
   "mismatched" type.  These macros generate type casts that will
   make such compilers happy.

   Examples of compilers that generate warnings of this type are

      gcc version 2.2.2, hosted on NeXT workstations running
      NeXTStep 3.3

      Sun C compiler, version 4.2, running under Solaris.

-Author_and_Institution

   N.J. Bachman       (JPL)
   E.D. Wright        (JPL)

-Version

   -CSPICE Version 13.0.0, 25-JAN-2017 (NJB) (EDW) 

       Defined new abbreviation macro CONST_IVEC3.
       Added macros for

          dskgd_c 
          dskmi2_c
          dskb02_c
          dskd02_c
          dski02_c
          dskn02_c
          dskobj_c
          dskp02_c
          dskrb2_c
          dsksrf_c
          dskv02_c
          dskxv_c
          dskxsi_c
          dskw02_c
          latsrf_c
          limbpt_c
          oscltx_c
          pltar_c
          pltexp_c
          pltnp_c
          pltnrm_c
          pltvol_c
          srfnrm_c
          termpt_c

   -CSPICE Version 12.0.0, 03-DEC-2013 (NJB) (EDW) (SCK)

       Added include for SpiceZrnm.h to eliminate symbol conflict
       encountered from Icy and JNISpice under OS X 10.7.

       Added macros for

          eqncpv_c
          fovray_c
          spkcpo_c
          spkcpt_c
          spkcvo_c
          spkcvt_c
          spkpnv_c
          spkw20_c

   -CSPICE Version 11.0.0, 09-MAR-2009 (NJB) (EDW)

       Added macros for

          dvsep_c
          gfevnt_c
          gffove_c
          gfrfov_c
          gfsntc_c
          surfpv_c


   -CSPICE Version 10.0.0, 19-FEB-2008 (NJB) (EDW)

       Added macros for

          ilumin_c
          spkaps_c
          spkltc_c

   -CSPICE Version 9.0.0, 31-OCT-2005 (NJB)

       Added macros for

          qdq2av_c
          qxq_c

   -CSPICE Version 8.0.0, 23-FEB-2004 (NJB)

       Added macro for

          dafrs_c


   -CSPICE Version 7.0.0, 23-FEB-2004 (EDW)

       Added macro for

          srfxpt_c

   -CSPICE Version 6.0.1, 25-FEB-2003 (EDW) (NJB)

       Remove duplicate macro definitions for ekaced_c and
       ekacei_c. Visual Studio errored out when compiling
       code that included SpiceZim.h.

       Added macro for

          dasac_c

   -CSPICE Version 6.0.0, 17-AUG-2002 (NJB)

       Added macros for

          bschoc_c
          bschoi_c
          bsrchc_c
          bsrchd_c
          bsrchi_c
          esrchc_c
          isordv_c
          isrchc_c
          isrchd_c
          isrchi_c
          lstltc_c
          lstltd_c
          lstlti_c
          lstlec_c
          lstled_c
          lstlei_c
          orderc_c
          orderd_c
          orderi_c
          reordc_c
          reordd_c
          reordi_c
          reordl_c
          spkw18_c

   -CSPICE Version 5.0.0, 28-AUG-2001 (NJB)

       Added macros for

          conics_c
          illum_c
          invort_c
          pdpool_c
          prop2b_c
          q2m_c
          spkuds_c
          xposeg_c

   -CSPICE Version 4.0.0, 22-MAR-2000 (NJB)

       Added macros for

          spkw12_c
          spkw13_c

   -CSPICE Version 3.0.0, 27-AUG-1999 (NJB) (EDW)

       Fixed cut & paste error in macro nvp2pl_c.

       Added macros for

          axisar_c
          cgv2el_c
          dafps_c
          dafus_c
          diags2_c
          dvdot_c
          dvhat_c
          edlimb_c
          ekacli_c
          ekacld_c
          ekacli_c
          eul2xf_c
          el2cgv_c
          getelm_c
          inedpl_c
          isrot_c
          mequ_c
          npedln_c
          nplnpt_c
          rav2xf_c
          raxisa_c
          saelgv_c
          spk14a_c
          spkapo_c
          spkapp_c
          spkw02_c
          spkw03_c
          spkw05_c
          spkw08_c
          spkw09_c
          spkw10_c
          spkw15_c
          spkw17_c
          sumai_c
          trace_c
          vadd_g
          vhatg_c
          vlcomg_c
          vminug_c
          vrel_c
          vrelg_c
          vsepg_c
          vtmv_c
          vtmvg_c
          vupack_c
          vzerog_c
          xf2eul_c
          xf2rav_c

   -CSPICE Version 2.0.0, 07-MAR-1999 (NJB)

       Added macros for

          inrypl_c
          nvc2pl_c
          nvp2pl_c
          pl2nvc_c
          pl2nvp_c
          pl2psv_c
          psv2pl_c
          vprjp_c
          vprjpi_c

   -CSPICE Version 1.0.0, 24-JAN-1999 (NJB) (EDW)


-Index_Entries

   interface macros for CSPICE functions

*/


/*
Include Files:
*/


/*
Include the type definitions prior to defining the interface macros.
The macros reference the types.
*/
#ifndef  HAVE_SPICEDEFS_H
#include "SpiceZdf.h"
#endif


/*
Include those rename assignments for routines whose symbols will
collide with other libraries.
*/
#ifndef   HAVE_SPICERENAME_H
#include "SpiceZrnm.h" 
#endif


#ifndef HAVE_SPICEIFMACROS_H
#define HAVE_SPICEIFMACROS_H


/*
Macros used to abbreviate type casts:
*/

   #define  CONST_BOOL         ( ConstSpiceBoolean   *      )
   #define  CONST_DLADSC       ( ConstSpiceDLADescr  *      )
   #define  CONST_DSKDSC       ( ConstSpiceDSKDescr  *      )
   #define  CONST_ELLIPSE      ( ConstSpiceEllipse   *      )
   #define  CONST_IVEC         ( ConstSpiceInt       *      )
   #define  CONST_IVEC3        ( ConstSpiceInt      (*) [3] )
   #define  CONST_MAT          ( ConstSpiceDouble   (*) [3] )
   #define  CONST_MAT2         ( ConstSpiceDouble   (*) [2] )
   #define  CONST_MAT6         ( ConstSpiceDouble   (*) [6] )
   #define  CONST_PLANE        ( ConstSpicePlane     *      )
   #define  CONST_VEC3         ( ConstSpiceDouble   (*) [3] )
   #define  CONST_VEC4         ( ConstSpiceDouble   (*) [4] )
   #define  CONST_STR          ( ConstSpiceChar      *      )
   #define  CONST_VEC          ( ConstSpiceDouble    *      )
   #define  CONST_VOID         ( const void          *      )

/*
Macros that substitute for function calls:
*/

   #define  axisar_c( axis, angle, r )                                 \
                                                                       \
        (   axisar_c( CONST_VEC(axis), (angle), (r) )   )


   #define  bschoc_c( value, ndim, lenvals, array, order )             \
                                                                       \
        (   bschoc_c ( CONST_STR(value),  (ndim),          (lenvals),  \
                       CONST_VOID(array), CONST_IVEC(order)          ) )


   #define  bschoi_c( value, ndim, array, order )                      \
                                                                       \
        (   bschoi_c ( (value)         ,  (ndim),                      \
                       CONST_IVEC(array), CONST_IVEC(order) )  )


   #define  bsrchc_c( value, ndim, lenvals, array )                    \
                                                                       \
        (   bsrchc_c ( CONST_STR(value),  (ndim),  (lenvals),          \
                       CONST_VOID(array)                      ) )


   #define  bsrchd_c( value, ndim, array )                             \
                                                                       \
        (   bsrchd_c( (value),  (ndim),  CONST_VEC(array) )  )


   #define  bsrchi_c( value, ndim, array )                             \
                                                                       \
        (   bsrchi_c( (value),  (ndim),  CONST_IVEC(array) )  )


   #define  ckw01_c( handle, begtim, endtim, inst,  ref, avflag,       \
                     segid,  nrec,   sclkdp, quats, avvs        )      \
                                                                       \
        (   ckw01_c ( (handle),          (begtim),        (endtim),    \
                      (inst),            CONST_STR(ref),  (avflag),    \
                      CONST_STR(segid),  (nrec),                       \
                      CONST_VEC(sclkdp), CONST_VEC4(quats),            \
                      CONST_VEC3(avvs)                            )  )


   #define  ckw02_c( handle, begtim, endtim, inst,  ref,   segid,      \
                     nrec,   start,  stop,  quats,  avvs,  rates )     \
                                                                       \
        (   ckw02_c ( (handle),          (begtim),        (endtim),    \
                      (inst),            CONST_STR(ref),               \
                      CONST_STR(segid),  (nrec),                       \
                      CONST_VEC(start),  CONST_VEC(stop),              \
                      CONST_VEC4(quats), CONST_VEC3(avvs),             \
                      CONST_VEC(rates)                             )  )


   #define  ckw03_c( handle, begtim, endtim, inst,  ref,  avflag,      \
                     segid,  nrec,   sclkdp, quats, avvs, nints,       \
                     starts                                       )    \
                                                                       \
        (   ckw03_c ( (handle),          (begtim),        (endtim),    \
                      (inst),            CONST_STR(ref),  (avflag),    \
                      CONST_STR(segid),  (nrec),                       \
                      CONST_VEC(sclkdp), CONST_VEC4(quats),            \
                      CONST_VEC3(avvs),  (nints),                      \
                      CONST_VEC(starts)                            )  )


   #define  ckw05_c( handle, subtyp, degree, begtim, endtim, inst,     \
                     ref,    avflag, segid,  n,      sclkdp, packts,   \
                     rate,    nints, starts                          ) \
                                                                       \
        (   ckw05_c ( (handle),          (subtyp),        (degree),    \
                      (begtim),          (endtim),                     \
                      (inst),            CONST_STR(ref),  (avflag),    \
                      CONST_STR(segid),  (n),                          \
                      CONST_VEC(sclkdp), CONST_VOID(packts),           \
                      (rate),            (nints),                      \
                      CONST_VEC(starts)                            )  )


   #define  cgv2el_c( center, vec1, vec2, ellipse )                    \
                                                                       \
        (   cgv2el_c( CONST_VEC(center), CONST_VEC(vec1),              \
                      CONST_VEC(vec2),   (ellipse)        )   )


   #define  conics_c( elts, et, state )                                \
                                                                       \
        (   conics_c( CONST_VEC(elts), (et), (state) )  )


   #define  dafps_c( nd, ni, dc, ic, sum )                             \
                                                                       \
        (   dafps_c ( (nd), (ni), CONST_VEC(dc), CONST_IVEC(ic),       \
                      (sum)                                     )   )


   #define  dafrs_c( sum )                                             \
                                                                       \
        (   dafrs_c ( CONST_VEC( sum )  )   )


   #define  dafus_c( sum, nd, ni, dc, ic )                             \
                                                                       \
        (   dafus_c ( CONST_VEC(sum), (nd), (ni), (dc), (ic) )   )


   #define  dasac_c( handle, n, buflen, buffer )                       \
                                                                       \
        (   dasac_c ( (handle), (n), (buflen), CONST_VOID(buffer) )   )


   #define  det_c( m1 )                                                \
                                                                       \
        (   det_c ( CONST_MAT(m1) )   )


   #define  diags2_c( symmat, diag, rotate )                           \
                                                                       \
        (   diags2_c ( CONST_MAT2(symmat), (diag), (rotate) )   )


   #define  dskb02_c( handle, dladsc, nv,     np,     nvxtot,         \
                      vtxbds, voxsiz, voxori, vgrext, cgscal,         \
                      vtxnpl, voxnpt, voxnpl                  )       \
                                                                      \
        (   dskb02_c( (handle), CONST_DLADSC(dladsc), (nv), (np),     \
                      (nvxtot), (vtxbds), (voxsiz), (voxori),         \
                      (vgrext), (cgscal), (vtxnpl), (voxnpt),         \
                      (voxnpl)    )   )


   #define  dskd02_c( handle, dladsc, item, start, room, n, values )  \
                                                                      \
        (   dskd02_c ( (handle), CONST_DLADSC(dladsc), (item),        \
                       (start), (room), (n), (values)         )  )


   #define  dski02_c( handle, dladsc, item, start, room, n, values )  \
                                                                      \
        (   dski02_c ( (handle), CONST_DLADSC(dladsc), (item),        \
                       (start), (room), (n), (values)         )  )


   #define  dskgd_c( handle, dladsc, dskdsc )                         \
                                                                      \
        (   dskgd_c ( (handle), CONST_DLADSC(dladsc), (dskdsc) )  )


   #define  dskmi2_c( nv,     vrtces, np,     plates,                 \
                      finscl, corscl, worksz, voxpsz, voxlsz,         \
                      makvtl, spxisz, work,   spaixd, spaixi  )       \
                                                                      \
        (   dskmi2_c ( (nv),                CONST_VEC3(vrtces), (np), \
                       CONST_IVEC3(plates), (finscl),                 \
                       (corscl),            (worksz),  (voxpsz),      \
                       (voxlsz),            (makvtl),  (spxisz),      \
                       (work),              (spaixd),  (spaixi)  )  )


   #define  dskn02_c( handle, dladsc, plid, normal )                  \
                                                                      \
        (   dskn02_c ( (handle), CONST_DLADSC(dladsc), (plid),        \
                       (normal)                                )  )


   #define  dskobj_c( dsk, bodids )                                   \
                                                                      \
        (   dskobj_c ( CONST_STR(dsk), (bodids) )   )


   #define  dskp02_c( handle, dladsc, start, room, n, plates )        \
                                                                      \
        (   dskp02_c ( (handle), CONST_DLADSC(dladsc), (start),       \
                       (room), (n),  (plates)                   )  )


   #define  dskrb2_c( nv,     vrtces, np,     plates,                 \
                      corsys, corpar, mncor3, mxcor3   )              \
                                                                      \
        (   dskrb2_c ( (nv),                CONST_VEC3(vrtces), (np), \
                       CONST_IVEC3(plates), (corsys),                 \
                       (corpar),            (mncor3),  (mxcor3) )  )


   #define  dsksrf_c( dsk,    bodyid, srfids )                        \
                                                                      \
        (   dsksrf_c ( CONST_STR(dsk), (bodyid), (srfids) )   )


   #define  dskv02_c( handle, dladsc, start, room, n, vrtces )        \
                                                                      \
        (   dskv02_c ( (handle), CONST_DLADSC(dladsc), (start),       \
                       (room), (n),  (vrtces)                   )  )


   #define  dskw02_c( handle, center, surfce, dclass,                 \
                      frame,  corsys, corpar, mncor1,                 \
                      mxcor1, mncor2, mxcor2, mncor3,                 \
                      mxcor3, first,  last,   nv,                     \
                      vrtces, np,     plates, spaixd, spaixi )        \
                                                                      \
        (   dskw02_c ( (handle), (center), (surfce), (dclass),        \
                       CONST_STR(frame),   (corsys),                  \
                       CONST_VEC(corpar),  (mncor1), (mxcor1),        \
                       (mncor2),           (mxcor2), (mncor3),        \
                       (mxcor3),           (first),  (last),          \
                       (nv),               CONST_VEC3(vrtces),        \
                       (np),               CONST_IVEC3(plates),       \
                       (spaixd),           (spaixi)             )   )


   #define  dskxsi_c( pri,    target, nsurf,  srflst, et,             \
                      fixref, vertex, raydir, maxd,   maxi,           \
                      xpt,    handle, dladsc, dskdsc, dc,             \
                      ic,     found                         )         \
                                                                      \
        (   dskxsi_c( (pri),              CONST_STR(target), (nsurf), \
                      CONST_IVEC(srflst), (et),                       \
                      CONST_STR(fixref),  CONST_VEC(vertex),          \
                      CONST_VEC(raydir),  (maxd),            (maxi),  \
                      (xpt),              (handle),          (dladsc),\
                      (dskdsc),           (dc),              (ic),    \
                      (found)                                     )   )


   #define   dskxv_c( pri,   target, nsurf,  srflst, et,    fixref,   \
                      nrays, vtxarr, dirarr, xptarr, fndarr        )  \
                                                                      \
           ( dskxv_c( (pri),              CONST_STR(target), (nsurf), \
                      CONST_IVEC(srflst), (et),                       \
                      CONST_STR(fixref),  (nrays),                    \
                      CONST_VEC3(vtxarr), CONST_VEC3(dirarr),         \
                      (xptarr),           (fndarr)            )   )


   #define  dvdot_c( s1, s2 )                                         \
                                                                      \
           ( dvdot_c ( CONST_VEC(s1), CONST_VEC(s2) )   )


   #define  dvhat_c( v1, v2 )                                         \
                                                                      \
           ( dvhat_c ( CONST_VEC(v1), (v2) )   )


   #define  dvsep_c( s1, s2 )                                         \
                                                                      \
           ( dvsep_c ( CONST_VEC(s1), CONST_VEC(s2) )   )


   #define  edlimb_c( a, b, c, viewpt, limb )                          \
                                                                       \
        (   edlimb_c( (a), (b), (c), CONST_VEC(viewpt), (limb) )   )


   #define  ekacec_c( handle, segno,  recno, column, nvals, vallen,    \
                      cvals,  isnull                               )   \
                                                                       \
        (   ekacec_c( (handle), (segno), (recno), CONST_STR(column),   \
                      (nvals),  (vallen), CONST_VOID(cvals),           \
                      (isnull)                                      )  )


   #define  ekaced_c( handle, segno,  recno, column, nvals,            \
                      dvals,  isnull                               )   \
                                                                       \
        (   ekaced_c( (handle), (segno), (recno), CONST_STR(column),   \
                      (nvals),  CONST_VEC(dvals), (isnull)          )  )


   #define  ekacei_c( handle, segno,  recno, column, nvals,            \
                      ivals,  isnull                               )   \
                                                                       \
        (   ekacei_c( (handle), (segno), (recno), CONST_STR(column),   \
                      (nvals),  CONST_IVEC(ivals), (isnull)         )  )


   #define  ekaclc_c( handle, segno,  column, vallen, cvals, entszs,   \
                      nlflgs, rcptrs, wkindx                         ) \
                                                                       \
        (   ekaclc_c( (handle), (segno),  (column),  (vallen),         \
                      CONST_VOID(cvals),  CONST_IVEC(entszs),          \
                      CONST_BOOL(nlflgs), CONST_IVEC(rcptrs),          \
                      (wkindx)                                      )  )


   #define  ekacld_c( handle, segno,  column, dvals, entszs, nlflgs,   \
                      rcptrs, wkindx                                 ) \
                                                                       \
        (   ekacld_c( (handle),           (segno),           (column), \
                      CONST_VEC(dvals),   CONST_IVEC(entszs),          \
                      CONST_BOOL(nlflgs), CONST_IVEC(rcptrs),          \
                      (wkindx)                                      )  )


   #define  ekacli_c( handle, segno,  column, ivals, entszs, nlflgs,   \
                      rcptrs, wkindx                                 ) \
                                                                       \
        (   ekacli_c( (handle),           (segno),           (column), \
                      CONST_IVEC(ivals),  CONST_IVEC(entszs),          \
                      CONST_BOOL(nlflgs), CONST_IVEC(rcptrs),          \
                      (wkindx)                                      )  )


   #define  ekbseg_c( handle, tabnam, ncols, cnmlen, cnames, declen,   \
                      decls,  segno                                 )  \
                                                                       \
        (   ekbseg_c( (handle), (tabnam), (ncols), (cnmlen),           \
                      CONST_VOID(cnames), (declen),                    \
                      CONST_VOID(decls),  (segno)             )  )


   #define  ekifld_c( handle, tabnam, ncols, nrows, cnmlen, cnames,    \
                      declen, decls,  segno, rcptrs                 )  \
                                                                       \
        (   ekifld_c( (handle), (tabnam), (ncols), (nrows), (cnmlen),  \
                      CONST_VOID(cnames), (declen),                    \
                      CONST_VOID(decls),  (segno), (rcptrs)         )  )


   #define  ekucec_c( handle, segno,  recno, column, nvals, vallen,    \
                      cvals,  isnull                               )   \
                                                                       \
        (   ekucec_c( (handle), (segno), (recno), CONST_STR(column),   \
                      (nvals),  (vallen), CONST_VOID(cvals),           \
                      (isnull)                                      )  )

   #define  ekuced_c( handle, segno,  recno, column, nvals,            \
                      dvals,  isnull                               )   \
                                                                       \
        (   ekuced_c( (handle), (segno), (recno),   CONST_STR(column), \
                      (nvals),  CONST_VOID(dvals), (isnull)         )  )


   #define  ekucei_c( handle, segno,  recno, column, nvals,            \
                      ivals,  isnull                               )   \
                                                                       \
        (   ekucei_c( (handle), (segno), (recno),   CONST_STR(column), \
                      (nvals),  CONST_VOID(ivals), (isnull)         )  )


   #define  el2cgv_c( ellipse, center, smajor, sminor )                \
                                                                       \
        (   el2cgv_c( CONST_ELLIPSE(ellipse), (center),                \
                      (smajor),               (sminor)  )   )


   #define  eqncpv_c( et, epoch, eqel, rapol, decpol, state )          \
                                                                       \
        (   eqncpv_c ( (et), (epoch), CONST_VEC(eqel), (rapol),        \
                      (decpol), (state) )  )


   #define  esrchc_c( value, ndim, lenvals, array )                    \
                                                                       \
        (   esrchc_c ( CONST_STR(value),  (ndim),  (lenvals),          \
                       CONST_VOID(array)                      ) )


   #define  eul2xf_c( eulang, axisa, axisb, axisc, xform )             \
                                                                       \
        (   eul2xf_c ( CONST_VEC(eulang), (axisa), (axisb), (axisc),   \
                       (xform)                                     )  )

   #define  fovray_c( inst,   raydir, rframe, abcorr, observer,        \
                      et,     visible       )                          \
                                                                       \
        (   fovray_c( (inst),    CONST_VEC(raydir), (rframe),          \
                      (abcorr), (observer), (et), (visible)   )   )

   #define  getelm_c( frstyr, lineln, lines, epoch, elems )            \
                                                                       \
        (   getelm_c ( (frstyr), (lineln), CONST_VOID(lines),          \
                       (epoch),  (elems)                      )   )


   #define  gfevnt_c( udstep, udrefn, gquant, qnpars, lenvals,         \
                      qpnams, qcpars, qdpars, qipars, qlpars,          \
                      op,     refval, tol,    adjust, rpt,             \
                      udrepi, udrepu, udrepf, nintvls,                 \
                      bail,   udbail, cnfine, result         )         \
                                                                       \
         ( gfevnt_c( (udstep),           (udrefn),  (gquant),          \
                     (qnpars),           (lenvals), CONST_VOID(qpnams),\
                     CONST_VOID(qcpars), (qdpars),  (qipars),          \
                     (qlpars),           (op),      (refval),          \
                     (tol),              (adjust),  (rpt),             \
                     (udrepi),           (udrepu),  (udrepf),          \
                     (nintvls),          (bail),                       \
                     (udbail),           (cnfine),  (result)     )   )


   #define  gffove_c( inst,   tshape, raydir, target, tframe,          \
                      abcorr, obsrvr, tol,    udstep, udrefn,          \
                      rpt,    udrepi, udrepu, udrepf, bail,            \
                      udbail, cnfine, result                 )         \
                                                                       \
        (   gffove_c( (inst),   (tshape), CONST_VEC(raydir),           \
                      (target), (tframe), (abcorr),                    \
                      (obsrvr), (tol),    (udstep),                    \
                      (udrefn), (rpt),    (udrepi),                    \
                      (udrepu), (udrepf), (bail),                      \
                      (udbail), (cnfine), (result) )   )


   #define  gfrfov_c( inst,   raydir, rframe, abcorr, obsrvr,          \
                      step,   cnfine, result                 )         \
                                                                       \
        (   gfrfov_c( (inst),    CONST_VEC(raydir), (rframe),          \
                      (abcorr), (obsrvr),           (step),            \
                      (cnfine), (result)                      )   )


   #define  gfsntc_c( target, fixref, method, abcorr,  obsrvr,         \
                      dref,   dvec,   crdsys, coord,   relate,         \
                      refval, adjust, step,   nintvls, cnfine,         \
                      result                                    )      \
                                                                       \
        (   gfsntc_c( (target),        (fixref),  (method),            \
                      (abcorr),        (obsrvr),  (dref),              \
                      CONST_VEC(dvec), (crdsys),  (coord),             \
                      (relate),        (refval),  (adjust),            \
                      (step),          (nintvls), (cnfine), (result) )  )


   #define  illum_c( target, et,    abcorr, obsrvr,                    \
                     spoint, phase, solar,  emissn )                   \
                                                                       \
        (   illum_c ( (target),          (et),    (abcorr), (obsrvr),  \
                      CONST_VEC(spoint), (phase), (solar),  (emissn) )  )


   #define  ilumin_c( method, target, et,     fixref,                  \
                      abcorr, obsrvr, spoint, trgepc,                  \
                      srfvec, phase, solar,   emissn   )               \
                                                                       \
       (   ilumin_c ( (method), (target), (et),    (fixref),           \
                      (abcorr), (obsrvr), CONST_VEC(spoint), (trgepc), \
                      (srfvec), (phase),  (solar), (emissn)          )  )


   #define  inedpl_c( a, b, c, plane, ellipse, found )                 \
                                                                       \
        (   inedpl_c ( (a),                (b),         (c),           \
                       CONST_PLANE(plane), (ellipse),   (found) )   )


   #define  inrypl_c( vertex, dir, plane, nxpts, xpt )                 \
                                                                       \
        (   inrypl_c ( CONST_VEC(vertex),   CONST_VEC(dir),            \
                       CONST_PLANE(plane),  (nxpts),        (xpt) )   )


   #define  invert_c( m1, m2 )                                         \
                                                                       \
        (   invert_c ( CONST_MAT(m1), (m2) )   )


   #define  invort_c( m, mit )                                         \
                                                                       \
        (   invort_c ( CONST_MAT(m), (mit) )   )


   #define  isordv_c( array, n )                                       \
                                                                       \
        (   isordv_c ( CONST_IVEC(array), (n) )  )


   #define  isrchc_c( value, ndim, lenvals, array )                    \
                                                                       \
        (   isrchc_c ( CONST_STR(value),  (ndim),  (lenvals),          \
                       CONST_VOID(array)                      ) )

   #define  isrchd_c( value, ndim, array )                             \
                                                                       \
        (   isrchd_c( (value),  (ndim),  CONST_VEC(array) )  )


   #define  isrchi_c( value, ndim, array )                             \
                                                                       \
        (   isrchi_c( (value),  (ndim),  CONST_IVEC(array) )  )


   #define  isrot_c( m, ntol, dtol )                                   \
                                                                       \
        (   isrot_c ( CONST_MAT(m), (ntol), (dtol) )   )


   #define  latsrf_c( method, target, et,    fixref,                   \
                      npts,   lonlat, srfpts         )                 \
                                                                       \
        (   latsrf_c( CONST_STR(method), CONST_STR(target), (et),      \
                      CONST_STR(fixref), (npts), CONST_MAT2(lonlat),   \
                      (srfpts)                                     )  )


   #define  limbpt_c( method, target, et,     fixref,                  \
                      abcorr, corloc, obsrvr, refvec,                  \
                      rolstp, ncuts,  schstp, soltol,                  \
                      maxn,   npts,   points, epochs,                  \
                      tangts                          )                \
                                                                       \
       (   limbpt_c( CONST_STR(method), CONST_STR(target),  (et),      \
                     CONST_STR(fixref), CONST_STR(abcorr),             \
                     CONST_STR(corloc), CONST_STR(obsrvr),             \
                     CONST_VEC(refvec), (rolstp),           (ncuts),   \
                     (schstp),          (soltol),           (maxn),    \
                     (npts),            (points),           (epochs),  \
                     (tangts)                                      )  )


   #define  lmpool_c( cvals, lenvals, n )                              \
                                                                       \
        (   lmpool_c( CONST_VOID(cvals), (lenvals), (n) )  )


   #define  lstltc_c( value, ndim, lenvals, array )                    \
                                                                       \
        (   lstltc_c ( CONST_STR(value),  (ndim),  (lenvals),          \
                       CONST_VOID(array)                      ) )


   #define  lstled_c( value, ndim, array )                             \
                                                                       \
        (   lstled_c( (value),  (ndim),  CONST_VEC(array) )  )


   #define  lstlei_c( value, ndim, array )                             \
                                                                       \
        (   lstlei_c( (value),  (ndim),  CONST_IVEC(array) )  )


   #define  lstlec_c( value, ndim, lenvals, array )                    \
                                                                       \
        (   lstlec_c ( CONST_STR(value),  (ndim),  (lenvals),          \
                       CONST_VOID(array)                      ) )


   #define  lstltd_c( value, ndim, array )                             \
                                                                       \
        (   lstltd_c( (value),  (ndim),  CONST_VEC(array) )  )


   #define  lstlti_c( value, ndim, array )                             \
                                                                       \
        (   lstlti_c( (value),  (ndim),  CONST_IVEC(array) )  )


   #define  m2eul_c( r, axis3,  axis2,  axis1,                         \
                        angle3, angle2, angle1 )                       \
                                                                       \
        (   m2eul_c ( CONST_MAT(r), (axis3),  (axis2),  (axis1),       \
                                    (angle3), (angle2), (angle1) )   )

   #define  m2q_c( r, q )                                              \
                                                                       \
        (   m2q_c ( CONST_MAT(r), (q) )   )


   #define  mequ_c( m1, m2 )                                           \
                                                                       \
           ( mequ_c  ( CONST_MAT(m1), m2 ) )


   #define  mequg_c( m1, nr, nc, mout )                                \
                                                                       \
        (   mequg_c  ( CONST_MAT(m1), (nr), (nc), mout )   )


   #define  mtxm_c( m1, m2, mout )                                     \
                                                                       \
        (   mtxm_c ( CONST_MAT(m1), CONST_MAT(m2), (mout) )   )


   #define  mtxmg_c( m1, m2, ncol1, nr1r2, ncol2, mout )               \
                                                                       \
        (   mtxmg_c ( CONST_MAT(m1), CONST_MAT(m2),                    \
                      (ncol1),       (nr1r2),       (ncol2), (mout) )  )


   #define  mtxv_c( m1, vin, vout )                                    \
                                                                       \
        (   mtxv_c ( CONST_MAT(m1), CONST_VEC(vin), (vout) )   )


   #define  mtxvg_c( m1, v2, nrow1, nc1r2, vout )                      \
                                                                       \
        (   mtxvg_c( CONST_VOID(m1), CONST_VOID(v2),                   \
                    (nrow1),        (nc1r2),       (vout) )   )

   #define  mxm_c( m1, m2, mout )                                      \
                                                                       \
        (   mxm_c ( CONST_MAT(m1), CONST_MAT(m2), (mout) )   )


   #define  mxmg_c( m1, m2, row1, col1, col2, mout )                   \
                                                                       \
        (   mxmg_c ( CONST_VOID(m1), CONST_VOID(m2),                   \
                     (row1), (col1), (col2), (mout) )   )


   #define  mxmt_c( m1, m2, mout )                                     \
                                                                       \
        (   mxmt_c ( CONST_MAT(m1), CONST_MAT(m2), (mout) )   )


   #define  mxmtg_c( m1, m2, nrow1, nc1c2, nrow2, mout )               \
                                                                       \
        (   mxmtg_c ( CONST_VOID(m1), CONST_VOID(m2),                  \
                      (nrow1),        (nc1c2),                         \
                      (nrow2),        (mout)             )   )


   #define  mxv_c( m1, vin, vout )                                     \
                                                                       \
        (   mxv_c ( CONST_MAT(m1), CONST_VEC(vin), (vout) )   )


   #define  mxvg_c( m1, v2, nrow1, nc1r2, vout )                       \
                                                                       \
        (   mxvg_c( CONST_VOID(m1), CONST_VOID(v2),                    \
                    (nrow1),        (nc1r2),       (vout) )   )

   #define  nearpt_c( positn, a, b, c, npoint, alt )                   \
                                                                       \
        (   nearpt_c ( CONST_VEC(positn), (a),  (b),  (c),             \
                       (npoint),          (alt)            )   )


   #define  npedln_c( a, b, c, linept, linedr, pnear, dist )           \
                                                                       \
        (   npedln_c ( (a),               (b),               (c),      \
                       CONST_VEC(linept), CONST_VEC(linedr),           \
                       (pnear),           (dist)                 )   )


   #define  nplnpt_c( linpt, lindir, point, pnear, dist )              \
                                                                       \
        (   nplnpt_c ( CONST_VEC(linpt), CONST_VEC(lindir),            \
                       CONST_VEC(point), (pnear), (dist )   )   )


   #define  nvc2pl_c( normal, constant, plane )                        \
                                                                       \
        (   nvc2pl_c ( CONST_VEC(normal), (constant), (plane) )  )


   #define  nvp2pl_c( normal, point, plane )                           \
                                                                       \
        (   nvp2pl_c( CONST_VEC(normal), CONST_VEC(point), (plane) )  )


   #define  orderc_c( lenvals, array, ndim, iorder )                   \
                                                                       \
        (   orderc_c ( (lenvals), CONST_VOID(array), (ndim), (iorder)) )


   #define  orderd_c( array, ndim, iorder )                            \
                                                                       \
        (   orderd_c ( CONST_VEC(array), (ndim), (iorder) )  )


   #define  orderi_c( array, ndim, iorder )                            \
                                                                       \
        (   orderi_c ( CONST_IVEC(array), (ndim), (iorder) )  )


   #define  oscelt_c( state, et, mu, elts )                            \
                                                                       \
        (   oscelt_c ( CONST_VEC(state), (et), (mu), (elts)  )   )


   #define  oscltx_c( state, et, mu, elts )                            \
                                                                       \
        (   oscltx_c ( CONST_VEC(state), (et), (mu), (elts)  )   )


   #define  pcpool_c( name, n, lenvals, cvals )                        \
                                                                       \
        (   pcpool_c ( (name), (n), (lenvals), CONST_VOID(cvals) )  )


   #define  pdpool_c( name, n, dvals )                                 \
                                                                       \
        (   pdpool_c ( (name), (n), CONST_VEC(dvals) )  )


   #define  pipool_c( name, n, ivals )                                 \
                                                                       \
        (   pipool_c ( (name), (n), CONST_IVEC(ivals) )  )


   #define  pl2nvc_c( plane, normal, constant )                        \
                                                                       \
        (   pl2nvc_c ( CONST_PLANE(plane),  (normal), (constant) )  )


   #define  pl2nvp_c( plane, normal, point )                           \
                                                                       \
        (   pl2nvp_c ( CONST_PLANE(plane),  (normal), (point) )  )


   #define  pl2psv_c( plane, point, span1, span2 )                     \
                                                                       \
        (   pl2psv_c( CONST_PLANE(plane), (point), (span1), (span2) )  )


   #define  pltar_c( nv, vrtces, np, plates )                          \
                                                                       \
        (   pltar_c( (nv), CONST_VEC3(vrtces),                         \
                     (np), CONST_IVEC3(plates) )   )


   #define  pltexp_c( iverts, delta, overts )                          \
                                                                       \
        (   pltexp_c( CONST_VEC3(iverts), (delta), (overts) )  )


   #define  pltnp_c( point, v1, v2, v3, pnear, dist )                  \
                                                                       \
        (   pltnp_c( CONST_VEC(point), CONST_VEC(v1), CONST_VEC(v2),   \
                     CONST_VEC(v3),    (pnear),       (dist)       )  )


   #define  pltnrm_c( v1, v2, v3, normal )                             \
                                                                       \
        (   pltnrm_c( CONST_VEC(v1), CONST_VEC(v2), CONST_VEC(v3),     \
                     (normal)  )   )


   #define  pltvol_c( nv, vrtces, np, plates )                         \
                                                                       \
        (   pltvol_c( (nv), CONST_VEC3(vrtces),                        \
                      (np), CONST_IVEC3(plates) )   )


   #define  prop2b_c( gm, pvinit, dt, pvprop )                         \
                                                                       \
        (   prop2b_c ( (gm),  CONST_VEC(pvinit), (dt), (pvprop)  )   )


   #define  psv2pl_c( point, span1, span2, plane )                     \
                                                                       \
        (   psv2pl_c ( CONST_VEC(point),  CONST_VEC(span1),            \
                       CONST_VEC(span2),  (plane)           )   )


   #define  qdq2av_c( q, dq, av )                                      \
                                                                       \
        (   qdq2av_c ( CONST_VEC(q), CONST_VEC(dq),  (av) )    )


   #define  q2m_c( q, r )                                              \
                                                                       \
        (   q2m_c ( CONST_VEC(q), (r) )    )


   #define  qxq_c( q1, q2, qout )                                      \
                                                                       \
        (   qxq_c ( CONST_VEC(q1), CONST_VEC(q2),  (qout) )    )


   #define  rav2xf_c( rot, av, xform )                                 \
                                                                       \
        (   rav2xf_c ( CONST_MAT(rot), CONST_VEC(av), (xform) )   )


   #define  raxisa_c( matrix, axis, angle )                            \
                                                                       \
        (   raxisa_c ( CONST_MAT(matrix), (axis), (angle) )   );


   #define  reccyl_c( rectan, r, lon, z )                              \
                                                                       \
        (   reccyl_c ( CONST_VEC(rectan), (r), (lon), (z)  )   )


   #define  recgeo_c( rectan, re, f, lon, lat, alt )                   \
                                                                       \
        (   recgeo_c ( CONST_VEC(rectan), (re),   (f),                 \
                       (lon),             (lat),  (alt) )   )

   #define  reclat_c( rectan, r, lon, lat )                            \
                                                                       \
        (   reclat_c ( CONST_VEC(rectan), (r), (lon), (lat)  )   )


   #define  recrad_c( rectan, radius, ra, dec )                        \
                                                                       \
        (   recrad_c ( CONST_VEC(rectan), (radius), (ra), (dec)  )   )


   #define  recsph_c( rectan, r, colat, lon )                          \
                                                                       \
        (   recsph_c ( CONST_VEC(rectan), (r), (colat), (lon)  )   )


   #define  reordd_c( iorder, ndim, array )                            \
                                                                       \
        (   reordd_c ( CONST_IVEC(iorder), (ndim), (array) )  )


   #define  reordi_c( iorder, ndim, array )                            \
                                                                       \
        (   reordi_c ( CONST_IVEC(iorder), (ndim), (array) )  )


   #define  reordl_c( iorder, ndim, array )                            \
                                                                       \
        (   reordl_c ( CONST_IVEC(iorder), (ndim), (array) )  )


   #define  rotmat_c( m1, angle, iaxis, mout  )                        \
                                                                       \
        (   rotmat_c ( CONST_MAT(m1), (angle), (iaxis), (mout)  )   )


   #define  rotvec_c( v1, angle, iaxis, vout )                         \
                                                                       \
        (   rotvec_c ( CONST_VEC(v1), (angle), (iaxis), (vout)  )   )


   #define  saelgv_c( vec1, vec2, smajor, sminor )                     \
                                                                       \
        (   saelgv_c ( CONST_VEC(vec1),  CONST_VEC(vec2),              \
                       (smajor),         (sminor)         )   )


   #define  spk14a_c( handle, ncsets, coeffs, epochs )                 \
                                                                       \
        (   spk14a_c ( (handle),           (ncsets),                   \
                       CONST_VEC(coeffs),  CONST_VEC(epochs) )  )


   #define  spkapo_c( targ, et, ref, sobs, abcorr, ptarg, lt )         \
                                                                       \
        (   spkapo_c ( (targ),   (et),    (ref), CONST_VEC(sobs),      \
                       (abcorr), (ptarg), (lt)                   )  )


   #define  spkapp_c( targ, et, ref, sobs, abcorr, starg, lt )         \
                                                                       \
        (   spkapp_c ( (targ),   (et),    (ref), CONST_VEC(sobs),      \
                       (abcorr), (starg), (lt)                   )  )


   #define  spkaps_c( targ,   et,    ref, abcorr, sobs,                \
                      accobs, starg, lt,  dlt           )              \
                                                                       \
        (   spkaps_c ( (targ),   (et),  (ref),  (abcorr),              \
                       CONST_VEC(sobs), CONST_VEC(accobs),             \
                       (starg),  (lt),  (dlt)              )   )


   #define  spkcpo_c( target,   et,       outref,   refloc,            \
                      abcorr,   obspos,   obsctr,                      \
                      obsref,   state,    lt               )           \
                                                                       \
        (   spkcpo_c( (target), (et),    (outref), (refloc),           \
                      (abcorr), CONST_VEC(obspos), (obsctr),           \
                      (obsref), (state),  (lt)              )  )


   #define  spkcpt_c( trgpos,   trgctr,   trgref,                      \
                      et,       outref,   refloc,   abcorr,            \
                      obsrvr,   state,    lt               )           \
                                                                       \
        (   spkcpt_c( CONST_VEC(trgpos), (trgctr), (trgref),           \
                      (et),    (outref), (refloc), (abcorr),           \
                      (obsrvr),          (state),  (lt)      )  )


   #define  spkcvo_c( target,   et,       outref,   refloc,            \
                      abcorr,   obssta,   obsepc,   obsctr,            \
                      obsref,   state,    lt               )           \
                                                                       \
        (   spkcvo_c( (target),  (et),    (outref), (refloc),          \
                      (abcorr),  CONST_VEC(obssta), (obsepc),          \
                      (obsctr),  (obsref), (state), (lt)     )  )


   #define  spkcvt_c( trgsta,   trgepc,   trgctr,   trgref,            \
                      et,       outref,   refloc,   abcorr,            \
                      obsrvr,   state,    lt               )           \
                                                                       \
        (   spkcvt_c( CONST_VEC(trgsta),  (trgepc), (trgctr),          \
                      (trgref), (et),     (outref), (refloc),          \
                      (abcorr), (obsrvr), (state),  (lt)     )  )


   #define  spkltc_c( targ, et, ref, abcorr, sobs, starg, lt, dlt )    \
                                                                       \
        (   spkltc_c ( (targ),   (et),  (ref),    (abcorr),            \
                       CONST_VEC(sobs), (starg),  (lt),     (dlt) )  )


   #define  spkpvn_c( handle, descr, et, ref, state, center )          \
                                                                       \
        (   spkpvn_c ( (handle), CONST_VEC(descr), (et),               \
                       (ref),    (state),          (center) )  )

   #define  spkuds_c( descr, body, center, frame, type,                \
                      first, last, begin,  end         )               \
                                                                       \
        (   spkuds_c ( CONST_VEC(descr), (body), (center), (frame),    \
                       (type),  (first), (last), (begin),  (end)    )  )


   #define  spkw02_c( handle, body,   center, frame,  first,  last,    \
                      segid,  intlen, n,      polydg, cdata,  btime )  \
                                                                       \
        (   spkw02_c ( (handle), (body),   (center),         (frame),  \
                       (first),  (last),   (segid),          (intlen), \
                       (n),      (polydg), CONST_VEC(cdata), (btime) ) )


   #define  spkw03_c( handle, body,   center, frame,  first,  last,    \
                      segid,  intlen, n,      polydg, cdata,  btime )  \
                                                                       \
        (   spkw03_c ( (handle), (body),   (center),         (frame),  \
                       (first),  (last),   (segid),          (intlen), \
                       (n),      (polydg), CONST_VEC(cdata), (btime) ) )



   #define  spkw05_c( handle, body,   center, frame,  first,  last,    \
                      segid,  gm,     n,      states, epochs        )  \
                                                                       \
        (   spkw05_c ( (handle),  (body),   (center),   (frame),       \
                       (first),   (last),   (segid),    (gm),          \
                       (n),                                            \
                       CONST_MAT6(states),  CONST_VEC(epochs)    )   )


   #define  spkw08_c( handle, body,   center, frame,  first,  last,    \
                      segid,  degree, n,      states, epoch1, step )   \
                                                                       \
        (   spkw08_c ( (handle),  (body),   (center),   (frame),       \
                       (first),   (last),   (segid),    (degree),      \
                       (n),       CONST_MAT6(states),   (epoch1),      \
                       (step)                                     )   )


   #define  spkw09_c( handle, body,   center, frame,  first,  last,    \
                      segid,  degree, n,      states, epochs       )   \
                                                                       \
        (   spkw09_c ( (handle), (body),   (center), (frame),          \
                       (first),  (last),   (segid),  (degree),  (n),   \
                       CONST_MAT6(states), CONST_VEC(epochs)        )  )


   #define  spkw10_c( handle, body,   center, frame,  first,  last,    \
                      segid,  consts, n,      elems,  epochs       )   \
                                                                       \
        (   spkw10_c ( (handle), (body),  (center), (frame),           \
                       (first),  (last),  (segid),  CONST_VEC(consts), \
                       (n),      CONST_VEC(elems),  CONST_VEC(epochs)) )


   #define  spkw12_c( handle, body,   center, frame,  first,  last,    \
                      segid,  degree, n,      states, epoch0, step )   \
                                                                       \
        (   spkw12_c ( (handle),  (body),   (center),   (frame),       \
                       (first),   (last),   (segid),    (degree),      \
                       (n),       CONST_MAT6(states),   (epoch0),      \
                       (step)                                     )   )


   #define  spkw13_c( handle, body,   center, frame,  first,  last,    \
                      segid,  degree, n,      states, epochs       )   \
                                                                       \
        (   spkw13_c ( (handle), (body),   (center), (frame),          \
                       (first),  (last),   (segid),  (degree),  (n),   \
                       CONST_MAT6(states), CONST_VEC(epochs)        )  )





   #define  spkw15_c( handle, body,   center, frame,  first,  last,    \
                      segid,  epoch,  tp,     pa,     p,      ecc,     \
                      j2flg,  pv,     gm,     j2,     radius         ) \
                                                                       \
        (   spkw15_c ( (handle), (body),  (center), (frame),           \
                       (first),  (last),  (segid),  (epoch),           \
                       CONST_VEC(tp),     CONST_VEC(pa),               \
                       (p),      (ecc),   (j2flg),  CONST_VEC(pv),     \
                       (gm),     (j2),    (radius)                )   )


   #define  spkw17_c( handle, body,   center, frame,  first,  last,    \
                      segid,  epoch,  eqel,   rapol,  decpol       )   \
                                                                       \
        (   spkw17_c ( (handle), (body),  (center), (frame),           \
                       (first),  (last),  (segid),  (epoch),           \
                       CONST_VEC(eqel),   (rapol),  (decpol)  )   )



   #define  spkw18_c( handle, subtyp, body,   center, frame,  first,   \
                      last,   segid,  degree, n,      packts, epochs ) \
                                                                       \
        (   spkw18_c ( (handle), (subtyp), (body),  (center), (frame), \
                       (first),  (last),   (segid), (degree), (n),     \
                       CONST_VOID(packts), CONST_VEC(epochs)        )  )


   #define  spkw20_c( handle, body,   center, frame,  first,  last,    \
                      segid,  intlen, n,      polydg, cdata,  dscale,  \
                      tscale, initjd, initfr                         ) \
                                                                       \
        (   spkw20_c ( (handle), (body),   (center),         (frame),  \
                       (first),  (last),   (segid),          (intlen), \
                       (n),      (polydg), CONST_VEC(cdata), (dscale), \
                       (tscale), (initjd), (initfr)                  ) )



   #define  srfxpt_c( method, target, et,    abcorr, obsrvr, dref,     \
                      dvec,   spoint, dist,  trgepc, obspos, found )   \
                                                                       \
        (   srfxpt_c ( (method), (target),  (et), (abcorr), (obsrvr),  \
                       (dref),   CONST_VEC(dvec), (spoint), (dist),    \
                       (trgepc), (obspos),        (found)          )   )


   #define  srfnrm_c( method, target, et,    fixref,                   \
                      npts,   srfpts, normls         )                 \
                                                                       \
        (   srfnrm_c ( CONST_STR(method),  CONST_STR(target), (et),    \
                       CONST_STR(fixref),  (npts),                     \
                       CONST_VEC3(srfpts), (normls)               )   )


   #define  stelab_c( pobj, vobj, appobj )                             \
                                                                       \
        (   stelab_c ( CONST_VEC(pobj), CONST_VEC(vobj), (appobj)  )   )


   #define  sumad_c( array, n )                                        \
                                                                       \
        (   sumad_c ( CONST_VEC(array), (n)  )   )


   #define  sumai_c( array, n )                                        \
                                                                       \
        (   sumai_c ( CONST_IVEC(array), (n)  )   )


   #define  surfnm_c( a, b, c, point, normal )                         \
                                                                       \
        (   surfnm_c ( (a), (b), (c), CONST_VEC(point), (normal) )   )


   #define  surfpt_c( positn, u, a, b, c, point, found )               \
                                                                       \
        (   surfpt_c ( CONST_VEC(positn), CONST_VEC(u),                \
                       (a),               (b),               (c),      \
                       (point),           (found)                 )   )


   #define  surfpv_c( stvrtx, stdir, a, b, c, stx, found )             \
                                                                       \
        (   surfpv_c ( CONST_VEC(stvrtx), CONST_VEC(stdir),            \
                       (a),               (b),               (c),      \
                       (stx),             (found)                 )   )


   #define  swpool_c( agent, nnames, lenvals, names )                  \
                                                                       \
        (   swpool_c( CONST_STR(agent), (nnames),                      \
                      (lenvals),        CONST_VOID(names)         )    )


   #define  termpt_c( method, ilusrc, target, et,     fixref,          \
                      abcorr, corloc, obsrvr, refvec,                  \
                      rolstp, ncuts,  schstp, soltol,                  \
                      maxn,   npts,   points, epochs,                  \
                      tangts                          )                \
                                                                       \
       (   termpt_c( CONST_STR(method), CONST_STR(ilusrc),             \
                     CONST_STR(target), (et),                          \
                     CONST_STR(fixref), CONST_STR(abcorr),             \
                     CONST_STR(corloc), CONST_STR(obsrvr),             \
                     CONST_VEC(refvec), (rolstp),           (ncuts),   \
                     (schstp),          (soltol),           (maxn),    \
                     (npts),            (points),           (epochs),  \
                     (tangts)                                      )  )


   #define  trace_c( m1 )                                              \
                                                                       \
           ( trace_c ( CONST_MAT(m1) ) )


   #define  twovec_c( axdef, indexa, plndef, indexp, mout )            \
                                                                       \
        (   twovec_c ( CONST_VEC(axdef),  (indexa),                    \
                       CONST_VEC(plndef), (indexp), (mout) )   )


   #define  ucrss_c( v1, v2, vout )                                    \
                                                                       \
        (   ucrss_c ( CONST_VEC(v1), CONST_VEC(v2), (vout) )   )


   #define  unorm_c( v1, vout, vmag )                                  \
                                                                       \
        (   unorm_c ( CONST_VEC(v1), (vout), (vmag) )   )


   #define  unormg_c( v1, ndim, vout, vmag )                           \
                                                                       \
        (   unormg_c ( CONST_VEC(v1), (ndim), (vout), (vmag) )   )


   #define  vadd_c( v1, v2, vout )                                     \
                                                                       \
        (   vadd_c ( CONST_VEC(v1), CONST_VEC(v2), (vout) )   )


   #define  vaddg_c( v1, v2, ndim,vout )                               \
                                                                       \
        (  vaddg_c ( CONST_VEC(v1), CONST_VEC(v2), (ndim), (vout) ) )


   #define  vcrss_c( v1, v2, vout )                                    \
                                                                       \
        (   vcrss_c ( CONST_VEC(v1), CONST_VEC(v2), (vout) )   )


   #define  vdist_c( v1, v2 )                                          \
                                                                       \
        (   vdist_c ( CONST_VEC(v1), CONST_VEC(v2) )   )


   #define  vdistg_c( v1, v2, ndim )                                   \
                                                                       \
        (   vdistg_c ( CONST_VEC(v1), CONST_VEC(v2), (ndim) )   )


   #define  vdot_c( v1, v2 )                                           \
                                                                       \
        (   vdot_c ( CONST_VEC(v1), CONST_VEC(v2) )   )


   #define  vdotg_c( v1, v2, ndim )                                    \
                                                                       \
        (   vdotg_c ( CONST_VEC(v1), CONST_VEC(v2), (ndim) )   )


   #define  vequ_c( vin, vout )                                        \
                                                                       \
        (   vequ_c ( CONST_VEC(vin), (vout) )   )


   #define  vequg_c( vin, ndim, vout )                                 \
                                                                       \
        (   vequg_c ( CONST_VEC(vin), (ndim), (vout) )   )


   #define  vhat_c( v1, vout )                                         \
                                                                       \
        (   vhat_c ( CONST_VEC(v1), (vout) )   )


   #define  vhatg_c( v1, ndim, vout )                                  \
                                                                       \
        (   vhatg_c ( CONST_VEC(v1), (ndim), (vout) )   )


   #define  vlcom3_c( a, v1, b, v2, c, v3, sum )                       \
                                                                       \
        (   vlcom3_c ( (a), CONST_VEC(v1),                             \
                       (b), CONST_VEC(v2),                             \
                       (c), CONST_VEC(v3), (sum) )   )


   #define  vlcom_c( a, v1, b, v2, sum )                               \
                                                                       \
        (   vlcom_c ( (a), CONST_VEC(v1),                              \
                      (b), CONST_VEC(v2), (sum) )   )


   #define  vlcomg_c( n, a, v1, b, v2, sum )                           \
                                                                       \
           ( vlcomg_c ( (n), (a), CONST_VEC(v1),                       \
                             (b), CONST_VEC(v2),  (sum) )   )


   #define  vminug_c( v1, ndim, vout )                                 \
                                                                       \
       (   vminug_c ( CONST_VEC(v1), (ndim), (vout) )   )


   #define  vminus_c( v1, vout )                                       \
                                                                       \
        (   vminus_c ( CONST_VEC(v1), (vout) )   )


   #define  vnorm_c( v1 )                                              \
                                                                       \
        (   vnorm_c ( CONST_VEC(v1) )   )


   #define  vnormg_c( v1, ndim )                                       \
                                                                       \
        (   vnormg_c ( CONST_VEC(v1), (ndim) )   )


   #define  vperp_c( a, b, p )                                         \
                                                                       \
        (   vperp_c ( CONST_VEC(a), CONST_VEC(b), (p) )   )


   #define  vprjp_c( vin, plane, vout )                                \
                                                                       \
        (   vprjp_c ( CONST_VEC(vin), CONST_PLANE(plane), (vout) )   )


   #define  vprjpi_c( vin, projpl, invpl, vout, found )                \
                                                                       \
        (   vprjpi_c( CONST_VEC(vin),     CONST_PLANE(projpl),         \
                      CONST_PLANE(invpl), (vout),           (found) ) )


   #define  vproj_c( a, b, p )                                         \
                                                                       \
        (   vproj_c ( CONST_VEC(a), CONST_VEC(b), (p) )   )


   #define  vrel_c( v1, v2 )                                           \
                                                                       \
           ( vrel_c ( CONST_VEC(v1), CONST_VEC(v2) )   )


   #define  vrelg_c( v1, v2, ndim )                                    \
                                                                       \
           ( vrelg_c ( CONST_VEC(v1), CONST_VEC(v2), (ndim) )   )


   #define  vrotv_c( v, axis, theta, r )                               \
                                                                       \
        (   vrotv_c ( CONST_VEC(v), CONST_VEC(axis), (theta), (r) )   )


   #define  vscl_c( s, v1, vout )                                      \
                                                                       \
        (   vscl_c ( (s), CONST_VEC(v1), (vout) )   )


   #define  vsclg_c( s, v1, ndim, vout )                               \
                                                                       \
        (   vsclg_c ( (s), CONST_VEC(v1), (ndim), (vout) )   )


   #define  vsep_c( v1, v2 )                                           \
                                                                       \
        (   vsep_c ( CONST_VEC(v1), CONST_VEC(v2) )   )


   #define  vsepg_c( v1, v2, ndim)                                     \
                                                                       \
           ( vsepg_c ( CONST_VEC(v1), CONST_VEC(v2), ndim )  )


   #define  vsub_c( v1, v2, vout )                                     \
                                                                       \
        (   vsub_c ( CONST_VEC(v1), CONST_VEC(v2), (vout) )   )


   #define  vsubg_c( v1, v2, ndim, vout )                              \
                                                                       \
        (   vsubg_c ( CONST_VEC(v1), CONST_VEC(v2),                    \
                      (ndim),        (vout)            )   )

   #define  vtmv_c( v1, mat, v2 )                                      \
                                                                       \
        ( vtmv_c ( CONST_VEC(v1), CONST_MAT(mat), CONST_VEC(v2) ) )


   #define  vtmvg_c( v1, mat, v2, nrow, ncol )                         \
                                                                       \
        ( vtmvg_c ( CONST_VOID(v1), CONST_VOID(mat), CONST_VOID(v2),   \
                   (nrow), (ncol)                                   )  )


   #define  vupack_c( v, x, y, z )                                     \
                                                                       \
        (   vupack_c ( CONST_VEC(v), (x), (y), (z) )   )


   #define  vzero_c( v1 )                                              \
                                                                       \
        (   vzero_c ( CONST_VEC(v1) )   )


   #define  vzerog_c( v1, ndim )                                       \
                                                                       \
           (   vzerog_c ( CONST_VEC(v1), (ndim) )   )


   #define  xf2eul_c( xform, axisa, axisb, axisc, eulang, unique )     \
                                                                       \
        (   xf2eul_c( CONST_MAT6(xform), (axisa), (axisb), (axisc),    \
                      (eulang),          (unique)                  )  )


   #define  xf2rav_c( xform, rot, av )                                 \
                                                                       \
        (   xf2rav_c( CONST_MAT6(xform), (rot), (av) )   )


   #define  xpose6_c( m1, mout )                                       \
                                                                       \
        (   xpose6_c ( CONST_MAT6(m1), (mout) )   )


   #define  xpose_c( m1, mout )                                        \
                                                                       \
        (   xpose_c ( CONST_MAT(m1), (mout) )   )


   #define  xposeg_c( matrix, nrow, ncol, mout )                       \
                                                                       \
        (   xposeg_c ( CONST_VOID(matrix), (nrow), (ncol), (mout) )   )


#endif
