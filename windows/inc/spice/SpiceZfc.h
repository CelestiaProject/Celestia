/*

-Header_File SpiceZfc.h ( f2c'd SPICELIB prototypes )

-Abstract

   Define prototypes for functions produced by converting Fortran
   SPICELIB routines to C using f2c.

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

-Author_and_Institution

   N.J. Bachman       (JPL)
   K.R. Gehringer     (JPL)

-Version

   - C-SPICELIB Version 5.0.0, 06-MAR-2005 (NJB)

        Added typedefs for pointers to functions.  This change was
        made to support CSPICE wrappers for geometry finder routines.

        Added typedefs for the SUN-SOLARIS-64BIT-GCC_C
        environment (these are identical to those for the 
        ALPHA-DIGITAL-UNIX_C environment).

   - C-SPICELIB Version 4.1.0, 24-MAY-2001 (WLT)

        Moved the #ifdef __cplusplus so that it appears after the
        typedefs.  This allows us to more easily wrap CSPICE in a
        namespace for C++.

   - C-SPICELIB Version 4.0.0, 09-FEB-1999 (NJB)  
   
        Updated to accommodate the Alpha/Digital Unix platform.
        Also updated to support inclusion in C++ code.
                  
   - C-SPICELIB Version 3.0.0, 02-NOV-1998 (NJB)  
   
        Updated for SPICELIB version N0049.
        
   - C-SPICELIB Version 2.0.0, 15-SEP-1997 (NJB)  
   
        Changed variable name "typid" to "typid" in prototype
        for zzfdat_.  This was done to enable compilation under
        Borland C++.
        
   - C-SPICELIB Version 1.0.0, 15-SEP-1997 (NJB) (KRG)

-Index_Entries

   protoypes of f2c'd SPICELIB functions

*/


#ifndef HAVE_SPICEF2C_H
#define HAVE_SPICEF2C_H



/*
   Include Files:

   Many of the prototypes below use data types defined by f2c.  We
   copy here the f2c definitions that occur in prototypes of functions
   produced by running f2c on Fortran SPICELIB routines.
   
   The reason we don't simply conditionally include f2c.h itself here
   is that f2c.h defines macros that conflict with stdlib.h on some
   systems.  It's simpler to just replicate the few typedefs we need.
*/

#if (    defined(CSPICE_ALPHA_DIGITAL_UNIX    )    \
      || defined(CSPICE_SUN_SOLARIS_64BIT_GCC )  )

   #define VOID      void
   
   typedef VOID      H_f;
   typedef int       integer;
   typedef double    doublereal;
   typedef int       logical;
   typedef int       ftnlen;
 

   /*
   Type H_fp is used for character return type.
   Type S_fp is used for subroutines.
   Type U_fp is used for functions of unknown type.
   */
   typedef VOID       (*H_fp)();
   typedef doublereal (*D_fp)();
   typedef doublereal (*E_fp)();
   typedef int        (*S_fp)();
   typedef int        (*U_fp)();
   typedef integer    (*I_fp)();
   typedef logical    (*L_fp)();

#else

   #define VOID      void
   
   typedef VOID      H_f;
   typedef long      integer;
   typedef double    doublereal;
   typedef long      logical;
   typedef long      ftnlen;

   /*
   Type H_fp is used for character return type.
   Type S_fp is used for subroutines.
   Type U_fp is used for functions of unknown type.
   */
   typedef VOID       (*H_fp)();
   typedef doublereal (*D_fp)();
   typedef doublereal (*E_fp)();
   typedef int        (*S_fp)();
   typedef int        (*U_fp)();
   typedef integer    (*I_fp)();
   typedef logical    (*L_fp)();

#endif


#ifdef __cplusplus
   extern "C" { 
#endif


/*
   Function prototypes for functions created by f2c are listed below.
   See the headers of the Fortran routines for descriptions of the
   routines' interfaces.

   The functions listed below are those expected to be called by
   C-SPICELIB wrappers.  Prototypes are not currently provided for other
   f2c'd functions.

*/

/*
-Prototypes
*/

extern logical accept_(logical *ok);
extern logical allowd_(void);
 
extern logical alltru_(logical *logcls, integer *n);
 
extern H_f ana_(char *ret_val, ftnlen ret_val_len, char *word, char *case__, ftnlen word_len, ftnlen case_len);
/*:ref: ucase_ 14 4 13 13 124 124 */
/*:ref: replch_ 14 8 13 13 13 13 124 124 124 124 */
/*:ref: ljust_ 14 4 13 13 124 124 */
/*:ref: isrchc_ 4 5 13 4 13 124 124 */
 
extern int appndc_(char *item, char *cell, ftnlen item_len, ftnlen cell_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardc_ 4 2 13 124 */
/*:ref: sizec_ 4 2 13 124 */
/*:ref: scardc_ 14 3 4 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int appndd_(doublereal *item, doublereal *cell);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardd_ 4 1 7 */
/*:ref: sized_ 4 1 7 */
/*:ref: scardd_ 14 2 4 7 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int appndi_(integer *item, integer *cell);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardi_ 4 1 4 */
/*:ref: sizei_ 4 1 4 */
/*:ref: scardi_ 14 2 4 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern logical approx_(doublereal *x, doublereal *y, doublereal *tol);
 
extern int astrip_(char *instr, char *asciib, char *asciie, char *outstr, ftnlen instr_len, ftnlen asciib_len, ftnlen asciie_len, ftnlen outstr_len);
/*:ref: lastnb_ 4 2 13 124 */
 
extern int axisar_(doublereal *axis, doublereal *angle, doublereal *r__);
/*:ref: ident_ 14 1 7 */
/*:ref: vrotv_ 14 4 7 7 7 7 */
/*:ref: vequ_ 14 2 7 7 */
 
extern doublereal b1900_(void);
 
extern doublereal b1950_(void);
 
extern logical badkpv_(char *caller, char *name__, char *comp, integer *size, integer *divby, char *type__, ftnlen caller_len, ftnlen name_len, ftnlen comp_len, ftnlen type_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dtpool_ 14 6 13 12 4 13 124 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: eqchr_ 12 4 13 13 124 124 */
 
extern logical bedec_(char *string, ftnlen string_len);
/*:ref: pos_ 4 5 13 13 4 124 124 */
/*:ref: beint_ 12 2 13 124 */
/*:ref: beuns_ 12 2 13 124 */
 
extern logical beint_(char *string, ftnlen string_len);
/*:ref: frstnb_ 4 2 13 124 */
/*:ref: beuns_ 12 2 13 124 */
 
extern logical benum_(char *string, ftnlen string_len);
/*:ref: cpos_ 4 5 13 13 4 124 124 */
/*:ref: bedec_ 12 2 13 124 */
/*:ref: beint_ 12 2 13 124 */
 
extern logical beuns_(char *string, ftnlen string_len);
/*:ref: frstnb_ 4 2 13 124 */
 
extern int bodc2n_(integer *code, char *name__, logical *found, ftnlen name_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: zzbodc2n_ 14 4 4 13 12 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int boddef_(char *name__, integer *code, ftnlen name_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: zzboddef_ 14 3 13 4 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int bodeul_(integer *body, doublereal *et, doublereal *ra, doublereal *dec, doublereal *w, doublereal *lambda);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: irfnum_ 14 3 13 4 124 */
/*:ref: pckeul_ 14 6 4 7 12 13 7 124 */
/*:ref: bodfnd_ 12 3 4 13 124 */
/*:ref: bodvcd_ 14 6 4 13 4 4 7 124 */
/*:ref: rpd_ 7 0 */
/*:ref: twopi_ 7 0 */
/*:ref: zzbodbry_ 4 1 4 */
/*:ref: repmi_ 14 7 13 13 4 13 124 124 124 */
/*:ref: gdpool_ 14 7 13 4 4 4 7 12 124 */
/*:ref: spd_ 7 0 */
/*:ref: j2000_ 7 0 */
/*:ref: cleard_ 14 2 4 7 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: vdotg_ 7 3 7 7 4 */
/*:ref: halfpi_ 7 0 */
/*:ref: irfrot_ 14 3 4 4 7 */
/*:ref: eul2m_ 14 7 7 7 7 4 4 4 7 */
/*:ref: mxm_ 14 3 7 7 7 */
/*:ref: m2eul_ 14 7 7 4 4 4 7 7 7 */
 
extern logical bodfnd_(integer *body, char *item, ftnlen item_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: intstr_ 14 3 4 13 124 */
/*:ref: suffix_ 14 5 13 4 13 124 124 */
/*:ref: dtpool_ 14 6 13 12 4 13 124 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int bodmat_(integer *body, doublereal *et, doublereal *tipm);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: irfnum_ 14 3 13 4 124 */
/*:ref: pckmat_ 14 5 4 7 4 7 12 */
/*:ref: zzbodbry_ 4 1 4 */
/*:ref: repmi_ 14 7 13 13 4 13 124 124 124 */
/*:ref: gdpool_ 14 7 13 4 4 4 7 12 124 */
/*:ref: spd_ 7 0 */
/*:ref: j2000_ 7 0 */
/*:ref: cleard_ 14 2 4 7 */
/*:ref: bodvcd_ 14 6 4 13 4 4 7 124 */
/*:ref: bodfnd_ 12 3 4 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: rpd_ 7 0 */
/*:ref: vdotg_ 7 3 7 7 4 */
/*:ref: twopi_ 7 0 */
/*:ref: halfpi_ 7 0 */
/*:ref: eul2m_ 14 7 7 7 7 4 4 4 7 */
/*:ref: irfrot_ 14 3 4 4 7 */
/*:ref: mxm_ 14 3 7 7 7 */
/*:ref: moved_ 14 3 7 4 7 */
 
extern int bodn2c_(char *name__, integer *code, logical *found, ftnlen name_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: zzbodn2c_ 14 4 13 4 12 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int bods2c_(char *name__, integer *code, logical *found, ftnlen name_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: zzbodn2c_ 14 4 13 4 12 124 */
/*:ref: beint_ 12 2 13 124 */
/*:ref: nparsi_ 14 6 13 4 13 4 124 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int bodvar_(integer *body, char *item, integer *dim, doublereal *values, ftnlen item_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: intstr_ 14 3 4 13 124 */
/*:ref: suffix_ 14 5 13 4 13 124 124 */
/*:ref: rtpool_ 14 5 13 4 7 12 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int bodvcd_(integer *bodyid, char *item, integer *maxn, integer *dim, doublereal *values, ftnlen item_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: intstr_ 14 3 4 13 124 */
/*:ref: suffix_ 14 5 13 4 13 124 124 */
/*:ref: dtpool_ 14 6 13 12 4 13 124 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: gdpool_ 14 7 13 4 4 4 7 12 124 */
 
extern int bodvrd_(char *bodynm, char *item, integer *maxn, integer *dim, doublereal *values, ftnlen bodynm_len, ftnlen item_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: bods2c_ 14 4 13 4 12 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: intstr_ 14 3 4 13 124 */
/*:ref: suffix_ 14 5 13 4 13 124 124 */
/*:ref: dtpool_ 14 6 13 12 4 13 124 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: gdpool_ 14 7 13 4 4 4 7 12 124 */
 
extern doublereal brcktd_(doublereal *number, doublereal *end1, doublereal *end2);
 
extern integer brckti_(integer *number, integer *end1, integer *end2);
 
extern integer bschoc_(char *value, integer *ndim, char *array, integer *order, ftnlen value_len, ftnlen array_len);
 
extern integer bschoi_(integer *value, integer *ndim, integer *array, integer *order);
 
extern integer bsrchc_(char *value, integer *ndim, char *array, ftnlen value_len, ftnlen array_len);
 
extern integer bsrchd_(doublereal *value, integer *ndim, doublereal *array);
 
extern integer bsrchi_(integer *value, integer *ndim, integer *array);
 
extern integer cardc_(char *cell, ftnlen cell_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dechar_ 14 3 13 4 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern integer cardd_(doublereal *cell);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern integer cardi_(integer *cell);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int cgv2el_(doublereal *center, doublereal *vec1, doublereal *vec2, doublereal *ellips);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: vequ_ 14 2 7 7 */
/*:ref: saelgv_ 14 4 7 7 7 7 */
/*:ref: chkout_ 14 2 13 124 */
 
extern integer chbase_(void);
 
extern int chbder_(doublereal *cp, integer *degp, doublereal *x2s, doublereal *x, integer *nderiv, doublereal *partdp, doublereal *dpdxs);
 
extern int chbint_(doublereal *cp, integer *degp, doublereal *x2s, doublereal *x, doublereal *p, doublereal *dpdx);
 
extern int chbval_(doublereal *cp, integer *degp, doublereal *x2s, doublereal *x, doublereal *p);
 
extern int chckid_(char *class__, integer *maxlen, char *id, ftnlen class_len, ftnlen id_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: frstnp_ 4 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: lastnb_ 4 2 13 124 */
 
extern int chgirf_(integer *refa, integer *refb, doublereal *rotab, char *name__, integer *index, ftnlen name_len);
extern int irfrot_(integer *refa, integer *refb, doublereal *rotab);
extern int irfnum_(char *name__, integer *index, ftnlen name_len);
extern int irfnam_(integer *index, char *name__, ftnlen name_len);
extern int irfdef_(integer *index);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: rotate_ 14 3 7 4 7 */
/*:ref: wdcnt_ 4 2 13 124 */
/*:ref: nthwd_ 14 6 13 4 13 4 124 124 */
/*:ref: nparsi_ 14 6 13 4 13 4 124 124 */
/*:ref: nparsd_ 14 6 13 7 13 4 124 124 */
/*:ref: convrt_ 14 6 7 13 13 7 124 124 */
/*:ref: rotmat_ 14 4 7 7 4 7 */
/*:ref: moved_ 14 3 7 4 7 */
/*:ref: isrchc_ 4 5 13 4 13 124 124 */
/*:ref: mxm_ 14 3 7 7 7 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: mxmt_ 14 3 7 7 7 */
/*:ref: eqstr_ 12 4 13 13 124 124 */
/*:ref: esrchc_ 4 5 13 4 13 124 124 */
 
extern int ckbsr_(char *fname, integer *handle, integer *inst, doublereal *sclkdp, doublereal *tol, logical *needav, doublereal *descr, char *segid, logical *found, ftnlen fname_len, ftnlen segid_len);
extern int cklpf_(char *fname, integer *handle, ftnlen fname_len);
extern int ckupf_(integer *handle);
extern int ckbss_(integer *inst, doublereal *sclkdp, doublereal *tol, logical *needav);
extern int cksns_(integer *handle, doublereal *descr, char *segid, logical *found, ftnlen segid_len);
extern int ckhave_(logical *found);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: lnktl_ 4 2 4 4 */
/*:ref: lnkfsl_ 14 3 4 4 4 */
/*:ref: lnkini_ 14 2 4 4 */
/*:ref: dafopr_ 14 3 13 4 124 */
/*:ref: failed_ 12 0 */
/*:ref: isrchi_ 4 3 4 4 4 */
/*:ref: dafcls_ 14 1 4 */
/*:ref: lnknxt_ 4 2 4 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: intmax_ 4 0 */
/*:ref: moved_ 14 3 7 4 7 */
/*:ref: lnkprv_ 4 2 4 4 */
/*:ref: dpmin_ 7 0 */
/*:ref: dpmax_ 7 0 */
/*:ref: cleard_ 14 2 4 7 */
/*:ref: dafbfs_ 14 1 4 */
/*:ref: daffna_ 14 1 12 */
/*:ref: dafgs_ 14 1 7 */
/*:ref: dafus_ 14 5 7 4 4 7 4 */
/*:ref: dafbbs_ 14 1 4 */
/*:ref: daffpa_ 14 1 12 */
/*:ref: dafps_ 14 5 4 4 7 4 7 */
/*:ref: lnknfn_ 4 1 4 */
/*:ref: lnkan_ 14 2 4 4 */
/*:ref: dafgn_ 14 2 13 124 */
/*:ref: lnkilb_ 14 3 4 4 4 */
/*:ref: lnkila_ 14 3 4 4 4 */
 
extern int ckcls_(integer *handle);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dafbfs_ 14 1 4 */
/*:ref: daffna_ 14 1 12 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errhan_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: dafcls_ 14 1 4 */
 
extern int ckcov_(char *ck, integer *idcode, logical *needav, char *level, doublereal *tol, char *timsys, doublereal *cover, ftnlen ck_len, ftnlen level_len, ftnlen timsys_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: eqstr_ 12 4 13 13 124 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: getfat_ 14 6 13 13 13 124 124 124 */
/*:ref: ckmeta_ 14 4 4 13 4 124 */
/*:ref: failed_ 12 0 */
/*:ref: dafopr_ 14 3 13 4 124 */
/*:ref: dafbfs_ 14 1 4 */
/*:ref: daffna_ 14 1 12 */
/*:ref: dafgs_ 14 1 7 */
/*:ref: dafus_ 14 5 7 4 4 7 4 */
/*:ref: sct2e_ 14 3 4 7 7 */
/*:ref: wninsd_ 14 3 7 7 7 */
/*:ref: zzckcv01_ 14 8 4 4 4 4 7 13 7 124 */
/*:ref: zzckcv02_ 14 8 4 4 4 4 7 13 7 124 */
/*:ref: zzckcv03_ 14 8 4 4 4 4 7 13 7 124 */
/*:ref: zzckcv04_ 14 8 4 4 4 4 7 13 7 124 */
/*:ref: zzckcv05_ 14 9 4 4 4 4 7 7 13 7 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: dafcls_ 14 1 4 */
 
extern int cke01_(logical *needav, doublereal *record, doublereal *cmat, doublereal *av, doublereal *clkout);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: q2m_ 14 2 7 7 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int cke02_(logical *needav, doublereal *record, doublereal *cmat, doublereal *av, doublereal *clkout);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: vequg_ 14 3 7 4 7 */
/*:ref: vequ_ 14 2 7 7 */
/*:ref: vnorm_ 7 1 7 */
/*:ref: axisar_ 14 3 7 7 7 */
/*:ref: q2m_ 14 2 7 7 */
/*:ref: mxmt_ 14 3 7 7 7 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int cke03_(logical *needav, doublereal *record, doublereal *cmat, doublereal *av, doublereal *clkout);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: moved_ 14 3 7 4 7 */
/*:ref: q2m_ 14 2 7 7 */
/*:ref: vequ_ 14 2 7 7 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: mtxm_ 14 3 7 7 7 */
/*:ref: raxisa_ 14 3 7 7 7 */
/*:ref: failed_ 12 0 */
/*:ref: axisar_ 14 3 7 7 7 */
/*:ref: mxmt_ 14 3 7 7 7 */
/*:ref: vlcom_ 14 5 7 7 7 7 7 */
 
extern int cke04_(logical *needav, doublereal *record, doublereal *cmat, doublereal *av, doublereal *clkout);
/*:ref: chbval_ 14 5 7 4 7 7 7 */
/*:ref: vhatg_ 14 3 7 4 7 */
/*:ref: q2m_ 14 2 7 7 */
 
extern int cke05_(logical *needav, doublereal *record, doublereal *cmat, doublereal *av, doublereal *clkout);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: vminug_ 14 3 7 4 7 */
/*:ref: vdistg_ 7 3 7 7 4 */
/*:ref: moved_ 14 3 7 4 7 */
/*:ref: xpsgip_ 14 3 4 4 7 */
/*:ref: lgrind_ 14 7 4 7 7 7 7 7 7 */
/*:ref: vnormg_ 7 2 7 4 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: vsclg_ 14 4 7 7 4 7 */
/*:ref: vdotg_ 7 3 7 7 4 */
/*:ref: vsubg_ 14 4 7 7 4 7 */
/*:ref: qdq2av_ 14 3 7 7 7 */
/*:ref: vsclip_ 14 2 7 7 */
/*:ref: lgrint_ 7 5 4 7 7 7 7 */
/*:ref: vhatg_ 14 3 7 4 7 */
/*:ref: vscl_ 14 3 7 7 7 */
/*:ref: hrmint_ 14 7 4 7 7 7 7 7 7 */
/*:ref: q2m_ 14 2 7 7 */
 
extern int ckfrot_(integer *inst, doublereal *et, doublereal *rotate, integer *ref, logical *found);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: ckhave_ 14 1 12 */
/*:ref: ckmeta_ 14 4 4 13 4 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzsclk_ 12 2 4 4 */
/*:ref: sce2c_ 14 3 4 7 7 */
/*:ref: ckbss_ 14 4 4 7 7 12 */
/*:ref: cksns_ 14 5 4 7 13 12 124 */
/*:ref: failed_ 12 0 */
/*:ref: ckpfs_ 14 9 4 7 7 7 12 7 7 7 12 */
/*:ref: dafus_ 14 5 7 4 4 7 4 */
/*:ref: xpose_ 14 2 7 7 */
 
extern int ckfxfm_(integer *inst, doublereal *et, doublereal *xform, integer *ref, logical *found);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: ckmeta_ 14 4 4 13 4 124 */
/*:ref: ckhave_ 14 1 12 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzsclk_ 12 2 4 4 */
/*:ref: sce2c_ 14 3 4 7 7 */
/*:ref: ckbss_ 14 4 4 7 7 12 */
/*:ref: cksns_ 14 5 4 7 13 12 124 */
/*:ref: failed_ 12 0 */
/*:ref: ckpfs_ 14 9 4 7 7 7 12 7 7 7 12 */
/*:ref: dafus_ 14 5 7 4 4 7 4 */
/*:ref: rav2xf_ 14 3 7 7 7 */
/*:ref: invstm_ 14 2 7 7 */
 
extern int ckgp_(integer *inst, doublereal *sclkdp, doublereal *tol, char *ref, doublereal *cmat, doublereal *clkout, logical *found, ftnlen ref_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: ckbss_ 14 4 4 7 7 12 */
/*:ref: cksns_ 14 5 4 7 13 12 124 */
/*:ref: failed_ 12 0 */
/*:ref: ckpfs_ 14 9 4 7 7 7 12 7 7 7 12 */
/*:ref: dafus_ 14 5 7 4 4 7 4 */
/*:ref: namfrm_ 14 3 13 4 124 */
/*:ref: frinfo_ 14 5 4 4 4 4 12 */
/*:ref: ckmeta_ 14 4 4 13 4 124 */
/*:ref: sct2e_ 14 3 4 7 7 */
/*:ref: frmchg_ 14 4 4 4 7 7 */
/*:ref: xf2rav_ 14 3 7 7 7 */
/*:ref: mxm_ 14 3 7 7 7 */
/*:ref: moved_ 14 3 7 4 7 */
 
extern int ckgpav_(integer *inst, doublereal *sclkdp, doublereal *tol, char *ref, doublereal *cmat, doublereal *av, doublereal *clkout, logical *found, ftnlen ref_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: ckbss_ 14 4 4 7 7 12 */
/*:ref: cksns_ 14 5 4 7 13 12 124 */
/*:ref: failed_ 12 0 */
/*:ref: ckpfs_ 14 9 4 7 7 7 12 7 7 7 12 */
/*:ref: dafus_ 14 5 7 4 4 7 4 */
/*:ref: namfrm_ 14 3 13 4 124 */
/*:ref: frinfo_ 14 5 4 4 4 4 12 */
/*:ref: ckmeta_ 14 4 4 13 4 124 */
/*:ref: sct2e_ 14 3 4 7 7 */
/*:ref: frmchg_ 14 4 4 4 7 7 */
/*:ref: xf2rav_ 14 3 7 7 7 */
/*:ref: mxm_ 14 3 7 7 7 */
/*:ref: moved_ 14 3 7 4 7 */
/*:ref: mtxv_ 14 3 7 7 7 */
/*:ref: vadd_ 14 3 7 7 7 */
 
extern int ckgr01_(integer *handle, doublereal *descr, integer *recno, doublereal *record);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dafus_ 14 5 7 4 4 7 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dafgda_ 14 4 4 4 4 7 */
 
extern int ckgr02_(integer *handle, doublereal *descr, integer *recno, doublereal *record);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dafus_ 14 5 7 4 4 7 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: cknr02_ 14 3 4 7 4 */
/*:ref: dafgda_ 14 4 4 4 4 7 */
/*:ref: moved_ 14 3 7 4 7 */
 
extern int ckgr03_(integer *handle, doublereal *descr, integer *recno, doublereal *record);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dafus_ 14 5 7 4 4 7 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dafgda_ 14 4 4 4 4 7 */
 
extern int ckgr04_(integer *handle, doublereal *descr, integer *recno, doublereal *record);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dafus_ 14 5 7 4 4 7 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: cknr04_ 14 3 4 7 4 */
/*:ref: sgfpkt_ 14 6 4 7 4 4 7 4 */
/*:ref: zzck4d2i_ 14 4 7 4 7 4 */
 
extern int ckgr05_(integer *handle, doublereal *descr, integer *recno, doublereal *record);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dafus_ 14 5 7 4 4 7 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dafgda_ 14 4 4 4 4 7 */
 
extern int ckmeta_(integer *ckid, char *meta, integer *idcode, ftnlen meta_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: bschoi_ 4 4 4 4 4 4 */
/*:ref: cvpool_ 14 3 13 12 124 */
/*:ref: gipool_ 14 7 13 4 4 4 4 12 124 */
/*:ref: orderi_ 14 3 4 4 4 */
/*:ref: intstr_ 14 3 4 13 124 */
/*:ref: prefix_ 14 5 13 4 13 124 124 */
/*:ref: suffix_ 14 5 13 4 13 124 124 */
/*:ref: swpool_ 14 5 13 4 13 124 124 */
/*:ref: cmprss_ 14 7 13 4 13 13 124 124 124 */
/*:ref: ljust_ 14 4 13 13 124 124 */
/*:ref: ucase_ 14 4 13 13 124 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int cknr01_(integer *handle, doublereal *descr, integer *nrec);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dafus_ 14 5 7 4 4 7 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dafgda_ 14 4 4 4 4 7 */
 
extern int cknr02_(integer *handle, doublereal *descr, integer *nrec);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dafus_ 14 5 7 4 4 7 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int cknr03_(integer *handle, doublereal *descr, integer *nrec);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dafus_ 14 5 7 4 4 7 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dafgda_ 14 4 4 4 4 7 */
 
extern int cknr04_(integer *handle, doublereal *descr, integer *nrec);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dafus_ 14 5 7 4 4 7 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: sgmeta_ 14 4 4 7 4 4 */
 
extern int cknr05_(integer *handle, doublereal *descr, integer *nrec);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dafus_ 14 5 7 4 4 7 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dafgda_ 14 4 4 4 4 7 */
 
extern int ckobj_(char *ck, integer *ids, ftnlen ck_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: getfat_ 14 6 13 13 13 124 124 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dafopr_ 14 3 13 4 124 */
/*:ref: failed_ 12 0 */
/*:ref: dafbfs_ 14 1 4 */
/*:ref: daffna_ 14 1 12 */
/*:ref: dafgs_ 14 1 7 */
/*:ref: dafus_ 14 5 7 4 4 7 4 */
/*:ref: insrti_ 14 2 4 4 */
/*:ref: dafcls_ 14 1 4 */
 
extern int ckopn_(char *name__, char *ifname, integer *ncomch, integer *handle, ftnlen name_len, ftnlen ifname_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dafonw_ 14 10 13 13 4 4 13 4 4 124 124 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int ckpfs_(integer *handle, doublereal *descr, doublereal *sclkdp, doublereal *tol, logical *needav, doublereal *cmat, doublereal *av, doublereal *clkout, logical *found);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dafus_ 14 5 7 4 4 7 4 */
/*:ref: ckr01_ 14 7 4 7 7 7 12 7 12 */
/*:ref: cke01_ 14 5 12 7 7 7 7 */
/*:ref: ckr02_ 14 6 4 7 7 7 7 12 */
/*:ref: cke02_ 14 5 12 7 7 7 7 */
/*:ref: ckr03_ 14 7 4 7 7 7 12 7 12 */
/*:ref: cke03_ 14 5 12 7 7 7 7 */
/*:ref: ckr04_ 14 7 4 7 7 7 12 7 12 */
/*:ref: cke04_ 14 5 12 7 7 7 7 */
/*:ref: ckr05_ 14 7 4 7 7 7 12 7 12 */
/*:ref: cke05_ 14 5 12 7 7 7 7 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int ckr01_(integer *handle, doublereal *descr, doublereal *sclkdp, doublereal *tol, logical *needav, doublereal *record, logical *found);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dafus_ 14 5 7 4 4 7 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dafgda_ 14 4 4 4 4 7 */
/*:ref: lstled_ 4 3 7 4 7 */
/*:ref: lstcld_ 4 3 7 4 7 */
 
extern int ckr02_(integer *handle, doublereal *descr, doublereal *sclkdp, doublereal *tol, doublereal *record, logical *found);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dafus_ 14 5 7 4 4 7 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dafgda_ 14 4 4 4 4 7 */
/*:ref: lstled_ 4 3 7 4 7 */
/*:ref: vequg_ 14 3 7 4 7 */
 
extern int ckr03_(integer *handle, doublereal *descr, doublereal *sclkdp, doublereal *tol, logical *needav, doublereal *record, logical *found);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dafus_ 14 5 7 4 4 7 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dafgda_ 14 4 4 4 4 7 */
/*:ref: failed_ 12 0 */
/*:ref: lstltd_ 4 3 7 4 7 */
/*:ref: lstled_ 4 3 7 4 7 */
/*:ref: moved_ 14 3 7 4 7 */
/*:ref: dpmax_ 7 0 */
 
extern int ckr04_(integer *handle, doublereal *descr, doublereal *sclkdp, doublereal *tol, logical *needav, doublereal *record, logical *found);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dafus_ 14 5 7 4 4 7 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: cknr04_ 14 3 4 7 4 */
/*:ref: sgfrvi_ 14 6 4 7 7 7 4 12 */
/*:ref: failed_ 12 0 */
/*:ref: sgfpkt_ 14 6 4 7 4 4 7 4 */
/*:ref: zzck4d2i_ 14 4 7 4 7 4 */
 
extern int ckr05_(integer *handle, doublereal *descr, doublereal *sclkdp, doublereal *tol, logical *needav, doublereal *record, logical *found);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dafus_ 14 5 7 4 4 7 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: brcktd_ 7 3 7 7 7 */
/*:ref: dafgda_ 14 4 4 4 4 7 */
/*:ref: failed_ 12 0 */
/*:ref: odd_ 12 1 4 */
/*:ref: lstltd_ 4 3 7 4 7 */
/*:ref: lstled_ 4 3 7 4 7 */
/*:ref: dpmax_ 7 0 */
/*:ref: moved_ 14 3 7 4 7 */
 
extern int ckw01_(integer *handle, doublereal *begtim, doublereal *endtim, integer *inst, char *ref, logical *avflag, char *segid, integer *nrec, doublereal *sclkdp, doublereal *quats, doublereal *avvs, ftnlen ref_len, ftnlen segid_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: namfrm_ 14 3 13 4 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: dafps_ 14 5 4 4 7 4 7 */
/*:ref: lastnb_ 4 2 13 124 */
/*:ref: dafbna_ 14 4 4 7 13 124 */
/*:ref: failed_ 12 0 */
/*:ref: dafada_ 14 2 7 4 */
/*:ref: dafena_ 14 0 */
 
extern int ckw02_(integer *handle, doublereal *begtim, doublereal *endtim, integer *inst, char *ref, char *segid, integer *nrec, doublereal *start, doublereal *stop, doublereal *quats, doublereal *avvs, doublereal *rates, ftnlen ref_len, ftnlen segid_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: namfrm_ 14 3 13 4 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: dafps_ 14 5 4 4 7 4 7 */
/*:ref: lastnb_ 4 2 13 124 */
/*:ref: dafbna_ 14 4 4 7 13 124 */
/*:ref: failed_ 12 0 */
/*:ref: dafada_ 14 2 7 4 */
/*:ref: dafena_ 14 0 */
 
extern int ckw03_(integer *handle, doublereal *begtim, doublereal *endtim, integer *inst, char *ref, logical *avflag, char *segid, integer *nrec, doublereal *sclkdp, doublereal *quats, doublereal *avvs, integer *nints, doublereal *starts, ftnlen ref_len, ftnlen segid_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: namfrm_ 14 3 13 4 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: dafps_ 14 5 4 4 7 4 7 */
/*:ref: lastnb_ 4 2 13 124 */
/*:ref: dafbna_ 14 4 4 7 13 124 */
/*:ref: failed_ 12 0 */
/*:ref: dafada_ 14 2 7 4 */
/*:ref: dafena_ 14 0 */
 
extern int ckw04a_(integer *handle, integer *npkts, integer *pktsiz, doublereal *pktdat, doublereal *sclkdp);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: errhan_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzck4i2d_ 14 4 4 4 7 7 */
/*:ref: sgwvpk_ 14 6 4 4 4 7 4 7 */
 
extern int ckw04b_(integer *handle, doublereal *begtim, integer *inst, char *ref, logical *avflag, char *segid, ftnlen ref_len, ftnlen segid_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: namfrm_ 14 3 13 4 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dafps_ 14 5 4 4 7 4 7 */
/*:ref: lastnb_ 4 2 13 124 */
/*:ref: sgbwvs_ 14 7 4 7 13 4 7 4 124 */
 
extern int ckw04e_(integer *handle, doublereal *endtim);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: sgwes_ 14 1 4 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dafbbs_ 14 1 4 */
/*:ref: daffpa_ 14 1 12 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: dafgs_ 14 1 7 */
/*:ref: dafus_ 14 5 7 4 4 7 4 */
/*:ref: dafps_ 14 5 4 4 7 4 7 */
/*:ref: dafrs_ 14 1 7 */
 
extern int ckw05_(integer *handle, integer *subtyp, integer *degree, doublereal *begtim, doublereal *endtim, integer *inst, char *ref, logical *avflag, char *segid, integer *n, doublereal *sclkdp, doublereal *packts, doublereal *rate, integer *nints, doublereal *starts, ftnlen ref_len, ftnlen segid_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: namfrm_ 14 3 13 4 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: lastnb_ 4 2 13 124 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: bsrchd_ 4 3 7 4 7 */
/*:ref: vnormg_ 7 2 7 4 */
/*:ref: odd_ 12 1 4 */
/*:ref: lstltd_ 4 3 7 4 7 */
/*:ref: dafps_ 14 5 4 4 7 4 7 */
/*:ref: dafbna_ 14 4 4 7 13 124 */
/*:ref: failed_ 12 0 */
/*:ref: dafada_ 14 2 7 4 */
/*:ref: dafena_ 14 0 */
 
extern int clearc_(integer *ndim, char *array, ftnlen array_len);
 
extern int cleard_(integer *ndim, doublereal *array);
 
extern int cleari_(integer *ndim, integer *array);
 
extern doublereal clight_(void);
 
extern int cmprss_(char *delim, integer *n, char *input, char *output, ftnlen delim_len, ftnlen input_len, ftnlen output_len);
 
extern int conics_(doublereal *elts, doublereal *et, doublereal *state);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: vscl_ 14 3 7 7 7 */
/*:ref: twopi_ 7 0 */
/*:ref: prop2b_ 14 4 7 7 7 7 */
 
extern int convrt_(doublereal *x, char *in, char *out, doublereal *y, ftnlen in_len, ftnlen out_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dpr_ 7 0 */
/*:ref: ucase_ 14 4 13 13 124 124 */
/*:ref: isrchc_ 4 5 13 4 13 124 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int copyc_(char *cell, char *copy, ftnlen cell_len, ftnlen copy_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardc_ 4 2 13 124 */
/*:ref: sizec_ 4 2 13 124 */
/*:ref: lastpc_ 4 2 13 124 */
/*:ref: scardc_ 14 3 4 13 124 */
/*:ref: excess_ 14 3 4 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
 
extern int copyd_(doublereal *cell, doublereal *copy);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardd_ 4 1 7 */
/*:ref: sized_ 4 1 7 */
/*:ref: scardd_ 14 2 4 7 */
/*:ref: excess_ 14 3 4 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int copyi_(integer *cell, integer *copy);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardi_ 4 1 4 */
/*:ref: sizei_ 4 1 4 */
/*:ref: scardi_ 14 2 4 4 */
/*:ref: excess_ 14 3 4 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern integer countc_(integer *unit, integer *bline, integer *eline, char *line, ftnlen line_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: errfnm_ 14 3 13 4 124 */
/*:ref: lastnb_ 4 2 13 124 */
/*:ref: astrip_ 14 8 13 13 13 13 124 124 124 124 */
 
extern integer cpos_(char *str, char *chars, integer *start, ftnlen str_len, ftnlen chars_len);
 
extern integer cposr_(char *str, char *chars, integer *start, ftnlen str_len, ftnlen chars_len);
 
extern int cyacip_(integer *nelt, char *dir, integer *ncycle, char *array, ftnlen dir_len, ftnlen array_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: nbwid_ 4 3 13 4 124 */
/*:ref: gcd_ 4 2 4 4 */
 
extern int cyadip_(integer *nelt, char *dir, integer *ncycle, doublereal *array, ftnlen dir_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: gcd_ 4 2 4 4 */
 
extern int cyaiip_(integer *nelt, char *dir, integer *ncycle, integer *array, ftnlen dir_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: gcd_ 4 2 4 4 */
 
extern int cyclac_(char *array, integer *nelt, char *dir, integer *ncycle, char *out, ftnlen array_len, ftnlen dir_len, ftnlen out_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: movec_ 14 5 13 4 13 124 124 */
/*:ref: nbwid_ 4 3 13 4 124 */
/*:ref: gcd_ 4 2 4 4 */
 
extern int cyclad_(doublereal *array, integer *nelt, char *dir, integer *ncycle, doublereal *out, ftnlen dir_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: moved_ 14 3 7 4 7 */
/*:ref: gcd_ 4 2 4 4 */
 
extern int cyclai_(integer *array, integer *nelt, char *dir, integer *ncycle, integer *out, ftnlen dir_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: movei_ 14 3 4 4 4 */
/*:ref: gcd_ 4 2 4 4 */
 
extern int cyclec_(char *instr, char *dir, integer *ncycle, char *outstr, ftnlen instr_len, ftnlen dir_len, ftnlen outstr_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: gcd_ 4 2 4 4 */
 
extern int cyllat_(doublereal *r__, doublereal *longc, doublereal *z__, doublereal *radius, doublereal *long__, doublereal *lat);
 
extern int cylrec_(doublereal *r__, doublereal *long__, doublereal *z__, doublereal *rectan);
 
extern int cylsph_(doublereal *r__, doublereal *longc, doublereal *z__, doublereal *radius, doublereal *colat, doublereal *long__);
 
extern doublereal dacosh_(doublereal *x);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int dafa2b_(char *ascii, char *binary, integer *resv, ftnlen ascii_len, ftnlen binary_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: txtopr_ 14 3 13 4 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: daft2b_ 14 4 4 13 4 124 */
 
extern int dafah_(char *fname, char *ftype, integer *nd, integer *ni, char *ifname, integer *resv, integer *handle, integer *unit, integer *fhset, char *access, ftnlen fname_len, ftnlen ftype_len, ftnlen ifname_len, ftnlen access_len);
extern int dafopr_(char *fname, integer *handle, ftnlen fname_len);
extern int dafopw_(char *fname, integer *handle, ftnlen fname_len);
extern int dafonw_(char *fname, char *ftype, integer *nd, integer *ni, char *ifname, integer *resv, integer *handle, ftnlen fname_len, ftnlen ftype_len, ftnlen ifname_len);
extern int dafopn_(char *fname, integer *nd, integer *ni, char *ifname, integer *resv, integer *handle, ftnlen fname_len, ftnlen ifname_len);
extern int dafcls_(integer *handle);
extern int dafhsf_(integer *handle, integer *nd, integer *ni);
extern int dafhlu_(integer *handle, integer *unit);
extern int dafluh_(integer *unit, integer *handle);
extern int dafhfn_(integer *handle, char *fname, ftnlen fname_len);
extern int daffnh_(char *fname, integer *handle, ftnlen fname_len);
extern int dafhof_(integer *fhset);
extern int dafsih_(integer *handle, char *access, ftnlen access_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: ssizei_ 14 2 4 4 */
/*:ref: zzddhopn_ 14 7 13 13 13 4 124 124 124 */
/*:ref: failed_ 12 0 */
/*:ref: isrchi_ 4 3 4 4 4 */
/*:ref: zzdafgfr_ 14 11 4 13 4 4 13 4 4 4 12 124 124 */
/*:ref: zzddhcls_ 14 4 4 13 12 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: insrti_ 14 2 4 4 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: ltrim_ 4 2 13 124 */
/*:ref: rtrim_ 4 2 13 124 */
/*:ref: cleard_ 14 2 4 7 */
/*:ref: dafrwa_ 14 3 4 4 4 */
/*:ref: zzddhhlu_ 14 5 4 13 12 4 124 */
/*:ref: zzplatfm_ 14 4 13 13 124 124 */
/*:ref: zzdafnfr_ 14 12 4 13 4 4 13 4 4 4 13 124 124 124 */
/*:ref: removi_ 14 2 4 4 */
/*:ref: zzddhluh_ 14 3 4 4 12 */
/*:ref: zzddhnfo_ 14 7 4 13 4 4 4 12 124 */
/*:ref: errfnm_ 14 3 13 4 124 */
/*:ref: zzddhfnh_ 14 4 13 4 12 124 */
/*:ref: copyi_ 14 2 4 4 */
/*:ref: ljust_ 14 4 13 13 124 124 */
/*:ref: ucase_ 14 4 13 13 124 124 */
/*:ref: elemi_ 12 2 4 4 */
 
extern int dafana_(integer *handle, doublereal *sum, char *name__, doublereal *data, integer *n, ftnlen name_len);
extern int dafbna_(integer *handle, doublereal *sum, char *name__, ftnlen name_len);
extern int dafada_(doublereal *data, integer *n);
extern int dafena_(void);
extern int dafcad_(integer *handle);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dafsih_ 14 3 4 13 124 */
/*:ref: failed_ 12 0 */
/*:ref: ssizei_ 14 2 4 4 */
/*:ref: dafrfr_ 14 8 4 4 4 13 4 4 4 124 */
/*:ref: dafhof_ 14 1 4 */
/*:ref: elemi_ 12 2 4 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errhan_ 14 3 13 4 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: dafhsf_ 14 3 4 4 4 */
/*:ref: moved_ 14 3 7 4 7 */
/*:ref: dafhfn_ 14 3 4 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: dafwda_ 14 4 4 4 4 7 */
/*:ref: dafus_ 14 5 7 4 4 7 4 */
/*:ref: dafps_ 14 5 4 4 7 4 7 */
/*:ref: dafrdr_ 14 6 4 4 4 4 7 12 */
/*:ref: dafrcr_ 14 4 4 4 13 124 */
/*:ref: dafwdr_ 14 3 4 4 7 */
/*:ref: dafwcr_ 14 4 4 4 13 124 */
/*:ref: dafarw_ 14 3 4 4 4 */
/*:ref: cleard_ 14 2 4 7 */
/*:ref: dafrwa_ 14 3 4 4 4 */
/*:ref: dafwfr_ 14 8 4 4 4 13 4 4 4 124 */
 
extern int dafarr_(integer *handle, integer *resv);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dafsih_ 14 3 4 13 124 */
/*:ref: failed_ 12 0 */
/*:ref: dafrfr_ 14 8 4 4 4 13 4 4 4 124 */
/*:ref: dafarw_ 14 3 4 4 4 */
/*:ref: dafwdr_ 14 3 4 4 7 */
/*:ref: dafrdr_ 14 6 4 4 4 4 7 12 */
/*:ref: dafrcr_ 14 4 4 4 13 124 */
/*:ref: dafwcr_ 14 4 4 4 13 124 */
/*:ref: dafwfr_ 14 8 4 4 4 13 4 4 4 124 */
/*:ref: dafbfs_ 14 1 4 */
/*:ref: daffna_ 14 1 12 */
/*:ref: dafgs_ 14 1 7 */
/*:ref: dafus_ 14 5 7 4 4 7 4 */
/*:ref: dafps_ 14 5 4 4 7 4 7 */
/*:ref: dafws_ 14 1 7 */
 
extern int dafb2a_(char *binary, char *ascii, ftnlen binary_len, ftnlen ascii_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: txtopn_ 14 3 13 4 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dafb2t_ 14 3 13 4 124 */
 
extern int dafb2t_(char *binary, integer *text, ftnlen binary_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dafopr_ 14 3 13 4 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzddhhlu_ 14 5 4 13 12 4 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: dafrfr_ 14 8 4 4 4 13 4 4 4 124 */
/*:ref: dafcls_ 14 1 4 */
/*:ref: errfnm_ 14 3 13 4 124 */
/*:ref: dafbfs_ 14 1 4 */
/*:ref: daffna_ 14 1 12 */
/*:ref: dafgs_ 14 1 7 */
/*:ref: dafgn_ 14 2 13 124 */
/*:ref: dafus_ 14 5 7 4 4 7 4 */
/*:ref: dafgda_ 14 4 4 4 4 7 */
 
extern int dafbt_(char *binfil, integer *xfrlun, ftnlen binfil_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dafopr_ 14 3 13 4 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzddhhlu_ 14 5 4 13 12 4 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errfnm_ 14 3 13 4 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: dafrfr_ 14 8 4 4 4 13 4 4 4 124 */
/*:ref: wrenci_ 14 3 4 4 4 */
/*:ref: dafbfs_ 14 1 4 */
/*:ref: daffna_ 14 1 12 */
/*:ref: dafgs_ 14 1 7 */
/*:ref: dafus_ 14 5 7 4 4 7 4 */
/*:ref: dafgn_ 14 2 13 124 */
/*:ref: repmi_ 14 7 13 13 4 13 124 124 124 */
/*:ref: rtrim_ 4 2 13 124 */
/*:ref: wrencd_ 14 3 4 4 7 */
/*:ref: dafgda_ 14 4 4 4 4 7 */
/*:ref: dafcls_ 14 1 4 */
 
extern int daffa_(integer *handle, doublereal *sum, char *name__, logical *found, ftnlen name_len);
extern int dafbfs_(integer *handle);
extern int daffna_(logical *found);
extern int dafbbs_(integer *handle);
extern int daffpa_(logical *found);
extern int dafgs_(doublereal *sum);
extern int dafgn_(char *name__, ftnlen name_len);
extern int dafgh_(integer *handle);
extern int dafrs_(doublereal *sum);
extern int dafrn_(char *name__, ftnlen name_len);
extern int dafws_(doublereal *sum);
extern int dafcs_(integer *handle);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dafsih_ 14 3 4 13 124 */
/*:ref: failed_ 12 0 */
/*:ref: ssizei_ 14 2 4 4 */
/*:ref: dafhof_ 14 1 4 */
/*:ref: elemi_ 12 2 4 4 */
/*:ref: dafrfr_ 14 8 4 4 4 13 4 4 4 124 */
/*:ref: dafgsr_ 14 6 4 4 4 4 7 12 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: dafhfn_ 14 3 4 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: dafhsf_ 14 3 4 4 4 */
/*:ref: moved_ 14 3 7 4 7 */
/*:ref: dafrcr_ 14 4 4 4 13 124 */
/*:ref: dafus_ 14 5 7 4 4 7 4 */
/*:ref: movei_ 14 3 4 4 4 */
/*:ref: dafps_ 14 5 4 4 7 4 7 */
/*:ref: dafwdr_ 14 3 4 4 7 */
/*:ref: dafwcr_ 14 4 4 4 13 124 */
 
extern int dafgda_(integer *handle, integer *begin, integer *end, doublereal *data);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dafarw_ 14 3 4 4 4 */
/*:ref: dafgdr_ 14 6 4 4 4 4 7 12 */
/*:ref: cleard_ 14 2 4 7 */
 
extern int dafps_(integer *nd, integer *ni, doublereal *dc, integer *ic, doublereal *sum);
extern int dafus_(doublereal *sum, integer *nd, integer *ni, doublereal *dc, integer *ic);
/*:ref: moved_ 14 3 7 4 7 */
/*:ref: movei_ 14 3 4 4 4 */
 
extern int dafra_(integer *handle, integer *iorder, integer *n);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: isordv_ 12 2 4 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: dafbfs_ 14 1 4 */
/*:ref: daffna_ 14 1 12 */
/*:ref: failed_ 12 0 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: dafgs_ 14 1 7 */
/*:ref: dafgn_ 14 2 13 124 */
/*:ref: dafws_ 14 1 7 */
/*:ref: dafrn_ 14 2 13 124 */
 
extern int dafrcr_(integer *handle, integer *recno, char *crec, ftnlen crec_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dafsih_ 14 3 4 13 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: zzddhhlu_ 14 5 4 13 12 4 124 */
 
extern int dafrda_(integer *handle, integer *begin, integer *end, doublereal *data);
/*:ref: return_ 12 0 */
/*:ref: zzddhisn_ 14 3 4 12 12 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errhan_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: dafarw_ 14 3 4 4 4 */
/*:ref: dafrdr_ 14 6 4 4 4 4 7 12 */
/*:ref: cleard_ 14 2 4 7 */
 
extern int dafrfr_(integer *handle, integer *nd, integer *ni, char *ifname, integer *fward, integer *bward, integer *free, ftnlen ifname_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dafsih_ 14 3 4 13 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzdafgfr_ 14 11 4 13 4 4 13 4 4 4 12 124 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
 
extern int dafrrr_(integer *handle, integer *resv);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dafsih_ 14 3 4 13 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dafrfr_ 14 8 4 4 4 13 4 4 4 124 */
/*:ref: dafrdr_ 14 6 4 4 4 4 7 12 */
/*:ref: dafarw_ 14 3 4 4 4 */
/*:ref: dafwdr_ 14 3 4 4 7 */
/*:ref: dafrcr_ 14 4 4 4 13 124 */
/*:ref: dafwcr_ 14 4 4 4 13 124 */
/*:ref: dafwfr_ 14 8 4 4 4 13 4 4 4 124 */
/*:ref: dafbfs_ 14 1 4 */
/*:ref: daffna_ 14 1 12 */
/*:ref: dafgs_ 14 1 7 */
/*:ref: dafus_ 14 5 7 4 4 7 4 */
/*:ref: dafps_ 14 5 4 4 7 4 7 */
/*:ref: dafws_ 14 1 7 */
 
extern int dafrwa_(integer *recno, integer *wordno, integer *addr__);
extern int dafarw_(integer *addr__, integer *recno, integer *wordno);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int dafrwd_(integer *handle, integer *recno, integer *begin, integer *end, doublereal *drec, doublereal *data, logical *found, integer *reads, integer *reqs);
extern int dafgdr_(integer *handle, integer *recno, integer *begin, integer *end, doublereal *data, logical *found);
extern int dafgsr_(integer *handle, integer *recno, integer *begin, integer *end, doublereal *data, logical *found);
extern int dafrdr_(integer *handle, integer *recno, integer *begin, integer *end, doublereal *data, logical *found);
extern int dafwdr_(integer *handle, integer *recno, doublereal *drec);
extern int dafnrr_(integer *reads, integer *reqs);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: minai_ 14 4 4 4 4 4 */
/*:ref: zzdafgdr_ 14 4 4 4 7 12 */
/*:ref: failed_ 12 0 */
/*:ref: moved_ 14 3 7 4 7 */
/*:ref: zzddhrcm_ 14 3 4 4 4 */
/*:ref: dafhsf_ 14 3 4 4 4 */
/*:ref: zzdafgsr_ 14 6 4 4 4 4 7 12 */
/*:ref: zzddhisn_ 14 3 4 12 12 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errhan_ 14 3 13 4 124 */
/*:ref: zzddhhlu_ 14 5 4 13 12 4 124 */
/*:ref: errint_ 14 3 13 4 124 */
 
extern int daft2b_(integer *text, char *binary, integer *resv, ftnlen binary_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errfnm_ 14 3 13 4 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: idw2at_ 14 6 13 13 13 124 124 124 */
/*:ref: dafopn_ 14 8 13 4 4 13 4 4 124 124 */
/*:ref: failed_ 12 0 */
/*:ref: dafcls_ 14 1 4 */
/*:ref: dafps_ 14 5 4 4 7 4 7 */
/*:ref: dafbna_ 14 4 4 7 13 124 */
/*:ref: dafada_ 14 2 7 4 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: dafena_ 14 0 */
 
extern int daftb_(integer *xfrlun, char *binfil, ftnlen binfil_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errfnm_ 14 3 13 4 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: idw2at_ 14 6 13 13 13 124 124 124 */
/*:ref: rdenci_ 14 3 4 4 4 */
/*:ref: failed_ 12 0 */
/*:ref: dafonw_ 14 10 13 13 4 4 13 4 4 124 124 124 */
/*:ref: dafopn_ 14 8 13 4 4 13 4 4 124 124 */
/*:ref: nextwd_ 14 6 13 13 13 124 124 124 */
/*:ref: nparsi_ 14 6 13 4 13 4 124 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: rdencd_ 14 3 4 4 7 */
/*:ref: dafps_ 14 5 4 4 7 4 7 */
/*:ref: dafbna_ 14 4 4 7 13 124 */
/*:ref: dafada_ 14 2 7 4 */
/*:ref: dafena_ 14 0 */
/*:ref: dafcls_ 14 1 4 */
 
extern int dafwcr_(integer *handle, integer *recno, char *crec, ftnlen crec_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: zzddhhlu_ 14 5 4 13 12 4 124 */
/*:ref: dafsih_ 14 3 4 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int dafwda_(integer *handle, integer *begin, integer *end, doublereal *data);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dafarw_ 14 3 4 4 4 */
/*:ref: dafrdr_ 14 6 4 4 4 4 7 12 */
/*:ref: cleard_ 14 2 4 7 */
/*:ref: moved_ 14 3 7 4 7 */
/*:ref: dafwdr_ 14 3 4 4 7 */
 
extern int dafwfr_(integer *handle, integer *nd, integer *ni, char *ifname, integer *fward, integer *bward, integer *free, ftnlen ifname_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dafsih_ 14 3 4 13 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzddhhlu_ 14 5 4 13 12 4 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errfnm_ 14 3 13 4 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
 
extern int dasa2l_(integer *handle, integer *type__, integer *addrss, integer *clbase, integer *clsize, integer *recno, integer *wordno);
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dashlu_ 14 2 4 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: errfnm_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: isrchi_ 4 3 4 4 4 */
/*:ref: dasham_ 14 3 4 13 124 */
/*:ref: dashfs_ 14 9 4 4 4 4 4 4 4 4 4 */
/*:ref: dasrri_ 14 5 4 4 4 4 4 */
 
extern int dasac_(integer *handle, integer *n, char *buffer, ftnlen buffer_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dassih_ 14 3 4 13 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dashlu_ 14 2 4 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errfnm_ 14 3 13 4 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: lastnb_ 4 2 13 124 */
/*:ref: dasrfr_ 14 9 4 13 13 4 4 4 4 124 124 */
/*:ref: dasacr_ 14 2 4 4 */
/*:ref: dasioc_ 14 6 13 4 4 13 124 124 */
/*:ref: daswfr_ 14 9 4 13 13 4 4 4 4 124 124 */
 
extern int dasacr_(integer *handle, integer *n);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dassih_ 14 3 4 13 124 */
/*:ref: dashlu_ 14 2 4 4 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: daswbr_ 14 1 4 */
/*:ref: dashfs_ 14 9 4 4 4 4 4 4 4 4 4 */
/*:ref: maxai_ 14 4 4 4 4 4 */
/*:ref: cleari_ 14 2 4 4 */
/*:ref: dasioi_ 14 5 13 4 4 4 124 */
/*:ref: dasioc_ 14 6 13 4 4 13 124 124 */
/*:ref: dasiod_ 14 5 13 4 4 7 124 */
/*:ref: dasufs_ 14 9 4 4 4 4 4 4 4 4 4 */
 
extern int dasacu_(integer *comlun, char *begmrk, char *endmrk, logical *insbln, integer *handle, ftnlen begmrk_len, ftnlen endmrk_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dassih_ 14 3 4 13 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dasrfr_ 14 9 4 13 13 4 4 4 4 124 124 */
/*:ref: getlun_ 14 1 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: readln_ 14 4 4 13 12 124 */
/*:ref: ljust_ 14 4 13 13 124 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: errfnm_ 14 3 13 4 124 */
/*:ref: readla_ 14 6 4 4 4 13 12 124 */
/*:ref: lastnb_ 4 2 13 124 */
/*:ref: writla_ 14 4 4 13 4 124 */
/*:ref: dasac_ 14 4 4 4 13 124 */
 
extern int dasadc_(integer *handle, integer *n, integer *bpos, integer *epos, char *data, ftnlen data_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dashfs_ 14 9 4 4 4 4 4 4 4 4 4 */
/*:ref: dasa2l_ 14 7 4 4 4 4 4 4 4 */
/*:ref: failed_ 12 0 */
/*:ref: daswrc_ 14 4 4 4 13 124 */
/*:ref: dasurc_ 14 6 4 4 4 4 13 124 */
/*:ref: dascud_ 14 3 4 4 4 */
 
extern int dasadd_(integer *handle, integer *n, doublereal *data);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dashfs_ 14 9 4 4 4 4 4 4 4 4 4 */
/*:ref: dasa2l_ 14 7 4 4 4 4 4 4 4 */
/*:ref: failed_ 12 0 */
/*:ref: moved_ 14 3 7 4 7 */
/*:ref: daswrd_ 14 3 4 4 7 */
/*:ref: dasurd_ 14 5 4 4 4 4 7 */
/*:ref: dascud_ 14 3 4 4 4 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int dasadi_(integer *handle, integer *n, integer *data);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dashfs_ 14 9 4 4 4 4 4 4 4 4 4 */
/*:ref: dasa2l_ 14 7 4 4 4 4 4 4 4 */
/*:ref: failed_ 12 0 */
/*:ref: movei_ 14 3 4 4 4 */
/*:ref: daswri_ 14 3 4 4 4 */
/*:ref: dasuri_ 14 5 4 4 4 4 4 */
/*:ref: dascud_ 14 3 4 4 4 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int dasbt_(char *binfil, integer *xfrlun, ftnlen binfil_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dasopr_ 14 3 13 4 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dasrfr_ 14 9 4 13 13 4 4 4 4 124 124 */
/*:ref: dashlu_ 14 2 4 4 */
/*:ref: dascls_ 14 1 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: errfnm_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: wrenci_ 14 3 4 4 4 */
/*:ref: repmi_ 14 7 13 13 4 13 124 124 124 */
/*:ref: rtrim_ 4 2 13 124 */
/*:ref: dasioc_ 14 6 13 4 4 13 124 124 */
/*:ref: wrencc_ 14 4 4 4 13 124 */
/*:ref: daslla_ 14 4 4 4 4 4 */
/*:ref: dasrdc_ 14 7 4 4 4 4 4 13 124 */
/*:ref: dasrdd_ 14 4 4 4 4 7 */
/*:ref: wrencd_ 14 3 4 4 7 */
/*:ref: dasrdi_ 14 4 4 4 4 4 */
 
extern int dascls_(integer *handle);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: ssizei_ 14 2 4 4 */
/*:ref: dashof_ 14 1 4 */
/*:ref: elemi_ 12 2 4 4 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dasham_ 14 3 4 13 124 */
/*:ref: daswbr_ 14 1 4 */
/*:ref: dashlu_ 14 2 4 4 */
/*:ref: failed_ 12 0 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: dassdr_ 14 1 4 */
/*:ref: dasllc_ 14 1 4 */
 
extern int dascud_(integer *handle, integer *type__, integer *nwords);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dashfs_ 14 9 4 4 4 4 4 4 4 4 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: maxai_ 14 4 4 4 4 4 */
/*:ref: dasuri_ 14 5 4 4 4 4 4 */
/*:ref: dasufs_ 14 9 4 4 4 4 4 4 4 4 4 */
/*:ref: dasrri_ 14 5 4 4 4 4 4 */
/*:ref: cleari_ 14 2 4 4 */
/*:ref: daswri_ 14 3 4 4 4 */
 
extern int dasdc_(integer *handle);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dassih_ 14 3 4 13 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dasrfr_ 14 9 4 13 13 4 4 4 4 124 124 */
/*:ref: dasrcr_ 14 2 4 4 */
/*:ref: daswfr_ 14 9 4 13 13 4 4 4 4 124 124 */
 
extern int dasec_(integer *handle, integer *bufsiz, integer *n, char *buffer, logical *done, ftnlen buffer_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dassih_ 14 3 4 13 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: dashlu_ 14 2 4 4 */
/*:ref: isrchi_ 4 3 4 4 4 */
/*:ref: dasrfr_ 14 9 4 13 13 4 4 4 4 124 124 */
/*:ref: dasioc_ 14 6 13 4 4 13 124 124 */
/*:ref: errfnm_ 14 3 13 4 124 */
 
extern int dasecu_(integer *handle, integer *comlun, logical *comnts);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dassih_ 14 3 4 13 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dasec_ 14 6 4 4 4 13 12 124 */
/*:ref: writla_ 14 4 4 13 4 124 */
 
extern int dasfm_(char *fname, char *ftype, char *ifname, integer *handle, integer *unit, integer *free, integer *lastla, integer *lastrc, integer *lastwd, integer *nresvr, integer *nresvc, integer *ncomr, integer *ncomc, integer *fhset, char *access, ftnlen fname_len, ftnlen ftype_len, ftnlen ifname_len, ftnlen access_len);
extern int dasopr_(char *fname, integer *handle, ftnlen fname_len);
extern int dasopw_(char *fname, integer *handle, ftnlen fname_len);
extern int dasonw_(char *fname, char *ftype, char *ifname, integer *ncomr, integer *handle, ftnlen fname_len, ftnlen ftype_len, ftnlen ifname_len);
extern int dasopn_(char *fname, char *ifname, integer *handle, ftnlen fname_len, ftnlen ifname_len);
extern int dasops_(integer *handle);
extern int dasllc_(integer *handle);
extern int dashfs_(integer *handle, integer *nresvr, integer *nresvc, integer *ncomr, integer *ncomc, integer *free, integer *lastla, integer *lastrc, integer *lastwd);
extern int dasufs_(integer *handle, integer *nresvr, integer *nresvc, integer *ncomr, integer *ncomc, integer *free, integer *lastla, integer *lastrc, integer *lastwd);
extern int dashlu_(integer *handle, integer *unit);
extern int dasluh_(integer *unit, integer *handle);
extern int dashfn_(integer *handle, char *fname, ftnlen fname_len);
extern int dasfnh_(char *fname, integer *handle, ftnlen fname_len);
extern int dashof_(integer *fhset);
extern int dassih_(integer *handle, char *access, ftnlen access_len);
extern int dasham_(integer *handle, char *access, ftnlen access_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: lnkini_ 14 2 4 4 */
/*:ref: ssizei_ 14 2 4 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: exists_ 12 2 13 124 */
/*:ref: rtrim_ 4 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: lnknxt_ 4 2 4 4 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: lnknfn_ 4 1 4 */
/*:ref: getlun_ 14 1 4 */
/*:ref: failed_ 12 0 */
/*:ref: zzddhppf_ 14 3 4 4 4 */
/*:ref: zzplatfm_ 14 4 13 13 124 124 */
/*:ref: isrchc_ 4 5 13 4 13 124 124 */
/*:ref: idw2at_ 14 6 13 13 13 124 124 124 */
/*:ref: lnkan_ 14 2 4 4 */
/*:ref: lnkilb_ 14 3 4 4 4 */
/*:ref: cleari_ 14 2 4 4 */
/*:ref: dasioi_ 14 5 13 4 4 4 124 */
/*:ref: maxai_ 14 4 4 4 4 4 */
/*:ref: insrti_ 14 2 4 4 */
/*:ref: ltrim_ 4 2 13 124 */
/*:ref: zzdasnfr_ 14 11 4 13 13 4 4 4 4 13 124 124 124 */
/*:ref: lnkfsl_ 14 3 4 4 4 */
/*:ref: removi_ 14 2 4 4 */
/*:ref: errfnm_ 14 3 13 4 124 */
/*:ref: copyi_ 14 2 4 4 */
/*:ref: ljust_ 14 4 13 13 124 124 */
/*:ref: ucase_ 14 4 13 13 124 124 */
/*:ref: elemi_ 12 2 4 4 */
 
extern int dasioc_(char *action, integer *unit, integer *recno, char *record, ftnlen action_len, ftnlen record_len);
/*:ref: return_ 12 0 */
/*:ref: eqstr_ 12 4 13 13 124 124 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errfnm_ 14 3 13 4 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
 
extern int dasiod_(char *action, integer *unit, integer *recno, doublereal *record, ftnlen action_len);
/*:ref: return_ 12 0 */
/*:ref: eqstr_ 12 4 13 13 124 124 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errfnm_ 14 3 13 4 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
 
extern int dasioi_(char *action, integer *unit, integer *recno, integer *record, ftnlen action_len);
/*:ref: return_ 12 0 */
/*:ref: eqstr_ 12 4 13 13 124 124 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errfnm_ 14 3 13 4 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
 
extern int daslla_(integer *handle, integer *lastc, integer *lastd, integer *lasti);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dashfs_ 14 9 4 4 4 4 4 4 4 4 4 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int dasrcr_(integer *handle, integer *n);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dassih_ 14 3 4 13 124 */
/*:ref: dashlu_ 14 2 4 4 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: daswbr_ 14 1 4 */
/*:ref: dashfs_ 14 9 4 4 4 4 4 4 4 4 4 */
/*:ref: maxai_ 14 4 4 4 4 4 */
/*:ref: cleari_ 14 2 4 4 */
/*:ref: dasioi_ 14 5 13 4 4 4 124 */
/*:ref: dasioc_ 14 6 13 4 4 13 124 124 */
/*:ref: dasiod_ 14 5 13 4 4 7 124 */
/*:ref: dasufs_ 14 9 4 4 4 4 4 4 4 4 4 */
 
extern int dasrdc_(integer *handle, integer *first, integer *last, integer *bpos, integer *epos, char *data, ftnlen data_len);
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dasa2l_ 14 7 4 4 4 4 4 4 4 */
/*:ref: failed_ 12 0 */
/*:ref: dasrrc_ 14 6 4 4 4 4 13 124 */
 
extern int dasrdd_(integer *handle, integer *first, integer *last, doublereal *data);
/*:ref: dasa2l_ 14 7 4 4 4 4 4 4 4 */
/*:ref: dasrrd_ 14 5 4 4 4 4 7 */
/*:ref: failed_ 12 0 */
 
extern int dasrdi_(integer *handle, integer *first, integer *last, integer *data);
/*:ref: dasa2l_ 14 7 4 4 4 4 4 4 4 */
/*:ref: dasrri_ 14 5 4 4 4 4 4 */
/*:ref: failed_ 12 0 */
 
extern int dasrfr_(integer *handle, char *idword, char *ifname, integer *nresvr, integer *nresvc, integer *ncomr, integer *ncomc, ftnlen idword_len, ftnlen ifname_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dashlu_ 14 2 4 4 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errfnm_ 14 3 13 4 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
 
extern int dasrwr_(integer *handle, integer *recno, char *recc, doublereal *recd, integer *reci, integer *first, integer *last, doublereal *datad, integer *datai, char *datac, ftnlen recc_len, ftnlen datac_len);
extern int dasrrd_(integer *handle, integer *recno, integer *first, integer *last, doublereal *datad);
extern int dasrri_(integer *handle, integer *recno, integer *first, integer *last, integer *datai);
extern int dasrrc_(integer *handle, integer *recno, integer *first, integer *last, char *datac, ftnlen datac_len);
extern int daswrd_(integer *handle, integer *recno, doublereal *recd);
extern int daswri_(integer *handle, integer *recno, integer *reci);
extern int daswrc_(integer *handle, integer *recno, char *recc, ftnlen recc_len);
extern int dasurd_(integer *handle, integer *recno, integer *first, integer *last, doublereal *datad);
extern int dasuri_(integer *handle, integer *recno, integer *first, integer *last, integer *datai);
extern int dasurc_(integer *handle, integer *recno, integer *first, integer *last, char *datac, ftnlen datac_len);
extern int daswbr_(integer *handle);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: lnkini_ 14 2 4 4 */
/*:ref: dashlu_ 14 2 4 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: errfnm_ 14 3 13 4 124 */
/*:ref: lnkxsl_ 14 3 4 4 4 */
/*:ref: lnkilb_ 14 3 4 4 4 */
/*:ref: moved_ 14 3 7 4 7 */
/*:ref: lnktl_ 4 2 4 4 */
/*:ref: dasiod_ 14 5 13 4 4 7 124 */
/*:ref: lnkan_ 14 2 4 4 */
/*:ref: failed_ 12 0 */
/*:ref: movei_ 14 3 4 4 4 */
/*:ref: dasioi_ 14 5 13 4 4 4 124 */
/*:ref: dasioc_ 14 6 13 4 4 13 124 124 */
/*:ref: dassih_ 14 3 4 13 124 */
/*:ref: lnkfsl_ 14 3 4 4 4 */
 
extern int dassdr_(integer *handle);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dassih_ 14 3 4 13 124 */
/*:ref: dashlu_ 14 2 4 4 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: daswbr_ 14 1 4 */
/*:ref: dasops_ 14 1 4 */
/*:ref: cleari_ 14 2 4 4 */
/*:ref: dashfs_ 14 9 4 4 4 4 4 4 4 4 4 */
/*:ref: maxai_ 14 4 4 4 4 4 */
/*:ref: dasrri_ 14 5 4 4 4 4 4 */
/*:ref: dasadi_ 14 3 4 4 4 */
/*:ref: sumai_ 4 2 4 4 */
/*:ref: dasrdi_ 14 4 4 4 4 4 */
/*:ref: dasudi_ 14 4 4 4 4 4 */
/*:ref: dasioc_ 14 6 13 4 4 13 124 124 */
/*:ref: dasiod_ 14 5 13 4 4 7 124 */
/*:ref: dasioi_ 14 5 13 4 4 4 124 */
/*:ref: dasufs_ 14 9 4 4 4 4 4 4 4 4 4 */
/*:ref: dasllc_ 14 1 4 */
 
extern int dastb_(integer *xfrlun, char *binfil, ftnlen binfil_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errfnm_ 14 3 13 4 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: idw2at_ 14 6 13 13 13 124 124 124 */
/*:ref: dasonw_ 14 8 13 13 13 4 4 124 124 124 */
/*:ref: failed_ 12 0 */
/*:ref: daswfr_ 14 9 4 13 13 4 4 4 4 124 124 */
/*:ref: dascls_ 14 1 4 */
/*:ref: rdenci_ 14 3 4 4 4 */
/*:ref: dashlu_ 14 2 4 4 */
/*:ref: dasacr_ 14 2 4 4 */
/*:ref: nextwd_ 14 6 13 13 13 124 124 124 */
/*:ref: nparsi_ 14 6 13 4 13 4 124 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: rdencc_ 14 4 4 4 13 124 */
/*:ref: dasioc_ 14 6 13 4 4 13 124 124 */
/*:ref: dasadc_ 14 6 4 4 4 4 13 124 */
/*:ref: rdencd_ 14 3 4 4 7 */
/*:ref: dasadd_ 14 3 4 4 7 */
/*:ref: dasadi_ 14 3 4 4 4 */
 
extern int dasudc_(integer *handle, integer *first, integer *last, integer *bpos, integer *epos, char *data, ftnlen data_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: daslla_ 14 4 4 4 4 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dasa2l_ 14 7 4 4 4 4 4 4 4 */
/*:ref: failed_ 12 0 */
/*:ref: dasurc_ 14 6 4 4 4 4 13 124 */
 
extern int dasudd_(integer *handle, integer *first, integer *last, doublereal *data);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: daslla_ 14 4 4 4 4 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dasa2l_ 14 7 4 4 4 4 4 4 4 */
/*:ref: failed_ 12 0 */
/*:ref: dasurd_ 14 5 4 4 4 4 7 */
 
extern int dasudi_(integer *handle, integer *first, integer *last, integer *data);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: daslla_ 14 4 4 4 4 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dasa2l_ 14 7 4 4 4 4 4 4 4 */
/*:ref: failed_ 12 0 */
/*:ref: dasuri_ 14 5 4 4 4 4 4 */
 
extern int daswfr_(integer *handle, char *idword, char *ifname, integer *nresvr, integer *nresvc, integer *ncomr, integer *ncomc, ftnlen idword_len, ftnlen ifname_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dassih_ 14 3 4 13 124 */
/*:ref: dashlu_ 14 2 4 4 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errfnm_ 14 3 13 4 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: dashfs_ 14 9 4 4 4 4 4 4 4 4 4 */
/*:ref: dasufs_ 14 9 4 4 4 4 4 4 4 4 4 */
 
extern doublereal datanh_(doublereal *x);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern doublereal dcbrt_(doublereal *x);
 
extern int dcyldr_(doublereal *x, doublereal *y, doublereal *z__, doublereal *jacobi);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: vpack_ 14 4 7 7 7 7 */
/*:ref: reccyl_ 14 4 7 7 7 7 */
/*:ref: drdcyl_ 14 4 7 7 7 7 */
/*:ref: invort_ 14 2 7 7 */
 
extern int delfil_(char *filnam, ftnlen filnam_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: getlun_ 14 1 4 */
 
extern int deltet_(doublereal *epoch, char *eptype, doublereal *delta, ftnlen eptype_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: ucase_ 14 4 13 13 124 124 */
/*:ref: gdpool_ 14 7 13 4 4 4 7 12 124 */
/*:ref: dtpool_ 14 6 13 12 4 13 124 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
 
extern doublereal det_(doublereal *m1);
 
extern int dgeodr_(doublereal *x, doublereal *y, doublereal *z__, doublereal *re, doublereal *f, doublereal *jacobi);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: vpack_ 14 4 7 7 7 7 */
/*:ref: recgeo_ 14 6 7 7 7 7 7 7 */
/*:ref: drdgeo_ 14 6 7 7 7 7 7 7 */
/*:ref: invort_ 14 2 7 7 */
 
extern int diags2_(doublereal *symmat, doublereal *diag, doublereal *rotate);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: moved_ 14 3 7 4 7 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: rquad_ 14 5 7 7 7 7 7 */
/*:ref: vhatg_ 14 3 7 4 7 */
 
extern int diffc_(char *a, char *b, char *c__, ftnlen a_len, ftnlen b_len, ftnlen c_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: cardc_ 4 2 13 124 */
/*:ref: sizec_ 4 2 13 124 */
/*:ref: scardc_ 14 3 4 13 124 */
/*:ref: excess_ 14 3 4 13 124 */
 
extern int diffd_(doublereal *a, doublereal *b, doublereal *c__);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardd_ 4 1 7 */
/*:ref: sized_ 4 1 7 */
/*:ref: scardd_ 14 2 4 7 */
/*:ref: excess_ 14 3 4 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int diffi_(integer *a, integer *b, integer *c__);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardi_ 4 1 4 */
/*:ref: sizei_ 4 1 4 */
/*:ref: scardi_ 14 2 4 4 */
/*:ref: excess_ 14 3 4 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int dlatdr_(doublereal *x, doublereal *y, doublereal *z__, doublereal *jacobi);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: vpack_ 14 4 7 7 7 7 */
/*:ref: reclat_ 14 4 7 7 7 7 */
/*:ref: drdlat_ 14 4 7 7 7 7 */
/*:ref: invort_ 14 2 7 7 */
 
extern int dnearp_(doublereal *state, doublereal *a, doublereal *b, doublereal *c__, doublereal *dnear, doublereal *dalt, logical *found);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: nearpt_ 14 6 7 7 7 7 7 7 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: vsub_ 14 3 7 7 7 */
/*:ref: mxv_ 14 3 7 7 7 */
/*:ref: unorm_ 14 3 7 7 7 */
/*:ref: vdot_ 7 2 7 7 */
/*:ref: vtmv_ 7 3 7 7 7 */
/*:ref: vlcom_ 14 5 7 7 7 7 7 */
 
extern int dp2hx_(doublereal *number, char *string, integer *length, ftnlen string_len);
/*:ref: int2hx_ 14 4 4 13 4 124 */
 
extern int dpfmt_(doublereal *x, char *pictur, char *str, ftnlen pictur_len, ftnlen str_len);
/*:ref: pos_ 4 5 13 13 4 124 124 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: zzvststr_ 14 4 7 13 4 124 */
/*:ref: dpstr_ 14 4 7 4 13 124 */
/*:ref: ljust_ 14 4 13 13 124 124 */
/*:ref: rjust_ 14 4 13 13 124 124 */
/*:ref: zzvsbstr_ 14 6 4 4 12 13 12 124 */
/*:ref: ncpos_ 4 5 13 13 4 124 124 */
 
extern int dpgrdr_(char *body, doublereal *x, doublereal *y, doublereal *z__, doublereal *re, doublereal *f, doublereal *jacobi, ftnlen body_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: bods2c_ 14 4 13 4 12 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: repmi_ 14 7 13 13 4 13 124 124 124 */
/*:ref: gcpool_ 14 8 13 4 4 4 13 12 124 124 */
/*:ref: cmprss_ 14 7 13 4 13 13 124 124 124 */
/*:ref: ucase_ 14 4 13 13 124 124 */
/*:ref: plnsns_ 4 1 4 */
/*:ref: dgeodr_ 14 6 7 7 7 7 7 7 */
 
extern doublereal dpr_(void);
 
extern int dpspce_(doublereal *time, doublereal *geophs, doublereal *elems, doublereal *state);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: twopi_ 7 0 */
/*:ref: halfpi_ 7 0 */
/*:ref: zzdpinit_ 14 6 7 7 7 7 7 7 */
/*:ref: zzdpper_ 14 6 7 7 7 7 7 7 */
/*:ref: zzdpsec_ 14 9 7 7 7 7 7 7 7 7 7 */
/*:ref: latrec_ 14 4 7 7 7 7 */
/*:ref: vlcom_ 14 5 7 7 7 7 7 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int dpstr_(doublereal *x, integer *sigdig, char *string, ftnlen string_len);
/*:ref: intstr_ 14 3 4 13 124 */
 
extern int dpstrf_(doublereal *x, integer *sigdig, char *format, char *string, ftnlen format_len, ftnlen string_len);
/*:ref: dpstr_ 14 4 7 4 13 124 */
/*:ref: zzvststr_ 14 4 7 13 4 124 */
/*:ref: zzvsbstr_ 14 6 4 4 12 13 12 124 */
 
extern int drdcyl_(doublereal *r__, doublereal *long__, doublereal *z__, doublereal *jacobi);
 
extern int drdgeo_(doublereal *long__, doublereal *lat, doublereal *alt, doublereal *re, doublereal *f, doublereal *jacobi);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int drdlat_(doublereal *r__, doublereal *long__, doublereal *lat, doublereal *jacobi);
 
extern int drdpgr_(char *body, doublereal *lon, doublereal *lat, doublereal *alt, doublereal *re, doublereal *f, doublereal *jacobi, ftnlen body_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: bods2c_ 14 4 13 4 12 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: repmi_ 14 7 13 13 4 13 124 124 124 */
/*:ref: gcpool_ 14 8 13 4 4 4 13 12 124 124 */
/*:ref: cmprss_ 14 7 13 4 13 13 124 124 124 */
/*:ref: ucase_ 14 4 13 13 124 124 */
/*:ref: plnsns_ 4 1 4 */
/*:ref: drdgeo_ 14 6 7 7 7 7 7 7 */
 
extern int drdsph_(doublereal *r__, doublereal *colat, doublereal *long__, doublereal *jacobi);
 
extern int drotat_(doublereal *angle, integer *iaxis, doublereal *dmout);
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int dsphdr_(doublereal *x, doublereal *y, doublereal *z__, doublereal *jacobi);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: vpack_ 14 4 7 7 7 7 */
/*:ref: recsph_ 14 4 7 7 7 7 */
/*:ref: drdsph_ 14 4 7 7 7 7 */
/*:ref: invort_ 14 2 7 7 */
 
extern int ducrss_(doublereal *s1, doublereal *s2, doublereal *sout);
/*:ref: dvcrss_ 14 3 7 7 7 */
/*:ref: dvhat_ 14 2 7 7 */
 
extern int dvcrss_(doublereal *s1, doublereal *s2, doublereal *sout);
/*:ref: vcrss_ 14 3 7 7 7 */
/*:ref: vequ_ 14 2 7 7 */
/*:ref: vadd_ 14 3 7 7 7 */
 
extern doublereal dvdot_(doublereal *s1, doublereal *s2);
 
extern int dvhat_(doublereal *s1, doublereal *sout);
/*:ref: unorm_ 14 3 7 7 7 */
/*:ref: vequ_ 14 2 7 7 */
/*:ref: vperp_ 14 3 7 7 7 */
/*:ref: vsclip_ 14 2 7 7 */
 
extern int dxtrct_(char *keywd, integer *maxwds, char *string, integer *nfound, integer *parsed, doublereal *values, ftnlen keywd_len, ftnlen string_len);
/*:ref: wdindx_ 4 4 13 13 124 124 */
/*:ref: lastnb_ 4 2 13 124 */
/*:ref: nblen_ 4 2 13 124 */
/*:ref: fndnwd_ 14 5 13 4 4 4 124 */
/*:ref: nparsd_ 14 6 13 7 13 4 124 124 */
 
extern int edlimb_(doublereal *a, doublereal *b, doublereal *c__, doublereal *viewpt, doublereal *limb);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: vscl_ 14 3 7 7 7 */
/*:ref: nvc2pl_ 14 3 7 7 7 */
/*:ref: inedpl_ 14 6 7 7 7 7 7 12 */
/*:ref: vsclg_ 14 4 7 7 4 7 */
/*:ref: moved_ 14 3 7 4 7 */
 
extern int ekacec_(integer *handle, integer *segno, integer *recno, char *column, integer *nvals, char *cvals, logical *isnull, ftnlen column_len, ftnlen cvals_len);
/*:ref: zzeksdsc_ 14 3 4 4 4 */
/*:ref: zzekcdsc_ 14 5 4 4 13 4 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dashlu_ 14 2 4 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: errfnm_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzektrdp_ 14 4 4 4 4 4 */
/*:ref: zzekad03_ 14 7 4 4 4 4 13 12 124 */
/*:ref: zzekad06_ 14 8 4 4 4 4 4 13 12 124 */
 
extern int ekaced_(integer *handle, integer *segno, integer *recno, char *column, integer *nvals, doublereal *dvals, logical *isnull, ftnlen column_len);
/*:ref: zzeksdsc_ 14 3 4 4 4 */
/*:ref: zzekcdsc_ 14 5 4 4 13 4 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dashlu_ 14 2 4 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: errfnm_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzektrdp_ 14 4 4 4 4 4 */
/*:ref: zzekad02_ 14 6 4 4 4 4 7 12 */
/*:ref: zzekad05_ 14 7 4 4 4 4 4 7 12 */
 
extern int ekacei_(integer *handle, integer *segno, integer *recno, char *column, integer *nvals, integer *ivals, logical *isnull, ftnlen column_len);
/*:ref: zzeksdsc_ 14 3 4 4 4 */
/*:ref: zzekcdsc_ 14 5 4 4 13 4 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dashlu_ 14 2 4 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: errfnm_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzektrdp_ 14 4 4 4 4 4 */
/*:ref: zzekad01_ 14 6 4 4 4 4 4 12 */
/*:ref: zzekad04_ 14 7 4 4 4 4 4 4 12 */
 
extern int ekaclc_(integer *handle, integer *segno, char *column, char *cvals, integer *entszs, logical *nlflgs, integer *rcptrs, integer *wkindx, ftnlen column_len, ftnlen cvals_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: zzeksdsc_ 14 3 4 4 4 */
/*:ref: zzekcdsc_ 14 5 4 4 13 4 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: zzekac03_ 14 8 4 4 4 13 12 4 4 124 */
/*:ref: zzekac06_ 14 7 4 4 4 13 4 12 124 */
/*:ref: zzekac09_ 14 7 4 4 4 13 12 4 124 */
 
extern int ekacld_(integer *handle, integer *segno, char *column, doublereal *dvals, integer *entszs, logical *nlflgs, integer *rcptrs, integer *wkindx, ftnlen column_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: zzeksdsc_ 14 3 4 4 4 */
/*:ref: zzekcdsc_ 14 5 4 4 13 4 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: zzekac02_ 14 7 4 4 4 7 12 4 4 */
/*:ref: zzekac05_ 14 6 4 4 4 7 4 12 */
/*:ref: zzekac08_ 14 6 4 4 4 7 12 4 */
 
extern int ekacli_(integer *handle, integer *segno, char *column, integer *ivals, integer *entszs, logical *nlflgs, integer *rcptrs, integer *wkindx, ftnlen column_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: zzeksdsc_ 14 3 4 4 4 */
/*:ref: zzekcdsc_ 14 5 4 4 13 4 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: zzekac01_ 14 7 4 4 4 4 12 4 4 */
/*:ref: zzekac04_ 14 6 4 4 4 4 4 12 */
/*:ref: zzekac07_ 14 6 4 4 4 4 12 4 */
 
extern int ekappr_(integer *handle, integer *segno, integer *recno);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: zzekpgch_ 14 3 4 13 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzekmloc_ 14 4 4 4 4 4 */
/*:ref: dasrdi_ 14 4 4 4 4 4 */
/*:ref: ekinsr_ 14 3 4 4 4 */
 
extern int ekbseg_(integer *handle, char *tabnam, integer *ncols, char *cnames, char *decls, integer *segno, ftnlen tabnam_len, ftnlen cnames_len, ftnlen decls_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: zzekpgch_ 14 3 4 13 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: ssizei_ 14 2 4 4 */
/*:ref: lxdfid_ 14 1 4 */
/*:ref: chckid_ 14 5 13 4 13 124 124 */
/*:ref: lxidnt_ 14 6 4 13 4 4 4 124 */
/*:ref: lastnb_ 4 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: zzekpdec_ 14 3 13 4 124 */
/*:ref: zzekstyp_ 4 2 4 4 */
/*:ref: zzekbs01_ 14 8 4 13 4 13 4 4 124 124 */
/*:ref: zzekbs02_ 14 8 4 13 4 13 4 4 124 124 */
 
extern int ekcls_(integer *handle);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dascls_ 14 1 4 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int ekdelr_(integer *handle, integer *segno, integer *recno);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: zzekpgch_ 14 3 4 13 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzekmloc_ 14 4 4 4 4 4 */
/*:ref: dasrdi_ 14 4 4 4 4 4 */
/*:ref: zzekrbck_ 14 6 13 4 4 4 4 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: zzektrdp_ 14 4 4 4 4 4 */
/*:ref: zzekde01_ 14 4 4 4 4 4 */
/*:ref: zzekde02_ 14 4 4 4 4 4 */
/*:ref: zzekde03_ 14 4 4 4 4 4 */
/*:ref: zzekde04_ 14 4 4 4 4 4 */
/*:ref: zzekde05_ 14 4 4 4 4 4 */
/*:ref: zzekde06_ 14 4 4 4 4 4 */
/*:ref: zzekrp2n_ 4 3 4 4 4 */
/*:ref: dashlu_ 14 2 4 4 */
/*:ref: zzekcnam_ 14 4 4 4 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: errfnm_ 14 3 13 4 124 */
/*:ref: zzekpgpg_ 14 4 4 4 4 4 */
/*:ref: zzekglnk_ 14 4 4 4 4 4 */
/*:ref: zzekslnk_ 14 4 4 4 4 4 */
/*:ref: zzekdps_ 14 4 4 4 4 4 */
/*:ref: zzektrdl_ 14 3 4 4 4 */
/*:ref: dasudi_ 14 4 4 4 4 4 */
 
extern int ekffld_(integer *handle, integer *segno, integer *rcptrs);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: zzeksdsc_ 14 3 4 4 4 */
/*:ref: zzekff01_ 14 3 4 4 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int ekfind_(char *query, integer *nmrows, logical *error, char *errmsg, ftnlen query_len, ftnlen errmsg_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: zzekqini_ 14 6 4 4 4 13 7 124 */
/*:ref: zzekscan_ 14 17 13 4 4 4 4 4 4 4 7 13 4 4 12 13 124 124 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzekpars_ 14 19 13 4 4 4 4 4 7 13 4 4 4 13 7 12 13 124 124 124 124 */
/*:ref: zzeknres_ 14 9 13 4 13 12 13 4 124 124 124 */
/*:ref: zzektres_ 14 10 13 4 13 7 12 13 4 124 124 124 */
/*:ref: zzeksemc_ 14 9 13 4 13 12 13 4 124 124 124 */
/*:ref: eksrch_ 14 8 4 13 7 4 12 13 124 124 */
 
extern int ekifld_(integer *handle, char *tabnam, integer *ncols, integer *nrows, char *cnames, char *decls, integer *segno, integer *rcptrs, ftnlen tabnam_len, ftnlen cnames_len, ftnlen decls_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: ekbseg_ 14 9 4 13 4 13 13 4 124 124 124 */
/*:ref: failed_ 12 0 */
/*:ref: zzekmloc_ 14 4 4 4 4 4 */
/*:ref: dasudi_ 14 4 4 4 4 4 */
/*:ref: zzeksdsc_ 14 3 4 4 4 */
/*:ref: zzekif01_ 14 3 4 4 4 */
/*:ref: zzekif02_ 14 2 4 4 */
 
extern int ekinsr_(integer *handle, integer *segno, integer *recno);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: zzekpgch_ 14 3 4 13 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzekmloc_ 14 4 4 4 4 4 */
/*:ref: dasrdi_ 14 4 4 4 4 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: cleari_ 14 2 4 4 */
/*:ref: filli_ 14 3 4 4 4 */
/*:ref: ekshdw_ 14 2 4 12 */
/*:ref: zzekpgbs_ 14 3 4 4 4 */
/*:ref: dasudi_ 14 4 4 4 4 4 */
/*:ref: zzekaps_ 14 6 4 4 4 12 4 4 */
/*:ref: zzektrin_ 14 4 4 4 4 4 */
/*:ref: zzekrbck_ 14 6 13 4 4 4 4 124 */
 
extern integer eknseg_(integer *handle);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: zzekpgch_ 14 3 4 13 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzektrbs_ 4 1 4 */
/*:ref: dasrdi_ 14 4 4 4 4 4 */
/*:ref: zzektrsz_ 4 2 4 4 */
 
extern int ekopn_(char *fname, char *ifname, integer *ncomch, integer *handle, ftnlen fname_len, ftnlen ifname_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dasonw_ 14 8 13 13 13 4 4 124 124 124 */
/*:ref: failed_ 12 0 */
/*:ref: zzekpgin_ 14 1 4 */
/*:ref: zzekpgan_ 14 4 4 4 4 4 */
/*:ref: zzektrit_ 14 2 4 4 */
/*:ref: dasudi_ 14 4 4 4 4 4 */
 
extern int ekopr_(char *fname, integer *handle, ftnlen fname_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dasopr_ 14 3 13 4 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzekpgch_ 14 3 4 13 124 */
 
extern int ekops_(integer *handle);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dasops_ 14 1 4 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzekpgin_ 14 1 4 */
/*:ref: zzekpgan_ 14 4 4 4 4 4 */
/*:ref: zzektrit_ 14 2 4 4 */
/*:ref: dasudi_ 14 4 4 4 4 4 */
 
extern int ekopw_(char *fname, integer *handle, ftnlen fname_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dasopw_ 14 3 13 4 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzekpgch_ 14 3 4 13 124 */
 
extern int ekpsel_(char *query, integer *n, integer *xbegs, integer *xends, char *xtypes, char *xclass, char *tabs, char *cols, logical *error, char *errmsg, ftnlen query_len, ftnlen xtypes_len, ftnlen xclass_len, ftnlen tabs_len, ftnlen cols_len, ftnlen errmsg_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: zzekqini_ 14 6 4 4 4 13 7 124 */
/*:ref: zzekencd_ 14 10 13 4 13 7 12 13 4 124 124 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzekreqi_ 14 4 4 13 4 124 */
/*:ref: zzekqsel_ 14 12 4 13 4 4 4 13 4 13 4 124 124 124 */
/*:ref: zzekqtab_ 14 8 4 13 4 13 13 124 124 124 */
/*:ref: ekcii_ 14 6 13 4 13 4 124 124 */
 
extern int ekqmgr_(integer *cindex, integer *elment, char *eqryc, doublereal *eqryd, integer *eqryi, char *fname, integer *row, integer *selidx, char *column, integer *handle, integer *n, char *table, integer *attdsc, integer *ccount, logical *found, integer *nelt, integer *nmrows, logical *semerr, char *errmsg, char *cdata, doublereal *ddata, integer *idata, logical *null, ftnlen eqryc_len, ftnlen fname_len, ftnlen column_len, ftnlen table_len, ftnlen errmsg_len, ftnlen cdata_len);
extern int eklef_(char *fname, integer *handle, ftnlen fname_len);
extern int ekuef_(integer *handle);
extern int ekntab_(integer *n);
extern int ektnam_(integer *n, char *table, ftnlen table_len);
extern int ekccnt_(char *table, integer *ccount, ftnlen table_len);
extern int ekcii_(char *table, integer *cindex, char *column, integer *attdsc, ftnlen table_len, ftnlen column_len);
extern int eksrch_(integer *eqryi, char *eqryc, doublereal *eqryd, integer *nmrows, logical *semerr, char *errmsg, ftnlen eqryc_len, ftnlen errmsg_len);
extern int eknelt_(integer *selidx, integer *row, integer *nelt);
extern int ekgc_(integer *selidx, integer *row, integer *elment, char *cdata, logical *null, logical *found, ftnlen cdata_len);
extern int ekgd_(integer *selidx, integer *row, integer *elment, doublereal *ddata, logical *null, logical *found);
extern int ekgi_(integer *selidx, integer *row, integer *elment, integer *idata, logical *null, logical *found);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: lnkini_ 14 2 4 4 */
/*:ref: ekopr_ 14 3 13 4 124 */
/*:ref: failed_ 12 0 */
/*:ref: dascls_ 14 1 4 */
/*:ref: lnknxt_ 4 2 4 4 */
/*:ref: zzekpgch_ 14 3 4 13 124 */
/*:ref: lnknfn_ 4 1 4 */
/*:ref: dashlu_ 14 2 4 4 */
/*:ref: ekcls_ 14 1 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errfnm_ 14 3 13 4 124 */
/*:ref: eknseg_ 4 1 4 */
/*:ref: lnkan_ 14 2 4 4 */
/*:ref: lnkilb_ 14 3 4 4 4 */
/*:ref: zzeksinf_ 14 8 4 4 13 4 13 4 124 124 */
/*:ref: ssizec_ 14 3 4 13 124 */
/*:ref: movec_ 14 5 13 4 13 124 124 */
/*:ref: validc_ 14 4 4 4 13 124 */
/*:ref: cardc_ 4 2 13 124 */
/*:ref: movei_ 14 3 4 4 4 */
/*:ref: isrchc_ 4 5 13 4 13 124 124 */
/*:ref: lnkfsl_ 14 3 4 4 4 */
/*:ref: lnktl_ 4 2 4 4 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: eqstr_ 12 4 13 13 124 124 */
/*:ref: zzekreqi_ 14 4 4 13 4 124 */
/*:ref: zzekqtab_ 14 8 4 13 4 13 13 124 124 124 */
/*:ref: ssizei_ 14 2 4 4 */
/*:ref: appndc_ 14 4 13 13 124 124 */
/*:ref: appndi_ 14 2 4 4 */
/*:ref: zzekstop_ 14 1 4 */
/*:ref: zzeksdec_ 14 1 4 */
/*:ref: cleari_ 14 2 4 4 */
/*:ref: zzekqcnj_ 14 3 4 4 4 */
/*:ref: zzekqcon_ 14 24 4 13 7 4 4 13 4 13 4 4 13 4 13 4 4 4 4 7 4 124 124 124 124 124 */
/*:ref: zzekspsh_ 14 2 4 4 */
/*:ref: zzeksupd_ 14 3 4 4 4 */
/*:ref: zzekkey_ 14 20 4 4 4 4 4 4 4 4 13 4 4 7 4 12 4 4 4 4 12 124 */
/*:ref: zzekixlk_ 14 4 4 4 4 4 */
/*:ref: zzekrplk_ 14 4 4 4 4 4 */
/*:ref: zzekrmch_ 12 15 4 12 4 4 4 4 4 4 4 13 4 4 7 4 124 */
/*:ref: zzekvmch_ 12 13 4 12 4 4 4 4 4 4 4 4 4 4 4 */
/*:ref: zzekjsqz_ 14 1 4 */
/*:ref: zzekjoin_ 14 18 4 4 4 12 4 4 4 4 4 4 4 4 4 4 4 4 4 4 */
/*:ref: zzeksrd_ 14 3 4 4 4 */
/*:ref: zzekweed_ 14 3 4 4 4 */
/*:ref: zzekvset_ 14 2 4 4 */
/*:ref: zzekqsel_ 14 12 4 13 4 4 4 13 4 13 4 124 124 124 */
/*:ref: zzekqord_ 14 11 4 13 4 13 4 13 4 4 124 124 124 */
/*:ref: zzekjsrt_ 14 13 4 4 4 4 4 4 4 4 4 4 4 4 4 */
/*:ref: zzekvcal_ 14 3 4 4 4 */
/*:ref: zzekesiz_ 4 4 4 4 4 4 */
/*:ref: zzekrsc_ 14 10 4 4 4 4 4 4 13 12 12 124 */
/*:ref: zzekrsd_ 14 8 4 4 4 4 4 7 12 12 */
/*:ref: zzekrsi_ 14 8 4 4 4 4 4 4 12 12 */
 
extern int ekrcec_(integer *handle, integer *segno, integer *recno, char *column, integer *nvals, char *cvals, logical *isnull, ftnlen column_len, ftnlen cvals_len);
/*:ref: zzeksdsc_ 14 3 4 4 4 */
/*:ref: zzekcdsc_ 14 5 4 4 13 4 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dashlu_ 14 2 4 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: errfnm_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzektrdp_ 14 4 4 4 4 4 */
/*:ref: zzekrd03_ 14 8 4 4 4 4 4 13 12 124 */
/*:ref: zzekesiz_ 4 4 4 4 4 4 */
/*:ref: zzekrd06_ 14 10 4 4 4 4 4 4 13 12 12 124 */
/*:ref: zzekrd09_ 14 8 4 4 4 4 4 13 12 124 */
 
extern int ekrced_(integer *handle, integer *segno, integer *recno, char *column, integer *nvals, doublereal *dvals, logical *isnull, ftnlen column_len);
/*:ref: zzeksdsc_ 14 3 4 4 4 */
/*:ref: zzekcdsc_ 14 5 4 4 13 4 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dashlu_ 14 2 4 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: errfnm_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzektrdp_ 14 4 4 4 4 4 */
/*:ref: zzekrd02_ 14 6 4 4 4 4 7 12 */
/*:ref: zzekesiz_ 4 4 4 4 4 4 */
/*:ref: zzekrd05_ 14 9 4 4 4 4 4 4 7 12 12 */
/*:ref: zzekrd08_ 14 6 4 4 4 4 7 12 */
 
extern int ekrcei_(integer *handle, integer *segno, integer *recno, char *column, integer *nvals, integer *ivals, logical *isnull, ftnlen column_len);
/*:ref: zzeksdsc_ 14 3 4 4 4 */
/*:ref: zzekcdsc_ 14 5 4 4 13 4 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dashlu_ 14 2 4 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: errfnm_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzektrdp_ 14 4 4 4 4 4 */
/*:ref: zzekrd01_ 14 6 4 4 4 4 4 12 */
/*:ref: zzekesiz_ 4 4 4 4 4 4 */
/*:ref: zzekrd04_ 14 9 4 4 4 4 4 4 4 12 12 */
/*:ref: zzekrd07_ 14 6 4 4 4 4 4 12 */
 
extern int ekshdw_(integer *handle, logical *isshad);
 
extern int ekssum_(integer *handle, integer *segno, char *tabnam, integer *nrows, integer *ncols, char *cnames, char *dtypes, integer *sizes, integer *strlns, logical *indexd, logical *nullok, ftnlen tabnam_len, ftnlen cnames_len, ftnlen dtypes_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: zzeksinf_ 14 8 4 4 13 4 13 4 124 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int ekucec_(integer *handle, integer *segno, integer *recno, char *column, integer *nvals, char *cvals, logical *isnull, ftnlen column_len, ftnlen cvals_len);
/*:ref: zzeksdsc_ 14 3 4 4 4 */
/*:ref: zzekcdsc_ 14 5 4 4 13 4 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dashlu_ 14 2 4 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: errfnm_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzektrdp_ 14 4 4 4 4 4 */
/*:ref: ekshdw_ 14 2 4 12 */
/*:ref: zzekrbck_ 14 6 13 4 4 4 4 124 */
/*:ref: zzekue03_ 14 7 4 4 4 4 13 12 124 */
/*:ref: zzekue06_ 14 8 4 4 4 4 4 13 12 124 */
 
extern int ekuced_(integer *handle, integer *segno, integer *recno, char *column, integer *nvals, doublereal *dvals, logical *isnull, ftnlen column_len);
/*:ref: zzeksdsc_ 14 3 4 4 4 */
/*:ref: zzekcdsc_ 14 5 4 4 13 4 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dashlu_ 14 2 4 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: errfnm_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzektrdp_ 14 4 4 4 4 4 */
/*:ref: ekshdw_ 14 2 4 12 */
/*:ref: zzekrbck_ 14 6 13 4 4 4 4 124 */
/*:ref: zzekue02_ 14 6 4 4 4 4 7 12 */
/*:ref: zzekue05_ 14 7 4 4 4 4 4 7 12 */
 
extern int ekucei_(integer *handle, integer *segno, integer *recno, char *column, integer *nvals, integer *ivals, logical *isnull, ftnlen column_len);
/*:ref: zzeksdsc_ 14 3 4 4 4 */
/*:ref: zzekcdsc_ 14 5 4 4 13 4 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dashlu_ 14 2 4 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: errfnm_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzektrdp_ 14 4 4 4 4 4 */
/*:ref: ekshdw_ 14 2 4 12 */
/*:ref: zzekrbck_ 14 6 13 4 4 4 4 124 */
/*:ref: zzekue01_ 14 6 4 4 4 4 4 12 */
/*:ref: zzekue04_ 14 7 4 4 4 4 4 4 12 */
 
extern int el2cgv_(doublereal *ellips, doublereal *center, doublereal *smajor, doublereal *sminor);
/*:ref: vequ_ 14 2 7 7 */
 
extern logical elemc_(char *item, char *a, ftnlen item_len, ftnlen a_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: bsrchc_ 4 5 13 4 13 124 124 */
/*:ref: cardc_ 4 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern logical elemd_(doublereal *item, doublereal *a);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: bsrchd_ 4 3 7 4 7 */
/*:ref: cardd_ 4 1 7 */
/*:ref: chkout_ 14 2 13 124 */
 
extern logical elemi_(integer *item, integer *a);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: bsrchi_ 4 3 4 4 4 */
/*:ref: cardi_ 4 1 4 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int elltof_(doublereal *ma, doublereal *ecc, doublereal *e);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: pi_ 7 0 */
/*:ref: twopi_ 7 0 */
/*:ref: halfpi_ 7 0 */
/*:ref: dcbrt_ 7 1 7 */
 
extern int enchar_(integer *number, char *string, ftnlen string_len);
extern int dechar_(char *string, integer *number, ftnlen string_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: chbase_ 4 0 */
 
extern logical eqchr_(char *a, char *b, ftnlen a_len, ftnlen b_len);
extern logical nechr_(char *a, char *b, ftnlen a_len, ftnlen b_len);
 
extern int eqncpv_(doublereal *et, doublereal *epoch, doublereal *eqel, doublereal *rapol, doublereal *decpol, doublereal *state);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: twopi_ 7 0 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: kepleq_ 7 3 7 7 7 */
/*:ref: vlcom_ 14 5 7 7 7 7 7 */
/*:ref: vlcom3_ 14 7 7 7 7 7 7 7 7 */
/*:ref: mxv_ 14 3 7 7 7 */
 
extern logical eqstr_(char *a, char *b, ftnlen a_len, ftnlen b_len);
 
extern int erract_(char *op, char *action, ftnlen op_len, ftnlen action_len);
/*:ref: chkin_ 14 2 13 124 */
/*:ref: ljust_ 14 4 13 13 124 124 */
/*:ref: ucase_ 14 4 13 13 124 124 */
/*:ref: getact_ 14 1 4 */
/*:ref: isrchc_ 4 5 13 4 13 124 124 */
/*:ref: putact_ 14 1 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int errch_(char *marker, char *string, ftnlen marker_len, ftnlen string_len);
/*:ref: allowd_ 12 0 */
/*:ref: lastnb_ 4 2 13 124 */
/*:ref: getlms_ 14 2 13 124 */
/*:ref: frstnb_ 4 2 13 124 */
/*:ref: nblen_ 4 2 13 124 */
/*:ref: putlms_ 14 2 13 124 */
 
extern int errdev_(char *op, char *device, ftnlen op_len, ftnlen device_len);
/*:ref: chkin_ 14 2 13 124 */
/*:ref: ljust_ 14 4 13 13 124 124 */
/*:ref: ucase_ 14 4 13 13 124 124 */
/*:ref: getdev_ 14 2 13 124 */
/*:ref: lastnb_ 4 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: putdev_ 14 2 13 124 */
 
extern int errdp_(char *marker, doublereal *dpnum, ftnlen marker_len);
/*:ref: allowd_ 12 0 */
/*:ref: lastnb_ 4 2 13 124 */
/*:ref: getlms_ 14 2 13 124 */
/*:ref: dpstr_ 14 4 7 4 13 124 */
/*:ref: ljust_ 14 4 13 13 124 124 */
/*:ref: frstnb_ 4 2 13 124 */
/*:ref: putlms_ 14 2 13 124 */
 
extern int errfnm_(char *marker, integer *unit, ftnlen marker_len);
/*:ref: errch_ 14 4 13 13 124 124 */
 
extern int errhan_(char *marker, integer *handle, ftnlen marker_len);
/*:ref: zzddhnfo_ 14 7 4 13 4 4 4 12 124 */
/*:ref: intstr_ 14 3 4 13 124 */
/*:ref: suffix_ 14 5 13 4 13 124 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
 
extern int errint_(char *marker, integer *integr, ftnlen marker_len);
/*:ref: allowd_ 12 0 */
/*:ref: lastnb_ 4 2 13 124 */
/*:ref: getlms_ 14 2 13 124 */
/*:ref: intstr_ 14 3 4 13 124 */
/*:ref: frstnb_ 4 2 13 124 */
/*:ref: putlms_ 14 2 13 124 */
 
extern int errprt_(char *op, char *list, ftnlen op_len, ftnlen list_len);
/*:ref: chkin_ 14 2 13 124 */
/*:ref: msgsel_ 12 2 13 124 */
/*:ref: ljust_ 14 4 13 13 124 124 */
/*:ref: ucase_ 14 4 13 13 124 124 */
/*:ref: suffix_ 14 5 13 4 13 124 124 */
/*:ref: lparse_ 14 8 13 13 4 4 13 124 124 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: setprt_ 12 5 12 12 12 12 12 */
/*:ref: chkout_ 14 2 13 124 */
 
extern integer esrchc_(char *value, integer *ndim, char *array, ftnlen value_len, ftnlen array_len);
/*:ref: eqstr_ 12 4 13 13 124 124 */
 
extern int et2lst_(doublereal *et, integer *body, doublereal *long__, char *type__, integer *hr, integer *mn, integer *sc, char *time, char *ampm, ftnlen type_len, ftnlen time_len, ftnlen ampm_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: ljust_ 14 4 13 13 124 124 */
/*:ref: ucase_ 14 4 13 13 124 124 */
/*:ref: bodc2n_ 14 4 4 13 12 124 */
/*:ref: intstr_ 14 3 4 13 124 */
/*:ref: pgrrec_ 14 8 13 7 7 7 7 7 7 124 */
/*:ref: reclat_ 14 4 7 7 7 7 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: cidfrm_ 14 5 4 4 13 12 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: spkez_ 14 9 4 7 13 13 4 7 7 124 124 */
/*:ref: rmaind_ 14 4 7 7 7 7 */
/*:ref: twopi_ 7 0 */
/*:ref: pi_ 7 0 */
/*:ref: brcktd_ 7 3 7 7 7 */
/*:ref: dpfmt_ 14 5 7 13 13 124 124 */
 
extern int et2utc_(doublereal *et, char *format, integer *prec, char *utcstr, ftnlen format_len, ftnlen utcstr_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: ucase_ 14 4 13 13 124 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: ttrans_ 14 5 13 13 7 124 124 */
/*:ref: failed_ 12 0 */
/*:ref: dpstrf_ 14 6 7 4 13 13 124 124 */
/*:ref: prefix_ 14 5 13 4 13 124 124 */
/*:ref: unitim_ 7 5 7 13 13 124 124 */
/*:ref: intstr_ 14 3 4 13 124 */
/*:ref: suffix_ 14 5 13 4 13 124 124 */
/*:ref: errint_ 14 3 13 4 124 */
 
extern int etcal_(doublereal *et, char *string, ftnlen string_len);
/*:ref: spd_ 7 0 */
/*:ref: intmax_ 4 0 */
/*:ref: intmin_ 4 0 */
/*:ref: lstlti_ 4 3 4 4 4 */
/*:ref: intstr_ 14 3 4 13 124 */
/*:ref: dpstrf_ 14 6 7 4 13 13 124 124 */
/*:ref: ljust_ 14 4 13 13 124 124 */
/*:ref: cmprss_ 14 7 13 4 13 13 124 124 124 */
 
extern int eul2m_(doublereal *angle3, doublereal *angle2, doublereal *angle1, integer *axis3, integer *axis2, integer *axis1, doublereal *r__);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: rotate_ 14 3 7 4 7 */
/*:ref: rotmat_ 14 4 7 7 4 7 */
 
extern int ev2lin_(doublereal *et, doublereal *geophs, doublereal *elems, doublereal *state);
/*:ref: twopi_ 7 0 */
/*:ref: brcktd_ 7 3 7 7 7 */
 
extern logical even_(integer *i__);
 
extern doublereal exact_(doublereal *number, doublereal *value, doublereal *tol);
 
extern int excess_(integer *number, char *struct__, ftnlen struct_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: intstr_ 14 3 4 13 124 */
/*:ref: prefix_ 14 5 13 4 13 124 124 */
/*:ref: suffix_ 14 5 13 4 13 124 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern logical exists_(char *file, ftnlen file_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: rtrim_ 4 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
 
extern int expln_(char *msg, char *expl, ftnlen msg_len, ftnlen expl_len);
 
extern integer fetchc_(integer *nth, char *set, ftnlen set_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardc_ 4 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern integer fetchd_(integer *nth, doublereal *set);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardd_ 4 1 7 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern integer fetchi_(integer *nth, integer *set);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardi_ 4 1 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int fillc_(char *value, integer *ndim, char *array, ftnlen value_len, ftnlen array_len);
 
extern int filld_(doublereal *value, integer *ndim, doublereal *array);
 
extern int filli_(integer *value, integer *ndim, integer *array);
 
extern int fn2lun_(char *filnam, integer *lunit, ftnlen filnam_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: errint_ 14 3 13 4 124 */
 
extern int fndlun_(integer *unit);
extern int reslun_(integer *unit);
extern int frelun_(integer *unit);
 
extern int fndnwd_(char *string, integer *start, integer *b, integer *e, ftnlen string_len);
 
extern int frame_(doublereal *x, doublereal *y, doublereal *z__);
/*:ref: vhatip_ 14 1 7 */
 
extern int framex_(char *cname, char *frname, integer *frcode, integer *cent, integer *class__, integer *clssid, logical *found, ftnlen cname_len, ftnlen frname_len);
extern int namfrm_(char *frname, integer *frcode, ftnlen frname_len);
extern int frmnam_(integer *frcode, char *frname, ftnlen frname_len);
extern int frinfo_(integer *frcode, integer *cent, integer *class__, integer *clssid, logical *found);
extern int cidfrm_(integer *cent, integer *frcode, char *frname, logical *found, ftnlen frname_len);
extern int cnmfrm_(char *cname, integer *frcode, char *frname, logical *found, ftnlen cname_len, ftnlen frname_len);
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: return_ 12 0 */
/*:ref: zzfdat_ 14 10 4 13 4 4 4 4 4 4 4 124 */
/*:ref: ljust_ 14 4 13 13 124 124 */
/*:ref: ucase_ 14 4 13 13 124 124 */
/*:ref: bschoc_ 4 6 13 4 13 4 124 124 */
/*:ref: prefix_ 14 5 13 4 13 124 124 */
/*:ref: gipool_ 14 7 13 4 4 4 4 12 124 */
/*:ref: bschoi_ 4 4 4 4 4 4 */
/*:ref: repmi_ 14 7 13 13 4 13 124 124 124 */
/*:ref: gcpool_ 14 8 13 4 4 4 13 12 124 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: zzdynbid_ 14 6 13 4 13 4 124 124 */
/*:ref: failed_ 12 0 */
/*:ref: zzdynvai_ 14 8 13 4 13 4 4 4 124 124 */
/*:ref: dtpool_ 14 6 13 12 4 13 124 124 */
/*:ref: bodc2n_ 14 4 4 13 12 124 */
/*:ref: repmc_ 14 8 13 13 13 13 124 124 124 124 */
/*:ref: bodn2c_ 14 4 13 4 12 124 */
 
extern int frmchg_(integer *frame1, integer *frame2, doublereal *et, doublereal *xform);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: frinfo_ 14 5 4 4 4 4 12 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: frmget_ 14 5 4 7 7 4 12 */
/*:ref: zzmsxf_ 14 3 7 4 7 */
/*:ref: isrchi_ 4 3 4 4 4 */
/*:ref: frmnam_ 14 3 4 13 124 */
/*:ref: rtrim_ 4 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: invstm_ 14 2 7 7 */
 
extern int frmget_(integer *infrm, doublereal *et, doublereal *xform, integer *outfrm, logical *found);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: frinfo_ 14 5 4 4 4 4 12 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: tisbod_ 14 5 13 4 7 7 124 */
/*:ref: invstm_ 14 2 7 7 */
/*:ref: namfrm_ 14 3 13 4 124 */
/*:ref: irfrot_ 14 3 4 4 7 */
/*:ref: ckfxfm_ 14 5 4 7 7 4 12 */
/*:ref: tkfram_ 14 4 4 7 4 12 */
/*:ref: zzdynfrm_ 14 5 4 4 7 7 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: failed_ 12 0 */
 
extern integer frstnb_(char *string, ftnlen string_len);
 
extern integer frstnp_(char *string, ftnlen string_len);
 
extern integer frstpc_(char *string, ftnlen string_len);
 
extern integer gcd_(integer *a, integer *b);
 
extern int georec_(doublereal *long__, doublereal *lat, doublereal *alt, doublereal *re, doublereal *f, doublereal *rectan);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: surfnm_ 14 5 7 7 7 7 7 */
/*:ref: vlcom_ 14 5 7 7 7 7 7 */
 
extern int getelm_(integer *frstyr, char *lines, doublereal *epoch, doublereal *elems, ftnlen lines_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: zzgetelm_ 14 8 4 13 7 7 12 13 124 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int getfat_(char *file, char *arch, char *kertyp, ftnlen file_len, ftnlen arch_len, ftnlen kertyp_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzddhfnh_ 14 4 13 4 12 124 */
/*:ref: zzddhnfo_ 14 7 4 13 4 4 4 12 124 */
/*:ref: zzddhgsd_ 14 5 13 4 13 124 124 */
/*:ref: zzddhhlu_ 14 5 4 13 12 4 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: ssizei_ 14 2 4 4 */
/*:ref: dashof_ 14 1 4 */
/*:ref: cardi_ 4 1 4 */
/*:ref: dashlu_ 14 2 4 4 */
/*:ref: getlun_ 14 1 4 */
/*:ref: ljust_ 14 4 13 13 124 124 */
/*:ref: ucase_ 14 4 13 13 124 124 */
/*:ref: nextwd_ 14 6 13 13 13 124 124 124 */
/*:ref: idw2at_ 14 6 13 13 13 124 124 124 */
/*:ref: dafopr_ 14 3 13 4 124 */
/*:ref: zzckspk_ 14 3 4 13 124 */
/*:ref: dafcls_ 14 1 4 */
 
extern int getfov_(integer *instid, integer *room, char *shape, char *frame, doublereal *bsight, integer *n, doublereal *bounds, ftnlen shape_len, ftnlen frame_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: repmi_ 14 7 13 13 4 13 124 124 124 */
/*:ref: gcpool_ 14 8 13 4 4 4 13 12 124 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: rtrim_ 4 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: ucase_ 14 4 13 13 124 124 */
/*:ref: ljust_ 14 4 13 13 124 124 */
/*:ref: bsrchc_ 4 5 13 4 13 124 124 */
/*:ref: dtpool_ 14 6 13 12 4 13 124 124 */
/*:ref: gdpool_ 14 7 13 4 4 4 7 12 124 */
/*:ref: eqstr_ 12 4 13 13 124 124 */
/*:ref: suffix_ 14 5 13 4 13 124 124 */
/*:ref: vcrss_ 14 3 7 7 7 */
/*:ref: vnorm_ 7 1 7 */
/*:ref: convrt_ 14 6 7 13 13 7 124 124 */
/*:ref: vrotv_ 14 4 7 7 7 7 */
/*:ref: vequ_ 14 2 7 7 */
/*:ref: unorm_ 14 3 7 7 7 */
/*:ref: vperp_ 14 3 7 7 7 */
/*:ref: vhat_ 14 2 7 7 */
/*:ref: vlcom_ 14 5 7 7 7 7 7 */
/*:ref: vscl_ 14 3 7 7 7 */
 
extern int getlun_(integer *unit);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: fndlun_ 14 1 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
 
extern int getmsg_(char *option, char *msg, ftnlen option_len, ftnlen msg_len);
/*:ref: ljust_ 14 4 13 13 124 124 */
/*:ref: ucase_ 14 4 13 13 124 124 */
/*:ref: getsms_ 14 2 13 124 */
/*:ref: expln_ 14 4 13 13 124 124 */
/*:ref: getlms_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
 
extern doublereal halfpi_(void);
 
extern int hrmesp_(integer *n, doublereal *first, doublereal *step, doublereal *yvals, doublereal *x, doublereal *work, doublereal *f, doublereal *df);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int hrmint_(integer *n, doublereal *xvals, doublereal *yvals, doublereal *x, doublereal *work, doublereal *f, doublereal *df);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: errdp_ 14 3 13 7 124 */
 
extern int hx2dp_(char *string, doublereal *number, logical *error, char *errmsg, ftnlen string_len, ftnlen errmsg_len);
/*:ref: dpmin_ 7 0 */
/*:ref: dpmax_ 7 0 */
/*:ref: repmc_ 14 8 13 13 13 13 124 124 124 124 */
/*:ref: hx2int_ 14 6 13 4 12 13 124 124 */
 
extern int hx2int_(char *string, integer *number, logical *error, char *errmsg, ftnlen string_len, ftnlen errmsg_len);
/*:ref: intmin_ 4 0 */
/*:ref: intmax_ 4 0 */
/*:ref: repmc_ 14 8 13 13 13 13 124 124 124 124 */
 
extern int hyptof_(doublereal *ma, doublereal *ecc, doublereal *f);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dpmax_ 7 0 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dcbrt_ 7 1 7 */
 
extern int ident_(doublereal *matrix);
 
extern int idw2at_(char *idword, char *arch, char *type__, ftnlen idword_len, ftnlen arch_len, ftnlen type_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: pos_ 4 5 13 13 4 124 124 */
 
extern int illum_(char *target, doublereal *et, char *abcorr, char *obsrvr, doublereal *spoint, doublereal *phase, doublereal *solar, doublereal *emissn, ftnlen target_len, ftnlen abcorr_len, ftnlen obsrvr_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: bods2c_ 14 4 13 4 12 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: cidfrm_ 14 5 4 4 13 12 124 */
/*:ref: spkez_ 14 9 4 7 13 13 4 7 7 124 124 */
/*:ref: eqstr_ 12 4 13 13 124 124 */
/*:ref: vminus_ 14 2 7 7 */
/*:ref: vequ_ 14 2 7 7 */
/*:ref: vsub_ 14 3 7 7 7 */
/*:ref: bodvcd_ 14 6 4 13 4 4 7 124 */
/*:ref: surfnm_ 14 5 7 7 7 7 7 */
/*:ref: vsep_ 7 2 7 7 */
 
extern int inedpl_(doublereal *a, doublereal *b, doublereal *c__, doublereal *plane, doublereal *ellips, logical *found);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: pl2nvc_ 14 3 7 7 7 */
/*:ref: vzero_ 12 1 7 */
/*:ref: pl2psv_ 14 4 7 7 7 7 */
/*:ref: vnorm_ 7 1 7 */
/*:ref: psv2pl_ 14 4 7 7 7 7 */
/*:ref: brcktd_ 7 3 7 7 7 */
/*:ref: vsclip_ 14 2 7 7 */
/*:ref: cgv2el_ 14 4 7 7 7 7 */
 
extern int inelpl_(doublereal *ellips, doublereal *plane, integer *nxpts, doublereal *xpt1, doublereal *xpt2);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: el2cgv_ 14 4 7 7 7 7 */
/*:ref: pl2nvp_ 14 3 7 7 7 */
/*:ref: vsub_ 14 3 7 7 7 */
/*:ref: nvp2pl_ 14 3 7 7 7 */
/*:ref: pl2nvc_ 14 3 7 7 7 */
/*:ref: vdot_ 7 2 7 7 */
/*:ref: vzerog_ 12 2 7 4 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: vnormg_ 7 2 7 4 */
/*:ref: vlcom3_ 14 7 7 7 7 7 7 7 7 */
 
extern int inrypl_(doublereal *vertex, doublereal *dir, doublereal *plane, integer *nxpts, doublereal *xpt);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dpmax_ 7 0 */
/*:ref: vnorm_ 7 1 7 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: pl2nvc_ 14 3 7 7 7 */
/*:ref: vhat_ 14 2 7 7 */
/*:ref: vzero_ 12 1 7 */
/*:ref: vscl_ 14 3 7 7 7 */
/*:ref: vequ_ 14 2 7 7 */
/*:ref: vdot_ 7 2 7 7 */
/*:ref: smsgnd_ 12 2 7 7 */
/*:ref: cleard_ 14 2 4 7 */
/*:ref: vlcom_ 14 5 7 7 7 7 7 */
/*:ref: vsclip_ 14 2 7 7 */
 
extern int inslac_(char *elts, integer *ne, integer *loc, char *array, integer *na, ftnlen elts_len, ftnlen array_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int inslad_(doublereal *elts, integer *ne, integer *loc, doublereal *array, integer *na);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int inslai_(integer *elts, integer *ne, integer *loc, integer *array, integer *na);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int insrtc_(char *item, char *a, ftnlen item_len, ftnlen a_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: sizec_ 4 2 13 124 */
/*:ref: cardc_ 4 2 13 124 */
/*:ref: lstlec_ 4 5 13 4 13 124 124 */
/*:ref: scardc_ 14 3 4 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int insrtd_(doublereal *item, doublereal *a);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: sized_ 4 1 7 */
/*:ref: cardd_ 4 1 7 */
/*:ref: lstled_ 4 3 7 4 7 */
/*:ref: scardd_ 14 2 4 7 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int insrti_(integer *item, integer *a);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: sizei_ 4 1 4 */
/*:ref: cardi_ 4 1 4 */
/*:ref: lstlei_ 4 3 4 4 4 */
/*:ref: scardi_ 14 2 4 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int inssub_(char *in, char *sub, integer *loc, char *out, ftnlen in_len, ftnlen sub_len, ftnlen out_len);
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int int2hx_(integer *number, char *string, integer *length, ftnlen string_len);
 
extern int interc_(char *a, char *b, char *c__, ftnlen a_len, ftnlen b_len, ftnlen c_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: cardc_ 4 2 13 124 */
/*:ref: sizec_ 4 2 13 124 */
/*:ref: scardc_ 14 3 4 13 124 */
/*:ref: excess_ 14 3 4 13 124 */
 
extern int interd_(doublereal *a, doublereal *b, doublereal *c__);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardd_ 4 1 7 */
/*:ref: sized_ 4 1 7 */
/*:ref: scardd_ 14 2 4 7 */
/*:ref: excess_ 14 3 4 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int interi_(integer *a, integer *b, integer *c__);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardi_ 4 1 4 */
/*:ref: sizei_ 4 1 4 */
/*:ref: scardi_ 14 2 4 4 */
/*:ref: excess_ 14 3 4 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int intord_(integer *n, char *string, ftnlen string_len);
/*:ref: inttxt_ 14 3 4 13 124 */
/*:ref: lastnb_ 4 2 13 124 */
/*:ref: suffix_ 14 5 13 4 13 124 124 */
 
extern int intstr_(integer *number, char *string, ftnlen string_len);
 
extern int inttxt_(integer *n, char *string, ftnlen string_len);
/*:ref: suffix_ 14 5 13 4 13 124 124 */
 
extern int invert_(doublereal *m1, doublereal *mout);
/*:ref: det_ 7 1 7 */
/*:ref: filld_ 14 3 7 4 7 */
/*:ref: vsclg_ 14 4 7 7 4 7 */
 
extern int invort_(doublereal *m, doublereal *mit);
/*:ref: dpmax_ 7 0 */
/*:ref: unorm_ 14 3 7 7 7 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: vsclip_ 14 2 7 7 */
/*:ref: xpose_ 14 2 7 7 */
 
extern int invstm_(doublereal *mat, doublereal *invmat);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: xposbl_ 14 5 7 4 4 4 7 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int ioerr_(char *action, char *file, integer *iostat, ftnlen action_len, ftnlen file_len);
/*:ref: suffix_ 14 5 13 4 13 124 124 */
/*:ref: intstr_ 14 3 4 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
 
extern int irftrn_(char *refa, char *refb, doublereal *rotab, ftnlen refa_len, ftnlen refb_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: irfnum_ 14 3 13 4 124 */
/*:ref: irfrot_ 14 3 4 4 7 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int iso2utc_(char *tstrng, char *utcstr, char *error, ftnlen tstrng_len, ftnlen utcstr_len, ftnlen error_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: ljust_ 14 4 13 13 124 124 */
/*:ref: rtrim_ 4 2 13 124 */
/*:ref: bsrchc_ 4 5 13 4 13 124 124 */
/*:ref: repmc_ 14 8 13 13 13 13 124 124 124 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern logical isopen_(char *file, ftnlen file_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
 
extern logical isordv_(integer *array, integer *n);
 
extern integer isrchc_(char *value, integer *ndim, char *array, ftnlen value_len, ftnlen array_len);
 
extern integer isrchd_(doublereal *value, integer *ndim, doublereal *array);
 
extern integer isrchi_(integer *value, integer *ndim, integer *array);
 
extern logical isrot_(doublereal *m, doublereal *ntol, doublereal *dtol);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: unorm_ 14 3 7 7 7 */
/*:ref: det_ 7 1 7 */
/*:ref: brcktd_ 7 3 7 7 7 */
 
extern doublereal j1900_(void);
 
extern doublereal j1950_(void);
 
extern doublereal j2000_(void);
 
extern doublereal j2100_(void);
 
extern int jul2gr_(integer *year, integer *month, integer *day, integer *doy);
extern int gr2jul_(integer *year, integer *month, integer *day, integer *doy);
/*:ref: rmaini_ 14 4 4 4 4 4 */
/*:ref: lstlti_ 4 3 4 4 4 */
 
extern doublereal jyear_(void);
 
extern int keeper_(integer *which, char *kind, char *file, integer *count, char *filtyp, integer *handle, char *source, logical *found, ftnlen kind_len, ftnlen file_len, ftnlen filtyp_len, ftnlen source_len);
extern int furnsh_(char *file, ftnlen file_len);
extern int ktotal_(char *kind, integer *count, ftnlen kind_len);
extern int kdata_(integer *which, char *kind, char *file, char *filtyp, char *source, integer *handle, logical *found, ftnlen kind_len, ftnlen file_len, ftnlen filtyp_len, ftnlen source_len);
extern int kinfo_(char *file, char *filtyp, char *source, integer *handle, logical *found, ftnlen file_len, ftnlen filtyp_len, ftnlen source_len);
extern int unload_(char *file, ftnlen file_len);
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: return_ 12 0 */
/*:ref: swpool_ 14 5 13 4 13 124 124 */
/*:ref: cvpool_ 14 3 13 12 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: zzldker_ 14 7 13 13 13 4 124 124 124 */
/*:ref: failed_ 12 0 */
/*:ref: dtpool_ 14 6 13 12 4 13 124 124 */
/*:ref: badkpv_ 12 10 13 13 13 4 4 13 124 124 124 124 */
/*:ref: stpool_ 14 9 13 4 13 13 4 12 124 124 124 */
/*:ref: pos_ 4 5 13 13 4 124 124 */
/*:ref: gcpool_ 14 8 13 4 4 4 13 12 124 124 */
/*:ref: rtrim_ 4 2 13 124 */
/*:ref: samsub_ 12 8 13 4 4 13 4 4 124 124 */
/*:ref: repsub_ 14 8 13 4 4 13 13 124 124 124 */
/*:ref: repmot_ 14 9 13 13 4 13 13 124 124 124 124 */
/*:ref: repmc_ 14 8 13 13 13 13 124 124 124 124 */
/*:ref: dvpool_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: fndnwd_ 14 5 13 4 4 4 124 */
/*:ref: eqstr_ 12 4 13 13 124 124 */
/*:ref: isrchc_ 4 5 13 4 13 124 124 */
/*:ref: spkuef_ 14 1 4 */
/*:ref: ckupf_ 14 1 4 */
/*:ref: pckuof_ 14 1 4 */
/*:ref: ekuef_ 14 1 4 */
/*:ref: clpool_ 14 0 */
/*:ref: remlac_ 14 5 4 4 13 4 124 */
/*:ref: remlai_ 14 4 4 4 4 4 */
/*:ref: ldpool_ 14 2 13 124 */
/*:ref: spklef_ 14 3 13 4 124 */
/*:ref: cklpf_ 14 3 13 4 124 */
/*:ref: pcklof_ 14 3 13 4 124 */
/*:ref: eklef_ 14 3 13 4 124 */
 
extern doublereal kepleq_(doublereal *ml, doublereal *h__, doublereal *k);
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: kpsolv_ 7 1 7 */
 
extern doublereal kpsolv_(doublereal *evec);
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int kxtrct_(char *keywd, char *terms, integer *nterms, char *string, logical *found, char *substr, ftnlen keywd_len, ftnlen terms_len, ftnlen string_len, ftnlen substr_len);
/*:ref: wdindx_ 4 4 13 13 124 124 */
/*:ref: nblen_ 4 2 13 124 */
/*:ref: fndnwd_ 14 5 13 4 4 4 124 */
/*:ref: isrchc_ 4 5 13 4 13 124 124 */
/*:ref: shiftl_ 14 7 13 4 13 13 124 124 124 */
 
extern integer lastnb_(char *string, ftnlen string_len);
 
extern integer lastpc_(char *string, ftnlen string_len);
 
extern int latcyl_(doublereal *radius, doublereal *long__, doublereal *lat, doublereal *r__, doublereal *longc, doublereal *z__);
 
extern int latrec_(doublereal *radius, doublereal *long__, doublereal *lat, doublereal *rectan);
 
extern int latsph_(doublereal *radius, doublereal *long__, doublereal *lat, doublereal *rho, doublereal *colat, doublereal *longs);
/*:ref: halfpi_ 7 0 */
 
extern int lbuild_(char *items, integer *n, char *delim, char *list, ftnlen items_len, ftnlen delim_len, ftnlen list_len);
/*:ref: frstnb_ 4 2 13 124 */
/*:ref: lastnb_ 4 2 13 124 */
/*:ref: suffix_ 14 5 13 4 13 124 124 */
 
extern int lcase_(char *in, char *out, ftnlen in_len, ftnlen out_len);
 
extern doublereal lgresp_(integer *n, doublereal *first, doublereal *step, doublereal *yvals, doublereal *work, doublereal *x);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int lgrind_(integer *n, doublereal *xvals, doublereal *yvals, doublereal *work, doublereal *x, doublereal *p, doublereal *dp);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: errdp_ 14 3 13 7 124 */
 
extern doublereal lgrint_(integer *n, doublereal *xvals, doublereal *yvals, doublereal *work, doublereal *x);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: errdp_ 14 3 13 7 124 */
 
extern int ljust_(char *input, char *output, ftnlen input_len, ftnlen output_len);
 
extern int lnkan_(integer *pool, integer *new__);
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int lnkfsl_(integer *head, integer *tail, integer *pool);
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern integer lnkhl_(integer *node, integer *pool);
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int lnkila_(integer *prev, integer *list, integer *pool);
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int lnkilb_(integer *list, integer *next, integer *pool);
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int lnkini_(integer *size, integer *pool);
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern integer lnknfn_(integer *pool);
 
extern integer lnknxt_(integer *node, integer *pool);
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern integer lnkprv_(integer *node, integer *pool);
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern integer lnksiz_(integer *pool);
 
extern integer lnktl_(integer *node, integer *pool);
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int lnkxsl_(integer *head, integer *tail, integer *pool);
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int locati_(integer *id, integer *idsz, integer *list, integer *pool, integer *at, logical *presnt);
/*:ref: chkin_ 14 2 13 124 */
/*:ref: lnknfn_ 4 1 4 */
/*:ref: lnksiz_ 4 1 4 */
/*:ref: lnkan_ 14 2 4 4 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: lnkxsl_ 14 3 4 4 4 */
/*:ref: lnkilb_ 14 3 4 4 4 */
 
extern int locln_(integer *unit, char *bmark, char *emark, char *line, integer *bline, integer *eline, logical *found, ftnlen bmark_len, ftnlen emark_len, ftnlen line_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: errfnm_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: ljust_ 14 4 13 13 124 124 */
/*:ref: ltrim_ 4 2 13 124 */
 
extern int lparse_(char *list, char *delim, integer *nmax, integer *n, char *items, ftnlen list_len, ftnlen delim_len, ftnlen items_len);
 
extern int lparsm_(char *list, char *delims, integer *nmax, integer *n, char *items, ftnlen list_len, ftnlen delims_len, ftnlen items_len);
 
extern int lparss_(char *list, char *delims, char *set, ftnlen list_len, ftnlen delims_len, ftnlen set_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: sizec_ 4 2 13 124 */
/*:ref: scardc_ 14 3 4 13 124 */
/*:ref: insrtc_ 14 4 13 13 124 124 */
/*:ref: lastnb_ 4 2 13 124 */
/*:ref: validc_ 14 4 4 4 13 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
 
extern doublereal lspcn_(char *body, doublereal *et, char *abcorr, ftnlen body_len, ftnlen abcorr_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: bods2c_ 14 4 13 4 12 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: tipbod_ 14 5 13 4 7 7 124 */
/*:ref: spkgeo_ 14 7 4 7 13 4 7 7 124 */
/*:ref: ucrss_ 14 3 7 7 7 */
/*:ref: twovec_ 14 5 7 4 7 4 7 */
/*:ref: failed_ 12 0 */
/*:ref: spkezr_ 14 11 13 7 13 13 13 7 7 124 124 124 124 */
/*:ref: mxv_ 14 3 7 7 7 */
/*:ref: recrad_ 14 4 7 7 7 7 */
 
extern integer lstcld_(doublereal *x, integer *n, doublereal *array);
 
extern integer lstcli_(integer *x, integer *n, integer *array);
 
extern integer lstlec_(char *string, integer *n, char *array, ftnlen string_len, ftnlen array_len);
 
extern integer lstled_(doublereal *x, integer *n, doublereal *array);
 
extern integer lstlei_(integer *x, integer *n, integer *array);
 
extern integer lstltc_(char *string, integer *n, char *array, ftnlen string_len, ftnlen array_len);
 
extern integer lstltd_(doublereal *x, integer *n, doublereal *array);
 
extern integer lstlti_(integer *x, integer *n, integer *array);
 
extern int ltime_(doublereal *etobs, integer *obs, char *dir, integer *targ, doublereal *ettarg, doublereal *elapsd, ftnlen dir_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: rtrim_ 4 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: clight_ 7 0 */
/*:ref: spkgeo_ 14 7 4 7 13 4 7 7 124 */
/*:ref: vdist_ 7 2 7 7 */
/*:ref: failed_ 12 0 */
 
extern integer ltrim_(char *string, ftnlen string_len);
/*:ref: frstnb_ 4 2 13 124 */
 
extern int lun2fn_(integer *lunit, char *filnam, ftnlen filnam_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int lx4dec_(char *string, integer *first, integer *last, integer *nchar, ftnlen string_len);
/*:ref: lx4uns_ 14 5 13 4 4 4 124 */
/*:ref: lx4sgn_ 14 5 13 4 4 4 124 */
 
extern int lx4num_(char *string, integer *first, integer *last, integer *nchar, ftnlen string_len);
/*:ref: lx4dec_ 14 5 13 4 4 4 124 */
/*:ref: lx4sgn_ 14 5 13 4 4 4 124 */
 
extern int lx4sgn_(char *string, integer *first, integer *last, integer *nchar, ftnlen string_len);
/*:ref: lx4uns_ 14 5 13 4 4 4 124 */
 
extern int lx4uns_(char *string, integer *first, integer *last, integer *nchar, ftnlen string_len);
 
extern int lxname_(char *hdchrs, char *tlchrs, char *string, integer *first, integer *last, integer *idspec, integer *nchar, ftnlen hdchrs_len, ftnlen tlchrs_len, ftnlen string_len);
extern int lxidnt_(integer *idspec, char *string, integer *first, integer *last, integer *nchar, ftnlen string_len);
extern int lxdfid_(integer *idspec);
extern int lxcsid_(char *hdchrs, char *tlchrs, integer *idspec, ftnlen hdchrs_len, ftnlen tlchrs_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: rtrim_ 4 2 13 124 */
/*:ref: bsrchi_ 4 3 4 4 4 */
/*:ref: ssizei_ 14 2 4 4 */
/*:ref: validi_ 14 3 4 4 4 */
/*:ref: scardi_ 14 2 4 4 */
/*:ref: appndi_ 14 2 4 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: insrti_ 14 2 4 4 */
/*:ref: cardi_ 4 1 4 */
 
extern int lxqstr_(char *string, char *qchar, integer *first, integer *last, integer *nchar, ftnlen string_len, ftnlen qchar_len);
 
extern int m2eul_(doublereal *r__, integer *axis3, integer *axis2, integer *axis1, doublereal *angle3, doublereal *angle2, doublereal *angle1);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: isrot_ 12 3 7 7 7 */
/*:ref: vhat_ 14 2 7 7 */
/*:ref: cleard_ 14 2 4 7 */
/*:ref: mxm_ 14 3 7 7 7 */
/*:ref: mtxm_ 14 3 7 7 7 */
 
extern int m2q_(doublereal *r__, doublereal *q);
/*:ref: isrot_ 12 3 7 7 7 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern logical matchi_(char *string, char *templ, char *wstr, char *wchr, ftnlen string_len, ftnlen templ_len, ftnlen wstr_len, ftnlen wchr_len);
/*:ref: frstnb_ 4 2 13 124 */
/*:ref: lastnb_ 4 2 13 124 */
/*:ref: samch_ 12 6 13 4 13 4 124 124 */
/*:ref: nechr_ 12 4 13 13 124 124 */
/*:ref: samchi_ 12 6 13 4 13 4 124 124 */
 
extern logical matchw_(char *string, char *templ, char *wstr, char *wchr, ftnlen string_len, ftnlen templ_len, ftnlen wstr_len, ftnlen wchr_len);
/*:ref: frstnb_ 4 2 13 124 */
/*:ref: lastnb_ 4 2 13 124 */
/*:ref: samch_ 12 6 13 4 13 4 124 124 */
 
extern int maxac_(char *array, integer *ndim, char *maxval, integer *loc, ftnlen array_len, ftnlen maxval_len);
 
extern int maxad_(doublereal *array, integer *ndim, doublereal *maxval, integer *loc);
 
extern int maxai_(integer *array, integer *ndim, integer *maxval, integer *loc);
 
extern int mequ_(doublereal *m1, doublereal *mout);
/*:ref: moved_ 14 3 7 4 7 */
 
extern int mequg_(doublereal *m1, integer *nr, integer *nc, doublereal *mout);
/*:ref: moved_ 14 3 7 4 7 */
 
extern int minac_(char *array, integer *ndim, char *minval, integer *loc, ftnlen array_len, ftnlen minval_len);
 
extern int minad_(doublereal *array, integer *ndim, doublereal *minval, integer *loc);
 
extern int minai_(integer *array, integer *ndim, integer *minval, integer *loc);
 
extern int movec_(char *arrfrm, integer *ndim, char *arrto, ftnlen arrfrm_len, ftnlen arrto_len);
 
extern int movei_(integer *arrfrm, integer *ndim, integer *arrto);
 
extern int mtxm_(doublereal *m1, doublereal *m2, doublereal *mout);
/*:ref: moved_ 14 3 7 4 7 */
 
extern int mtxmg_(doublereal *m1, doublereal *m2, integer *nc1, integer *nr1r2, integer *nc2, doublereal *mout);
 
extern int mtxv_(doublereal *matrix, doublereal *vin, doublereal *vout);
 
extern int mtxvg_(doublereal *m1, doublereal *v2, integer *nc1, integer *nr1r2, doublereal *vout);
 
extern int mxm_(doublereal *m1, doublereal *m2, doublereal *mout);
/*:ref: moved_ 14 3 7 4 7 */
 
extern int mxmg_(doublereal *m1, doublereal *m2, integer *row1, integer *col1, integer *col2, doublereal *mout);
 
extern int mxmt_(doublereal *m1, doublereal *m2, doublereal *mout);
/*:ref: moved_ 14 3 7 4 7 */
 
extern int mxmtg_(doublereal *m1, doublereal *m2, integer *nr1, integer *nc1c2, integer *nr2, doublereal *mout);
 
extern int mxv_(doublereal *matrix, doublereal *vin, doublereal *vout);
 
extern int mxvg_(doublereal *m1, doublereal *v2, integer *nr1, integer *nc1r2, doublereal *vout);
 
extern integer nblen_(char *string, ftnlen string_len);
/*:ref: lastnb_ 4 2 13 124 */
/*:ref: frstnb_ 4 2 13 124 */
 
extern integer nbwid_(char *array, integer *nelt, ftnlen array_len);
 
extern integer ncpos_(char *str, char *chars, integer *start, ftnlen str_len, ftnlen chars_len);
 
extern integer ncposr_(char *str, char *chars, integer *start, ftnlen str_len, ftnlen chars_len);
 
extern int nearpt_(doublereal *positn, doublereal *a, doublereal *b, doublereal *c__, doublereal *npoint, doublereal *alt);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: vequ_ 14 2 7 7 */
/*:ref: orderd_ 14 3 7 4 4 */
/*:ref: reordd_ 14 3 4 4 7 */
/*:ref: vsclip_ 14 2 7 7 */
/*:ref: vnorm_ 7 1 7 */
/*:ref: dpmax_ 7 0 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: touchd_ 7 1 7 */
/*:ref: brcktd_ 7 3 7 7 7 */
/*:ref: approx_ 12 3 7 7 7 */
/*:ref: vdist_ 7 2 7 7 */
/*:ref: surfnm_ 14 5 7 7 7 7 7 */
/*:ref: vperp_ 14 3 7 7 7 */
/*:ref: vadd_ 14 3 7 7 7 */
 
extern int nextwd_(char *string, char *next, char *rest, ftnlen string_len, ftnlen next_len, ftnlen rest_len);
/*:ref: ljust_ 14 4 13 13 124 124 */
 
extern logical notru_(logical *logcls, integer *n);
 
extern int nparsd_(char *string, doublereal *x, char *error, integer *ptr, ftnlen string_len, ftnlen error_len);
/*:ref: dpmax_ 7 0 */
/*:ref: zzinssub_ 14 7 13 13 4 13 124 124 124 */
/*:ref: prefix_ 14 5 13 4 13 124 124 */
/*:ref: pi_ 7 0 */
 
extern int nparsi_(char *string, integer *n, char *error, integer *pnter, ftnlen string_len, ftnlen error_len);
/*:ref: intmax_ 4 0 */
/*:ref: intmin_ 4 0 */
/*:ref: nparsd_ 14 6 13 7 13 4 124 124 */
 
extern int npedln_(doublereal *a, doublereal *b, doublereal *c__, doublereal *linept, doublereal *linedr, doublereal *pnear, doublereal *dist);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: unorm_ 14 3 7 7 7 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: vminus_ 14 2 7 7 */
/*:ref: surfpt_ 14 7 7 7 7 7 7 7 12 */
/*:ref: vscl_ 14 3 7 7 7 */
/*:ref: nvc2pl_ 14 3 7 7 7 */
/*:ref: inedpl_ 14 6 7 7 7 7 7 12 */
/*:ref: pjelpl_ 14 3 7 7 7 */
/*:ref: vprjp_ 14 3 7 7 7 */
/*:ref: npelpt_ 14 4 7 7 7 7 */
/*:ref: failed_ 12 0 */
/*:ref: vprjpi_ 14 5 7 7 7 7 12 */
/*:ref: vsclip_ 14 2 7 7 */
 
extern int npelpt_(doublereal *point, doublereal *ellips, doublereal *pnear, doublereal *dist);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: el2cgv_ 14 4 7 7 7 7 */
/*:ref: vnorm_ 7 1 7 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: vsclip_ 14 2 7 7 */
/*:ref: vsub_ 14 3 7 7 7 */
/*:ref: twovec_ 14 5 7 4 7 4 7 */
/*:ref: mxv_ 14 3 7 7 7 */
/*:ref: vequ_ 14 2 7 7 */
/*:ref: vpack_ 14 4 7 7 7 7 */
/*:ref: nearpt_ 14 6 7 7 7 7 7 7 */
/*:ref: mtxv_ 14 3 7 7 7 */
/*:ref: vadd_ 14 3 7 7 7 */
/*:ref: vdist_ 7 2 7 7 */
 
extern int nplnpt_(doublereal *linpt, doublereal *lindir, doublereal *point, doublereal *pnear, doublereal *dist);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: vzero_ 12 1 7 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: vsub_ 14 3 7 7 7 */
/*:ref: vproj_ 14 3 7 7 7 */
/*:ref: vadd_ 14 3 7 7 7 */
/*:ref: vdist_ 7 2 7 7 */
 
extern int nthwd_(char *string, integer *nth, char *word, integer *loc, ftnlen string_len, ftnlen word_len);
 
extern int nvc2pl_(doublereal *normal, doublereal *const__, doublereal *plane);
/*:ref: return_ 12 0 */
/*:ref: unorm_ 14 3 7 7 7 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: vminus_ 14 2 7 7 */
/*:ref: vequ_ 14 2 7 7 */
 
extern int nvp2pl_(doublereal *normal, doublereal *point, doublereal *plane);
/*:ref: return_ 12 0 */
/*:ref: vzero_ 12 1 7 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: vhat_ 14 2 7 7 */
/*:ref: vdot_ 7 2 7 7 */
/*:ref: vminus_ 14 2 7 7 */
/*:ref: vequ_ 14 2 7 7 */
 
extern logical odd_(integer *i__);
 
extern logical opsgnd_(doublereal *x, doublereal *y);
 
extern logical opsgni_(integer *x, integer *y);
 
extern integer ordc_(char *item, char *set, ftnlen item_len, ftnlen set_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: bsrchc_ 4 5 13 4 13 124 124 */
/*:ref: cardc_ 4 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern integer ordd_(doublereal *item, doublereal *set);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: bsrchd_ 4 3 7 4 7 */
/*:ref: cardd_ 4 1 7 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int orderc_(char *array, integer *ndim, integer *iorder, ftnlen array_len);
/*:ref: swapi_ 14 2 4 4 */
 
extern int orderd_(doublereal *array, integer *ndim, integer *iorder);
/*:ref: swapi_ 14 2 4 4 */
 
extern int orderi_(integer *array, integer *ndim, integer *iorder);
/*:ref: swapi_ 14 2 4 4 */
 
extern integer ordi_(integer *item, integer *set);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: bsrchi_ 4 3 4 4 4 */
/*:ref: cardi_ 4 1 4 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int oscelt_(doublereal *state, doublereal *et, doublereal *mu, doublereal *elts);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: vequ_ 14 2 7 7 */
/*:ref: vzero_ 12 1 7 */
/*:ref: vnorm_ 7 1 7 */
/*:ref: vcrss_ 14 3 7 7 7 */
/*:ref: vpack_ 14 4 7 7 7 7 */
/*:ref: vlcom_ 14 5 7 7 7 7 7 */
/*:ref: vdot_ 7 2 7 7 */
/*:ref: vsclip_ 14 2 7 7 */
/*:ref: exact_ 7 3 7 7 7 */
/*:ref: vsep_ 7 2 7 7 */
/*:ref: pi_ 7 0 */
/*:ref: twopi_ 7 0 */
/*:ref: ucrss_ 14 3 7 7 7 */
/*:ref: vhat_ 14 2 7 7 */
/*:ref: dacosh_ 7 1 7 */
 
extern int outmsg_(char *list, ftnlen list_len);
/*:ref: lparse_ 14 8 13 13 4 4 13 124 124 124 */
/*:ref: ucase_ 14 4 13 13 124 124 */
/*:ref: getdev_ 14 2 13 124 */
/*:ref: wrline_ 14 4 13 13 124 124 */
/*:ref: msgsel_ 12 2 13 124 */
/*:ref: tkvrsn_ 14 4 13 13 124 124 */
/*:ref: getsms_ 14 2 13 124 */
/*:ref: expln_ 14 4 13 13 124 124 */
/*:ref: rtrim_ 4 2 13 124 */
/*:ref: getlms_ 14 2 13 124 */
/*:ref: wdcnt_ 4 2 13 124 */
/*:ref: nextwd_ 14 6 13 13 13 124 124 124 */
/*:ref: ljust_ 14 4 13 13 124 124 */
/*:ref: trcdep_ 14 1 4 */
/*:ref: trcnam_ 14 3 4 13 124 */
/*:ref: lastnb_ 4 2 13 124 */
/*:ref: suffix_ 14 5 13 4 13 124 124 */
 
extern int packac_(char *in, integer *pack, integer *npack, integer *maxout, integer *nout, char *out, ftnlen in_len, ftnlen out_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int packad_(doublereal *in, integer *pack, integer *npack, integer *maxout, integer *nout, doublereal *out);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int packai_(integer *in, integer *pack, integer *npack, integer *maxout, integer *nout, integer *out);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int parsqs_(char *string, char *qchar, char *value, integer *length, logical *error, char *errmsg, integer *ptr, ftnlen string_len, ftnlen qchar_len, ftnlen value_len, ftnlen errmsg_len);
/*:ref: frstnb_ 4 2 13 124 */
/*:ref: lastnb_ 4 2 13 124 */
 
extern int partof_(doublereal *ma, doublereal *d__);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dcbrt_ 7 1 7 */
 
extern int pck03a_(integer *handle, integer *ncsets, doublereal *coeffs, doublereal *epochs);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errhan_ 14 3 13 4 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: sgwfpk_ 14 5 4 4 7 4 7 */
 
extern int pck03b_(integer *handle, char *segid, integer *body, char *frame, doublereal *first, doublereal *last, integer *chbdeg, ftnlen segid_len, ftnlen frame_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: pckpds_ 14 7 4 13 4 7 7 7 124 */
/*:ref: failed_ 12 0 */
/*:ref: sgbwfs_ 14 8 4 7 13 4 7 4 4 124 */
 
extern int pck03e_(integer *handle);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: sgwes_ 14 1 4 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int pckbsr_(char *fname, integer *handle, integer *body, doublereal *et, doublereal *descr, char *ident, logical *found, ftnlen fname_len, ftnlen ident_len);
extern int pcklof_(char *fname, integer *handle, ftnlen fname_len);
extern int pckuof_(integer *handle);
extern int pcksfs_(integer *body, doublereal *et, integer *handle, doublereal *descr, char *ident, logical *found, ftnlen ident_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: lnkini_ 14 2 4 4 */
/*:ref: dafopr_ 14 3 13 4 124 */
/*:ref: failed_ 12 0 */
/*:ref: isrchi_ 4 3 4 4 4 */
/*:ref: dafcls_ 14 1 4 */
/*:ref: lnknxt_ 4 2 4 4 */
/*:ref: lnkfsl_ 14 3 4 4 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: intmax_ 4 0 */
/*:ref: moved_ 14 3 7 4 7 */
/*:ref: lnkprv_ 4 2 4 4 */
/*:ref: dpmin_ 7 0 */
/*:ref: dpmax_ 7 0 */
/*:ref: cleard_ 14 2 4 7 */
/*:ref: dafbfs_ 14 1 4 */
/*:ref: daffna_ 14 1 12 */
/*:ref: dafgs_ 14 1 7 */
/*:ref: dafus_ 14 5 7 4 4 7 4 */
/*:ref: dafbbs_ 14 1 4 */
/*:ref: daffpa_ 14 1 12 */
/*:ref: lnknfn_ 4 1 4 */
/*:ref: lnkan_ 14 2 4 4 */
/*:ref: dafgn_ 14 2 13 124 */
/*:ref: lnkilb_ 14 3 4 4 4 */
/*:ref: lnkila_ 14 3 4 4 4 */
/*:ref: lnktl_ 4 2 4 4 */
 
extern int pckcls_(integer *handle);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dafbfs_ 14 1 4 */
/*:ref: daffna_ 14 1 12 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errhan_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: dafcls_ 14 1 4 */
 
extern int pcke02_(doublereal *et, doublereal *record, doublereal *eulang);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: spke02_ 14 3 7 7 7 */
/*:ref: twopi_ 7 0 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int pcke03_(doublereal *et, doublereal *record, doublereal *rotmat);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: chbval_ 14 5 7 4 7 7 7 */
/*:ref: rpd_ 7 0 */
/*:ref: halfpi_ 7 0 */
/*:ref: eul2m_ 14 7 7 7 7 4 4 4 7 */
/*:ref: vcrss_ 14 3 7 7 7 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int pckeul_(integer *body, doublereal *et, logical *found, char *ref, doublereal *eulang, ftnlen ref_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: pcksfs_ 14 7 4 7 4 7 13 12 124 */
/*:ref: dafus_ 14 5 7 4 4 7 4 */
/*:ref: irfnam_ 14 3 4 13 124 */
/*:ref: pckr02_ 14 4 4 7 7 7 */
/*:ref: pcke02_ 14 3 7 7 7 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int pckmat_(integer *body, doublereal *et, integer *ref, doublereal *tsipm, logical *found);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: pcksfs_ 14 7 4 7 4 7 13 12 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dafus_ 14 5 7 4 4 7 4 */
/*:ref: pckr02_ 14 4 4 7 7 7 */
/*:ref: pcke02_ 14 3 7 7 7 */
/*:ref: eul2xf_ 14 5 7 4 4 4 7 */
/*:ref: sgfcon_ 14 5 4 7 4 4 7 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: pckr03_ 14 4 4 7 7 7 */
/*:ref: pcke03_ 14 3 7 7 7 */
 
extern int pckopn_(char *name__, char *ifname, integer *ncomch, integer *handle, ftnlen name_len, ftnlen ifname_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dafonw_ 14 10 13 13 4 4 13 4 4 124 124 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int pckpds_(integer *body, char *frame, integer *type__, doublereal *first, doublereal *last, doublereal *descr, ftnlen frame_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: irfnum_ 14 3 13 4 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: etcal_ 14 3 7 13 124 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: dafps_ 14 5 4 4 7 4 7 */
 
extern int pckr02_(integer *handle, doublereal *descr, doublereal *et, doublereal *record);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dafus_ 14 5 7 4 4 7 4 */
/*:ref: dafgda_ 14 4 4 4 4 7 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int pckr03_(integer *handle, doublereal *descr, doublereal *et, doublereal *record);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: sgfcon_ 14 5 4 7 4 4 7 */
/*:ref: sgfrvi_ 14 6 4 7 7 7 4 12 */
/*:ref: sgfpkt_ 14 6 4 7 4 4 7 4 */
 
extern int pckuds_(doublereal *descr, integer *body, integer *frame, integer *type__, doublereal *first, doublereal *last, integer *begin, integer *end);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dafus_ 14 5 7 4 4 7 4 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int pckw02_(integer *handle, integer *body, char *frame, doublereal *first, doublereal *last, char *segid, doublereal *intlen, integer *n, integer *polydg, doublereal *cdata, doublereal *btime, ftnlen frame_len, ftnlen segid_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: irfnum_ 14 3 13 4 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: etcal_ 14 3 7 13 124 */
/*:ref: chckid_ 14 5 13 4 13 124 124 */
/*:ref: failed_ 12 0 */
/*:ref: dafps_ 14 5 4 4 7 4 7 */
/*:ref: dafbna_ 14 4 4 7 13 124 */
/*:ref: dafada_ 14 2 7 4 */
/*:ref: dafena_ 14 0 */
 
extern integer pcwid_(char *array, integer *nelt, ftnlen array_len);
 
extern int pgrrec_(char *body, doublereal *lon, doublereal *lat, doublereal *alt, doublereal *re, doublereal *f, doublereal *rectan, ftnlen body_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: bods2c_ 14 4 13 4 12 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: repmi_ 14 7 13 13 4 13 124 124 124 */
/*:ref: gcpool_ 14 8 13 4 4 4 13 12 124 124 */
/*:ref: cmprss_ 14 7 13 4 13 13 124 124 124 */
/*:ref: ucase_ 14 4 13 13 124 124 */
/*:ref: plnsns_ 4 1 4 */
/*:ref: georec_ 14 6 7 7 7 7 7 7 */
 
extern doublereal pi_(void);
 
extern int pjelpl_(doublereal *elin, doublereal *plane, doublereal *elout);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: el2cgv_ 14 4 7 7 7 7 */
/*:ref: pl2nvc_ 14 3 7 7 7 */
/*:ref: vperp_ 14 3 7 7 7 */
/*:ref: vprjp_ 14 3 7 7 7 */
/*:ref: cgv2el_ 14 4 7 7 7 7 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int pl2nvc_(doublereal *plane, doublereal *normal, doublereal *const__);
/*:ref: vequ_ 14 2 7 7 */
 
extern int pl2nvp_(doublereal *plane, doublereal *normal, doublereal *point);
/*:ref: pl2nvc_ 14 3 7 7 7 */
/*:ref: vscl_ 14 3 7 7 7 */
 
extern int pl2psv_(doublereal *plane, doublereal *point, doublereal *span1, doublereal *span2);
/*:ref: pl2nvp_ 14 3 7 7 7 */
/*:ref: frame_ 14 3 7 7 7 */
 
extern integer plnsns_(integer *bodid);
/*:ref: repmi_ 14 7 13 13 4 13 124 124 124 */
/*:ref: dtpool_ 14 6 13 12 4 13 124 124 */
/*:ref: gdpool_ 14 7 13 4 4 4 7 12 124 */
 
extern int polyds_(doublereal *coeffs, integer *deg, integer *nderiv, doublereal *t, doublereal *p);
 
extern int pool_(char *kernel, integer *unit, char *name__, char *names, integer *nnames, char *agent, integer *n, doublereal *values, logical *found, logical *update, integer *start, integer *room, char *cvals, integer *ivals, char *type__, ftnlen kernel_len, ftnlen name_len, ftnlen names_len, ftnlen agent_len, ftnlen cvals_len, ftnlen type_len);
extern int clpool_(void);
extern int ldpool_(char *kernel, ftnlen kernel_len);
extern int rtpool_(char *name__, integer *n, doublereal *values, logical *found, ftnlen name_len);
extern int expool_(char *name__, logical *found, ftnlen name_len);
extern int wrpool_(integer *unit);
extern int swpool_(char *agent, integer *nnames, char *names, ftnlen agent_len, ftnlen names_len);
extern int cvpool_(char *agent, logical *update, ftnlen agent_len);
extern int gcpool_(char *name__, integer *start, integer *room, integer *n, char *cvals, logical *found, ftnlen name_len, ftnlen cvals_len);
extern int gdpool_(char *name__, integer *start, integer *room, integer *n, doublereal *values, logical *found, ftnlen name_len);
extern int gipool_(char *name__, integer *start, integer *room, integer *n, integer *ivals, logical *found, ftnlen name_len);
extern int dtpool_(char *name__, logical *found, integer *n, char *type__, ftnlen name_len, ftnlen type_len);
extern int pcpool_(char *name__, integer *n, char *cvals, ftnlen name_len, ftnlen cvals_len);
extern int pdpool_(char *name__, integer *n, doublereal *values, ftnlen name_len);
extern int pipool_(char *name__, integer *n, integer *ivals, ftnlen name_len);
extern int lmpool_(char *cvals, integer *n, ftnlen cvals_len);
extern int szpool_(char *name__, integer *n, logical *found, ftnlen name_len);
extern int dvpool_(char *name__, ftnlen name_len);
extern int gnpool_(char *name__, integer *start, integer *room, integer *n, char *cvals, logical *found, ftnlen name_len, ftnlen cvals_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzpini_ 14 26 12 4 4 4 13 13 4 4 4 4 4 4 4 13 4 13 13 13 13 124 124 124 124 124 124 124 */
/*:ref: lnkini_ 14 2 4 4 */
/*:ref: copyc_ 14 4 13 13 124 124 */
/*:ref: cardc_ 4 2 13 124 */
/*:ref: syfetc_ 14 9 4 13 4 13 13 12 124 124 124 */
/*:ref: sygetc_ 14 11 13 13 4 13 4 13 12 124 124 124 124 */
/*:ref: validc_ 14 4 4 4 13 124 */
/*:ref: unionc_ 14 6 13 13 13 124 124 124 */
/*:ref: rdknew_ 14 2 13 124 */
/*:ref: zzrvar_ 14 13 4 4 13 4 4 7 4 13 13 12 124 124 124 */
/*:ref: failed_ 12 0 */
/*:ref: cltext_ 14 2 13 124 */
/*:ref: zzhash_ 4 2 13 124 */
/*:ref: ioerr_ 14 5 13 13 4 124 124 */
/*:ref: rtrim_ 4 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: bsrchc_ 4 5 13 4 13 124 124 */
/*:ref: sypshc_ 14 9 13 13 13 4 13 124 124 124 124 */
/*:ref: syordc_ 14 7 13 13 4 13 124 124 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: insrtc_ 14 4 13 13 124 124 */
/*:ref: elemc_ 12 4 13 13 124 124 */
/*:ref: removc_ 14 4 13 13 124 124 */
/*:ref: intmax_ 4 0 */
/*:ref: intmin_ 4 0 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: zzgpnm_ 14 15 4 4 13 4 4 7 4 13 13 12 4 4 124 124 124 */
/*:ref: lnknfn_ 4 1 4 */
/*:ref: zzcln_ 14 7 4 4 4 4 4 4 4 */
/*:ref: lnkfsl_ 14 3 4 4 4 */
/*:ref: lnkan_ 14 2 4 4 */
/*:ref: lnkila_ 14 3 4 4 4 */
/*:ref: zzrvbf_ 14 17 13 4 4 4 4 13 4 4 7 4 13 13 12 124 124 124 124 */
/*:ref: eqstr_ 12 4 13 13 124 124 */
/*:ref: matchi_ 12 8 13 13 13 13 124 124 124 124 */
 
extern integer pos_(char *str, char *substr, integer *start, ftnlen str_len, ftnlen substr_len);
 
extern integer posr_(char *str, char *substr, integer *start, ftnlen str_len, ftnlen substr_len);
 
extern int prefix_(char *pref, integer *spaces, char *string, ftnlen pref_len, ftnlen string_len);
/*:ref: lastnb_ 4 2 13 124 */
/*:ref: shiftr_ 14 7 13 4 13 13 124 124 124 */
 
extern doublereal prodad_(doublereal *array, integer *n);
 
extern integer prodai_(integer *array, integer *n);
 
extern int prompt_(char *prmpt, char *string, ftnlen prmpt_len, ftnlen string_len);
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: errint_ 14 3 13 4 124 */
 
extern int prop2b_(doublereal *gm, doublereal *pvinit, doublereal *dt, doublereal *pvprop);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: vzero_ 12 1 7 */
/*:ref: vnorm_ 7 1 7 */
/*:ref: vdot_ 7 2 7 7 */
/*:ref: vcrss_ 14 3 7 7 7 */
/*:ref: vlcom_ 14 5 7 7 7 7 7 */
/*:ref: dpmax_ 7 0 */
/*:ref: brckti_ 4 3 4 4 4 */
/*:ref: brcktd_ 7 3 7 7 7 */
/*:ref: stmp03_ 14 5 7 7 7 7 7 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: vequg_ 14 3 7 4 7 */
 
extern int prsdp_(char *string, doublereal *dpval, ftnlen string_len);
/*:ref: nparsd_ 14 6 13 7 13 4 124 124 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int prsint_(char *string, integer *intval, ftnlen string_len);
/*:ref: nparsi_ 14 6 13 4 13 4 124 124 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int prtenc_(integer *number, char *string, ftnlen string_len);
extern int prtdec_(char *string, integer *number, ftnlen string_len);
/*:ref: chkin_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern logical prtpkg_(logical *short__, logical *long__, logical *expl, logical *trace, logical *dfault, char *type__, ftnlen type_len);
extern logical setprt_(logical *short__, logical *expl, logical *long__, logical *trace, logical *dfault);
extern logical msgsel_(char *type__, ftnlen type_len);
/*:ref: getdev_ 14 2 13 124 */
/*:ref: wrline_ 14 4 13 13 124 124 */
/*:ref: ljust_ 14 4 13 13 124 124 */
/*:ref: ucase_ 14 4 13 13 124 124 */
 
extern int psv2pl_(doublereal *point, doublereal *span1, doublereal *span2, doublereal *plane);
/*:ref: return_ 12 0 */
/*:ref: ucrss_ 14 3 7 7 7 */
/*:ref: vzero_ 12 1 7 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: vdot_ 7 2 7 7 */
/*:ref: vminus_ 14 2 7 7 */
/*:ref: vequ_ 14 2 7 7 */
 
extern int putact_(integer *action);
extern int getact_(integer *action);
 
extern int putdev_(char *device, ftnlen device_len);
extern int getdev_(char *device, ftnlen device_len);
 
extern int putlms_(char *msg, ftnlen msg_len);
extern int getlms_(char *msg, ftnlen msg_len);
 
extern int putsms_(char *msg, ftnlen msg_len);
extern int getsms_(char *msg, ftnlen msg_len);
 
extern int pxform_(char *from, char *to, doublereal *et, doublereal *rotate, ftnlen from_len, ftnlen to_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: namfrm_ 14 3 13 4 124 */
/*:ref: refchg_ 14 4 4 4 7 7 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int q2m_(doublereal *q, doublereal *r__);
 
extern int qderiv_(integer *n, doublereal *f0, doublereal *f2, doublereal *delta, doublereal *dfdt);
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: vlcomg_ 14 6 4 7 7 7 7 7 */
 
extern int qdq2av_(doublereal *q, doublereal *dq, doublereal *av);
/*:ref: vhatg_ 14 3 7 4 7 */
/*:ref: vminus_ 14 2 7 7 */
/*:ref: qxq_ 14 3 7 7 7 */
/*:ref: vscl_ 14 3 7 7 7 */
 
extern int quote_(char *in, char *left, char *right, char *out, ftnlen in_len, ftnlen left_len, ftnlen right_len, ftnlen out_len);
/*:ref: suffix_ 14 5 13 4 13 124 124 */
/*:ref: frstnb_ 4 2 13 124 */
/*:ref: lastnb_ 4 2 13 124 */
/*:ref: prefix_ 14 5 13 4 13 124 124 */
 
extern int qxq_(doublereal *q1, doublereal *q2, doublereal *qout);
/*:ref: vdot_ 7 2 7 7 */
/*:ref: vcrss_ 14 3 7 7 7 */
/*:ref: vlcom3_ 14 7 7 7 7 7 7 7 7 */
 
extern int radrec_(doublereal *range, doublereal *ra, doublereal *dec, doublereal *rectan);
/*:ref: latrec_ 14 4 7 7 7 7 */
 
extern int rav2xf_(doublereal *rot, doublereal *av, doublereal *xform);
/*:ref: mxm_ 14 3 7 7 7 */
 
extern int raxisa_(doublereal *matrix, doublereal *axis, doublereal *angle);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: m2q_ 14 2 7 7 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: vzero_ 12 1 7 */
/*:ref: pi_ 7 0 */
/*:ref: vhat_ 14 2 7 7 */
/*:ref: vnorm_ 7 1 7 */
 
extern int rdencc_(integer *unit, integer *n, char *data, ftnlen data_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: hx2int_ 14 6 13 4 12 13 124 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
 
extern int rdencd_(integer *unit, integer *n, doublereal *data);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: hx2dp_ 14 6 13 7 12 13 124 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
 
extern int rdenci_(integer *unit, integer *n, integer *data);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: hx2int_ 14 6 13 4 12 13 124 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
 
extern int rdker_(char *kernel, char *line, integer *number, logical *eof, ftnlen kernel_len, ftnlen line_len);
extern int rdknew_(char *kernel, ftnlen kernel_len);
extern int rdkdat_(char *line, logical *eof, ftnlen line_len);
extern int rdklin_(char *kernel, integer *number, ftnlen kernel_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: cltext_ 14 2 13 124 */
/*:ref: zzsetnnread_ 14 1 12 */
/*:ref: rdtext_ 14 5 13 13 12 124 124 */
/*:ref: ljust_ 14 4 13 13 124 124 */
/*:ref: failed_ 12 0 */
 
extern int rdkvar_(char *tabsym, integer *tabptr, doublereal *tabval, char *name__, logical *eof, ftnlen tabsym_len, ftnlen name_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: failed_ 12 0 */
/*:ref: rdkdat_ 14 3 13 12 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: replch_ 14 8 13 13 13 13 124 124 124 124 */
/*:ref: nextwd_ 14 6 13 13 13 124 124 124 */
/*:ref: sydeld_ 14 6 13 13 4 7 124 124 */
/*:ref: tparse_ 14 5 13 7 13 124 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: nparsd_ 14 6 13 7 13 4 124 124 */
/*:ref: syenqd_ 14 7 13 7 13 4 7 124 124 */
 
extern int rdnbl_(char *file, char *line, logical *eof, ftnlen file_len, ftnlen line_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: rdtext_ 14 5 13 13 12 124 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int rdtext_(char *file, char *line, logical *eof, ftnlen file_len, ftnlen line_len);
extern int cltext_(char *file, ftnlen file_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: isrchi_ 4 3 4 4 4 */
/*:ref: getlun_ 14 1 4 */
 
extern int readla_(integer *unit, integer *maxlin, integer *numlin, char *array, logical *eof, ftnlen array_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: readln_ 14 4 4 13 12 124 */
/*:ref: failed_ 12 0 */
 
extern int readln_(integer *unit, char *line, logical *eof, ftnlen line_len);
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errfnm_ 14 3 13 4 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int reccyl_(doublereal *rectan, doublereal *r__, doublereal *long__, doublereal *z__);
/*:ref: twopi_ 7 0 */
 
extern int recgeo_(doublereal *rectan, doublereal *re, doublereal *f, doublereal *long__, doublereal *lat, doublereal *alt);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: nearpt_ 14 6 7 7 7 7 7 7 */
/*:ref: surfnm_ 14 5 7 7 7 7 7 */
/*:ref: reclat_ 14 4 7 7 7 7 */
 
extern int reclat_(doublereal *rectan, doublereal *radius, doublereal *long__, doublereal *lat);
 
extern int recpgr_(char *body, doublereal *rectan, doublereal *re, doublereal *f, doublereal *lon, doublereal *lat, doublereal *alt, ftnlen body_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: bods2c_ 14 4 13 4 12 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: repmi_ 14 7 13 13 4 13 124 124 124 */
/*:ref: gcpool_ 14 8 13 4 4 4 13 12 124 124 */
/*:ref: cmprss_ 14 7 13 4 13 13 124 124 124 */
/*:ref: ucase_ 14 4 13 13 124 124 */
/*:ref: plnsns_ 4 1 4 */
/*:ref: recgeo_ 14 6 7 7 7 7 7 7 */
/*:ref: twopi_ 7 0 */
/*:ref: brcktd_ 7 3 7 7 7 */
 
extern int recrad_(doublereal *rectan, doublereal *range, doublereal *ra, doublereal *dec);
/*:ref: reclat_ 14 4 7 7 7 7 */
/*:ref: twopi_ 7 0 */
 
extern int recsph_(doublereal *rectan, doublereal *r__, doublereal *colat, doublereal *long__);
 
extern int refchg_(integer *frame1, integer *frame2, doublereal *et, doublereal *rotate);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: ident_ 14 1 7 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: frinfo_ 14 5 4 4 4 4 12 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: rotget_ 14 5 4 7 7 4 12 */
/*:ref: zzrxr_ 14 3 7 4 7 */
/*:ref: isrchi_ 4 3 4 4 4 */
/*:ref: frmnam_ 14 3 4 13 124 */
/*:ref: rtrim_ 4 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: xpose_ 14 2 7 7 */
 
extern int remlac_(integer *ne, integer *loc, char *array, integer *na, ftnlen array_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int remlad_(integer *ne, integer *loc, doublereal *array, integer *na);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int remlai_(integer *ne, integer *loc, integer *array, integer *na);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int removc_(char *item, char *a, ftnlen item_len, ftnlen a_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardc_ 4 2 13 124 */
/*:ref: bsrchc_ 4 5 13 4 13 124 124 */
/*:ref: scardc_ 14 3 4 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int removd_(doublereal *item, doublereal *a);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardd_ 4 1 7 */
/*:ref: bsrchd_ 4 3 7 4 7 */
/*:ref: scardd_ 14 2 4 7 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int removi_(integer *item, integer *a);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardi_ 4 1 4 */
/*:ref: bsrchi_ 4 3 4 4 4 */
/*:ref: scardi_ 14 2 4 4 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int remsub_(char *in, integer *left, integer *right, char *out, ftnlen in_len, ftnlen out_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: lastnb_ 4 2 13 124 */
 
extern int reordc_(integer *iorder, integer *ndim, char *array, ftnlen array_len);
 
extern int reordd_(integer *iorder, integer *ndim, doublereal *array);
 
extern int reordi_(integer *iorder, integer *ndim, integer *array);
 
extern int reordl_(integer *iorder, integer *ndim, logical *array);
 
extern int replch_(char *instr, char *old, char *new__, char *outstr, ftnlen instr_len, ftnlen old_len, ftnlen new_len, ftnlen outstr_len);
 
extern int replwd_(char *instr, integer *nth, char *new__, char *outstr, ftnlen instr_len, ftnlen new_len, ftnlen outstr_len);
/*:ref: nthwd_ 14 6 13 4 13 4 124 124 */
/*:ref: fndnwd_ 14 5 13 4 4 4 124 */
/*:ref: frstnb_ 4 2 13 124 */
/*:ref: lastnb_ 4 2 13 124 */
 
extern int repmc_(char *in, char *marker, char *value, char *out, ftnlen in_len, ftnlen marker_len, ftnlen value_len, ftnlen out_len);
/*:ref: frstnb_ 4 2 13 124 */
/*:ref: lastnb_ 4 2 13 124 */
/*:ref: zzrepsub_ 14 8 13 4 4 13 13 124 124 124 */
 
extern int repmct_(char *in, char *marker, integer *value, char *case__, char *out, ftnlen in_len, ftnlen marker_len, ftnlen case_len, ftnlen out_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: ljust_ 14 4 13 13 124 124 */
/*:ref: ucase_ 14 4 13 13 124 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: frstnb_ 4 2 13 124 */
/*:ref: lastnb_ 4 2 13 124 */
/*:ref: inttxt_ 14 3 4 13 124 */
/*:ref: lcase_ 14 4 13 13 124 124 */
/*:ref: repsub_ 14 8 13 4 4 13 13 124 124 124 */
 
extern int repmd_(char *in, char *marker, doublereal *value, integer *sigdig, char *out, ftnlen in_len, ftnlen marker_len, ftnlen out_len);
/*:ref: frstnb_ 4 2 13 124 */
/*:ref: lastnb_ 4 2 13 124 */
/*:ref: dpstr_ 14 4 7 4 13 124 */
/*:ref: zzrepsub_ 14 8 13 4 4 13 13 124 124 124 */
 
extern int repmf_(char *in, char *marker, doublereal *value, integer *sigdig, char *format, char *out, ftnlen in_len, ftnlen marker_len, ftnlen format_len, ftnlen out_len);
/*:ref: frstnb_ 4 2 13 124 */
/*:ref: lastnb_ 4 2 13 124 */
/*:ref: ljust_ 14 4 13 13 124 124 */
/*:ref: ucase_ 14 4 13 13 124 124 */
/*:ref: dpstrf_ 14 6 7 4 13 13 124 124 */
/*:ref: zzrepsub_ 14 8 13 4 4 13 13 124 124 124 */
 
extern int repmi_(char *in, char *marker, integer *value, char *out, ftnlen in_len, ftnlen marker_len, ftnlen out_len);
/*:ref: frstnb_ 4 2 13 124 */
/*:ref: lastnb_ 4 2 13 124 */
/*:ref: intstr_ 14 3 4 13 124 */
/*:ref: zzrepsub_ 14 8 13 4 4 13 13 124 124 124 */
 
extern int repmot_(char *in, char *marker, integer *value, char *case__, char *out, ftnlen in_len, ftnlen marker_len, ftnlen case_len, ftnlen out_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: ljust_ 14 4 13 13 124 124 */
/*:ref: ucase_ 14 4 13 13 124 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: frstnb_ 4 2 13 124 */
/*:ref: lastnb_ 4 2 13 124 */
/*:ref: intord_ 14 3 4 13 124 */
/*:ref: lcase_ 14 4 13 13 124 124 */
/*:ref: repsub_ 14 8 13 4 4 13 13 124 124 124 */
 
extern int repsub_(char *in, integer *left, integer *right, char *string, char *out, ftnlen in_len, ftnlen string_len, ftnlen out_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: sumai_ 4 2 4 4 */
 
extern int reset_(void);
/*:ref: seterr_ 12 1 12 */
/*:ref: putsms_ 14 2 13 124 */
/*:ref: putlms_ 14 2 13 124 */
/*:ref: accept_ 12 1 12 */
 
extern logical return_(void);
/*:ref: getact_ 14 1 4 */
/*:ref: failed_ 12 0 */
 
extern int rjust_(char *input, char *output, ftnlen input_len, ftnlen output_len);
/*:ref: frstnb_ 4 2 13 124 */
/*:ref: lastnb_ 4 2 13 124 */
 
extern int rmaind_(doublereal *num, doublereal *denom, doublereal *q, doublereal *rem);
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int rmaini_(integer *num, integer *denom, integer *q, integer *rem);
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int rmdupc_(integer *nelt, char *array, ftnlen array_len);
/*:ref: shellc_ 14 3 4 13 124 */
 
extern int rmdupd_(integer *nelt, doublereal *array);
/*:ref: shelld_ 14 2 4 7 */
 
extern int rmdupi_(integer *nelt, integer *array);
/*:ref: shelli_ 14 2 4 4 */
 
extern int rotate_(doublereal *angle, integer *iaxis, doublereal *mout);
 
extern int rotget_(integer *infrm, doublereal *et, doublereal *rotate, integer *outfrm, logical *found);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: frinfo_ 14 5 4 4 4 4 12 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: irfrot_ 14 3 4 4 7 */
/*:ref: tipbod_ 14 5 13 4 7 7 124 */
/*:ref: xpose_ 14 2 7 7 */
/*:ref: namfrm_ 14 3 13 4 124 */
/*:ref: failed_ 12 0 */
/*:ref: ckfrot_ 14 5 4 7 7 4 12 */
/*:ref: tkfram_ 14 4 4 7 4 12 */
/*:ref: zzdynrot_ 14 5 4 4 7 7 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
 
extern int rotmat_(doublereal *m1, doublereal *angle, integer *iaxis, doublereal *mout);
/*:ref: moved_ 14 3 7 4 7 */
 
extern int rotvec_(doublereal *v1, doublereal *angle, integer *iaxis, doublereal *vout);
 
extern doublereal rpd_(void);
 
extern int rquad_(doublereal *a, doublereal *b, doublereal *c__, doublereal *root1, doublereal *root2);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: moved_ 14 3 7 4 7 */
 
extern integer rtrim_(char *string, ftnlen string_len);
/*:ref: lastnb_ 4 2 13 124 */
 
extern int saelgv_(doublereal *vec1, doublereal *vec2, doublereal *smajor, doublereal *sminor);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: moved_ 14 3 7 4 7 */
/*:ref: vnorm_ 7 1 7 */
/*:ref: cleard_ 14 2 4 7 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: vdot_ 7 2 7 7 */
/*:ref: diags2_ 14 3 7 7 7 */
/*:ref: vlcom_ 14 5 7 7 7 7 7 */
/*:ref: vsclip_ 14 2 7 7 */
 
extern logical samch_(char *str1, integer *l1, char *str2, integer *l2, ftnlen str1_len, ftnlen str2_len);
 
extern logical samchi_(char *str1, integer *l1, char *str2, integer *l2, ftnlen str1_len, ftnlen str2_len);
/*:ref: eqchr_ 12 4 13 13 124 124 */
 
extern logical sameai_(integer *a1, integer *a2, integer *ndim);
 
extern logical samsbi_(char *str1, integer *b1, integer *e1, char *str2, integer *b2, integer *e2, ftnlen str1_len, ftnlen str2_len);
/*:ref: nechr_ 12 4 13 13 124 124 */
 
extern logical samsub_(char *str1, integer *b1, integer *e1, char *str2, integer *b2, integer *e2, ftnlen str1_len, ftnlen str2_len);
 
extern int sc01_(integer *sc, char *clkstr, doublereal *ticks, doublereal *sclkdp, doublereal *et, ftnlen clkstr_len);
extern int sctk01_(integer *sc, char *clkstr, doublereal *ticks, ftnlen clkstr_len);
extern int scfm01_(integer *sc, doublereal *ticks, char *clkstr, ftnlen clkstr_len);
extern int scte01_(integer *sc, doublereal *sclkdp, doublereal *et);
extern int scet01_(integer *sc, doublereal *et, doublereal *sclkdp);
extern int scec01_(integer *sc, doublereal *et, doublereal *sclkdp);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: movec_ 14 5 13 4 13 124 124 */
/*:ref: suffix_ 14 5 13 4 13 124 124 */
/*:ref: repmi_ 14 7 13 13 4 13 124 124 124 */
/*:ref: swpool_ 14 5 13 4 13 124 124 */
/*:ref: cvpool_ 14 3 13 12 124 */
/*:ref: scli01_ 14 6 13 4 4 4 4 124 */
/*:ref: scld01_ 14 6 13 4 4 4 7 124 */
/*:ref: failed_ 12 0 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: lparsm_ 14 8 13 13 4 4 13 124 124 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: nparsd_ 14 6 13 7 13 4 124 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sumai_ 4 2 4 4 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: dpstrf_ 14 6 7 4 13 13 124 124 */
/*:ref: prefix_ 14 5 13 4 13 124 124 */
/*:ref: unitim_ 7 5 7 13 13 124 124 */
 
extern int scanit_(char *string, integer *start, integer *room, integer *nmarks, char *marks, integer *mrklen, integer *pnters, integer *ntokns, integer *ident, integer *beg, integer *end, ftnlen string_len, ftnlen marks_len);
extern int scanpr_(integer *nmarks, char *marks, integer *mrklen, integer *pnters, ftnlen marks_len);
extern int scan_(char *string, char *marks, integer *mrklen, integer *pnters, integer *room, integer *start, integer *ntokns, integer *ident, integer *beg, integer *end, ftnlen string_len, ftnlen marks_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: ljust_ 14 4 13 13 124 124 */
/*:ref: rmdupc_ 14 3 4 13 124 */
/*:ref: rtrim_ 4 2 13 124 */
/*:ref: ncpos_ 4 5 13 13 4 124 124 */
 
extern int scanrj_(integer *ids, integer *n, integer *ntokns, integer *ident, integer *beg, integer *end);
/*:ref: isrchi_ 4 3 4 4 4 */
 
extern int scardc_(integer *card, char *cell, ftnlen cell_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dechar_ 14 3 13 4 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: enchar_ 14 3 4 13 124 */
 
extern int scardd_(integer *card, doublereal *cell);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int scardi_(integer *card, integer *cell);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int scdecd_(integer *sc, doublereal *sclkdp, char *sclkch, ftnlen sclkch_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: scpart_ 14 4 4 4 7 7 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: lstled_ 4 3 7 4 7 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: scfmt_ 14 4 4 7 13 124 */
/*:ref: intstr_ 14 3 4 13 124 */
/*:ref: suffix_ 14 5 13 4 13 124 124 */
/*:ref: lastnb_ 4 2 13 124 */
/*:ref: prefix_ 14 5 13 4 13 124 124 */
 
extern int sce2c_(integer *sc, doublereal *et, doublereal *sclkdp);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: sctype_ 4 1 4 */
/*:ref: scec01_ 14 3 4 7 7 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int sce2s_(integer *sc, doublereal *et, char *sclkch, ftnlen sclkch_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: sce2t_ 14 3 4 7 7 */
/*:ref: scdecd_ 14 4 4 7 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int sce2t_(integer *sc, doublereal *et, doublereal *sclkdp);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: sctype_ 4 1 4 */
/*:ref: scet01_ 14 3 4 7 7 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int scencd_(integer *sc, char *sclkch, doublereal *sclkdp, ftnlen sclkch_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cpos_ 4 5 13 13 4 124 124 */
/*:ref: sctiks_ 14 4 4 13 7 124 */
/*:ref: scpart_ 14 4 4 4 7 7 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: nparsi_ 14 6 13 4 13 4 124 124 */
 
extern int scfmt_(integer *sc, doublereal *ticks, char *clkstr, ftnlen clkstr_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: sctype_ 4 1 4 */
/*:ref: scfm01_ 14 4 4 7 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int sclu01_(char *name__, integer *sc, integer *maxnv, integer *n, integer *ival, doublereal *dval, ftnlen name_len);
extern int scli01_(char *name__, integer *sc, integer *maxnv, integer *n, integer *ival, ftnlen name_len);
extern int scld01_(char *name__, integer *sc, integer *maxnv, integer *n, doublereal *dval, ftnlen name_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: suffix_ 14 5 13 4 13 124 124 */
/*:ref: repmi_ 14 7 13 13 4 13 124 124 124 */
/*:ref: rtpool_ 14 5 13 4 7 12 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: isrchc_ 4 5 13 4 13 124 124 */
/*:ref: repmc_ 14 8 13 13 13 13 124 124 124 124 */
/*:ref: repmd_ 14 8 13 13 7 4 13 124 124 124 */
 
extern int scpars_(integer *sc, char *sclkch, logical *error, char *msg, doublereal *sclkdp, ftnlen sclkch_len, ftnlen msg_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: cpos_ 4 5 13 13 4 124 124 */
/*:ref: sctype_ 4 1 4 */
/*:ref: failed_ 12 0 */
/*:ref: scps01_ 14 7 4 13 12 13 7 124 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: scpart_ 14 4 4 4 7 7 */
/*:ref: repmc_ 14 8 13 13 13 13 124 124 124 124 */
/*:ref: nparsi_ 14 6 13 4 13 4 124 124 */
/*:ref: repmi_ 14 7 13 13 4 13 124 124 124 */
 
extern int scpart_(integer *sc, integer *nparts, doublereal *pstart, doublereal *pstop);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: suffix_ 14 5 13 4 13 124 124 */
/*:ref: repmi_ 14 7 13 13 4 13 124 124 124 */
/*:ref: swpool_ 14 5 13 4 13 124 124 */
/*:ref: cvpool_ 14 3 13 12 124 */
/*:ref: scld01_ 14 6 13 4 4 4 7 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int scps01_(integer *sc, char *clkstr, logical *error, char *msg, doublereal *ticks, ftnlen clkstr_len, ftnlen msg_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: scli01_ 14 6 13 4 4 4 4 124 */
/*:ref: scld01_ 14 6 13 4 4 4 7 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: lparsm_ 14 8 13 13 4 4 13 124 124 124 */
/*:ref: repmc_ 14 8 13 13 13 13 124 124 124 124 */
/*:ref: repmi_ 14 7 13 13 4 13 124 124 124 */
/*:ref: nparsd_ 14 6 13 7 13 4 124 124 */
 
extern int scs2e_(integer *sc, char *sclkch, doublereal *et, ftnlen sclkch_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: scencd_ 14 4 4 13 7 124 */
/*:ref: sct2e_ 14 3 4 7 7 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int sct2e_(integer *sc, doublereal *sclkdp, doublereal *et);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: sctype_ 4 1 4 */
/*:ref: scte01_ 14 3 4 7 7 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int sctiks_(integer *sc, char *clkstr, doublereal *ticks, ftnlen clkstr_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: sctype_ 4 1 4 */
/*:ref: sctk01_ 14 4 4 13 7 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int sctran_(char *clknam, integer *clkid, logical *found, ftnlen clknam_len);
extern int scn2id_(char *clknam, integer *clkid, logical *found, ftnlen clknam_len);
extern int scid2n_(integer *clkid, char *clknam, logical *found, ftnlen clknam_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: ucase_ 14 4 13 13 124 124 */
/*:ref: posr_ 4 5 13 13 4 124 124 */
/*:ref: rtrim_ 4 2 13 124 */
/*:ref: bodn2c_ 14 4 13 4 12 124 */
/*:ref: bodc2n_ 14 4 4 13 12 124 */
/*:ref: suffix_ 14 5 13 4 13 124 124 */
 
extern integer sctype_(integer *sc);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: suffix_ 14 5 13 4 13 124 124 */
/*:ref: repmi_ 14 7 13 13 4 13 124 124 124 */
/*:ref: swpool_ 14 5 13 4 13 124 124 */
/*:ref: cvpool_ 14 3 13 12 124 */
/*:ref: scli01_ 14 6 13 4 4 4 4 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int sdiffc_(char *a, char *b, char *c__, ftnlen a_len, ftnlen b_len, ftnlen c_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: cardc_ 4 2 13 124 */
/*:ref: sizec_ 4 2 13 124 */
/*:ref: scardc_ 14 3 4 13 124 */
/*:ref: excess_ 14 3 4 13 124 */
 
extern int sdiffd_(doublereal *a, doublereal *b, doublereal *c__);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardd_ 4 1 7 */
/*:ref: sized_ 4 1 7 */
/*:ref: scardd_ 14 2 4 7 */
/*:ref: excess_ 14 3 4 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int sdiffi_(integer *a, integer *b, integer *c__);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardi_ 4 1 4 */
/*:ref: sizei_ 4 1 4 */
/*:ref: scardi_ 14 2 4 4 */
/*:ref: excess_ 14 3 4 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern logical setc_(char *a, char *op, char *b, ftnlen a_len, ftnlen op_len, ftnlen b_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardc_ 4 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
 
extern logical setd_(doublereal *a, char *op, doublereal *b, ftnlen op_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardd_ 4 1 7 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
 
extern logical seterr_(logical *status);
extern logical failed_(void);
 
extern logical seti_(integer *a, char *op, integer *b, ftnlen op_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardi_ 4 1 4 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
 
extern int setmsg_(char *msg, ftnlen msg_len);
/*:ref: allowd_ 12 0 */
/*:ref: putlms_ 14 2 13 124 */
 
extern int sgfcon_(integer *handle, doublereal *descr, integer *first, integer *last, doublereal *values);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: sgmeta_ 14 4 4 7 4 4 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: dafgda_ 14 4 4 4 4 7 */
 
extern int sgfpkt_(integer *handle, doublereal *descr, integer *first, integer *last, doublereal *values, integer *ends);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: sgmeta_ 14 4 4 7 4 4 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: dafgda_ 14 4 4 4 4 7 */
 
extern int sgfref_(integer *handle, doublereal *descr, integer *first, integer *last, doublereal *values);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: sgmeta_ 14 4 4 7 4 4 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: dafgda_ 14 4 4 4 4 7 */
 
extern int sgfrvi_(integer *handle, doublereal *descr, doublereal *x, doublereal *value, integer *indx, logical *found);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: intmax_ 4 0 */
/*:ref: sgmeta_ 14 4 4 7 4 4 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: dafgda_ 14 4 4 4 4 7 */
/*:ref: lstled_ 4 3 7 4 7 */
/*:ref: errdp_ 14 3 13 7 124 */
 
extern int sgmeta_(integer *handle, doublereal *descr, integer *mnemon, integer *value);
/*:ref: return_ 12 0 */
/*:ref: moved_ 14 3 7 4 7 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dafhsf_ 14 3 4 4 4 */
/*:ref: failed_ 12 0 */
/*:ref: dafgda_ 14 4 4 4 4 7 */
 
extern int sgseqw_(integer *handle, doublereal *descr, char *segid, integer *nconst, doublereal *const__, integer *npkts, integer *pktsiz, doublereal *pktdat, integer *nrefs, doublereal *refdat, integer *idxtyp, ftnlen segid_len);
extern int sgbwfs_(integer *handle, doublereal *descr, char *segid, integer *nconst, doublereal *const__, integer *pktsiz, integer *idxtyp, ftnlen segid_len);
extern int sgbwvs_(integer *handle, doublereal *descr, char *segid, integer *nconst, doublereal *const__, integer *idxtyp, ftnlen segid_len);
extern int sgwfpk_(integer *handle, integer *npkts, doublereal *pktdat, integer *nrefs, doublereal *refdat);
extern int sgwvpk_(integer *handle, integer *npkts, integer *pktsiz, doublereal *pktdat, integer *nrefs, doublereal *refdat);
extern int sgwes_(integer *handle);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dafsih_ 14 3 4 13 124 */
/*:ref: failed_ 12 0 */
/*:ref: isrchi_ 4 3 4 4 4 */
/*:ref: errhan_ 14 3 13 4 124 */
/*:ref: dafhsf_ 14 3 4 4 4 */
/*:ref: lastnb_ 4 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: dafrfr_ 14 8 4 4 4 13 4 4 4 124 */
/*:ref: dafbna_ 14 4 4 7 13 124 */
/*:ref: dafada_ 14 2 7 4 */
/*:ref: dafcad_ 14 1 4 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: dafgda_ 14 4 4 4 4 7 */
/*:ref: dafena_ 14 0 */
 
extern int sharpr_(doublereal *rot);
/*:ref: vhatip_ 14 1 7 */
/*:ref: ucrss_ 14 3 7 7 7 */
 
extern int shellc_(integer *ndim, char *array, ftnlen array_len);
/*:ref: swapc_ 14 4 13 13 124 124 */
 
extern int shelld_(integer *ndim, doublereal *array);
/*:ref: swapd_ 14 2 7 7 */
 
extern int shelli_(integer *ndim, integer *array);
/*:ref: swapi_ 14 2 4 4 */
 
extern int shiftc_(char *in, char *dir, integer *nshift, char *fillc, char *out, ftnlen in_len, ftnlen dir_len, ftnlen fillc_len, ftnlen out_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: shiftl_ 14 7 13 4 13 13 124 124 124 */
/*:ref: shiftr_ 14 7 13 4 13 13 124 124 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int shiftl_(char *in, integer *nshift, char *fillc, char *out, ftnlen in_len, ftnlen fillc_len, ftnlen out_len);
 
extern int shiftr_(char *in, integer *nshift, char *fillc, char *out, ftnlen in_len, ftnlen fillc_len, ftnlen out_len);
 
extern int sigdgt_(char *in, char *out, ftnlen in_len, ftnlen out_len);
/*:ref: frstnb_ 4 2 13 124 */
/*:ref: lastnb_ 4 2 13 124 */
/*:ref: cpos_ 4 5 13 13 4 124 124 */
 
extern int sigerr_(char *msg, ftnlen msg_len);
/*:ref: getact_ 14 1 4 */
/*:ref: failed_ 12 0 */
/*:ref: seterr_ 12 1 12 */
/*:ref: putsms_ 14 2 13 124 */
/*:ref: freeze_ 14 0 */
/*:ref: outmsg_ 14 2 13 124 */
/*:ref: accept_ 12 1 12 */
/*:ref: byebye_ 14 2 13 124 */
 
extern integer sizec_(char *cell, ftnlen cell_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dechar_ 14 3 13 4 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern integer sized_(doublereal *cell);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern integer sizei_(integer *cell);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern logical smsgnd_(doublereal *x, doublereal *y);
 
extern logical smsgni_(integer *x, integer *y);
 
extern logical somfls_(logical *logcls, integer *n);
 
extern logical somtru_(logical *logcls, integer *n);
 
extern int spca2b_(char *text, char *binary, ftnlen text_len, ftnlen binary_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: txtopr_ 14 3 13 4 124 */
/*:ref: spct2b_ 14 3 4 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int spcac_(integer *handle, integer *unit, char *bmark, char *emark, ftnlen bmark_len, ftnlen emark_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dafsih_ 14 3 4 13 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: locln_ 14 10 4 13 13 13 4 4 12 124 124 124 */
/*:ref: countc_ 4 5 4 4 4 13 124 */
/*:ref: dafrfr_ 14 8 4 4 4 13 4 4 4 124 */
/*:ref: zzddhhlu_ 14 5 4 13 12 4 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: errfnm_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: dafarr_ 14 2 4 4 */
/*:ref: lastnb_ 4 2 13 124 */
 
extern int spcb2a_(char *binary, char *text, ftnlen binary_len, ftnlen text_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: txtopn_ 14 3 13 4 124 */
/*:ref: spcb2t_ 14 3 13 4 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int spcb2t_(char *binary, integer *unit, ftnlen binary_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dafb2t_ 14 3 13 4 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errfnm_ 14 3 13 4 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dafopr_ 14 3 13 4 124 */
/*:ref: spcec_ 14 2 4 4 */
/*:ref: dafcls_ 14 1 4 */
 
extern int spcdc_(integer *handle);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dafrfr_ 14 8 4 4 4 13 4 4 4 124 */
/*:ref: dafrrr_ 14 2 4 4 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int spcec_(integer *handle, integer *unit);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dafsih_ 14 3 4 13 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dafrfr_ 14 8 4 4 4 13 4 4 4 124 */
/*:ref: zzddhhlu_ 14 5 4 13 12 4 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: errfnm_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
 
extern int spcopn_(char *spc, char *ifname, integer *handle, ftnlen spc_len, ftnlen ifname_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dafopn_ 14 8 13 4 4 13 4 4 124 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int spcrfl_(integer *handle, char *line, logical *eoc, ftnlen line_len);
extern int spcrnl_(char *line, logical *eoc, ftnlen line_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dafrfr_ 14 8 4 4 4 13 4 4 4 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzddhhlu_ 14 5 4 13 12 4 124 */
/*:ref: failed_ 12 0 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: errfnm_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: pos_ 4 5 13 13 4 124 124 */
 
extern int spct2b_(integer *unit, char *binary, ftnlen binary_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: daft2b_ 14 4 4 13 4 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: errfnm_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: ltrim_ 4 2 13 124 */
/*:ref: getlun_ 14 1 4 */
/*:ref: rtrim_ 4 2 13 124 */
/*:ref: dafopw_ 14 3 13 4 124 */
/*:ref: spcac_ 14 6 4 4 13 13 124 124 */
/*:ref: dafcls_ 14 1 4 */
 
extern doublereal spd_(void);
 
extern int sphcyl_(doublereal *radius, doublereal *colat, doublereal *slong, doublereal *r__, doublereal *long__, doublereal *z__);
 
extern int sphlat_(doublereal *r__, doublereal *colat, doublereal *longs, doublereal *radius, doublereal *long__, doublereal *lat);
/*:ref: halfpi_ 7 0 */
 
extern int sphrec_(doublereal *r__, doublereal *colat, doublereal *long__, doublereal *rectan);
 
extern doublereal sphsd_(doublereal *radius, doublereal *long1, doublereal *lat1, doublereal *long2, doublereal *lat2);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: brcktd_ 7 3 7 7 7 */
 
extern int spk14a_(integer *handle, integer *ncsets, doublereal *coeffs, doublereal *epochs);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errhan_ 14 3 13 4 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: sgwfpk_ 14 5 4 4 7 4 7 */
 
extern int spk14b_(integer *handle, char *segid, integer *body, integer *center, char *frame, doublereal *first, doublereal *last, integer *chbdeg, ftnlen segid_len, ftnlen frame_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: spkpds_ 14 8 4 4 13 4 7 7 7 124 */
/*:ref: failed_ 12 0 */
/*:ref: sgbwfs_ 14 8 4 7 13 4 7 4 4 124 */
 
extern int spk14e_(integer *handle);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: sgwes_ 14 1 4 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int spkapo_(integer *targ, doublereal *et, char *ref, doublereal *sobs, char *abcorr, doublereal *ptarg, doublereal *lt, ftnlen ref_len, ftnlen abcorr_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cmprss_ 14 7 13 4 13 13 124 124 124 */
/*:ref: ucase_ 14 4 13 13 124 124 */
/*:ref: isrchc_ 4 5 13 4 13 124 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: odd_ 12 1 4 */
/*:ref: irfnum_ 14 3 13 4 124 */
/*:ref: spkgps_ 14 7 4 7 13 4 7 7 124 */
/*:ref: vsub_ 14 3 7 7 7 */
/*:ref: vequ_ 14 2 7 7 */
/*:ref: vnorm_ 7 1 7 */
/*:ref: clight_ 7 0 */
/*:ref: stlabx_ 14 3 7 7 7 */
/*:ref: stelab_ 14 3 7 7 7 */
 
extern int spkapp_(integer *targ, doublereal *et, char *ref, doublereal *sobs, char *abcorr, doublereal *starg, doublereal *lt, ftnlen ref_len, ftnlen abcorr_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cmprss_ 14 7 13 4 13 13 124 124 124 */
/*:ref: ucase_ 14 4 13 13 124 124 */
/*:ref: isrchc_ 4 5 13 4 13 124 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: odd_ 12 1 4 */
/*:ref: irfnum_ 14 3 13 4 124 */
/*:ref: spkssb_ 14 5 4 7 13 7 124 */
/*:ref: vsubg_ 14 4 7 7 4 7 */
/*:ref: moved_ 14 3 7 4 7 */
/*:ref: vnorm_ 7 1 7 */
/*:ref: clight_ 7 0 */
/*:ref: stlabx_ 14 3 7 7 7 */
/*:ref: vequ_ 14 2 7 7 */
/*:ref: stelab_ 14 3 7 7 7 */
 
extern int spkbsr_(char *fname, integer *handle, integer *body, doublereal *et, doublereal *descr, char *ident, logical *found, ftnlen fname_len, ftnlen ident_len);
extern int spklef_(char *fname, integer *handle, ftnlen fname_len);
extern int spkuef_(integer *handle);
extern int spksfs_(integer *body, doublereal *et, integer *handle, doublereal *descr, char *ident, logical *found, ftnlen ident_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: lnkini_ 14 2 4 4 */
/*:ref: dafopr_ 14 3 13 4 124 */
/*:ref: failed_ 12 0 */
/*:ref: isrchi_ 4 3 4 4 4 */
/*:ref: dafcls_ 14 1 4 */
/*:ref: lnknxt_ 4 2 4 4 */
/*:ref: lnkfsl_ 14 3 4 4 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: intmax_ 4 0 */
/*:ref: moved_ 14 3 7 4 7 */
/*:ref: lnkprv_ 4 2 4 4 */
/*:ref: dpmin_ 7 0 */
/*:ref: dpmax_ 7 0 */
/*:ref: cleard_ 14 2 4 7 */
/*:ref: dafbfs_ 14 1 4 */
/*:ref: daffna_ 14 1 12 */
/*:ref: dafgs_ 14 1 7 */
/*:ref: dafus_ 14 5 7 4 4 7 4 */
/*:ref: dafbbs_ 14 1 4 */
/*:ref: daffpa_ 14 1 12 */
/*:ref: lnknfn_ 4 1 4 */
/*:ref: lnkan_ 14 2 4 4 */
/*:ref: dafgn_ 14 2 13 124 */
/*:ref: lnkilb_ 14 3 4 4 4 */
/*:ref: lnkila_ 14 3 4 4 4 */
/*:ref: lnktl_ 4 2 4 4 */
 
extern int spkcls_(integer *handle);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dafbfs_ 14 1 4 */
/*:ref: daffna_ 14 1 12 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errhan_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: dafcls_ 14 1 4 */
 
extern int spkcov_(char *spk, integer *idcode, doublereal *cover, ftnlen spk_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: getfat_ 14 6 13 13 13 124 124 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dafopr_ 14 3 13 4 124 */
/*:ref: failed_ 12 0 */
/*:ref: dafbfs_ 14 1 4 */
/*:ref: daffna_ 14 1 12 */
/*:ref: dafgs_ 14 1 7 */
/*:ref: dafus_ 14 5 7 4 4 7 4 */
/*:ref: wninsd_ 14 3 7 7 7 */
/*:ref: dafcls_ 14 1 4 */
 
extern int spke01_(doublereal *et, doublereal *record, doublereal *state);
/*:ref: return_ 12 0 */
/*:ref: moved_ 14 3 7 4 7 */
 
extern int spke02_(doublereal *et, doublereal *record, doublereal *xyzdot);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: chbint_ 14 6 7 4 7 7 7 7 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int spke03_(doublereal *et, doublereal *record, doublereal *state);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: chbval_ 14 5 7 4 7 7 7 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int spke05_(doublereal *et, doublereal *record, doublereal *state);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: moved_ 14 3 7 4 7 */
/*:ref: prop2b_ 14 4 7 7 7 7 */
/*:ref: pi_ 7 0 */
/*:ref: vlcomg_ 14 6 4 7 7 7 7 7 */
/*:ref: vlcom_ 14 5 7 7 7 7 7 */
/*:ref: vadd_ 14 3 7 7 7 */
/*:ref: vequ_ 14 2 7 7 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int spke08_(doublereal *et, doublereal *record, doublereal *state);
/*:ref: return_ 12 0 */
/*:ref: xposeg_ 14 4 7 4 4 7 */
/*:ref: moved_ 14 3 7 4 7 */
/*:ref: lgresp_ 7 6 4 7 7 7 7 7 */
 
extern int spke09_(doublereal *et, doublereal *record, doublereal *state);
/*:ref: return_ 12 0 */
/*:ref: xposeg_ 14 4 7 4 4 7 */
/*:ref: moved_ 14 3 7 4 7 */
/*:ref: lgrint_ 7 5 4 7 7 7 7 */
 
extern int spke10_(doublereal *et, doublereal *record, doublereal *state);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: pi_ 7 0 */
/*:ref: twopi_ 7 0 */
/*:ref: spd_ 7 0 */
/*:ref: ev2lin_ 14 4 7 7 7 7 */
/*:ref: dpspce_ 14 4 7 7 7 7 */
/*:ref: brcktd_ 7 3 7 7 7 */
/*:ref: zzmobliq_ 14 3 7 7 7 */
/*:ref: eul2m_ 14 7 7 7 7 4 4 4 7 */
/*:ref: mtxv_ 14 3 7 7 7 */
/*:ref: moved_ 14 3 7 4 7 */
/*:ref: vlcomg_ 14 6 4 7 7 7 7 7 */
/*:ref: vlcom_ 14 5 7 7 7 7 7 */
/*:ref: vadd_ 14 3 7 7 7 */
/*:ref: vequ_ 14 2 7 7 */
/*:ref: zzeprcss_ 14 2 7 7 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int spke12_(doublereal *et, doublereal *record, doublereal *state);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: hrmesp_ 14 8 4 7 7 7 7 7 7 7 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int spke13_(doublereal *et, doublereal *record, doublereal *state);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: hrmint_ 14 7 4 7 7 7 7 7 7 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int spke14_(doublereal *et, doublereal *record, doublereal *state);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: chbval_ 14 5 7 4 7 7 7 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int spke15_(doublereal *et, doublereal *recin, doublereal *state);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: vequ_ 14 2 7 7 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: vzero_ 12 1 7 */
/*:ref: vhatip_ 14 1 7 */
/*:ref: vdot_ 7 2 7 7 */
/*:ref: vsep_ 7 2 7 7 */
/*:ref: dpr_ 7 0 */
/*:ref: vscl_ 14 3 7 7 7 */
/*:ref: vcrss_ 14 3 7 7 7 */
/*:ref: vsclip_ 14 2 7 7 */
/*:ref: prop2b_ 14 4 7 7 7 7 */
/*:ref: twopi_ 7 0 */
/*:ref: pi_ 7 0 */
/*:ref: vrotv_ 14 4 7 7 7 7 */
/*:ref: moved_ 14 3 7 4 7 */
 
extern int spke17_(doublereal *et, doublereal *recin, doublereal *state);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: eqncpv_ 14 6 7 7 7 7 7 7 */
 
extern int spke18_(doublereal *et, doublereal *record, doublereal *state);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: xpsgip_ 14 3 4 4 7 */
/*:ref: lgrint_ 7 5 4 7 7 7 7 */
/*:ref: hrmint_ 14 7 4 7 7 7 7 7 7 */
/*:ref: vequ_ 14 2 7 7 */
 
extern int spkez_(integer *targ, doublereal *et, char *ref, char *abcorr, integer *obs, doublereal *starg, doublereal *lt, ftnlen ref_len, ftnlen abcorr_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: namfrm_ 14 3 13 4 124 */
/*:ref: ltrim_ 4 2 13 124 */
/*:ref: eqchr_ 12 4 13 13 124 124 */
/*:ref: eqstr_ 12 4 13 13 124 124 */
/*:ref: spkgeo_ 14 7 4 7 13 4 7 7 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: frinfo_ 14 5 4 4 4 4 12 */
/*:ref: spkssb_ 14 5 4 7 13 7 124 */
/*:ref: spkapp_ 14 9 4 7 13 7 13 7 7 124 124 */
/*:ref: failed_ 12 0 */
/*:ref: frmchg_ 14 4 4 4 7 7 */
/*:ref: mxvg_ 14 5 7 7 4 4 7 */
 
extern int spkezp_(integer *targ, doublereal *et, char *ref, char *abcorr, integer *obs, doublereal *ptarg, doublereal *lt, ftnlen ref_len, ftnlen abcorr_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: namfrm_ 14 3 13 4 124 */
/*:ref: ltrim_ 4 2 13 124 */
/*:ref: eqchr_ 12 4 13 13 124 124 */
/*:ref: eqstr_ 12 4 13 13 124 124 */
/*:ref: spkgps_ 14 7 4 7 13 4 7 7 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: frinfo_ 14 5 4 4 4 4 12 */
/*:ref: spkssb_ 14 5 4 7 13 7 124 */
/*:ref: spkapo_ 14 9 4 7 13 7 13 7 7 124 124 */
/*:ref: failed_ 12 0 */
/*:ref: refchg_ 14 4 4 4 7 7 */
/*:ref: mxv_ 14 3 7 7 7 */
 
extern int spkezr_(char *targ, doublereal *et, char *ref, char *abcorr, char *obs, doublereal *starg, doublereal *lt, ftnlen targ_len, ftnlen ref_len, ftnlen abcorr_len, ftnlen obs_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: zzbodn2c_ 14 4 13 4 12 124 */
/*:ref: beint_ 12 2 13 124 */
/*:ref: nparsi_ 14 6 13 4 13 4 124 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: spkez_ 14 9 4 7 13 13 4 7 7 124 124 */
 
extern int spkgeo_(integer *targ, doublereal *et, char *ref, integer *obs, doublereal *state, doublereal *lt, ftnlen ref_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cleard_ 14 2 4 7 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: irfnum_ 14 3 13 4 124 */
/*:ref: namfrm_ 14 3 13 4 124 */
/*:ref: frstnp_ 4 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: failed_ 12 0 */
/*:ref: spksfs_ 14 7 4 7 4 7 13 12 124 */
/*:ref: spkpvn_ 14 6 4 7 7 4 7 4 */
/*:ref: moved_ 14 3 7 4 7 */
/*:ref: irfrot_ 14 3 4 4 7 */
/*:ref: mxv_ 14 3 7 7 7 */
/*:ref: frmchg_ 14 4 4 4 7 7 */
/*:ref: mxvg_ 14 5 7 7 4 4 7 */
/*:ref: vaddg_ 14 4 7 7 4 7 */
/*:ref: isrchi_ 4 3 4 4 4 */
/*:ref: bodc2n_ 14 4 4 13 12 124 */
/*:ref: prefix_ 14 5 13 4 13 124 124 */
/*:ref: suffix_ 14 5 13 4 13 124 124 */
/*:ref: repmi_ 14 7 13 13 4 13 124 124 124 */
/*:ref: intstr_ 14 3 4 13 124 */
/*:ref: etcal_ 14 3 7 13 124 */
/*:ref: vsubg_ 14 4 7 7 4 7 */
/*:ref: vnorm_ 7 1 7 */
/*:ref: clight_ 7 0 */
 
extern int spkgps_(integer *targ, doublereal *et, char *ref, integer *obs, doublereal *pos, doublereal *lt, ftnlen ref_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cleard_ 14 2 4 7 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: irfnum_ 14 3 13 4 124 */
/*:ref: namfrm_ 14 3 13 4 124 */
/*:ref: frstnp_ 4 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: failed_ 12 0 */
/*:ref: spksfs_ 14 7 4 7 4 7 13 12 124 */
/*:ref: spkpvn_ 14 6 4 7 7 4 7 4 */
/*:ref: moved_ 14 3 7 4 7 */
/*:ref: irfrot_ 14 3 4 4 7 */
/*:ref: mxv_ 14 3 7 7 7 */
/*:ref: refchg_ 14 4 4 4 7 7 */
/*:ref: vadd_ 14 3 7 7 7 */
/*:ref: vequ_ 14 2 7 7 */
/*:ref: isrchi_ 4 3 4 4 4 */
/*:ref: bodc2n_ 14 4 4 13 12 124 */
/*:ref: prefix_ 14 5 13 4 13 124 124 */
/*:ref: suffix_ 14 5 13 4 13 124 124 */
/*:ref: repmi_ 14 7 13 13 4 13 124 124 124 */
/*:ref: intstr_ 14 3 4 13 124 */
/*:ref: etcal_ 14 3 7 13 124 */
/*:ref: vsub_ 14 3 7 7 7 */
/*:ref: vnorm_ 7 1 7 */
/*:ref: clight_ 7 0 */
 
extern int spkobj_(char *spk, integer *ids, ftnlen spk_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: getfat_ 14 6 13 13 13 124 124 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dafopr_ 14 3 13 4 124 */
/*:ref: failed_ 12 0 */
/*:ref: dafbfs_ 14 1 4 */
/*:ref: daffna_ 14 1 12 */
/*:ref: dafgs_ 14 1 7 */
/*:ref: dafus_ 14 5 7 4 4 7 4 */
/*:ref: insrti_ 14 2 4 4 */
/*:ref: dafcls_ 14 1 4 */
 
extern int spkopa_(char *file, integer *handle, ftnlen file_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: exists_ 12 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: getfat_ 14 6 13 13 13 124 124 124 */
/*:ref: failed_ 12 0 */
/*:ref: dafopw_ 14 3 13 4 124 */
 
extern int spkopn_(char *name__, char *ifname, integer *ncomch, integer *handle, ftnlen name_len, ftnlen ifname_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dafonw_ 14 10 13 13 4 4 13 4 4 124 124 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int spkpds_(integer *body, integer *center, char *frame, integer *type__, doublereal *first, doublereal *last, doublereal *descr, ftnlen frame_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: namfrm_ 14 3 13 4 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: etcal_ 14 3 7 13 124 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: dafps_ 14 5 4 4 7 4 7 */
 
extern int spkpos_(char *targ, doublereal *et, char *ref, char *abcorr, char *obs, doublereal *ptarg, doublereal *lt, ftnlen targ_len, ftnlen ref_len, ftnlen abcorr_len, ftnlen obs_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: zzbodn2c_ 14 4 13 4 12 124 */
/*:ref: beint_ 12 2 13 124 */
/*:ref: nparsi_ 14 6 13 4 13 4 124 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: spkezp_ 14 9 4 7 13 13 4 7 7 124 124 */
 
extern int spkpv_(integer *handle, doublereal *descr, doublereal *et, char *ref, doublereal *state, integer *center, ftnlen ref_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dafus_ 14 5 7 4 4 7 4 */
/*:ref: namfrm_ 14 3 13 4 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: spkpvn_ 14 6 4 7 7 4 7 4 */
/*:ref: frmchg_ 14 4 4 4 7 7 */
/*:ref: mxvg_ 14 5 7 7 4 4 7 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int spkpvn_(integer *handle, doublereal *descr, doublereal *et, integer *ref, doublereal *state, integer *center);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dafus_ 14 5 7 4 4 7 4 */
/*:ref: spkr01_ 14 4 4 7 7 7 */
/*:ref: spke01_ 14 3 7 7 7 */
/*:ref: spkr02_ 14 4 4 7 7 7 */
/*:ref: spke02_ 14 3 7 7 7 */
/*:ref: spkr03_ 14 4 4 7 7 7 */
/*:ref: spke03_ 14 3 7 7 7 */
/*:ref: spkr05_ 14 4 4 7 7 7 */
/*:ref: spke05_ 14 3 7 7 7 */
/*:ref: spkr08_ 14 4 4 7 7 7 */
/*:ref: spke08_ 14 3 7 7 7 */
/*:ref: spkr09_ 14 4 4 7 7 7 */
/*:ref: spke09_ 14 3 7 7 7 */
/*:ref: spkr10_ 14 4 4 7 7 7 */
/*:ref: spke10_ 14 3 7 7 7 */
/*:ref: spkr12_ 14 4 4 7 7 7 */
/*:ref: spke12_ 14 3 7 7 7 */
/*:ref: spkr13_ 14 4 4 7 7 7 */
/*:ref: spke13_ 14 3 7 7 7 */
/*:ref: sgfcon_ 14 5 4 7 4 4 7 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: spkr14_ 14 4 4 7 7 7 */
/*:ref: spke14_ 14 3 7 7 7 */
/*:ref: spkr15_ 14 4 4 7 7 7 */
/*:ref: spke15_ 14 3 7 7 7 */
/*:ref: spkr17_ 14 4 4 7 7 7 */
/*:ref: spke17_ 14 3 7 7 7 */
/*:ref: spkr18_ 14 4 4 7 7 7 */
/*:ref: spke18_ 14 3 7 7 7 */
 
extern int spkr01_(integer *handle, doublereal *descr, doublereal *et, doublereal *record);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dafus_ 14 5 7 4 4 7 4 */
/*:ref: dafgda_ 14 4 4 4 4 7 */
/*:ref: lstltd_ 4 3 7 4 7 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int spkr02_(integer *handle, doublereal *descr, doublereal *et, doublereal *record);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dafus_ 14 5 7 4 4 7 4 */
/*:ref: dafgda_ 14 4 4 4 4 7 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int spkr03_(integer *handle, doublereal *descr, doublereal *et, doublereal *record);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dafus_ 14 5 7 4 4 7 4 */
/*:ref: dafgda_ 14 4 4 4 4 7 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int spkr05_(integer *handle, doublereal *descr, doublereal *et, doublereal *record);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dafus_ 14 5 7 4 4 7 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dafgda_ 14 4 4 4 4 7 */
/*:ref: lstltd_ 4 3 7 4 7 */
/*:ref: moved_ 14 3 7 4 7 */
 
extern int spkr08_(integer *handle, doublereal *descr, doublereal *et, doublereal *record);
/*:ref: return_ 12 0 */
/*:ref: dafus_ 14 5 7 4 4 7 4 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: dafgda_ 14 4 4 4 4 7 */
/*:ref: odd_ 12 1 4 */
 
extern int spkr09_(integer *handle, doublereal *descr, doublereal *et, doublereal *record);
/*:ref: return_ 12 0 */
/*:ref: dafus_ 14 5 7 4 4 7 4 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: dafgda_ 14 4 4 4 4 7 */
/*:ref: lstltd_ 4 3 7 4 7 */
/*:ref: odd_ 12 1 4 */
 
extern int spkr10_(integer *handle, doublereal *descr, doublereal *et, doublereal *record);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: sgfcon_ 14 5 4 7 4 4 7 */
/*:ref: sgfrvi_ 14 6 4 7 7 7 4 12 */
/*:ref: sgmeta_ 14 4 4 7 4 4 */
/*:ref: sgfpkt_ 14 6 4 7 4 4 7 4 */
/*:ref: moved_ 14 3 7 4 7 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int spkr12_(integer *handle, doublereal *descr, doublereal *et, doublereal *record);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: spkr08_ 14 4 4 7 7 7 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int spkr13_(integer *handle, doublereal *descr, doublereal *et, doublereal *record);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: spkr09_ 14 4 4 7 7 7 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int spkr14_(integer *handle, doublereal *descr, doublereal *et, doublereal *record);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: sgfcon_ 14 5 4 7 4 4 7 */
/*:ref: sgfrvi_ 14 6 4 7 7 7 4 12 */
/*:ref: sgfpkt_ 14 6 4 7 4 4 7 4 */
 
extern int spkr15_(integer *handle, doublereal *descr, doublereal *et, doublereal *record);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dafus_ 14 5 7 4 4 7 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dafgda_ 14 4 4 4 4 7 */
 
extern int spkr17_(integer *handle, doublereal *descr, doublereal *et, doublereal *record);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dafus_ 14 5 7 4 4 7 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dafgda_ 14 4 4 4 4 7 */
 
extern int spkr18_(integer *handle, doublereal *descr, doublereal *et, doublereal *record);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dafus_ 14 5 7 4 4 7 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: dafgda_ 14 4 4 4 4 7 */
/*:ref: failed_ 12 0 */
/*:ref: odd_ 12 1 4 */
/*:ref: lstltd_ 4 3 7 4 7 */
 
extern int spks01_(integer *handle, integer *baddr, integer *eaddr, doublereal *begin, doublereal *end);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dafgda_ 14 4 4 4 4 7 */
/*:ref: dafada_ 14 2 7 4 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int spks02_(integer *handle, integer *baddr, integer *eaddr, doublereal *begin, doublereal *end);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dafgda_ 14 4 4 4 4 7 */
/*:ref: dafada_ 14 2 7 4 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int spks03_(integer *handle, integer *baddr, integer *eaddr, doublereal *begin, doublereal *end);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dafgda_ 14 4 4 4 4 7 */
/*:ref: dafada_ 14 2 7 4 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int spks05_(integer *handle, integer *baddr, integer *eaddr, doublereal *begin, doublereal *end);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dafgda_ 14 4 4 4 4 7 */
/*:ref: dafada_ 14 2 7 4 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int spks08_(integer *handle, integer *baddr, integer *eaddr, doublereal *begin, doublereal *end);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dafgda_ 14 4 4 4 4 7 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dafada_ 14 2 7 4 */
 
extern int spks09_(integer *handle, integer *baddr, integer *eaddr, doublereal *begin, doublereal *end);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dafgda_ 14 4 4 4 4 7 */
/*:ref: dafada_ 14 2 7 4 */
 
extern int spks10_(integer *srchan, doublereal *srcdsc, integer *dsthan, doublereal *dstdsc, char *dstsid, ftnlen dstsid_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dafus_ 14 5 7 4 4 7 4 */
/*:ref: sgfcon_ 14 5 4 7 4 4 7 */
/*:ref: sgbwfs_ 14 8 4 7 13 4 7 4 4 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: sgfrvi_ 14 6 4 7 7 7 4 12 */
/*:ref: etcal_ 14 3 7 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: sgmeta_ 14 4 4 7 4 4 */
/*:ref: sgfpkt_ 14 6 4 7 4 4 7 4 */
/*:ref: sgfref_ 14 5 4 7 4 4 7 */
/*:ref: sgwfpk_ 14 5 4 4 7 4 7 */
/*:ref: sgwes_ 14 1 4 */
 
extern int spks12_(integer *handle, integer *baddr, integer *eaddr, doublereal *begin, doublereal *end);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: spks08_ 14 5 4 4 4 7 7 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int spks13_(integer *handle, integer *baddr, integer *eaddr, doublereal *begin, doublereal *end);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: spks09_ 14 5 4 4 4 7 7 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int spks14_(integer *srchan, doublereal *srcdsc, integer *dsthan, doublereal *dstdsc, char *dstsid, ftnlen dstsid_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dafus_ 14 5 7 4 4 7 4 */
/*:ref: irfnam_ 14 3 4 13 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: sgfcon_ 14 5 4 7 4 4 7 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: sgfrvi_ 14 6 4 7 7 7 4 12 */
/*:ref: spk14b_ 14 10 4 13 4 4 13 7 7 4 124 124 */
/*:ref: sgfpkt_ 14 6 4 7 4 4 7 4 */
/*:ref: sgfref_ 14 5 4 7 4 4 7 */
/*:ref: spk14a_ 14 4 4 4 7 7 */
/*:ref: spk14e_ 14 1 4 */
 
extern int spks15_(integer *handle, integer *baddr, integer *eaddr, doublereal *begin, doublereal *end);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dafgda_ 14 4 4 4 4 7 */
/*:ref: dafada_ 14 2 7 4 */
 
extern int spks17_(integer *handle, integer *baddr, integer *eaddr, doublereal *begin, doublereal *end);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dafgda_ 14 4 4 4 4 7 */
/*:ref: dafada_ 14 2 7 4 */
 
extern int spks18_(integer *handle, integer *baddr, integer *eaddr, doublereal *begin, doublereal *end);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dafgda_ 14 4 4 4 4 7 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: dafada_ 14 2 7 4 */
 
extern int spkssb_(integer *targ, doublereal *et, char *ref, doublereal *starg, ftnlen ref_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: spkgeo_ 14 7 4 7 13 4 7 7 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int spksub_(integer *handle, doublereal *descr, char *ident, doublereal *begin, doublereal *end, integer *newh, ftnlen ident_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dafus_ 14 5 7 4 4 7 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dafps_ 14 5 4 4 7 4 7 */
/*:ref: dafbna_ 14 4 4 7 13 124 */
/*:ref: spks01_ 14 5 4 4 4 7 7 */
/*:ref: dafena_ 14 0 */
/*:ref: spks02_ 14 5 4 4 4 7 7 */
/*:ref: spks03_ 14 5 4 4 4 7 7 */
/*:ref: spks05_ 14 5 4 4 4 7 7 */
/*:ref: spks08_ 14 5 4 4 4 7 7 */
/*:ref: spks09_ 14 5 4 4 4 7 7 */
/*:ref: spks10_ 14 6 4 7 4 7 13 124 */
/*:ref: spks12_ 14 5 4 4 4 7 7 */
/*:ref: spks13_ 14 5 4 4 4 7 7 */
/*:ref: spks14_ 14 6 4 7 4 7 13 124 */
/*:ref: spks15_ 14 5 4 4 4 7 7 */
/*:ref: spks17_ 14 5 4 4 4 7 7 */
/*:ref: spks18_ 14 5 4 4 4 7 7 */
/*:ref: errint_ 14 3 13 4 124 */
 
extern int spkuds_(doublereal *descr, integer *body, integer *center, integer *frame, integer *type__, doublereal *first, doublereal *last, integer *begin, integer *end);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dafus_ 14 5 7 4 4 7 4 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int spkw01_(integer *handle, integer *body, integer *center, char *frame, doublereal *first, doublereal *last, char *segid, integer *n, doublereal *dlines, doublereal *epochs, ftnlen frame_len, ftnlen segid_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: namfrm_ 14 3 13 4 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: lastnb_ 4 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: spkpds_ 14 8 4 4 13 4 7 7 7 124 */
/*:ref: dafbna_ 14 4 4 7 13 124 */
/*:ref: failed_ 12 0 */
/*:ref: dafada_ 14 2 7 4 */
/*:ref: dafena_ 14 0 */
 
extern int spkw02_(integer *handle, integer *body, integer *center, char *frame, doublereal *first, doublereal *last, char *segid, doublereal *intlen, integer *n, integer *polydg, doublereal *cdata, doublereal *btime, ftnlen frame_len, ftnlen segid_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: irfnum_ 14 3 13 4 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: etcal_ 14 3 7 13 124 */
/*:ref: chckid_ 14 5 13 4 13 124 124 */
/*:ref: failed_ 12 0 */
/*:ref: dafps_ 14 5 4 4 7 4 7 */
/*:ref: dafbna_ 14 4 4 7 13 124 */
/*:ref: dafada_ 14 2 7 4 */
/*:ref: dafena_ 14 0 */
 
extern int spkw03_(integer *handle, integer *body, integer *center, char *frame, doublereal *first, doublereal *last, char *segid, doublereal *intlen, integer *n, integer *polydg, doublereal *cdata, doublereal *btime, ftnlen frame_len, ftnlen segid_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: irfnum_ 14 3 13 4 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: etcal_ 14 3 7 13 124 */
/*:ref: chckid_ 14 5 13 4 13 124 124 */
/*:ref: failed_ 12 0 */
/*:ref: dafps_ 14 5 4 4 7 4 7 */
/*:ref: dafbna_ 14 4 4 7 13 124 */
/*:ref: dafada_ 14 2 7 4 */
/*:ref: dafena_ 14 0 */
 
extern int spkw05_(integer *handle, integer *body, integer *center, char *frame, doublereal *first, doublereal *last, char *segid, doublereal *gm, integer *n, doublereal *states, doublereal *epochs, ftnlen frame_len, ftnlen segid_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: irfnum_ 14 3 13 4 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: lastnb_ 4 2 13 124 */
/*:ref: dafps_ 14 5 4 4 7 4 7 */
/*:ref: dafbna_ 14 4 4 7 13 124 */
/*:ref: failed_ 12 0 */
/*:ref: dafada_ 14 2 7 4 */
/*:ref: dafena_ 14 0 */
 
extern int spkw08_(integer *handle, integer *body, integer *center, char *frame, doublereal *first, doublereal *last, char *segid, integer *degree, integer *n, doublereal *states, doublereal *epoch1, doublereal *step, ftnlen frame_len, ftnlen segid_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: namfrm_ 14 3 13 4 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: lastnb_ 4 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: dafps_ 14 5 4 4 7 4 7 */
/*:ref: dafbna_ 14 4 4 7 13 124 */
/*:ref: failed_ 12 0 */
/*:ref: dafada_ 14 2 7 4 */
/*:ref: dafena_ 14 0 */
 
extern int spkw09_(integer *handle, integer *body, integer *center, char *frame, doublereal *first, doublereal *last, char *segid, integer *degree, integer *n, doublereal *states, doublereal *epochs, ftnlen frame_len, ftnlen segid_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: namfrm_ 14 3 13 4 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: lastnb_ 4 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: dafps_ 14 5 4 4 7 4 7 */
/*:ref: dafbna_ 14 4 4 7 13 124 */
/*:ref: failed_ 12 0 */
/*:ref: dafada_ 14 2 7 4 */
/*:ref: dafena_ 14 0 */
 
extern int spkw10_(integer *handle, integer *body, integer *center, char *frame, doublereal *first, doublereal *last, char *segid, doublereal *consts, integer *n, doublereal *elems, doublereal *epochs, ftnlen frame_len, ftnlen segid_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: spkpds_ 14 8 4 4 13 4 7 7 7 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: sgbwfs_ 14 8 4 7 13 4 7 4 4 124 */
/*:ref: moved_ 14 3 7 4 7 */
/*:ref: zzwahr_ 14 2 7 7 */
/*:ref: sgwfpk_ 14 5 4 4 7 4 7 */
/*:ref: sgwes_ 14 1 4 */
 
extern int spkw12_(integer *handle, integer *body, integer *center, char *frame, doublereal *first, doublereal *last, char *segid, integer *degree, integer *n, doublereal *states, doublereal *epoch1, doublereal *step, ftnlen frame_len, ftnlen segid_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: namfrm_ 14 3 13 4 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: lastnb_ 4 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: even_ 12 1 4 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: spkpds_ 14 8 4 4 13 4 7 7 7 124 */
/*:ref: dafbna_ 14 4 4 7 13 124 */
/*:ref: failed_ 12 0 */
/*:ref: dafada_ 14 2 7 4 */
/*:ref: dafena_ 14 0 */
 
extern int spkw13_(integer *handle, integer *body, integer *center, char *frame, doublereal *first, doublereal *last, char *segid, integer *degree, integer *n, doublereal *states, doublereal *epochs, ftnlen frame_len, ftnlen segid_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: namfrm_ 14 3 13 4 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: lastnb_ 4 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: even_ 12 1 4 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: spkpds_ 14 8 4 4 13 4 7 7 7 124 */
/*:ref: dafbna_ 14 4 4 7 13 124 */
/*:ref: failed_ 12 0 */
/*:ref: dafada_ 14 2 7 4 */
/*:ref: dafena_ 14 0 */
 
extern int spkw15_(integer *handle, integer *body, integer *center, char *frame, doublereal *first, doublereal *last, char *segid, doublereal *epoch, doublereal *tp, doublereal *pa, doublereal *p, doublereal *ecc, doublereal *j2flg, doublereal *pv, doublereal *gm, doublereal *j2, doublereal *radius, ftnlen frame_len, ftnlen segid_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: vequ_ 14 2 7 7 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: vzero_ 12 1 7 */
/*:ref: vhat_ 14 2 7 7 */
/*:ref: vdot_ 7 2 7 7 */
/*:ref: vsep_ 7 2 7 7 */
/*:ref: dpr_ 7 0 */
/*:ref: lastnb_ 4 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: spkpds_ 14 8 4 4 13 4 7 7 7 124 */
/*:ref: failed_ 12 0 */
/*:ref: dafbna_ 14 4 4 7 13 124 */
/*:ref: dafada_ 14 2 7 4 */
/*:ref: dafena_ 14 0 */
 
extern int spkw17_(integer *handle, integer *body, integer *center, char *frame, doublereal *first, doublereal *last, char *segid, doublereal *epoch, doublereal *eqel, doublereal *rapol, doublereal *decpol, ftnlen frame_len, ftnlen segid_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: moved_ 14 3 7 4 7 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: lastnb_ 4 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: spkpds_ 14 8 4 4 13 4 7 7 7 124 */
/*:ref: failed_ 12 0 */
/*:ref: dafbna_ 14 4 4 7 13 124 */
/*:ref: dafada_ 14 2 7 4 */
/*:ref: dafena_ 14 0 */
 
extern int spkw18_(integer *handle, integer *subtyp, integer *body, integer *center, char *frame, doublereal *first, doublereal *last, char *segid, integer *degree, integer *n, doublereal *packts, doublereal *epochs, ftnlen frame_len, ftnlen segid_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: namfrm_ 14 3 13 4 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: lastnb_ 4 2 13 124 */
/*:ref: odd_ 12 1 4 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: failed_ 12 0 */
/*:ref: dafps_ 14 5 4 4 7 4 7 */
/*:ref: dafbna_ 14 4 4 7 13 124 */
/*:ref: dafada_ 14 2 7 4 */
/*:ref: dafena_ 14 0 */
 
extern int srfrec_(integer *body, doublereal *long__, doublereal *lat, doublereal *rectan);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: bodvcd_ 14 6 4 13 4 4 7 124 */
/*:ref: latrec_ 14 4 7 7 7 7 */
/*:ref: surfpt_ 14 7 7 7 7 7 7 7 12 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int srfxpt_(char *method, char *target, doublereal *et, char *abcorr, char *obsrvr, char *dref, doublereal *dvec, doublereal *spoint, doublereal *dist, doublereal *trgepc, doublereal *obspos, logical *found, ftnlen method_len, ftnlen target_len, ftnlen abcorr_len, ftnlen obsrvr_len, ftnlen dref_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: namfrm_ 14 3 13 4 124 */
/*:ref: bods2c_ 14 4 13 4 12 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: ljust_ 14 4 13 13 124 124 */
/*:ref: ucase_ 14 4 13 13 124 124 */
/*:ref: repmc_ 14 8 13 13 13 13 124 124 124 124 */
/*:ref: eqchr_ 12 4 13 13 124 124 */
/*:ref: cidfrm_ 14 5 4 4 13 12 124 */
/*:ref: spkezp_ 14 9 4 7 13 13 4 7 7 124 124 */
/*:ref: vminus_ 14 2 7 7 */
/*:ref: failed_ 12 0 */
/*:ref: frinfo_ 14 5 4 4 4 4 12 */
/*:ref: pxform_ 14 6 13 13 7 7 124 124 */
/*:ref: mxv_ 14 3 7 7 7 */
/*:ref: spkssb_ 14 5 4 7 13 7 124 */
/*:ref: stelab_ 14 3 7 7 7 */
/*:ref: stlabx_ 14 3 7 7 7 */
/*:ref: vsub_ 14 3 7 7 7 */
/*:ref: vadd_ 14 3 7 7 7 */
/*:ref: vequ_ 14 2 7 7 */
/*:ref: eqstr_ 12 4 13 13 124 124 */
/*:ref: bodvcd_ 14 6 4 13 4 4 7 124 */
/*:ref: vnorm_ 7 1 7 */
/*:ref: vsep_ 7 2 7 7 */
/*:ref: surfpt_ 14 7 7 7 7 7 7 7 12 */
/*:ref: npedln_ 14 7 7 7 7 7 7 7 7 */
/*:ref: vdist_ 7 2 7 7 */
/*:ref: clight_ 7 0 */
/*:ref: touchd_ 7 1 7 */
 
extern int ssizec_(integer *size, char *cell, ftnlen cell_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: enchar_ 14 3 4 13 124 */
 
extern int ssized_(integer *size, doublereal *cell);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int ssizei_(integer *size, integer *cell);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int stcc01_(char *catfnm, char *tabnam, logical *istyp1, char *errmsg, ftnlen catfnm_len, ftnlen tabnam_len, ftnlen errmsg_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: ekopr_ 14 3 13 4 124 */
/*:ref: eknseg_ 4 1 4 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: ekssum_ 14 14 4 4 13 4 4 13 13 4 4 12 12 124 124 124 */
/*:ref: isrchc_ 4 5 13 4 13 124 124 */
/*:ref: nblen_ 4 2 13 124 */
/*:ref: ekcls_ 14 1 4 */
 
extern int stcf01_(char *catnam, doublereal *westra, doublereal *eastra, doublereal *sthdec, doublereal *nthdec, integer *nstars, ftnlen catnam_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dpr_ 7 0 */
/*:ref: repmc_ 14 8 13 13 13 13 124 124 124 124 */
/*:ref: repmd_ 14 8 13 13 7 4 13 124 124 124 */
/*:ref: ekfind_ 14 6 13 4 12 13 124 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int stcg01_(integer *index, doublereal *ra, doublereal *dec, doublereal *rasig, doublereal *decsig, integer *catnum, char *sptype, doublereal *vmag, ftnlen sptype_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: ekgd_ 14 6 4 4 4 7 12 12 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: ekgi_ 14 6 4 4 4 4 12 12 */
/*:ref: ekgc_ 14 7 4 4 4 13 12 12 124 */
/*:ref: rpd_ 7 0 */
 
extern int stcl01_(char *catfnm, char *tabnam, integer *handle, ftnlen catfnm_len, ftnlen tabnam_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: stcc01_ 14 7 13 13 12 13 124 124 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: eklef_ 14 3 13 4 124 */
 
extern int stdio_(char *name__, integer *unit, ftnlen name_len);
/*:ref: ljust_ 14 4 13 13 124 124 */
/*:ref: ucase_ 14 4 13 13 124 124 */
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int stelab_(doublereal *pobj, doublereal *vobs, doublereal *appobj);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: vhat_ 14 2 7 7 */
/*:ref: clight_ 7 0 */
/*:ref: vscl_ 14 3 7 7 7 */
/*:ref: vdot_ 7 2 7 7 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: vcrss_ 14 3 7 7 7 */
/*:ref: vnorm_ 7 1 7 */
/*:ref: vrotv_ 14 4 7 7 7 7 */
/*:ref: moved_ 14 3 7 4 7 */
 
extern int stlabx_(doublereal *pobj, doublereal *vobs, doublereal *corpos);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: vminus_ 14 2 7 7 */
/*:ref: stelab_ 14 3 7 7 7 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int stmp03_(doublereal *x, doublereal *c0, doublereal *c1, doublereal *c2, doublereal *c3);
/*:ref: dpmax_ 7 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int stpool_(char *item, integer *nth, char *contin, char *string, integer *size, logical *found, ftnlen item_len, ftnlen contin_len, ftnlen string_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: rtrim_ 4 2 13 124 */
/*:ref: gcpool_ 14 8 13 4 4 4 13 12 124 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int str2et_(char *string, doublereal *et, ftnlen string_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: timdef_ 14 6 13 13 13 124 124 124 */
/*:ref: prefix_ 14 5 13 4 13 124 124 */
/*:ref: zzutcpm_ 14 7 13 4 7 7 4 12 124 */
/*:ref: tpartv_ 14 15 13 7 4 13 13 12 12 12 13 13 124 124 124 124 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: ttrans_ 14 5 13 13 7 124 124 */
/*:ref: tchckd_ 14 2 13 124 */
/*:ref: tparch_ 14 2 13 124 */
/*:ref: moved_ 14 3 7 4 7 */
/*:ref: tcheck_ 14 9 7 13 12 13 12 13 124 124 124 */
/*:ref: texpyr_ 14 1 4 */
/*:ref: jul2gr_ 14 4 4 4 4 4 */
/*:ref: repmc_ 14 8 13 13 13 13 124 124 124 124 */
/*:ref: repmi_ 14 7 13 13 4 13 124 124 124 */
/*:ref: dpfmt_ 14 5 7 13 13 124 124 */
/*:ref: gr2jul_ 14 4 4 4 4 4 */
 
extern int subpt_(char *method, char *target, doublereal *et, char *abcorr, char *obsrvr, doublereal *spoint, doublereal *alt, ftnlen method_len, ftnlen target_len, ftnlen abcorr_len, ftnlen obsrvr_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: bods2c_ 14 4 13 4 12 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: bodvcd_ 14 6 4 13 4 4 7 124 */
/*:ref: cidfrm_ 14 5 4 4 13 12 124 */
/*:ref: spkez_ 14 9 4 7 13 13 4 7 7 124 124 */
/*:ref: vminus_ 14 2 7 7 */
/*:ref: eqstr_ 12 4 13 13 124 124 */
/*:ref: nearpt_ 14 6 7 7 7 7 7 7 */
/*:ref: surfpt_ 14 7 7 7 7 7 7 7 12 */
/*:ref: vdist_ 7 2 7 7 */
 
extern int subsol_(char *method, char *target, doublereal *et, char *abcorr, char *obsrvr, doublereal *spoint, ftnlen method_len, ftnlen target_len, ftnlen abcorr_len, ftnlen obsrvr_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: bods2c_ 14 4 13 4 12 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: bodvcd_ 14 6 4 13 4 4 7 124 */
/*:ref: cidfrm_ 14 5 4 4 13 12 124 */
/*:ref: eqstr_ 12 4 13 13 124 124 */
/*:ref: ltime_ 14 7 7 4 13 4 7 7 124 */
/*:ref: spkpos_ 14 11 13 7 13 13 13 7 7 124 124 124 124 */
/*:ref: nearpt_ 14 6 7 7 7 7 7 7 */
/*:ref: surfpt_ 14 7 7 7 7 7 7 7 12 */
 
extern int suffix_(char *suff, integer *spaces, char *string, ftnlen suff_len, ftnlen string_len);
/*:ref: lastnb_ 4 2 13 124 */
 
extern doublereal sumad_(doublereal *array, integer *n);
 
extern integer sumai_(integer *array, integer *n);
 
extern int surfnm_(doublereal *a, doublereal *b, doublereal *c__, doublereal *point, doublereal *normal);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: vhatip_ 14 1 7 */
 
extern int surfpt_(doublereal *positn, doublereal *u, doublereal *a, doublereal *b, doublereal *c__, doublereal *point, logical *found);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: vzero_ 12 1 7 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: cleard_ 14 2 4 7 */
/*:ref: vperp_ 14 3 7 7 7 */
/*:ref: vsub_ 14 3 7 7 7 */
/*:ref: vnorm_ 7 1 7 */
/*:ref: vhat_ 14 2 7 7 */
/*:ref: vdot_ 7 2 7 7 */
/*:ref: vlcom_ 14 5 7 7 7 7 7 */
 
extern int swapac_(integer *n, integer *locn, integer *m, integer *locm, char *array, ftnlen array_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: swapc_ 14 4 13 13 124 124 */
/*:ref: cyacip_ 14 6 4 13 4 13 124 124 */
 
extern int swapad_(integer *n, integer *locn, integer *m, integer *locm, doublereal *array);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: swapd_ 14 2 7 7 */
/*:ref: cyadip_ 14 5 4 13 4 7 124 */
 
extern int swapai_(integer *n, integer *locn, integer *m, integer *locm, integer *array);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: swapi_ 14 2 4 4 */
/*:ref: cyaiip_ 14 5 4 13 4 4 124 */
 
extern int swapc_(char *a, char *b, ftnlen a_len, ftnlen b_len);
 
extern int swapd_(doublereal *a, doublereal *b);
 
extern int swapi_(integer *a, integer *b);
 
extern int sxform_(char *from, char *to, doublereal *et, doublereal *xform, ftnlen from_len, ftnlen to_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: namfrm_ 14 3 13 4 124 */
/*:ref: frmchg_ 14 4 4 4 7 7 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int sydelc_(char *name__, char *tabsym, integer *tabptr, char *tabval, ftnlen name_len, ftnlen tabsym_len, ftnlen tabval_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardc_ 4 2 13 124 */
/*:ref: cardi_ 4 1 4 */
/*:ref: bsrchc_ 4 5 13 4 13 124 124 */
/*:ref: sumai_ 4 2 4 4 */
/*:ref: remlac_ 14 5 4 4 13 4 124 */
/*:ref: scardc_ 14 3 4 13 124 */
/*:ref: remlai_ 14 4 4 4 4 4 */
/*:ref: scardi_ 14 2 4 4 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int sydeld_(char *name__, char *tabsym, integer *tabptr, doublereal *tabval, ftnlen name_len, ftnlen tabsym_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardc_ 4 2 13 124 */
/*:ref: cardi_ 4 1 4 */
/*:ref: cardd_ 4 1 7 */
/*:ref: bsrchc_ 4 5 13 4 13 124 124 */
/*:ref: sumai_ 4 2 4 4 */
/*:ref: remlac_ 14 5 4 4 13 4 124 */
/*:ref: scardc_ 14 3 4 13 124 */
/*:ref: remlai_ 14 4 4 4 4 4 */
/*:ref: scardi_ 14 2 4 4 */
/*:ref: remlad_ 14 4 4 4 7 4 */
/*:ref: scardd_ 14 2 4 7 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int sydeli_(char *name__, char *tabsym, integer *tabptr, integer *tabval, ftnlen name_len, ftnlen tabsym_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardc_ 4 2 13 124 */
/*:ref: cardi_ 4 1 4 */
/*:ref: bsrchc_ 4 5 13 4 13 124 124 */
/*:ref: sumai_ 4 2 4 4 */
/*:ref: remlac_ 14 5 4 4 13 4 124 */
/*:ref: scardc_ 14 3 4 13 124 */
/*:ref: remlai_ 14 4 4 4 4 4 */
/*:ref: scardi_ 14 2 4 4 */
/*:ref: chkout_ 14 2 13 124 */
 
extern integer sydimc_(char *name__, char *tabsym, integer *tabptr, char *tabval, ftnlen name_len, ftnlen tabsym_len, ftnlen tabval_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardc_ 4 2 13 124 */
/*:ref: bsrchc_ 4 5 13 4 13 124 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern integer sydimd_(char *name__, char *tabsym, integer *tabptr, doublereal *tabval, ftnlen name_len, ftnlen tabsym_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardc_ 4 2 13 124 */
/*:ref: bsrchc_ 4 5 13 4 13 124 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern integer sydimi_(char *name__, char *tabsym, integer *tabptr, integer *tabval, ftnlen name_len, ftnlen tabsym_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardc_ 4 2 13 124 */
/*:ref: bsrchc_ 4 5 13 4 13 124 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int sydupc_(char *name__, char *copy, char *tabsym, integer *tabptr, char *tabval, ftnlen name_len, ftnlen copy_len, ftnlen tabsym_len, ftnlen tabval_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardc_ 4 2 13 124 */
/*:ref: cardi_ 4 1 4 */
/*:ref: lstlec_ 4 5 13 4 13 124 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: sumai_ 4 2 4 4 */
/*:ref: sizec_ 4 2 13 124 */
/*:ref: sizei_ 4 1 4 */
/*:ref: remlac_ 14 5 4 4 13 4 124 */
/*:ref: scardc_ 14 3 4 13 124 */
/*:ref: inslac_ 14 7 13 4 4 13 4 124 124 */
/*:ref: inslai_ 14 5 4 4 4 4 4 */
/*:ref: scardi_ 14 2 4 4 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int sydupd_(char *name__, char *copy, char *tabsym, integer *tabptr, doublereal *tabval, ftnlen name_len, ftnlen copy_len, ftnlen tabsym_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardc_ 4 2 13 124 */
/*:ref: cardi_ 4 1 4 */
/*:ref: cardd_ 4 1 7 */
/*:ref: lstlec_ 4 5 13 4 13 124 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: sumai_ 4 2 4 4 */
/*:ref: sizec_ 4 2 13 124 */
/*:ref: sizei_ 4 1 4 */
/*:ref: sized_ 4 1 7 */
/*:ref: remlad_ 14 4 4 4 7 4 */
/*:ref: scardd_ 14 2 4 7 */
/*:ref: inslac_ 14 7 13 4 4 13 4 124 124 */
/*:ref: scardc_ 14 3 4 13 124 */
/*:ref: inslai_ 14 5 4 4 4 4 4 */
/*:ref: scardi_ 14 2 4 4 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int sydupi_(char *name__, char *copy, char *tabsym, integer *tabptr, integer *tabval, ftnlen name_len, ftnlen copy_len, ftnlen tabsym_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardc_ 4 2 13 124 */
/*:ref: cardi_ 4 1 4 */
/*:ref: lstlec_ 4 5 13 4 13 124 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: sumai_ 4 2 4 4 */
/*:ref: sizec_ 4 2 13 124 */
/*:ref: sizei_ 4 1 4 */
/*:ref: remlai_ 14 4 4 4 4 4 */
/*:ref: scardi_ 14 2 4 4 */
/*:ref: inslac_ 14 7 13 4 4 13 4 124 124 */
/*:ref: scardc_ 14 3 4 13 124 */
/*:ref: inslai_ 14 5 4 4 4 4 4 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int syenqc_(char *name__, char *value, char *tabsym, integer *tabptr, char *tabval, ftnlen name_len, ftnlen value_len, ftnlen tabsym_len, ftnlen tabval_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardc_ 4 2 13 124 */
/*:ref: lstlec_ 4 5 13 4 13 124 124 */
/*:ref: sysetc_ 14 9 13 13 13 4 13 124 124 124 124 */
/*:ref: sizec_ 4 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: sumai_ 4 2 4 4 */
/*:ref: inslac_ 14 7 13 4 4 13 4 124 124 */
/*:ref: scardc_ 14 3 4 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int syenqd_(char *name__, doublereal *value, char *tabsym, integer *tabptr, doublereal *tabval, ftnlen name_len, ftnlen tabsym_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardc_ 4 2 13 124 */
/*:ref: cardd_ 4 1 7 */
/*:ref: lstlec_ 4 5 13 4 13 124 124 */
/*:ref: sysetd_ 14 7 13 7 13 4 7 124 124 */
/*:ref: sized_ 4 1 7 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: sumai_ 4 2 4 4 */
/*:ref: inslad_ 14 5 7 4 4 7 4 */
/*:ref: scardd_ 14 2 4 7 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int syenqi_(char *name__, integer *value, char *tabsym, integer *tabptr, integer *tabval, ftnlen name_len, ftnlen tabsym_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardc_ 4 2 13 124 */
/*:ref: cardi_ 4 1 4 */
/*:ref: lstlec_ 4 5 13 4 13 124 124 */
/*:ref: syseti_ 14 7 13 4 13 4 4 124 124 */
/*:ref: sizei_ 4 1 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: sumai_ 4 2 4 4 */
/*:ref: inslai_ 14 5 4 4 4 4 4 */
/*:ref: scardi_ 14 2 4 4 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int syfetc_(integer *nth, char *tabsym, integer *tabptr, char *tabval, char *name__, logical *found, ftnlen tabsym_len, ftnlen tabval_len, ftnlen name_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardc_ 4 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int syfetd_(integer *nth, char *tabsym, integer *tabptr, doublereal *tabval, char *name__, logical *found, ftnlen tabsym_len, ftnlen name_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardc_ 4 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int syfeti_(integer *nth, char *tabsym, integer *tabptr, integer *tabval, char *name__, logical *found, ftnlen tabsym_len, ftnlen name_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardc_ 4 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int sygetc_(char *name__, char *tabsym, integer *tabptr, char *tabval, integer *n, char *values, logical *found, ftnlen name_len, ftnlen tabsym_len, ftnlen tabval_len, ftnlen values_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardc_ 4 2 13 124 */
/*:ref: bsrchc_ 4 5 13 4 13 124 124 */
/*:ref: sumai_ 4 2 4 4 */
/*:ref: movec_ 14 5 13 4 13 124 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int sygetd_(char *name__, char *tabsym, integer *tabptr, doublereal *tabval, integer *n, doublereal *values, logical *found, ftnlen name_len, ftnlen tabsym_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardc_ 4 2 13 124 */
/*:ref: bsrchc_ 4 5 13 4 13 124 124 */
/*:ref: sumai_ 4 2 4 4 */
/*:ref: moved_ 14 3 7 4 7 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int sygeti_(char *name__, char *tabsym, integer *tabptr, integer *tabval, integer *n, integer *values, logical *found, ftnlen name_len, ftnlen tabsym_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardc_ 4 2 13 124 */
/*:ref: bsrchc_ 4 5 13 4 13 124 124 */
/*:ref: sumai_ 4 2 4 4 */
/*:ref: movei_ 14 3 4 4 4 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int synthc_(char *name__, integer *nth, char *tabsym, integer *tabptr, char *tabval, char *value, logical *found, ftnlen name_len, ftnlen tabsym_len, ftnlen tabval_len, ftnlen value_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardc_ 4 2 13 124 */
/*:ref: bsrchc_ 4 5 13 4 13 124 124 */
/*:ref: sumai_ 4 2 4 4 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int synthd_(char *name__, integer *nth, char *tabsym, integer *tabptr, doublereal *tabval, doublereal *value, logical *found, ftnlen name_len, ftnlen tabsym_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardc_ 4 2 13 124 */
/*:ref: bsrchc_ 4 5 13 4 13 124 124 */
/*:ref: sumai_ 4 2 4 4 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int synthi_(char *name__, integer *nth, char *tabsym, integer *tabptr, integer *tabval, integer *value, logical *found, ftnlen name_len, ftnlen tabsym_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardc_ 4 2 13 124 */
/*:ref: bsrchc_ 4 5 13 4 13 124 124 */
/*:ref: sumai_ 4 2 4 4 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int syordc_(char *name__, char *tabsym, integer *tabptr, char *tabval, ftnlen name_len, ftnlen tabsym_len, ftnlen tabval_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardc_ 4 2 13 124 */
/*:ref: bsrchc_ 4 5 13 4 13 124 124 */
/*:ref: sumai_ 4 2 4 4 */
/*:ref: shellc_ 14 3 4 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int syordd_(char *name__, char *tabsym, integer *tabptr, doublereal *tabval, ftnlen name_len, ftnlen tabsym_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardc_ 4 2 13 124 */
/*:ref: bsrchc_ 4 5 13 4 13 124 124 */
/*:ref: sumai_ 4 2 4 4 */
/*:ref: shelld_ 14 2 4 7 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int syordi_(char *name__, char *tabsym, integer *tabptr, integer *tabval, ftnlen name_len, ftnlen tabsym_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardc_ 4 2 13 124 */
/*:ref: bsrchc_ 4 5 13 4 13 124 124 */
/*:ref: sumai_ 4 2 4 4 */
/*:ref: shelli_ 14 2 4 4 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int sypopc_(char *name__, char *tabsym, integer *tabptr, char *tabval, char *value, logical *found, ftnlen name_len, ftnlen tabsym_len, ftnlen tabval_len, ftnlen value_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardc_ 4 2 13 124 */
/*:ref: cardi_ 4 1 4 */
/*:ref: bsrchc_ 4 5 13 4 13 124 124 */
/*:ref: sumai_ 4 2 4 4 */
/*:ref: remlac_ 14 5 4 4 13 4 124 */
/*:ref: scardc_ 14 3 4 13 124 */
/*:ref: remlai_ 14 4 4 4 4 4 */
/*:ref: scardi_ 14 2 4 4 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int sypopd_(char *name__, char *tabsym, integer *tabptr, doublereal *tabval, doublereal *value, logical *found, ftnlen name_len, ftnlen tabsym_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardc_ 4 2 13 124 */
/*:ref: cardi_ 4 1 4 */
/*:ref: cardd_ 4 1 7 */
/*:ref: bsrchc_ 4 5 13 4 13 124 124 */
/*:ref: sumai_ 4 2 4 4 */
/*:ref: remlad_ 14 4 4 4 7 4 */
/*:ref: scardd_ 14 2 4 7 */
/*:ref: remlac_ 14 5 4 4 13 4 124 */
/*:ref: scardc_ 14 3 4 13 124 */
/*:ref: remlai_ 14 4 4 4 4 4 */
/*:ref: scardi_ 14 2 4 4 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int sypopi_(char *name__, char *tabsym, integer *tabptr, integer *tabval, integer *value, logical *found, ftnlen name_len, ftnlen tabsym_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardc_ 4 2 13 124 */
/*:ref: cardi_ 4 1 4 */
/*:ref: bsrchc_ 4 5 13 4 13 124 124 */
/*:ref: sumai_ 4 2 4 4 */
/*:ref: remlai_ 14 4 4 4 4 4 */
/*:ref: scardi_ 14 2 4 4 */
/*:ref: remlac_ 14 5 4 4 13 4 124 */
/*:ref: scardc_ 14 3 4 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int sypshc_(char *name__, char *value, char *tabsym, integer *tabptr, char *tabval, ftnlen name_len, ftnlen value_len, ftnlen tabsym_len, ftnlen tabval_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardc_ 4 2 13 124 */
/*:ref: lstlec_ 4 5 13 4 13 124 124 */
/*:ref: sysetc_ 14 9 13 13 13 4 13 124 124 124 124 */
/*:ref: sizec_ 4 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: sumai_ 4 2 4 4 */
/*:ref: inslac_ 14 7 13 4 4 13 4 124 124 */
/*:ref: scardc_ 14 3 4 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int sypshd_(char *name__, doublereal *value, char *tabsym, integer *tabptr, doublereal *tabval, ftnlen name_len, ftnlen tabsym_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardc_ 4 2 13 124 */
/*:ref: cardd_ 4 1 7 */
/*:ref: lstlec_ 4 5 13 4 13 124 124 */
/*:ref: sysetd_ 14 7 13 7 13 4 7 124 124 */
/*:ref: sized_ 4 1 7 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: sumai_ 4 2 4 4 */
/*:ref: inslad_ 14 5 7 4 4 7 4 */
/*:ref: scardd_ 14 2 4 7 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int sypshi_(char *name__, integer *value, char *tabsym, integer *tabptr, integer *tabval, ftnlen name_len, ftnlen tabsym_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardc_ 4 2 13 124 */
/*:ref: cardi_ 4 1 4 */
/*:ref: lstlec_ 4 5 13 4 13 124 124 */
/*:ref: syseti_ 14 7 13 4 13 4 4 124 124 */
/*:ref: sizei_ 4 1 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: sumai_ 4 2 4 4 */
/*:ref: inslai_ 14 5 4 4 4 4 4 */
/*:ref: scardi_ 14 2 4 4 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int syputc_(char *name__, char *values, integer *n, char *tabsym, integer *tabptr, char *tabval, ftnlen name_len, ftnlen values_len, ftnlen tabsym_len, ftnlen tabval_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: cardc_ 4 2 13 124 */
/*:ref: cardi_ 4 1 4 */
/*:ref: lstlec_ 4 5 13 4 13 124 124 */
/*:ref: sumai_ 4 2 4 4 */
/*:ref: sizec_ 4 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sizei_ 4 1 4 */
/*:ref: remlac_ 14 5 4 4 13 4 124 */
/*:ref: scardc_ 14 3 4 13 124 */
/*:ref: inslac_ 14 7 13 4 4 13 4 124 124 */
/*:ref: inslai_ 14 5 4 4 4 4 4 */
/*:ref: scardi_ 14 2 4 4 */
 
extern int syputd_(char *name__, doublereal *values, integer *n, char *tabsym, integer *tabptr, doublereal *tabval, ftnlen name_len, ftnlen tabsym_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: cardc_ 4 2 13 124 */
/*:ref: cardi_ 4 1 4 */
/*:ref: cardd_ 4 1 7 */
/*:ref: lstlec_ 4 5 13 4 13 124 124 */
/*:ref: sumai_ 4 2 4 4 */
/*:ref: sizec_ 4 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sizei_ 4 1 4 */
/*:ref: sized_ 4 1 7 */
/*:ref: remlad_ 14 4 4 4 7 4 */
/*:ref: scardd_ 14 2 4 7 */
/*:ref: inslac_ 14 7 13 4 4 13 4 124 124 */
/*:ref: scardc_ 14 3 4 13 124 */
/*:ref: inslai_ 14 5 4 4 4 4 4 */
/*:ref: scardi_ 14 2 4 4 */
/*:ref: inslad_ 14 5 7 4 4 7 4 */
 
extern int syputi_(char *name__, integer *values, integer *n, char *tabsym, integer *tabptr, integer *tabval, ftnlen name_len, ftnlen tabsym_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: cardc_ 4 2 13 124 */
/*:ref: cardi_ 4 1 4 */
/*:ref: lstlec_ 4 5 13 4 13 124 124 */
/*:ref: sumai_ 4 2 4 4 */
/*:ref: sizec_ 4 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sizei_ 4 1 4 */
/*:ref: remlai_ 14 4 4 4 4 4 */
/*:ref: scardi_ 14 2 4 4 */
/*:ref: inslac_ 14 7 13 4 4 13 4 124 124 */
/*:ref: scardc_ 14 3 4 13 124 */
/*:ref: inslai_ 14 5 4 4 4 4 4 */
 
extern int syrenc_(char *old, char *new__, char *tabsym, integer *tabptr, char *tabval, ftnlen old_len, ftnlen new_len, ftnlen tabsym_len, ftnlen tabval_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardc_ 4 2 13 124 */
/*:ref: bsrchc_ 4 5 13 4 13 124 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: sydelc_ 14 7 13 13 4 13 124 124 124 */
/*:ref: lstlec_ 4 5 13 4 13 124 124 */
/*:ref: sumai_ 4 2 4 4 */
/*:ref: swapac_ 14 6 4 4 4 4 13 124 */
/*:ref: swapai_ 14 5 4 4 4 4 4 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int syrend_(char *old, char *new__, char *tabsym, integer *tabptr, doublereal *tabval, ftnlen old_len, ftnlen new_len, ftnlen tabsym_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardc_ 4 2 13 124 */
/*:ref: bsrchc_ 4 5 13 4 13 124 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: sydeld_ 14 6 13 13 4 7 124 124 */
/*:ref: lstlec_ 4 5 13 4 13 124 124 */
/*:ref: sumai_ 4 2 4 4 */
/*:ref: swapad_ 14 5 4 4 4 4 7 */
/*:ref: swapac_ 14 6 4 4 4 4 13 124 */
/*:ref: swapai_ 14 5 4 4 4 4 4 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int syreni_(char *old, char *new__, char *tabsym, integer *tabptr, integer *tabval, ftnlen old_len, ftnlen new_len, ftnlen tabsym_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardc_ 4 2 13 124 */
/*:ref: bsrchc_ 4 5 13 4 13 124 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: sydeli_ 14 6 13 13 4 4 124 124 */
/*:ref: lstlec_ 4 5 13 4 13 124 124 */
/*:ref: sumai_ 4 2 4 4 */
/*:ref: swapai_ 14 5 4 4 4 4 4 */
/*:ref: swapac_ 14 6 4 4 4 4 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int syselc_(char *name__, integer *begin, integer *end, char *tabsym, integer *tabptr, char *tabval, char *values, logical *found, ftnlen name_len, ftnlen tabsym_len, ftnlen tabval_len, ftnlen values_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardc_ 4 2 13 124 */
/*:ref: bsrchc_ 4 5 13 4 13 124 124 */
/*:ref: sumai_ 4 2 4 4 */
/*:ref: movec_ 14 5 13 4 13 124 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int syseld_(char *name__, integer *begin, integer *end, char *tabsym, integer *tabptr, doublereal *tabval, doublereal *values, logical *found, ftnlen name_len, ftnlen tabsym_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardc_ 4 2 13 124 */
/*:ref: bsrchc_ 4 5 13 4 13 124 124 */
/*:ref: sumai_ 4 2 4 4 */
/*:ref: moved_ 14 3 7 4 7 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int syseli_(char *name__, integer *begin, integer *end, char *tabsym, integer *tabptr, integer *tabval, integer *values, logical *found, ftnlen name_len, ftnlen tabsym_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardc_ 4 2 13 124 */
/*:ref: bsrchc_ 4 5 13 4 13 124 124 */
/*:ref: sumai_ 4 2 4 4 */
/*:ref: movei_ 14 3 4 4 4 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int sysetc_(char *name__, char *value, char *tabsym, integer *tabptr, char *tabval, ftnlen name_len, ftnlen value_len, ftnlen tabsym_len, ftnlen tabval_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardc_ 4 2 13 124 */
/*:ref: cardi_ 4 1 4 */
/*:ref: lstlec_ 4 5 13 4 13 124 124 */
/*:ref: sumai_ 4 2 4 4 */
/*:ref: remlac_ 14 5 4 4 13 4 124 */
/*:ref: scardc_ 14 3 4 13 124 */
/*:ref: sizec_ 4 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: sizei_ 4 1 4 */
/*:ref: inslac_ 14 7 13 4 4 13 4 124 124 */
/*:ref: inslai_ 14 5 4 4 4 4 4 */
/*:ref: scardi_ 14 2 4 4 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int sysetd_(char *name__, doublereal *value, char *tabsym, integer *tabptr, doublereal *tabval, ftnlen name_len, ftnlen tabsym_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardc_ 4 2 13 124 */
/*:ref: cardi_ 4 1 4 */
/*:ref: cardd_ 4 1 7 */
/*:ref: lstlec_ 4 5 13 4 13 124 124 */
/*:ref: sumai_ 4 2 4 4 */
/*:ref: remlad_ 14 4 4 4 7 4 */
/*:ref: scardd_ 14 2 4 7 */
/*:ref: sizec_ 4 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: sizei_ 4 1 4 */
/*:ref: sized_ 4 1 7 */
/*:ref: inslac_ 14 7 13 4 4 13 4 124 124 */
/*:ref: scardc_ 14 3 4 13 124 */
/*:ref: inslai_ 14 5 4 4 4 4 4 */
/*:ref: scardi_ 14 2 4 4 */
/*:ref: inslad_ 14 5 7 4 4 7 4 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int syseti_(char *name__, integer *value, char *tabsym, integer *tabptr, integer *tabval, ftnlen name_len, ftnlen tabsym_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardc_ 4 2 13 124 */
/*:ref: cardi_ 4 1 4 */
/*:ref: lstlec_ 4 5 13 4 13 124 124 */
/*:ref: sumai_ 4 2 4 4 */
/*:ref: remlai_ 14 4 4 4 4 4 */
/*:ref: scardi_ 14 2 4 4 */
/*:ref: sizec_ 4 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: sizei_ 4 1 4 */
/*:ref: inslac_ 14 7 13 4 4 13 4 124 124 */
/*:ref: scardc_ 14 3 4 13 124 */
/*:ref: inslai_ 14 5 4 4 4 4 4 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int sytrnc_(char *name__, integer *i__, integer *j, char *tabsym, integer *tabptr, char *tabval, ftnlen name_len, ftnlen tabsym_len, ftnlen tabval_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardc_ 4 2 13 124 */
/*:ref: bsrchc_ 4 5 13 4 13 124 124 */
/*:ref: sumai_ 4 2 4 4 */
/*:ref: swapc_ 14 4 13 13 124 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int sytrnd_(char *name__, integer *i__, integer *j, char *tabsym, integer *tabptr, doublereal *tabval, ftnlen name_len, ftnlen tabsym_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardc_ 4 2 13 124 */
/*:ref: bsrchc_ 4 5 13 4 13 124 124 */
/*:ref: sumai_ 4 2 4 4 */
/*:ref: swapd_ 14 2 7 7 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int sytrni_(char *name__, integer *i__, integer *j, char *tabsym, integer *tabptr, integer *tabval, ftnlen name_len, ftnlen tabsym_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardc_ 4 2 13 124 */
/*:ref: bsrchc_ 4 5 13 4 13 124 124 */
/*:ref: sumai_ 4 2 4 4 */
/*:ref: swapi_ 14 2 4 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int tcheck_(doublereal *tvec, char *type__, logical *mods, char *modify, logical *ok, char *error, ftnlen type_len, ftnlen modify_len, ftnlen error_len);
extern int tparch_(char *type__, ftnlen type_len);
extern int tchckd_(char *type__, ftnlen type_len);
/*:ref: repmc_ 14 8 13 13 13 13 124 124 124 124 */
/*:ref: repmd_ 14 8 13 13 7 4 13 124 124 124 */
/*:ref: repmi_ 14 7 13 13 4 13 124 124 124 */
/*:ref: eqstr_ 12 4 13 13 124 124 */
 
extern int texpyr_(integer *year);
extern int tsetyr_(integer *year);
 
extern int timdef_(char *action, char *item, char *value, ftnlen action_len, ftnlen item_len, ftnlen value_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: ljust_ 14 4 13 13 124 124 */
/*:ref: ucase_ 14 4 13 13 124 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: isrchc_ 4 5 13 4 13 124 124 */
/*:ref: prefix_ 14 5 13 4 13 124 124 */
/*:ref: zzutcpm_ 14 7 13 4 7 7 4 12 124 */
 
extern int timout_(doublereal *et, char *pictur, char *output, ftnlen pictur_len, ftnlen output_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: scanpr_ 14 5 4 13 4 4 124 */
/*:ref: bsrchc_ 4 5 13 4 13 124 124 */
/*:ref: ljust_ 14 4 13 13 124 124 */
/*:ref: rtrim_ 4 2 13 124 */
/*:ref: scan_ 14 12 13 13 4 4 4 4 4 4 4 4 124 124 */
/*:ref: timdef_ 14 6 13 13 13 124 124 124 */
/*:ref: prefix_ 14 5 13 4 13 124 124 */
/*:ref: zzutcpm_ 14 7 13 4 7 7 4 12 124 */
/*:ref: isrchi_ 4 3 4 4 4 */
/*:ref: scanrj_ 14 6 4 4 4 4 4 4 */
/*:ref: unitim_ 7 5 7 13 13 124 124 */
/*:ref: spd_ 7 0 */
/*:ref: j2000_ 7 0 */
/*:ref: j1950_ 7 0 */
/*:ref: brckti_ 4 3 4 4 4 */
/*:ref: brcktd_ 7 3 7 7 7 */
/*:ref: dpfmt_ 14 5 7 13 13 124 124 */
/*:ref: ttrans_ 14 5 13 13 7 124 124 */
/*:ref: gr2jul_ 14 4 4 4 4 4 */
/*:ref: jul2gr_ 14 4 4 4 4 4 */
/*:ref: rmaind_ 14 4 7 7 7 7 */
/*:ref: ucase_ 14 4 13 13 124 124 */
/*:ref: lcase_ 14 4 13 13 124 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int tipbod_(char *ref, integer *body, doublereal *et, doublereal *tipm, ftnlen ref_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: irftrn_ 14 5 13 13 7 124 124 */
/*:ref: bodmat_ 14 3 4 7 7 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: mxm_ 14 3 7 7 7 */
/*:ref: moved_ 14 3 7 4 7 */
 
extern int tisbod_(char *ref, integer *body, doublereal *et, doublereal *tsipm, ftnlen ref_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: irfnum_ 14 3 13 4 124 */
/*:ref: pckmat_ 14 5 4 7 4 7 12 */
/*:ref: zzbodbry_ 4 1 4 */
/*:ref: repmi_ 14 7 13 13 4 13 124 124 124 */
/*:ref: gdpool_ 14 7 13 4 4 4 7 12 124 */
/*:ref: spd_ 7 0 */
/*:ref: j2000_ 7 0 */
/*:ref: cleard_ 14 2 4 7 */
/*:ref: bodvcd_ 14 6 4 13 4 4 7 124 */
/*:ref: bodfnd_ 12 3 4 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: rpd_ 7 0 */
/*:ref: vdotg_ 7 3 7 7 4 */
/*:ref: twopi_ 7 0 */
/*:ref: halfpi_ 7 0 */
/*:ref: failed_ 12 0 */
/*:ref: vpack_ 14 4 7 7 7 7 */
/*:ref: eul2xf_ 14 5 7 4 4 4 7 */
/*:ref: irfrot_ 14 3 4 4 7 */
/*:ref: mxm_ 14 3 7 7 7 */
 
extern int tkfram_(integer *id, doublereal *rot, integer *frame, logical *found);
/*:ref: lnkini_ 14 2 4 4 */
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: locati_ 14 6 4 4 4 4 4 12 */
/*:ref: ident_ 14 1 7 */
/*:ref: intstr_ 14 3 4 13 124 */
/*:ref: frmnam_ 14 3 4 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: rtrim_ 4 2 13 124 */
/*:ref: cvpool_ 14 3 13 12 124 */
/*:ref: dtpool_ 14 6 13 12 4 13 124 124 */
/*:ref: badkpv_ 12 10 13 13 13 4 4 13 124 124 124 124 */
/*:ref: gcpool_ 14 8 13 4 4 4 13 12 124 124 */
/*:ref: namfrm_ 14 3 13 4 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: ucase_ 14 4 13 13 124 124 */
/*:ref: gdpool_ 14 7 13 4 4 4 7 12 124 */
/*:ref: moved_ 14 3 7 4 7 */
/*:ref: sharpr_ 14 1 7 */
/*:ref: vdot_ 7 2 7 7 */
/*:ref: vsclip_ 14 2 7 7 */
/*:ref: gipool_ 14 7 13 4 4 4 4 12 124 */
/*:ref: convrt_ 14 6 7 13 13 7 124 124 */
/*:ref: failed_ 12 0 */
/*:ref: eul2m_ 14 7 7 7 7 4 4 4 7 */
/*:ref: vhatg_ 14 3 7 4 7 */
/*:ref: q2m_ 14 2 7 7 */
/*:ref: swpool_ 14 5 13 4 13 124 124 */
 
extern int tkvrsn_(char *item, char *verstr, ftnlen item_len, ftnlen verstr_len);
/*:ref: eqstr_ 12 4 13 13 124 124 */
 
extern int tostdo_(char *line, ftnlen line_len);
/*:ref: stdio_ 14 3 13 4 124 */
/*:ref: writln_ 14 3 13 4 124 */
 
extern H_f touchc_(char *ret_val, ftnlen ret_val_len, char *string, ftnlen string_len);
 
extern doublereal touchd_(doublereal *dp);
 
extern integer touchi_(integer *int__);
 
extern logical touchl_(logical *log__);
 
extern int tparse_(char *string, doublereal *sp2000, char *error, ftnlen string_len, ftnlen error_len);
/*:ref: tpartv_ 14 15 13 7 4 13 13 12 12 12 13 13 124 124 124 124 124 */
/*:ref: repmc_ 14 8 13 13 13 13 124 124 124 124 */
/*:ref: j2000_ 7 0 */
/*:ref: spd_ 7 0 */
/*:ref: tcheck_ 14 9 7 13 12 13 12 13 124 124 124 */
/*:ref: texpyr_ 14 1 4 */
/*:ref: rmaini_ 14 4 4 4 4 4 */
 
extern int tpartv_(char *string, doublereal *tvec, integer *ntvec, char *type__, char *modify, logical *mods, logical *yabbrv, logical *succes, char *pictur, char *error, ftnlen string_len, ftnlen type_len, ftnlen modify_len, ftnlen pictur_len, ftnlen error_len);
/*:ref: zztpats_ 12 6 4 4 13 13 124 124 */
/*:ref: zztokns_ 12 4 13 13 124 124 */
/*:ref: zzcmbt_ 12 5 13 13 12 124 124 */
/*:ref: zzsubt_ 12 5 13 13 12 124 124 */
/*:ref: zzrept_ 12 5 13 13 12 124 124 */
/*:ref: zzremt_ 12 2 13 124 */
/*:ref: zzist_ 12 2 13 124 */
/*:ref: zznote_ 12 4 13 4 4 124 */
/*:ref: ucase_ 14 4 13 13 124 124 */
/*:ref: zzunpck_ 12 11 13 12 7 4 13 13 13 124 124 124 124 */
/*:ref: suffix_ 14 5 13 4 13 124 124 */
/*:ref: intmax_ 4 0 */
/*:ref: zzvalt_ 12 6 13 4 4 13 124 124 */
/*:ref: zzgrep_ 12 2 13 124 */
/*:ref: bsrchc_ 4 5 13 4 13 124 124 */
/*:ref: isrchc_ 4 5 13 4 13 124 124 */
/*:ref: zzispt_ 12 4 13 4 4 124 */
/*:ref: zzinssub_ 14 7 13 13 4 13 124 124 124 */
/*:ref: prefix_ 14 5 13 4 13 124 124 */
/*:ref: repmc_ 14 8 13 13 13 13 124 124 124 124 */
/*:ref: rtrim_ 4 2 13 124 */
 
extern int tpictr_(char *sample, char *pictur, logical *ok, char *error, ftnlen sample_len, ftnlen pictur_len, ftnlen error_len);
/*:ref: tpartv_ 14 15 13 7 4 13 13 12 12 12 13 13 124 124 124 124 124 */
 
extern doublereal trace_(doublereal *matrix);
 
extern doublereal traceg_(doublereal *matrix, integer *ndim);
 
extern int trcpkg_(integer *depth, integer *index, char *module, char *trace, char *name__, ftnlen module_len, ftnlen trace_len, ftnlen name_len);
extern int chkin_(char *module, ftnlen module_len);
extern int chkout_(char *module, ftnlen module_len);
extern int trcdep_(integer *depth);
extern int trcmxd_(integer *depth);
extern int trcnam_(integer *index, char *name__, ftnlen name_len);
extern int qcktrc_(char *trace, ftnlen trace_len);
extern int freeze_(void);
extern int trcoff_(void);
/*:ref: wrline_ 14 4 13 13 124 124 */
/*:ref: frstnb_ 4 2 13 124 */
/*:ref: getdev_ 14 2 13 124 */
/*:ref: rtrim_ 4 2 13 124 */
/*:ref: getact_ 14 1 4 */
/*:ref: failed_ 12 0 */
/*:ref: intstr_ 14 3 4 13 124 */
/*:ref: suffix_ 14 5 13 4 13 124 124 */
 
extern int ttrans_(char *from, char *to, doublereal *tvec, ftnlen from_len, ftnlen to_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: spd_ 7 0 */
/*:ref: j2000_ 7 0 */
/*:ref: ssizec_ 14 3 4 13 124 */
/*:ref: insrtc_ 14 4 13 13 124 124 */
/*:ref: orderc_ 14 4 13 4 4 124 */
/*:ref: reordc_ 14 4 4 4 13 124 */
/*:ref: reordi_ 14 3 4 4 4 */
/*:ref: reordl_ 14 3 4 4 12 */
/*:ref: swpool_ 14 5 13 4 13 124 124 */
/*:ref: cvpool_ 14 3 13 12 124 */
/*:ref: rtpool_ 14 5 13 4 7 12 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: nextwd_ 14 6 13 13 13 124 124 124 */
/*:ref: ucase_ 14 4 13 13 124 124 */
/*:ref: bsrchc_ 4 5 13 4 13 124 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: rmaini_ 14 4 4 4 4 4 */
/*:ref: lstlei_ 4 3 4 4 4 */
/*:ref: odd_ 12 1 4 */
/*:ref: rmaind_ 14 4 7 7 7 7 */
/*:ref: elemc_ 12 4 13 13 124 124 */
/*:ref: unitim_ 7 5 7 13 13 124 124 */
/*:ref: lstled_ 4 3 7 4 7 */
/*:ref: lstlti_ 4 3 4 4 4 */
 
extern doublereal twopi_(void);
 
extern int twovec_(doublereal *axdef, integer *indexa, doublereal *plndef, integer *indexp, doublereal *mout);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: vhat_ 14 2 7 7 */
/*:ref: ucrss_ 14 3 7 7 7 */
/*:ref: xpose_ 14 2 7 7 */
/*:ref: moved_ 14 3 7 4 7 */
 
extern int twovxf_(doublereal *axdef, integer *indexa, doublereal *plndef, integer *indexp, doublereal *xform);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: zztwovxf_ 14 5 7 4 7 4 7 */
/*:ref: invstm_ 14 2 7 7 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int txtopn_(char *fname, integer *unit, ftnlen fname_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: getlun_ 14 1 4 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: errint_ 14 3 13 4 124 */
 
extern int txtopr_(char *fname, integer *unit, ftnlen fname_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: getlun_ 14 1 4 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: errint_ 14 3 13 4 124 */
 
extern doublereal tyear_(void);
 
extern int ucase_(char *in, char *out, ftnlen in_len, ftnlen out_len);
 
extern int ucrss_(doublereal *v1, doublereal *v2, doublereal *vout);
/*:ref: vnorm_ 7 1 7 */
 
extern int unionc_(char *a, char *b, char *c__, ftnlen a_len, ftnlen b_len, ftnlen c_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: cardc_ 4 2 13 124 */
/*:ref: sizec_ 4 2 13 124 */
/*:ref: scardc_ 14 3 4 13 124 */
/*:ref: excess_ 14 3 4 13 124 */
 
extern int uniond_(doublereal *a, doublereal *b, doublereal *c__);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardd_ 4 1 7 */
/*:ref: sized_ 4 1 7 */
/*:ref: scardd_ 14 2 4 7 */
/*:ref: excess_ 14 3 4 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int unioni_(integer *a, integer *b, integer *c__);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardi_ 4 1 4 */
/*:ref: sizei_ 4 1 4 */
/*:ref: scardi_ 14 2 4 4 */
/*:ref: excess_ 14 3 4 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern doublereal unitim_(doublereal *epoch, char *insys, char *outsys, ftnlen insys_len, ftnlen outsys_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: spd_ 7 0 */
/*:ref: j2000_ 7 0 */
/*:ref: validc_ 14 4 4 4 13 124 */
/*:ref: ssizec_ 14 3 4 13 124 */
/*:ref: unionc_ 14 6 13 13 13 124 124 124 */
/*:ref: swpool_ 14 5 13 4 13 124 124 */
/*:ref: cvpool_ 14 3 13 12 124 */
/*:ref: rtpool_ 14 5 13 4 7 12 124 */
/*:ref: somfls_ 12 2 12 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: ucase_ 14 4 13 13 124 124 */
/*:ref: insrtc_ 14 4 13 13 124 124 */
/*:ref: setc_ 12 6 13 13 13 124 124 124 */
/*:ref: elemc_ 12 4 13 13 124 124 */
 
extern int unorm_(doublereal *v1, doublereal *vout, doublereal *vmag);
/*:ref: vnorm_ 7 1 7 */
 
extern int unormg_(doublereal *v1, integer *ndim, doublereal *vout, doublereal *vmag);
/*:ref: vnormg_ 7 2 7 4 */
 
extern int utc2et_(char *utcstr, doublereal *et, ftnlen utcstr_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: tpartv_ 14 15 13 7 4 13 13 12 12 12 13 13 124 124 124 124 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: repmc_ 14 8 13 13 13 13 124 124 124 124 */
/*:ref: tcheck_ 14 9 7 13 12 13 12 13 124 124 124 */
/*:ref: texpyr_ 14 1 4 */
/*:ref: ttrans_ 14 5 13 13 7 124 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
 
extern int vadd_(doublereal *v1, doublereal *v2, doublereal *vout);
 
extern int vaddg_(doublereal *v1, doublereal *v2, integer *ndim, doublereal *vout);
 
extern int validc_(integer *size, integer *n, char *a, ftnlen a_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: rmdupc_ 14 3 4 13 124 */
/*:ref: ssizec_ 14 3 4 13 124 */
/*:ref: scardc_ 14 3 4 13 124 */
 
extern int validd_(integer *size, integer *n, doublereal *a);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: rmdupd_ 14 2 4 7 */
/*:ref: ssized_ 14 2 4 7 */
/*:ref: scardd_ 14 2 4 7 */
 
extern int validi_(integer *size, integer *n, integer *a);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: rmdupi_ 14 2 4 4 */
/*:ref: ssizei_ 14 2 4 4 */
/*:ref: scardi_ 14 2 4 4 */
 
extern int vcrss_(doublereal *v1, doublereal *v2, doublereal *vout);
 
extern doublereal vdist_(doublereal *v1, doublereal *v2);
/*:ref: vsub_ 14 3 7 7 7 */
/*:ref: vnorm_ 7 1 7 */
 
extern doublereal vdistg_(doublereal *v1, doublereal *v2, integer *ndim);
 
extern doublereal vdot_(doublereal *v1, doublereal *v2);
 
extern doublereal vdotg_(doublereal *v1, doublereal *v2, integer *ndim);
 
extern int vequ_(doublereal *vin, doublereal *vout);
 
extern int vequg_(doublereal *vin, integer *ndim, doublereal *vout);
 
extern int vhat_(doublereal *v1, doublereal *vout);
/*:ref: vnorm_ 7 1 7 */
 
extern int vhatg_(doublereal *v1, integer *ndim, doublereal *vout);
/*:ref: vnormg_ 7 2 7 4 */
 
extern int vhatip_(doublereal *v);
/*:ref: vnorm_ 7 1 7 */
 
extern int vlcom_(doublereal *a, doublereal *v1, doublereal *b, doublereal *v2, doublereal *sum);
 
extern int vlcom3_(doublereal *a, doublereal *v1, doublereal *b, doublereal *v2, doublereal *c__, doublereal *v3, doublereal *sum);
 
extern int vlcomg_(integer *n, doublereal *a, doublereal *v1, doublereal *b, doublereal *v2, doublereal *sum);
 
extern int vminug_(doublereal *vin, integer *ndim, doublereal *vout);
 
extern int vminus_(doublereal *v1, doublereal *vout);
 
extern doublereal vnorm_(doublereal *v1);
 
extern doublereal vnormg_(doublereal *v1, integer *ndim);
 
extern int vpack_(doublereal *x, doublereal *y, doublereal *z__, doublereal *v);
 
extern int vperp_(doublereal *a, doublereal *b, doublereal *p);
/*:ref: vproj_ 14 3 7 7 7 */
/*:ref: vsub_ 14 3 7 7 7 */
/*:ref: vsclip_ 14 2 7 7 */
 
extern int vprjp_(doublereal *vin, doublereal *plane, doublereal *vout);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: pl2nvc_ 14 3 7 7 7 */
/*:ref: vlcom_ 14 5 7 7 7 7 7 */
/*:ref: vdot_ 7 2 7 7 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int vprjpi_(doublereal *vin, doublereal *projpl, doublereal *invpl, doublereal *vout, logical *found);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: pl2nvc_ 14 3 7 7 7 */
/*:ref: vdot_ 7 2 7 7 */
/*:ref: dpmax_ 7 0 */
/*:ref: vlcom_ 14 5 7 7 7 7 7 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int vproj_(doublereal *a, doublereal *b, doublereal *p);
/*:ref: vdot_ 7 2 7 7 */
/*:ref: vscl_ 14 3 7 7 7 */
 
extern int vprojg_(doublereal *a, doublereal *b, integer *ndim, doublereal *p);
/*:ref: vdotg_ 7 3 7 7 4 */
/*:ref: vsclg_ 14 4 7 7 4 7 */
 
extern doublereal vrel_(doublereal *v1, doublereal *v2);
/*:ref: vdist_ 7 2 7 7 */
/*:ref: vnorm_ 7 1 7 */
 
extern doublereal vrelg_(doublereal *v1, doublereal *v2, integer *ndim);
/*:ref: vdistg_ 7 3 7 7 4 */
/*:ref: vnormg_ 7 2 7 4 */
 
extern int vrotv_(doublereal *v, doublereal *axis, doublereal *theta, doublereal *r__);
/*:ref: vnorm_ 7 1 7 */
/*:ref: moved_ 14 3 7 4 7 */
/*:ref: vhat_ 14 2 7 7 */
/*:ref: vproj_ 14 3 7 7 7 */
/*:ref: vsub_ 14 3 7 7 7 */
/*:ref: vcrss_ 14 3 7 7 7 */
/*:ref: vlcom_ 14 5 7 7 7 7 7 */
/*:ref: vadd_ 14 3 7 7 7 */
 
extern int vscl_(doublereal *s, doublereal *v1, doublereal *vout);
 
extern int vsclg_(doublereal *s, doublereal *v1, integer *ndim, doublereal *vout);
 
extern int vsclip_(doublereal *s, doublereal *v);
 
extern doublereal vsep_(doublereal *v1, doublereal *v2);
/*:ref: unorm_ 14 3 7 7 7 */
/*:ref: vdot_ 7 2 7 7 */
/*:ref: vnorm_ 7 1 7 */
/*:ref: pi_ 7 0 */
 
extern doublereal vsepg_(doublereal *v1, doublereal *v2, integer *ndim);
/*:ref: vnormg_ 7 2 7 4 */
/*:ref: vdotg_ 7 3 7 7 4 */
/*:ref: pi_ 7 0 */
 
extern int vsub_(doublereal *v1, doublereal *v2, doublereal *vout);
 
extern int vsubg_(doublereal *v1, doublereal *v2, integer *ndim, doublereal *vout);
 
extern doublereal vtmv_(doublereal *v1, doublereal *matrix, doublereal *v2);
 
extern doublereal vtmvg_(doublereal *v1, doublereal *matrix, doublereal *v2, integer *nrow, integer *ncol);
 
extern int vupack_(doublereal *v, doublereal *x, doublereal *y, doublereal *z__);
 
extern logical vzero_(doublereal *v);
 
extern logical vzerog_(doublereal *v, integer *ndim);
 
extern integer wdcnt_(char *string, ftnlen string_len);
 
extern integer wdindx_(char *string, char *word, ftnlen string_len, ftnlen word_len);
/*:ref: lastnb_ 4 2 13 124 */
/*:ref: frstnb_ 4 2 13 124 */
 
extern int wncomd_(doublereal *left, doublereal *right, doublereal *window, doublereal *result);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardd_ 4 1 7 */
/*:ref: scardd_ 14 2 4 7 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: wninsd_ 14 3 7 7 7 */
/*:ref: failed_ 12 0 */
 
extern int wncond_(doublereal *left, doublereal *right, doublereal *window);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: wnexpd_ 14 3 7 7 7 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int wndifd_(doublereal *a, doublereal *b, doublereal *c__);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardd_ 4 1 7 */
/*:ref: sized_ 4 1 7 */
/*:ref: ssized_ 14 2 4 7 */
/*:ref: copyd_ 14 2 7 7 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: scardd_ 14 2 4 7 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
 
extern logical wnelmd_(doublereal *point, doublereal *window);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardd_ 4 1 7 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int wnexpd_(doublereal *left, doublereal *right, doublereal *window);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardd_ 4 1 7 */
/*:ref: scardd_ 14 2 4 7 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int wnextd_(char *side, doublereal *window, ftnlen side_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardd_ 4 1 7 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int wnfetd_(doublereal *window, integer *n, doublereal *left, doublereal *right);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardd_ 4 1 7 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int wnfild_(doublereal *small, doublereal *window);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardd_ 4 1 7 */
/*:ref: scardd_ 14 2 4 7 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int wnfltd_(doublereal *small, doublereal *window);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardd_ 4 1 7 */
/*:ref: scardd_ 14 2 4 7 */
/*:ref: chkout_ 14 2 13 124 */
 
extern logical wnincd_(doublereal *left, doublereal *right, doublereal *window);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardd_ 4 1 7 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int wninsd_(doublereal *left, doublereal *right, doublereal *window);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: sized_ 4 1 7 */
/*:ref: cardd_ 4 1 7 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: scardd_ 14 2 4 7 */
/*:ref: excess_ 14 3 4 13 124 */
 
extern int wnintd_(doublereal *a, doublereal *b, doublereal *c__);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardd_ 4 1 7 */
/*:ref: sized_ 4 1 7 */
/*:ref: scardd_ 14 2 4 7 */
/*:ref: excess_ 14 3 4 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern logical wnreld_(doublereal *a, char *op, doublereal *b, ftnlen op_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardd_ 4 1 7 */
/*:ref: wnincd_ 12 3 7 7 7 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int wnsumd_(doublereal *window, doublereal *meas, doublereal *avg, doublereal *stddev, integer *short__, integer *long__);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardd_ 4 1 7 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int wnunid_(doublereal *a, doublereal *b, doublereal *c__);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cardd_ 4 1 7 */
/*:ref: sized_ 4 1 7 */
/*:ref: scardd_ 14 2 4 7 */
/*:ref: excess_ 14 3 4 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int wnvald_(integer *size, integer *n, doublereal *a);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: odd_ 12 1 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: ssized_ 14 2 4 7 */
/*:ref: scardd_ 14 2 4 7 */
/*:ref: wninsd_ 14 3 7 7 7 */
 
extern int wrencc_(integer *unit, integer *n, char *data, ftnlen data_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int wrencd_(integer *unit, integer *n, doublereal *data);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dp2hx_ 14 4 7 13 4 124 */
 
extern int wrenci_(integer *unit, integer *n, integer *data);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: int2hx_ 14 4 4 13 4 124 */
 
extern int writla_(integer *numlin, char *array, integer *unit, ftnlen array_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: writln_ 14 3 13 4 124 */
/*:ref: failed_ 12 0 */
 
extern int writln_(char *line, integer *unit, ftnlen line_len);
/*:ref: rtrim_ 4 2 13 124 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errfnm_ 14 3 13 4 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int wrkvar_(integer *unit, char *name__, char *dirctv, char *tabsym, integer *tabptr, doublereal *tabval, ftnlen name_len, ftnlen dirctv_len, ftnlen tabsym_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: sydimd_ 4 6 13 13 4 7 124 124 */
/*:ref: synthd_ 14 9 13 4 13 4 7 7 12 124 124 */
/*:ref: ljust_ 14 4 13 13 124 124 */
/*:ref: rjust_ 14 4 13 13 124 124 */
/*:ref: ioerr_ 14 5 13 13 4 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int wrline_(char *device, char *line, ftnlen device_len, ftnlen line_len);
extern int clline_(char *device, ftnlen device_len);
/*:ref: ljust_ 14 4 13 13 124 124 */
/*:ref: ucase_ 14 4 13 13 124 124 */
/*:ref: rtrim_ 4 2 13 124 */
/*:ref: ltrim_ 4 2 13 124 */
/*:ref: fndlun_ 14 1 4 */
/*:ref: suffix_ 14 5 13 4 13 124 124 */
/*:ref: intstr_ 14 3 4 13 124 */
 
extern int xf2eul_(doublereal *xform, integer *axisa, integer *axisb, integer *axisc, doublereal *eulang, logical *unique);
extern int eul2xf_(doublereal *eulang, integer *axisa, integer *axisb, integer *axisc, doublereal *xform);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: m2eul_ 14 7 7 4 4 4 7 7 7 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: mxmt_ 14 3 7 7 7 */
/*:ref: mxv_ 14 3 7 7 7 */
/*:ref: moved_ 14 3 7 4 7 */
/*:ref: eul2m_ 14 7 7 7 7 4 4 4 7 */
/*:ref: mxm_ 14 3 7 7 7 */
 
extern int xf2rav_(doublereal *xform, doublereal *rot, doublereal *av);
/*:ref: mtxm_ 14 3 7 7 7 */
 
extern int xposbl_(doublereal *bmat, integer *nrow, integer *ncol, integer *bsize, doublereal *btmat);
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int xpose_(doublereal *m1, doublereal *mout);
 
extern int xposeg_(doublereal *matrix, integer *nrow, integer *ncol, doublereal *xposem);
 
extern int xpsgip_(integer *nrow, integer *ncol, doublereal *matrix);
 
extern int zzascii_(char *file, char *line, logical *check, char *termin, ftnlen file_len, ftnlen line_len, ftnlen termin_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: zzplatfm_ 14 4 13 13 124 124 */
/*:ref: eqstr_ 12 4 13 13 124 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: getlun_ 14 1 4 */
/*:ref: rtrim_ 4 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: errint_ 14 3 13 4 124 */
 
extern int zzasryel_(char *extrem, doublereal *ellips, doublereal *vertex, doublereal *dir, doublereal *angle, doublereal *extpt, ftnlen extrem_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cmprss_ 14 7 13 4 13 13 124 124 124 */
/*:ref: ljust_ 14 4 13 13 124 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: el2cgv_ 14 4 7 7 7 7 */
/*:ref: vnorm_ 7 1 7 */
/*:ref: vzero_ 12 1 7 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: psv2pl_ 14 4 7 7 7 7 */
/*:ref: failed_ 12 0 */
/*:ref: vprjp_ 14 3 7 7 7 */
/*:ref: vdist_ 7 2 7 7 */
/*:ref: inrypl_ 14 5 7 7 7 4 7 */
/*:ref: vsub_ 14 3 7 7 7 */
/*:ref: vdot_ 7 2 7 7 */
/*:ref: twopi_ 7 0 */
/*:ref: vhat_ 14 2 7 7 */
/*:ref: vlcom3_ 14 7 7 7 7 7 7 7 7 */
/*:ref: touchd_ 7 1 7 */
/*:ref: swapd_ 14 2 7 7 */
/*:ref: vadd_ 14 3 7 7 7 */
/*:ref: vsep_ 7 2 7 7 */
 
extern int zzbodblt_(integer *room, char *names, char *nornam, integer *codes, integer *nvals, char *device, char *reqst, ftnlen names_len, ftnlen nornam_len, ftnlen device_len, ftnlen reqst_len);
extern int zzbodget_(integer *room, char *names, char *nornam, integer *codes, integer *nvals, ftnlen names_len, ftnlen nornam_len);
extern int zzbodlst_(char *device, char *reqst, ftnlen device_len, ftnlen reqst_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzidmap_ 14 3 4 13 124 */
/*:ref: ljust_ 14 4 13 13 124 124 */
/*:ref: ucase_ 14 4 13 13 124 124 */
/*:ref: cmprss_ 14 7 13 4 13 13 124 124 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: movec_ 14 5 13 4 13 124 124 */
/*:ref: movei_ 14 3 4 4 4 */
/*:ref: intstr_ 14 3 4 13 124 */
/*:ref: wrline_ 14 4 13 13 124 124 */
/*:ref: lastnb_ 4 2 13 124 */
/*:ref: eqstr_ 12 4 13 13 124 124 */
/*:ref: orderi_ 14 3 4 4 4 */
/*:ref: orderc_ 14 4 13 4 4 124 */
 
extern integer zzbodbry_(integer *body);
 
extern int zzbodini_(char *names, char *nornam, integer *codes, integer *nvals, integer *ordnom, integer *ordcod, integer *nocds, ftnlen names_len, ftnlen nornam_len);
/*:ref: orderc_ 14 4 13 4 4 124 */
/*:ref: orderi_ 14 3 4 4 4 */
 
extern int zzbodker_(char *names, char *nornam, integer *codes, integer *nvals, integer *ordnom, integer *ordcod, integer *nocds, logical *extker, ftnlen names_len, ftnlen nornam_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: gcpool_ 14 8 13 4 4 4 13 12 124 124 */
/*:ref: gipool_ 14 7 13 4 4 4 4 12 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dtpool_ 14 6 13 12 4 13 124 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: ljust_ 14 4 13 13 124 124 */
/*:ref: ucase_ 14 4 13 13 124 124 */
/*:ref: cmprss_ 14 7 13 4 13 13 124 124 124 */
/*:ref: orderc_ 14 4 13 4 4 124 */
/*:ref: zzbodini_ 14 9 13 13 4 4 4 4 4 124 124 */
 
extern int zzbodtrn_(char *name__, integer *code, logical *found, ftnlen name_len);
extern int zzbodn2c_(char *name__, integer *code, logical *found, ftnlen name_len);
extern int zzbodc2n_(integer *code, char *name__, logical *found, ftnlen name_len);
extern int zzboddef_(char *name__, integer *code, ftnlen name_len);
extern int zzbodkik_(void);
extern int zzbodrst_(void);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzbodget_ 14 7 4 13 13 4 4 124 124 */
/*:ref: failed_ 12 0 */
/*:ref: zzbodini_ 14 9 13 13 4 4 4 4 4 124 124 */
/*:ref: swpool_ 14 5 13 4 13 124 124 */
/*:ref: cvpool_ 14 3 13 12 124 */
/*:ref: zzbodker_ 14 10 13 13 4 4 4 4 4 12 124 124 */
/*:ref: ljust_ 14 4 13 13 124 124 */
/*:ref: ucase_ 14 4 13 13 124 124 */
/*:ref: cmprss_ 14 7 13 4 13 13 124 124 124 */
/*:ref: bschoc_ 4 6 13 4 13 4 124 124 */
/*:ref: bschoi_ 4 4 4 4 4 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
 
extern int zzbodvcd_(integer *bodyid, char *item, integer *maxn, integer *dim, doublereal *values, ftnlen item_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: intstr_ 14 3 4 13 124 */
/*:ref: suffix_ 14 5 13 4 13 124 124 */
/*:ref: dtpool_ 14 6 13 12 4 13 124 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: gdpool_ 14 7 13 4 4 4 7 12 124 */
 
extern int zzck4d2i_(doublereal *dpcoef, integer *nsets, doublereal *parcod, integer *i__);
 
extern int zzck4i2d_(integer *i__, integer *nsets, doublereal *parcod, doublereal *dpcoef);
 
extern int zzckcv01_(integer *handle, integer *arrbeg, integer *arrend, integer *sclkid, doublereal *tol, char *timsys, doublereal *schedl, ftnlen timsys_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: eqstr_ 12 4 13 13 124 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: dafgda_ 14 4 4 4 4 7 */
/*:ref: errhan_ 14 3 13 4 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sct2e_ 14 3 4 7 7 */
/*:ref: wninsd_ 14 3 7 7 7 */
 
extern int zzckcv02_(integer *handle, integer *arrbeg, integer *arrend, integer *sclkid, doublereal *tol, char *timsys, doublereal *schedl, ftnlen timsys_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: eqstr_ 12 4 13 13 124 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: dafgda_ 14 4 4 4 4 7 */
/*:ref: sct2e_ 14 3 4 7 7 */
/*:ref: wninsd_ 14 3 7 7 7 */
 
extern int zzckcv03_(integer *handle, integer *arrbeg, integer *arrend, integer *sclkid, doublereal *tol, char *timsys, doublereal *schedl, ftnlen timsys_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: eqstr_ 12 4 13 13 124 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: dafgda_ 14 4 4 4 4 7 */
/*:ref: errhan_ 14 3 13 4 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sct2e_ 14 3 4 7 7 */
/*:ref: wninsd_ 14 3 7 7 7 */
 
extern int zzckcv04_(integer *handle, integer *arrbeg, integer *arrend, integer *sclkid, doublereal *tol, char *timsys, doublereal *schedl, ftnlen timsys_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: eqstr_ 12 4 13 13 124 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: intmax_ 4 0 */
/*:ref: dafps_ 14 5 4 4 7 4 7 */
/*:ref: cknr04_ 14 3 4 7 4 */
/*:ref: sgfpkt_ 14 6 4 7 4 4 7 4 */
/*:ref: sct2e_ 14 3 4 7 7 */
/*:ref: wninsd_ 14 3 7 7 7 */
 
extern int zzckcv05_(integer *handle, integer *arrbeg, integer *arrend, integer *sclkid, doublereal *dc, doublereal *tol, char *timsys, doublereal *schedl, ftnlen timsys_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: eqstr_ 12 4 13 13 124 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: dafgda_ 14 4 4 4 4 7 */
/*:ref: errint_ 14 3 13 7 124 */
/*:ref: sct2e_ 14 3 4 7 7 */
/*:ref: wninsd_ 14 3 7 7 7 */
 
extern int zzckspk_(integer *handle, char *ckspk, ftnlen ckspk_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dafhsf_ 14 3 4 4 4 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dafbfs_ 14 1 4 */
/*:ref: daffna_ 14 1 12 */
/*:ref: failed_ 12 0 */
/*:ref: dafgs_ 14 1 7 */
/*:ref: dafus_ 14 5 7 4 4 7 4 */
/*:ref: zzsizeok_ 14 6 4 4 4 4 12 4 */
/*:ref: dafgda_ 14 4 4 4 4 7 */
 
extern int zzcln_(integer *lookat, integer *nameat, integer *namlst, integer *datlst, integer *nmpool, integer *chpool, integer *dppool);
/*:ref: chkin_ 14 2 13 124 */
/*:ref: lnkfsl_ 14 3 4 4 4 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int zzcorepc_(char *abcorr, doublereal *et, doublereal *lt, doublereal *etcorr, ftnlen abcorr_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: zzprscor_ 14 3 13 12 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int zzdafgdr_(integer *handle, integer *recno, doublereal *dprec, logical *found);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: zzddhgsd_ 14 5 13 4 13 124 124 */
/*:ref: zzplatfm_ 14 4 13 13 124 124 */
/*:ref: ucase_ 14 4 13 13 124 124 */
/*:ref: isrchc_ 4 5 13 4 13 124 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzddhnfo_ 14 7 4 13 4 4 4 12 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: zzddhhlu_ 14 5 4 13 12 4 124 */
/*:ref: failed_ 12 0 */
/*:ref: zzxlated_ 14 5 4 13 4 7 124 */
/*:ref: moved_ 14 3 7 4 7 */
 
extern int zzdafgfr_(integer *handle, char *idword, integer *nd, integer *ni, char *ifname, integer *fward, integer *bward, integer *free, logical *found, ftnlen idword_len, ftnlen ifname_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: zzddhgsd_ 14 5 13 4 13 124 124 */
/*:ref: zzplatfm_ 14 4 13 13 124 124 */
/*:ref: ucase_ 14 4 13 13 124 124 */
/*:ref: isrchc_ 4 5 13 4 13 124 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzddhnfo_ 14 7 4 13 4 4 4 12 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: zzddhhlu_ 14 5 4 13 12 4 124 */
/*:ref: failed_ 12 0 */
/*:ref: zzxlatei_ 14 5 4 13 4 4 124 */
 
extern int zzdafgsr_(integer *handle, integer *recno, integer *nd, integer *ni, doublereal *dprec, logical *found);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: zzddhgsd_ 14 5 13 4 13 124 124 */
/*:ref: zzplatfm_ 14 4 13 13 124 124 */
/*:ref: ucase_ 14 4 13 13 124 124 */
/*:ref: isrchc_ 4 5 13 4 13 124 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzddhnfo_ 14 7 4 13 4 4 4 12 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: zzddhhlu_ 14 5 4 13 12 4 124 */
/*:ref: failed_ 12 0 */
/*:ref: zzxlated_ 14 5 4 13 4 7 124 */
/*:ref: zzxlatei_ 14 5 4 13 4 4 124 */
/*:ref: moved_ 14 3 7 4 7 */
 
extern int zzdafnfr_(integer *lun, char *idword, integer *nd, integer *ni, char *ifname, integer *fward, integer *bward, integer *free, char *format, ftnlen idword_len, ftnlen ifname_len, ftnlen format_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: zzftpstr_ 14 8 13 13 13 13 124 124 124 124 */
/*:ref: rtrim_ 4 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errfnm_ 14 3 13 4 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int zzdasnfr_(integer *lun, char *idword, char *ifname, integer *nresvr, integer *nresvc, integer *ncomr, integer *ncomc, char *format, ftnlen idword_len, ftnlen ifname_len, ftnlen format_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: zzftpstr_ 14 8 13 13 13 13 124 124 124 124 */
/*:ref: rtrim_ 4 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errfnm_ 14 3 13 4 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern integer zzddhclu_(logical *utlck, integer *nut);
 
extern int zzddhf2h_(char *fname, integer *ftabs, integer *ftamh, integer *ftarc, integer *ftbff, integer *fthan, char *ftnam, integer *ftrtm, integer *nft, integer *utcst, integer *uthan, logical *utlck, integer *utlun, integer *nut, logical *exists, logical *opened, integer *handle, logical *found, ftnlen fname_len, ftnlen ftnam_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: rtrim_ 4 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: isrchi_ 4 3 4 4 4 */
/*:ref: zzddhgtu_ 14 6 4 4 12 4 4 4 */
/*:ref: failed_ 12 0 */
/*:ref: zzddhrmu_ 14 7 4 4 4 4 12 4 4 */
/*:ref: errch_ 14 4 13 13 124 124 */
 
extern int zzddhgsd_(char *class__, integer *id, char *label, ftnlen class_len, ftnlen label_len);
/*:ref: ljust_ 14 4 13 13 124 124 */
/*:ref: ucase_ 14 4 13 13 124 124 */
/*:ref: isrchc_ 4 5 13 4 13 124 124 */
 
extern int zzddhgtu_(integer *utcst, integer *uthan, logical *utlck, integer *utlun, integer *nut, integer *uindex);
/*:ref: return_ 12 0 */
/*:ref: getlun_ 14 1 4 */
/*:ref: failed_ 12 0 */
/*:ref: orderi_ 14 3 4 4 4 */
/*:ref: frelun_ 14 1 4 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int zzddhini_(integer *natbff, integer *supbff, integer *numsup, char *stramh, char *strarc, char *strbff, ftnlen stramh_len, ftnlen strarc_len, ftnlen strbff_len);
/*:ref: return_ 12 0 */
/*:ref: zzddhgsd_ 14 5 13 4 13 124 124 */
/*:ref: zzplatfm_ 14 4 13 13 124 124 */
/*:ref: ucase_ 14 4 13 13 124 124 */
/*:ref: isrchc_ 4 5 13 4 13 124 124 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: nextwd_ 14 6 13 13 13 124 124 124 */
 
extern int zzddhivf_(char *nsum, integer *bff, logical *found, ftnlen nsum_len);
 
extern int zzddhman_(logical *lock, char *arch, char *fname, char *method, integer *handle, integer *unit, integer *intamh, integer *intarc, integer *intbff, logical *native, logical *found, logical *kill, ftnlen arch_len, ftnlen fname_len, ftnlen method_len);
extern int zzddhopn_(char *fname, char *method, char *arch, integer *handle, ftnlen fname_len, ftnlen method_len, ftnlen arch_len);
extern int zzddhcls_(integer *handle, char *arch, logical *kill, ftnlen arch_len);
extern int zzddhhlu_(integer *handle, char *arch, logical *lock, integer *unit, ftnlen arch_len);
extern int zzddhunl_(integer *handle, char *arch, ftnlen arch_len);
extern int zzddhnfo_(integer *handle, char *fname, integer *intarc, integer *intbff, integer *intamh, logical *found, ftnlen fname_len);
extern int zzddhisn_(integer *handle, logical *native, logical *found);
extern int zzddhfnh_(char *fname, integer *handle, logical *found, ftnlen fname_len);
extern int zzddhluh_(integer *unit, integer *handle, logical *found);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzddhini_ 14 9 4 4 4 13 13 13 124 124 124 */
/*:ref: failed_ 12 0 */
/*:ref: zzpltchk_ 14 1 12 */
/*:ref: ljust_ 14 4 13 13 124 124 */
/*:ref: ucase_ 14 4 13 13 124 124 */
/*:ref: isrchc_ 4 5 13 4 13 124 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: zzddhclu_ 4 2 12 4 */
/*:ref: zzddhf2h_ 14 20 13 4 4 4 4 4 13 4 4 4 4 12 4 4 12 12 4 12 124 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: bsrchi_ 4 3 4 4 4 */
/*:ref: zzddhrcm_ 14 3 4 4 4 */
/*:ref: zzddhgtu_ 14 6 4 4 12 4 4 4 */
/*:ref: rtrim_ 4 2 13 124 */
/*:ref: repmc_ 14 8 13 13 13 13 124 124 124 124 */
/*:ref: zzddhppf_ 14 3 4 4 4 */
/*:ref: isrchi_ 4 3 4 4 4 */
/*:ref: zzddhrmu_ 14 7 4 4 4 4 12 4 4 */
/*:ref: frelun_ 14 1 4 */
 
extern int zzddhppf_(integer *unit, integer *arch, integer *bff);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: zzddhgsd_ 14 5 13 4 13 124 124 */
/*:ref: zzftpstr_ 14 8 13 13 13 13 124 124 124 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: errfnm_ 14 3 13 4 124 */
/*:ref: idw2at_ 14 6 13 13 13 124 124 124 */
/*:ref: isrchc_ 4 5 13 4 13 124 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: zzftpchk_ 14 3 13 12 124 */
/*:ref: pos_ 4 5 13 13 4 124 124 */
/*:ref: zzplatfm_ 14 4 13 13 124 124 */
/*:ref: ucase_ 14 4 13 13 124 124 */
/*:ref: zzddhivf_ 14 4 13 4 12 124 */
 
extern int zzddhrcm_(integer *nut, integer *utcst, integer *reqcnt);
/*:ref: intmax_ 4 0 */
 
extern int zzddhrmu_(integer *uindex, integer *nft, integer *utcst, integer *uthan, logical *utlck, integer *utlun, integer *nut);
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: reslun_ 14 1 4 */
 
extern int zzdynbid_(char *frname, integer *frcode, char *item, integer *idcode, ftnlen frname_len, ftnlen item_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: intstr_ 14 3 4 13 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: rtrim_ 4 2 13 124 */
/*:ref: repmi_ 14 7 13 13 4 13 124 124 124 */
/*:ref: repmc_ 14 8 13 13 13 13 124 124 124 124 */
/*:ref: dtpool_ 14 6 13 12 4 13 124 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: gcpool_ 14 8 13 4 4 4 13 12 124 124 */
/*:ref: bods2c_ 14 4 13 4 12 124 */
/*:ref: gipool_ 14 7 13 4 4 4 4 12 124 */
 
extern int zzdynfid_(char *frname, integer *frcode, char *item, integer *idcode, ftnlen frname_len, ftnlen item_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: intstr_ 14 3 4 13 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: rtrim_ 4 2 13 124 */
/*:ref: repmi_ 14 7 13 13 4 13 124 124 124 */
/*:ref: repmc_ 14 8 13 13 13 13 124 124 124 124 */
/*:ref: dtpool_ 14 6 13 12 4 13 124 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: gcpool_ 14 8 13 4 4 4 13 12 124 124 */
/*:ref: namfrm_ 14 3 13 4 124 */
/*:ref: beint_ 12 2 13 124 */
/*:ref: prsint_ 14 3 13 4 124 */
/*:ref: gipool_ 14 7 13 4 4 4 4 12 124 */
 
extern int zzdynfr0_(integer *infram, integer *center, doublereal *et, doublereal *xform, integer *basfrm);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: irfnum_ 14 3 13 4 124 */
/*:ref: bodn2c_ 14 4 13 4 12 124 */
/*:ref: cleard_ 14 2 4 7 */
/*:ref: frmnam_ 14 3 4 13 124 */
/*:ref: zzdynfid_ 14 6 13 4 13 4 124 124 */
/*:ref: zzdynvac_ 14 9 13 4 13 4 4 13 124 124 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: eqstr_ 12 4 13 13 124 124 */
/*:ref: cmprss_ 14 7 13 4 13 13 124 124 124 */
/*:ref: ucase_ 14 4 13 13 124 124 */
/*:ref: zzdynoad_ 14 9 13 4 13 4 4 7 12 124 124 */
/*:ref: zzdynoac_ 14 10 13 4 13 4 4 13 12 124 124 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: bodc2n_ 14 4 4 13 12 124 */
/*:ref: intstr_ 14 3 4 13 124 */
/*:ref: zzeprc76_ 14 2 7 7 */
/*:ref: invstm_ 14 2 7 7 */
/*:ref: zzenut80_ 14 2 7 7 */
/*:ref: mxmg_ 14 6 7 7 4 4 4 7 */
/*:ref: zzmobliq_ 14 3 7 7 7 */
/*:ref: eul2xf_ 14 5 7 4 4 4 7 */
/*:ref: zzfrmch1_ 14 4 4 4 7 7 */
/*:ref: moved_ 14 3 7 4 7 */
/*:ref: isrchc_ 4 5 13 4 13 124 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: zzdynbid_ 14 6 13 4 13 4 124 124 */
/*:ref: zzspkez1_ 14 9 4 7 13 13 4 7 7 124 124 */
/*:ref: vpack_ 14 4 7 7 7 7 */
/*:ref: vhat_ 14 2 7 7 */
/*:ref: qderiv_ 14 5 4 7 7 7 7 */
/*:ref: vequ_ 14 2 7 7 */
/*:ref: zzprscor_ 14 3 13 12 124 */
/*:ref: frinfo_ 14 5 4 4 4 4 12 */
/*:ref: zzspkzp1_ 14 9 4 7 13 13 4 7 7 124 124 */
/*:ref: zzcorepc_ 14 5 13 7 7 7 124 */
/*:ref: mxvg_ 14 5 7 7 4 4 7 */
/*:ref: cidfrm_ 14 5 4 4 13 12 124 */
/*:ref: bodvcd_ 14 6 4 13 4 4 7 124 */
/*:ref: vminug_ 14 3 7 4 7 */
/*:ref: dnearp_ 14 7 7 7 7 7 7 7 12 */
/*:ref: vsubg_ 14 4 7 7 4 7 */
/*:ref: zzspksb1_ 14 5 4 7 13 7 124 */
/*:ref: zzdynvad_ 14 8 13 4 13 4 4 7 124 124 */
/*:ref: convrt_ 14 6 7 13 13 7 124 124 */
/*:ref: latrec_ 14 4 7 7 7 7 */
/*:ref: stlabx_ 14 3 7 7 7 */
/*:ref: stelab_ 14 3 7 7 7 */
/*:ref: vsep_ 7 2 7 7 */
/*:ref: etcal_ 14 3 7 13 124 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: zztwovxf_ 14 5 7 4 7 4 7 */
/*:ref: zzdynvai_ 14 8 13 4 13 4 4 4 124 124 */
/*:ref: polyds_ 14 5 7 4 4 7 7 */
 
extern int zzdynfrm_(integer *infram, integer *center, doublereal *et, doublereal *xform, integer *basfrm);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: irfnum_ 14 3 13 4 124 */
/*:ref: bodn2c_ 14 4 13 4 12 124 */
/*:ref: cleard_ 14 2 4 7 */
/*:ref: frmnam_ 14 3 4 13 124 */
/*:ref: zzdynfid_ 14 6 13 4 13 4 124 124 */
/*:ref: zzdynvac_ 14 9 13 4 13 4 4 13 124 124 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: eqstr_ 12 4 13 13 124 124 */
/*:ref: cmprss_ 14 7 13 4 13 13 124 124 124 */
/*:ref: ucase_ 14 4 13 13 124 124 */
/*:ref: zzdynoad_ 14 9 13 4 13 4 4 7 12 124 124 */
/*:ref: zzdynoac_ 14 10 13 4 13 4 4 13 12 124 124 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: bodc2n_ 14 4 4 13 12 124 */
/*:ref: intstr_ 14 3 4 13 124 */
/*:ref: zzeprc76_ 14 2 7 7 */
/*:ref: invstm_ 14 2 7 7 */
/*:ref: zzenut80_ 14 2 7 7 */
/*:ref: mxmg_ 14 6 7 7 4 4 4 7 */
/*:ref: zzmobliq_ 14 3 7 7 7 */
/*:ref: eul2xf_ 14 5 7 4 4 4 7 */
/*:ref: zzfrmch0_ 14 4 4 4 7 7 */
/*:ref: moved_ 14 3 7 4 7 */
/*:ref: isrchc_ 4 5 13 4 13 124 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: zzdynbid_ 14 6 13 4 13 4 124 124 */
/*:ref: zzspkez0_ 14 9 4 7 13 13 4 7 7 124 124 */
/*:ref: vpack_ 14 4 7 7 7 7 */
/*:ref: vhat_ 14 2 7 7 */
/*:ref: qderiv_ 14 5 4 7 7 7 7 */
/*:ref: vequ_ 14 2 7 7 */
/*:ref: zzprscor_ 14 3 13 12 124 */
/*:ref: frinfo_ 14 5 4 4 4 4 12 */
/*:ref: zzspkzp0_ 14 9 4 7 13 13 4 7 7 124 124 */
/*:ref: zzcorepc_ 14 5 13 7 7 7 124 */
/*:ref: mxvg_ 14 5 7 7 4 4 7 */
/*:ref: cidfrm_ 14 5 4 4 13 12 124 */
/*:ref: bodvcd_ 14 6 4 13 4 4 7 124 */
/*:ref: vminug_ 14 3 7 4 7 */
/*:ref: dnearp_ 14 7 7 7 7 7 7 7 12 */
/*:ref: vsubg_ 14 4 7 7 4 7 */
/*:ref: zzspksb0_ 14 5 4 7 13 7 124 */
/*:ref: zzdynvad_ 14 8 13 4 13 4 4 7 124 124 */
/*:ref: convrt_ 14 6 7 13 13 7 124 124 */
/*:ref: latrec_ 14 4 7 7 7 7 */
/*:ref: stlabx_ 14 3 7 7 7 */
/*:ref: stelab_ 14 3 7 7 7 */
/*:ref: vsep_ 7 2 7 7 */
/*:ref: etcal_ 14 3 7 13 124 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: zztwovxf_ 14 5 7 4 7 4 7 */
/*:ref: zzdynvai_ 14 8 13 4 13 4 4 4 124 124 */
/*:ref: polyds_ 14 5 7 4 4 7 7 */
 
extern int zzdynoac_(char *frname, integer *frcode, char *item, integer *maxn, integer *n, char *values, logical *found, ftnlen frname_len, ftnlen item_len, ftnlen values_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: intstr_ 14 3 4 13 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: rtrim_ 4 2 13 124 */
/*:ref: repmi_ 14 7 13 13 4 13 124 124 124 */
/*:ref: repmc_ 14 8 13 13 13 13 124 124 124 124 */
/*:ref: dtpool_ 14 6 13 12 4 13 124 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: gcpool_ 14 8 13 4 4 4 13 12 124 124 */
 
extern int zzdynoad_(char *frname, integer *frcode, char *item, integer *maxn, integer *n, doublereal *values, logical *found, ftnlen frname_len, ftnlen item_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: intstr_ 14 3 4 13 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: rtrim_ 4 2 13 124 */
/*:ref: repmi_ 14 7 13 13 4 13 124 124 124 */
/*:ref: repmc_ 14 8 13 13 13 13 124 124 124 124 */
/*:ref: dtpool_ 14 6 13 12 4 13 124 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: gdpool_ 14 7 13 4 4 4 7 12 124 */
 
extern int zzdynrot_(integer *infram, integer *center, doublereal *et, doublereal *rotate, integer *basfrm);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: irfnum_ 14 3 13 4 124 */
/*:ref: bodn2c_ 14 4 13 4 12 124 */
/*:ref: cleard_ 14 2 4 7 */
/*:ref: frmnam_ 14 3 4 13 124 */
/*:ref: zzdynfid_ 14 6 13 4 13 4 124 124 */
/*:ref: zzdynvac_ 14 9 13 4 13 4 4 13 124 124 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: eqstr_ 12 4 13 13 124 124 */
/*:ref: cmprss_ 14 7 13 4 13 13 124 124 124 */
/*:ref: ucase_ 14 4 13 13 124 124 */
/*:ref: zzdynoad_ 14 9 13 4 13 4 4 7 12 124 124 */
/*:ref: zzdynoac_ 14 10 13 4 13 4 4 13 12 124 124 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: bodc2n_ 14 4 4 13 12 124 */
/*:ref: intstr_ 14 3 4 13 124 */
/*:ref: zzeprc76_ 14 2 7 7 */
/*:ref: moved_ 14 3 7 4 7 */
/*:ref: xpose_ 14 2 7 7 */
/*:ref: zzenut80_ 14 2 7 7 */
/*:ref: mxm_ 14 3 7 7 7 */
/*:ref: zzmobliq_ 14 3 7 7 7 */
/*:ref: eul2m_ 14 7 7 7 7 4 4 4 7 */
/*:ref: zzrefch0_ 14 4 4 4 7 7 */
/*:ref: isrchc_ 4 5 13 4 13 124 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: zzdynbid_ 14 6 13 4 13 4 124 124 */
/*:ref: zzspkzp0_ 14 9 4 7 13 13 4 7 7 124 124 */
/*:ref: zzspkez0_ 14 9 4 7 13 13 4 7 7 124 124 */
/*:ref: vhat_ 14 2 7 7 */
/*:ref: zzprscor_ 14 3 13 12 124 */
/*:ref: frinfo_ 14 5 4 4 4 4 12 */
/*:ref: zzcorepc_ 14 5 13 7 7 7 124 */
/*:ref: mxv_ 14 3 7 7 7 */
/*:ref: cidfrm_ 14 5 4 4 13 12 124 */
/*:ref: bodvcd_ 14 6 4 13 4 4 7 124 */
/*:ref: vminus_ 14 2 7 7 */
/*:ref: nearpt_ 14 6 7 7 7 7 7 7 */
/*:ref: vsub_ 14 3 7 7 7 */
/*:ref: zzspksb0_ 14 5 4 7 13 7 124 */
/*:ref: zzdynvad_ 14 8 13 4 13 4 4 7 124 124 */
/*:ref: convrt_ 14 6 7 13 13 7 124 124 */
/*:ref: latrec_ 14 4 7 7 7 7 */
/*:ref: vequ_ 14 2 7 7 */
/*:ref: stlabx_ 14 3 7 7 7 */
/*:ref: stelab_ 14 3 7 7 7 */
/*:ref: vsep_ 7 2 7 7 */
/*:ref: etcal_ 14 3 7 13 124 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: twovec_ 14 5 7 4 7 4 7 */
/*:ref: zzdynvai_ 14 8 13 4 13 4 4 4 124 124 */
/*:ref: polyds_ 14 5 7 4 4 7 7 */
 
extern int zzdynrt0_(integer *infram, integer *center, doublereal *et, doublereal *rotate, integer *basfrm);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: irfnum_ 14 3 13 4 124 */
/*:ref: bodn2c_ 14 4 13 4 12 124 */
/*:ref: cleard_ 14 2 4 7 */
/*:ref: frmnam_ 14 3 4 13 124 */
/*:ref: zzdynfid_ 14 6 13 4 13 4 124 124 */
/*:ref: zzdynvac_ 14 9 13 4 13 4 4 13 124 124 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: eqstr_ 12 4 13 13 124 124 */
/*:ref: cmprss_ 14 7 13 4 13 13 124 124 124 */
/*:ref: ucase_ 14 4 13 13 124 124 */
/*:ref: zzdynoad_ 14 9 13 4 13 4 4 7 12 124 124 */
/*:ref: zzdynoac_ 14 10 13 4 13 4 4 13 12 124 124 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: bodc2n_ 14 4 4 13 12 124 */
/*:ref: intstr_ 14 3 4 13 124 */
/*:ref: zzeprc76_ 14 2 7 7 */
/*:ref: moved_ 14 3 7 4 7 */
/*:ref: xpose_ 14 2 7 7 */
/*:ref: zzenut80_ 14 2 7 7 */
/*:ref: mxm_ 14 3 7 7 7 */
/*:ref: zzmobliq_ 14 3 7 7 7 */
/*:ref: eul2m_ 14 7 7 7 7 4 4 4 7 */
/*:ref: zzrefch1_ 14 4 4 4 7 7 */
/*:ref: isrchc_ 4 5 13 4 13 124 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: zzdynbid_ 14 6 13 4 13 4 124 124 */
/*:ref: zzspkzp1_ 14 9 4 7 13 13 4 7 7 124 124 */
/*:ref: zzspkez1_ 14 9 4 7 13 13 4 7 7 124 124 */
/*:ref: vhat_ 14 2 7 7 */
/*:ref: zzprscor_ 14 3 13 12 124 */
/*:ref: frinfo_ 14 5 4 4 4 4 12 */
/*:ref: zzcorepc_ 14 5 13 7 7 7 124 */
/*:ref: mxv_ 14 3 7 7 7 */
/*:ref: cidfrm_ 14 5 4 4 13 12 124 */
/*:ref: bodvcd_ 14 6 4 13 4 4 7 124 */
/*:ref: vminus_ 14 2 7 7 */
/*:ref: nearpt_ 14 6 7 7 7 7 7 7 */
/*:ref: vsub_ 14 3 7 7 7 */
/*:ref: zzspksb1_ 14 5 4 7 13 7 124 */
/*:ref: zzdynvad_ 14 8 13 4 13 4 4 7 124 124 */
/*:ref: convrt_ 14 6 7 13 13 7 124 124 */
/*:ref: latrec_ 14 4 7 7 7 7 */
/*:ref: stlabx_ 14 3 7 7 7 */
/*:ref: stelab_ 14 3 7 7 7 */
/*:ref: vequ_ 14 2 7 7 */
/*:ref: vsep_ 7 2 7 7 */
/*:ref: etcal_ 14 3 7 13 124 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: twovec_ 14 5 7 4 7 4 7 */
/*:ref: zzdynvai_ 14 8 13 4 13 4 4 4 124 124 */
/*:ref: polyds_ 14 5 7 4 4 7 7 */
 
extern int zzdynvac_(char *frname, integer *frcode, char *item, integer *maxn, integer *n, char *values, ftnlen frname_len, ftnlen item_len, ftnlen values_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: intstr_ 14 3 4 13 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: rtrim_ 4 2 13 124 */
/*:ref: repmi_ 14 7 13 13 4 13 124 124 124 */
/*:ref: repmc_ 14 8 13 13 13 13 124 124 124 124 */
/*:ref: dtpool_ 14 6 13 12 4 13 124 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: gcpool_ 14 8 13 4 4 4 13 12 124 124 */
 
extern int zzdynvad_(char *frname, integer *frcode, char *item, integer *maxn, integer *n, doublereal *values, ftnlen frname_len, ftnlen item_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: intstr_ 14 3 4 13 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: rtrim_ 4 2 13 124 */
/*:ref: repmi_ 14 7 13 13 4 13 124 124 124 */
/*:ref: repmc_ 14 8 13 13 13 13 124 124 124 124 */
/*:ref: dtpool_ 14 6 13 12 4 13 124 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: gdpool_ 14 7 13 4 4 4 7 12 124 */
 
extern int zzdynvai_(char *frname, integer *frcode, char *item, integer *maxn, integer *n, integer *values, ftnlen frname_len, ftnlen item_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: intstr_ 14 3 4 13 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: rtrim_ 4 2 13 124 */
/*:ref: repmi_ 14 7 13 13 4 13 124 124 124 */
/*:ref: repmc_ 14 8 13 13 13 13 124 124 124 124 */
/*:ref: dtpool_ 14 6 13 12 4 13 124 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: gipool_ 14 7 13 4 4 4 4 12 124 */
 
extern int zzekac01_(integer *handle, integer *segdsc, integer *coldsc, integer *ivals, logical *nlflgs, integer *rcptrs, integer *wkindx);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: zzekcnam_ 14 4 4 4 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzekspsh_ 14 2 4 4 */
/*:ref: zzekaps_ 14 6 4 4 4 12 4 4 */
/*:ref: cleari_ 14 2 4 4 */
/*:ref: zzekpgwi_ 14 3 4 4 4 */
/*:ref: zzekordi_ 14 5 4 12 12 4 4 */
/*:ref: zzektrit_ 14 2 4 4 */
/*:ref: zzektr1s_ 14 4 4 4 4 4 */
/*:ref: dasudi_ 14 4 4 4 4 4 */
 
extern int zzekac02_(integer *handle, integer *segdsc, integer *coldsc, doublereal *dvals, logical *nlflgs, integer *rcptrs, integer *wkindx);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: zzekcnam_ 14 4 4 4 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzekspsh_ 14 2 4 4 */
/*:ref: zzekaps_ 14 6 4 4 4 12 4 4 */
/*:ref: cleard_ 14 2 4 7 */
/*:ref: zzekpgwd_ 14 3 4 4 7 */
/*:ref: zzekordd_ 14 5 7 12 12 4 4 */
/*:ref: zzektrit_ 14 2 4 4 */
/*:ref: zzektr1s_ 14 4 4 4 4 4 */
/*:ref: dasudi_ 14 4 4 4 4 4 */
 
extern int zzekac03_(integer *handle, integer *segdsc, integer *coldsc, char *cvals, logical *nlflgs, integer *rcptrs, integer *wkindx, ftnlen cvals_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: zzekcnam_ 14 4 4 4 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzekspsh_ 14 2 4 4 */
/*:ref: zzekaps_ 14 6 4 4 4 12 4 4 */
/*:ref: prtenc_ 14 3 4 13 124 */
/*:ref: rtrim_ 4 2 13 124 */
/*:ref: prtdec_ 14 3 13 4 124 */
/*:ref: zzekpgwc_ 14 4 4 4 13 124 */
/*:ref: zzekordc_ 14 6 13 12 12 4 4 124 */
/*:ref: zzektrit_ 14 2 4 4 */
/*:ref: zzektr1s_ 14 4 4 4 4 4 */
/*:ref: dasudi_ 14 4 4 4 4 4 */
 
extern int zzekac04_(integer *handle, integer *segdsc, integer *coldsc, integer *ivals, integer *entszs, logical *nlflgs);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: zzekcnam_ 14 4 4 4 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzekspsh_ 14 2 4 4 */
/*:ref: zzekaps_ 14 6 4 4 4 12 4 4 */
/*:ref: cleari_ 14 2 4 4 */
/*:ref: zzekpgwi_ 14 3 4 4 4 */
/*:ref: zzeksfwd_ 14 4 4 4 4 4 */
 
extern int zzekac05_(integer *handle, integer *segdsc, integer *coldsc, doublereal *dvals, integer *entszs, logical *nlflgs);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: zzekcnam_ 14 4 4 4 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzekspsh_ 14 2 4 4 */
/*:ref: zzekaps_ 14 6 4 4 4 12 4 4 */
/*:ref: cleard_ 14 2 4 7 */
/*:ref: zzekpgwd_ 14 3 4 4 7 */
/*:ref: zzeksfwd_ 14 4 4 4 4 4 */
 
extern int zzekac06_(integer *handle, integer *segdsc, integer *coldsc, char *cvals, integer *entszs, logical *nlflgs, ftnlen cvals_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: zzekcnam_ 14 4 4 4 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzekspsh_ 14 2 4 4 */
/*:ref: zzekaps_ 14 6 4 4 4 12 4 4 */
/*:ref: prtenc_ 14 3 4 13 124 */
/*:ref: zzekpgwc_ 14 4 4 4 13 124 */
/*:ref: zzeksfwd_ 14 4 4 4 4 4 */
 
extern int zzekac07_(integer *handle, integer *segdsc, integer *coldsc, integer *ivals, logical *nlflgs, integer *wkindx);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: zzekcnam_ 14 4 4 4 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzekacps_ 14 6 4 4 4 4 4 4 */
/*:ref: cleari_ 14 2 4 4 */
/*:ref: zzekpgwi_ 14 3 4 4 4 */
/*:ref: dasudi_ 14 4 4 4 4 4 */
/*:ref: zzekordi_ 14 5 4 12 12 4 4 */
/*:ref: zzekwpai_ 14 6 4 4 4 4 4 4 */
/*:ref: zzekwpal_ 14 6 4 4 4 12 4 4 */
 
extern int zzekac08_(integer *handle, integer *segdsc, integer *coldsc, doublereal *dvals, logical *nlflgs, integer *wkindx);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: zzekcnam_ 14 4 4 4 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzekacps_ 14 6 4 4 4 4 4 4 */
/*:ref: cleard_ 14 2 4 7 */
/*:ref: zzekpgwd_ 14 3 4 4 7 */
/*:ref: dasudi_ 14 4 4 4 4 4 */
/*:ref: zzekordd_ 14 5 7 12 12 4 4 */
/*:ref: zzekwpai_ 14 6 4 4 4 4 4 4 */
/*:ref: zzekwpal_ 14 6 4 4 4 12 4 4 */
 
extern int zzekac09_(integer *handle, integer *segdsc, integer *coldsc, char *cvals, logical *nlflgs, integer *wkindx, ftnlen cvals_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: zzekcnam_ 14 4 4 4 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzekacps_ 14 6 4 4 4 4 4 4 */
/*:ref: zzekpgwc_ 14 4 4 4 13 124 */
/*:ref: zzekslnk_ 14 4 4 4 4 4 */
/*:ref: dasudi_ 14 4 4 4 4 4 */
/*:ref: zzekordc_ 14 6 13 12 12 4 4 124 */
/*:ref: zzekwpai_ 14 6 4 4 4 4 4 4 */
/*:ref: zzekwpal_ 14 6 4 4 4 12 4 4 */
 
extern int zzekacps_(integer *handle, integer *segdsc, integer *type__, integer *n, integer *p, integer *base);
/*:ref: zzekpgan_ 14 4 4 4 4 4 */
/*:ref: failed_ 12 0 */
/*:ref: zzekslnk_ 14 4 4 4 4 4 */
/*:ref: zzeksfwd_ 14 4 4 4 4 4 */
/*:ref: zzektrap_ 14 4 4 4 4 4 */
 
extern int zzekad01_(integer *handle, integer *segdsc, integer *coldsc, integer *recptr, integer *ival, logical *isnull);
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzekrp2n_ 4 3 4 4 4 */
/*:ref: dasrdi_ 14 4 4 4 4 4 */
/*:ref: dasudi_ 14 4 4 4 4 4 */
/*:ref: zzekpgbs_ 14 3 4 4 4 */
/*:ref: zzekglnk_ 14 4 4 4 4 4 */
/*:ref: zzekslnk_ 14 4 4 4 4 4 */
/*:ref: zzekaps_ 14 6 4 4 4 12 4 4 */
/*:ref: zzekiii1_ 14 6 4 4 4 4 4 12 */
 
extern int zzekad02_(integer *handle, integer *segdsc, integer *coldsc, integer *recptr, doublereal *dval, logical *isnull);
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzekrp2n_ 4 3 4 4 4 */
/*:ref: dasrdi_ 14 4 4 4 4 4 */
/*:ref: dasudi_ 14 4 4 4 4 4 */
/*:ref: zzekpgbs_ 14 3 4 4 4 */
/*:ref: dasudd_ 14 4 4 4 4 7 */
/*:ref: zzekglnk_ 14 4 4 4 4 4 */
/*:ref: zzekslnk_ 14 4 4 4 4 4 */
/*:ref: zzekaps_ 14 6 4 4 4 12 4 4 */
/*:ref: zzekiid1_ 14 6 4 4 4 7 4 12 */
 
extern int zzekad03_(integer *handle, integer *segdsc, integer *coldsc, integer *recptr, char *cval, logical *isnull, ftnlen cval_len);
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzekrp2n_ 4 3 4 4 4 */
/*:ref: rtrim_ 4 2 13 124 */
/*:ref: dasudi_ 14 4 4 4 4 4 */
/*:ref: zzekpgbs_ 14 3 4 4 4 */
/*:ref: zzeksei_ 14 3 4 4 4 */
/*:ref: dasudc_ 14 7 4 4 4 4 4 13 124 */
/*:ref: zzekglnk_ 14 4 4 4 4 4 */
/*:ref: zzekslnk_ 14 4 4 4 4 4 */
/*:ref: zzekaps_ 14 6 4 4 4 12 4 4 */
/*:ref: zzeksfwd_ 14 4 4 4 4 4 */
/*:ref: zzekiic1_ 14 7 4 4 4 13 4 12 124 */
 
extern int zzekad04_(integer *handle, integer *segdsc, integer *coldsc, integer *recptr, integer *nvals, integer *ivals, logical *isnull);
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzekrp2n_ 4 3 4 4 4 */
/*:ref: dasudi_ 14 4 4 4 4 4 */
/*:ref: zzekpgbs_ 14 3 4 4 4 */
/*:ref: zzekglnk_ 14 4 4 4 4 4 */
/*:ref: zzekslnk_ 14 4 4 4 4 4 */
/*:ref: zzekaps_ 14 6 4 4 4 12 4 4 */
/*:ref: zzeksfwd_ 14 4 4 4 4 4 */
 
extern int zzekad05_(integer *handle, integer *segdsc, integer *coldsc, integer *recptr, integer *nvals, doublereal *dvals, logical *isnull);
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzekrp2n_ 4 3 4 4 4 */
/*:ref: dasudi_ 14 4 4 4 4 4 */
/*:ref: zzekpgbs_ 14 3 4 4 4 */
/*:ref: dasudd_ 14 4 4 4 4 7 */
/*:ref: zzekglnk_ 14 4 4 4 4 4 */
/*:ref: zzekslnk_ 14 4 4 4 4 4 */
/*:ref: zzekaps_ 14 6 4 4 4 12 4 4 */
/*:ref: zzeksfwd_ 14 4 4 4 4 4 */
 
extern int zzekad06_(integer *handle, integer *segdsc, integer *coldsc, integer *recptr, integer *nvals, char *cvals, logical *isnull, ftnlen cvals_len);
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzekrp2n_ 4 3 4 4 4 */
/*:ref: dasudi_ 14 4 4 4 4 4 */
/*:ref: zzekpgbs_ 14 3 4 4 4 */
/*:ref: failed_ 12 0 */
/*:ref: zzeksei_ 14 3 4 4 4 */
/*:ref: zzekglnk_ 14 4 4 4 4 4 */
/*:ref: zzekslnk_ 14 4 4 4 4 4 */
/*:ref: dasudc_ 14 7 4 4 4 4 4 13 124 */
/*:ref: zzekaps_ 14 6 4 4 4 12 4 4 */
/*:ref: zzeksfwd_ 14 4 4 4 4 4 */
 
extern int zzekaps_(integer *handle, integer *segdsc, integer *type__, logical *new__, integer *p, integer *base);
/*:ref: zzekpgan_ 14 4 4 4 4 4 */
/*:ref: zzekpgal_ 14 4 4 4 4 4 */
/*:ref: failed_ 12 0 */
/*:ref: zzekslnk_ 14 4 4 4 4 4 */
/*:ref: zzeksfwd_ 14 4 4 4 4 4 */
/*:ref: zzektrap_ 14 4 4 4 4 4 */
 
extern int zzekbs01_(integer *handle, char *tabnam, integer *ncols, char *cnames, integer *cdscrs, integer *segno, ftnlen tabnam_len, ftnlen cnames_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: zzekpgch_ 14 3 4 13 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzekpgan_ 14 4 4 4 4 4 */
/*:ref: zzektrit_ 14 2 4 4 */
/*:ref: cleari_ 14 2 4 4 */
/*:ref: eknseg_ 4 1 4 */
/*:ref: ljust_ 14 4 13 13 124 124 */
/*:ref: ucase_ 14 4 13 13 124 124 */
/*:ref: zzekpgwc_ 14 4 4 4 13 124 */
/*:ref: zzekcix1_ 14 2 4 4 */
/*:ref: movei_ 14 3 4 4 4 */
/*:ref: zzekpgwi_ 14 3 4 4 4 */
/*:ref: zzekpgbs_ 14 3 4 4 4 */
/*:ref: dasrdi_ 14 4 4 4 4 4 */
/*:ref: zzektrap_ 14 4 4 4 4 4 */
 
extern int zzekbs02_(integer *handle, char *tabnam, integer *ncols, char *cnames, integer *cdscrs, integer *segno, ftnlen tabnam_len, ftnlen cnames_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: zzekpgch_ 14 3 4 13 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzekpgan_ 14 4 4 4 4 4 */
/*:ref: zzektrit_ 14 2 4 4 */
/*:ref: cleari_ 14 2 4 4 */
/*:ref: eknseg_ 4 1 4 */
/*:ref: ljust_ 14 4 13 13 124 124 */
/*:ref: ucase_ 14 4 13 13 124 124 */
/*:ref: zzekpgwc_ 14 4 4 4 13 124 */
/*:ref: movei_ 14 3 4 4 4 */
/*:ref: zzekpgwi_ 14 3 4 4 4 */
/*:ref: zzekpgbs_ 14 3 4 4 4 */
/*:ref: dasrdi_ 14 4 4 4 4 4 */
/*:ref: zzektrap_ 14 4 4 4 4 4 */
 
extern int zzekcchk_(char *query, integer *eqryi, char *eqryc, integer *ntab, char *tablst, char *alslst, integer *base, logical *error, char *errmsg, integer *errptr, ftnlen query_len, ftnlen eqryc_len, ftnlen tablst_len, ftnlen alslst_len, ftnlen errmsg_len);
/*:ref: zzekreqi_ 14 4 4 13 4 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: ekccnt_ 14 3 13 4 124 */
/*:ref: ekcii_ 14 6 13 4 13 4 124 124 */
/*:ref: repmc_ 14 8 13 13 13 13 124 124 124 124 */
/*:ref: isrchc_ 4 5 13 4 13 124 124 */
 
extern int zzekcdsc_(integer *handle, integer *segdsc, char *column, integer *coldsc, ftnlen column_len);
/*:ref: dasrdi_ 14 4 4 4 4 4 */
/*:ref: dasrdc_ 14 7 4 4 4 4 4 13 124 */
/*:ref: eqstr_ 12 4 13 13 124 124 */
/*:ref: dashlu_ 14 2 4 4 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: errfnm_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int zzekcix1_(integer *handle, integer *coldsc);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: zzekpgch_ 14 3 4 13 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzektrit_ 14 2 4 4 */
 
extern int zzekcnam_(integer *handle, integer *coldsc, char *column, ftnlen column_len);
/*:ref: dasrdc_ 14 7 4 4 4 4 4 13 124 */
 
extern int zzekde01_(integer *handle, integer *segdsc, integer *coldsc, integer *recptr);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: zzekpgch_ 14 3 4 13 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dasrdi_ 14 4 4 4 4 4 */
/*:ref: zzekixdl_ 14 4 4 4 4 4 */
/*:ref: zzekpgpg_ 14 4 4 4 4 4 */
/*:ref: zzekglnk_ 14 4 4 4 4 4 */
/*:ref: zzekslnk_ 14 4 4 4 4 4 */
/*:ref: zzekdps_ 14 4 4 4 4 4 */
/*:ref: dasudi_ 14 4 4 4 4 4 */
/*:ref: zzekrp2n_ 4 3 4 4 4 */
/*:ref: dashlu_ 14 2 4 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: errfnm_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
 
extern int zzekde02_(integer *handle, integer *segdsc, integer *coldsc, integer *recptr);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: zzekpgch_ 14 3 4 13 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dasrdi_ 14 4 4 4 4 4 */
/*:ref: zzekixdl_ 14 4 4 4 4 4 */
/*:ref: zzekpgpg_ 14 4 4 4 4 4 */
/*:ref: zzekglnk_ 14 4 4 4 4 4 */
/*:ref: zzekslnk_ 14 4 4 4 4 4 */
/*:ref: zzekdps_ 14 4 4 4 4 4 */
/*:ref: dasudi_ 14 4 4 4 4 4 */
/*:ref: zzekrp2n_ 4 3 4 4 4 */
/*:ref: dashlu_ 14 2 4 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: errfnm_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
 
extern int zzekde03_(integer *handle, integer *segdsc, integer *coldsc, integer *recptr);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: zzekpgch_ 14 3 4 13 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dasrdi_ 14 4 4 4 4 4 */
/*:ref: zzekixdl_ 14 4 4 4 4 4 */
/*:ref: zzekgei_ 14 3 4 4 4 */
/*:ref: dasudi_ 14 4 4 4 4 4 */
/*:ref: zzekpgpg_ 14 4 4 4 4 4 */
/*:ref: zzekgfwd_ 14 4 4 4 4 4 */
/*:ref: zzekglnk_ 14 4 4 4 4 4 */
/*:ref: zzekslnk_ 14 4 4 4 4 4 */
/*:ref: zzekdps_ 14 4 4 4 4 4 */
/*:ref: zzekrp2n_ 4 3 4 4 4 */
/*:ref: dashlu_ 14 2 4 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: errfnm_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
 
extern int zzekde04_(integer *handle, integer *segdsc, integer *coldsc, integer *recptr);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: zzekpgch_ 14 3 4 13 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dasrdi_ 14 4 4 4 4 4 */
/*:ref: dasudi_ 14 4 4 4 4 4 */
/*:ref: zzekpgpg_ 14 4 4 4 4 4 */
/*:ref: zzekgfwd_ 14 4 4 4 4 4 */
/*:ref: zzekglnk_ 14 4 4 4 4 4 */
/*:ref: zzekslnk_ 14 4 4 4 4 4 */
/*:ref: zzekdps_ 14 4 4 4 4 4 */
/*:ref: dashlu_ 14 2 4 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: errfnm_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
 
extern int zzekde05_(integer *handle, integer *segdsc, integer *coldsc, integer *recptr);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: zzekpgch_ 14 3 4 13 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dasrdi_ 14 4 4 4 4 4 */
/*:ref: dasrdd_ 14 4 4 4 4 7 */
/*:ref: dasudi_ 14 4 4 4 4 4 */
/*:ref: zzekpgpg_ 14 4 4 4 4 4 */
/*:ref: zzekgfwd_ 14 4 4 4 4 4 */
/*:ref: zzekglnk_ 14 4 4 4 4 4 */
/*:ref: zzekslnk_ 14 4 4 4 4 4 */
/*:ref: zzekdps_ 14 4 4 4 4 4 */
/*:ref: zzekrp2n_ 4 3 4 4 4 */
/*:ref: dashlu_ 14 2 4 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: errfnm_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
 
extern int zzekde06_(integer *handle, integer *segdsc, integer *coldsc, integer *recptr);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: zzekpgch_ 14 3 4 13 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dasrdi_ 14 4 4 4 4 4 */
/*:ref: zzekgei_ 14 3 4 4 4 */
/*:ref: dasudi_ 14 4 4 4 4 4 */
/*:ref: zzekpgpg_ 14 4 4 4 4 4 */
/*:ref: zzekgfwd_ 14 4 4 4 4 4 */
/*:ref: zzekglnk_ 14 4 4 4 4 4 */
/*:ref: zzekslnk_ 14 4 4 4 4 4 */
/*:ref: zzekdps_ 14 4 4 4 4 4 */
/*:ref: zzekrp2n_ 4 3 4 4 4 */
/*:ref: dashlu_ 14 2 4 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: errfnm_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
 
extern int zzekdps_(integer *handle, integer *segdsc, integer *type__, integer *p);
/*:ref: zzekpgfr_ 14 3 4 4 4 */
/*:ref: failed_ 12 0 */
/*:ref: dasudi_ 14 4 4 4 4 4 */
/*:ref: zzektrls_ 4 3 4 4 4 */
/*:ref: zzektrdl_ 14 3 4 4 4 */
 
extern integer zzekecmp_(integer *hans, integer *sgdscs, integer *cldscs, integer *rows, integer *elts);
/*:ref: zzekrsi_ 14 8 4 4 4 4 4 4 12 12 */
/*:ref: dashlu_ 14 2 4 4 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errfnm_ 14 3 13 4 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzekrsd_ 14 8 4 4 4 4 4 7 12 12 */
/*:ref: zzekrsc_ 14 10 4 4 4 4 4 4 13 12 12 124 */
 
extern int zzekencd_(char *query, integer *eqryi, char *eqryc, doublereal *eqryd, logical *error, char *errmsg, integer *errptr, ftnlen query_len, ftnlen eqryc_len, ftnlen errmsg_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: zzekqini_ 14 6 4 4 4 13 7 124 */
/*:ref: zzekscan_ 14 17 13 4 4 4 4 4 4 4 7 13 4 4 12 13 124 124 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzekpars_ 14 19 13 4 4 4 4 4 7 13 4 4 4 13 7 12 13 124 124 124 124 */
/*:ref: zzeknres_ 14 9 13 4 13 12 13 4 124 124 124 */
/*:ref: zzektres_ 14 10 13 4 13 7 12 13 4 124 124 124 */
/*:ref: zzeksemc_ 14 9 13 4 13 12 13 4 124 124 124 */
 
extern int zzekerc1_(integer *handle, integer *segdsc, integer *coldsc, char *ckey, integer *recptr, logical *null, integer *prvidx, integer *prvptr, ftnlen ckey_len);
/*:ref: failed_ 12 0 */
/*:ref: zzektrsz_ 4 2 4 4 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzektrdp_ 14 4 4 4 4 4 */
/*:ref: zzekscmp_ 12 12 4 4 4 4 4 4 4 13 7 4 12 124 */
 
extern int zzekerd1_(integer *handle, integer *segdsc, integer *coldsc, doublereal *dkey, integer *recptr, logical *null, integer *prvidx, integer *prvptr);
/*:ref: failed_ 12 0 */
/*:ref: zzektrsz_ 4 2 4 4 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzektrdp_ 14 4 4 4 4 4 */
/*:ref: zzekscmp_ 12 12 4 4 4 4 4 4 4 13 7 4 12 124 */
 
extern int zzekeri1_(integer *handle, integer *segdsc, integer *coldsc, integer *ikey, integer *recptr, logical *null, integer *prvidx, integer *prvptr);
/*:ref: failed_ 12 0 */
/*:ref: zzektrsz_ 4 2 4 4 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzektrdp_ 14 4 4 4 4 4 */
/*:ref: zzekscmp_ 12 12 4 4 4 4 4 4 4 13 7 4 12 124 */
 
extern integer zzekesiz_(integer *handle, integer *segdsc, integer *coldsc, integer *recptr);
/*:ref: zzeksz04_ 4 4 4 4 4 4 */
/*:ref: zzeksz05_ 4 4 4 4 4 4 */
/*:ref: zzeksz06_ 4 4 4 4 4 4 */
/*:ref: dashlu_ 14 2 4 4 */
/*:ref: zzekcnam_ 14 4 4 4 13 124 */
/*:ref: zzekrp2n_ 4 3 4 4 4 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: errfnm_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int zzekff01_(integer *handle, integer *segno, integer *rcptrs);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: zzeksrd_ 14 3 4 4 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzekmloc_ 14 4 4 4 4 4 */
/*:ref: dasrdi_ 14 4 4 4 4 4 */
/*:ref: zzekpgpg_ 14 4 4 4 4 4 */
/*:ref: cleari_ 14 2 4 4 */
/*:ref: zzekpgwi_ 14 3 4 4 4 */
/*:ref: zzektrit_ 14 2 4 4 */
/*:ref: zzektr1s_ 14 4 4 4 4 4 */
/*:ref: dasudi_ 14 4 4 4 4 4 */
 
extern int zzekfrx_(integer *handle, integer *segdsc, integer *coldsc, integer *recptr, integer *pos);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: zzekrsc_ 14 10 4 4 4 4 4 4 13 12 12 124 */
/*:ref: zzekrsd_ 14 8 4 4 4 4 4 7 12 12 */
/*:ref: zzekrsi_ 14 8 4 4 4 4 4 4 12 12 */
/*:ref: dashlu_ 14 2 4 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errfnm_ 14 3 13 4 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzekrp2n_ 4 3 4 4 4 */
/*:ref: zzeklerc_ 14 9 4 4 4 13 4 12 4 4 124 */
/*:ref: zzeklerd_ 14 8 4 4 4 7 4 12 4 4 */
/*:ref: zzekleri_ 14 8 4 4 4 4 4 12 4 4 */
 
extern int zzekgcdp_(integer *handle, integer *segdsc, integer *coldsc, integer *recptr, integer *datptr);
/*:ref: zzekrp2n_ 4 3 4 4 4 */
/*:ref: dashlu_ 14 2 4 4 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: errfnm_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dasrdi_ 14 4 4 4 4 4 */
 
extern int zzekgei_(integer *handle, integer *addrss, integer *ival);
/*:ref: dasrdc_ 14 7 4 4 4 4 4 13 124 */
/*:ref: prtdec_ 14 3 13 4 124 */
 
extern int zzekgfwd_(integer *handle, integer *type__, integer *p, integer *fward);
/*:ref: zzekpgbs_ 14 3 4 4 4 */
/*:ref: failed_ 12 0 */
/*:ref: zzekgei_ 14 3 4 4 4 */
/*:ref: dasrdd_ 14 4 4 4 4 7 */
/*:ref: dasrdi_ 14 4 4 4 4 4 */
 
extern int zzekglnk_(integer *handle, integer *type__, integer *p, integer *nlinks);
/*:ref: zzekpgbs_ 14 3 4 4 4 */
/*:ref: failed_ 12 0 */
/*:ref: zzekgei_ 14 3 4 4 4 */
/*:ref: dasrdd_ 14 4 4 4 4 7 */
/*:ref: dasrdi_ 14 4 4 4 4 4 */
 
extern int zzekgrcp_(integer *handle, integer *recptr, integer *ptr);
/*:ref: zzekpgch_ 14 3 4 13 124 */
/*:ref: failed_ 12 0 */
/*:ref: dasrdi_ 14 4 4 4 4 4 */
 
extern int zzekgrs_(integer *handle, integer *recptr, integer *status);
/*:ref: dasrdi_ 14 4 4 4 4 4 */
 
extern int zzekif01_(integer *handle, integer *segno, integer *rcptrs);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: zzeksdsc_ 14 3 4 4 4 */
/*:ref: zzekstop_ 14 1 4 */
/*:ref: zzeksdec_ 14 1 4 */
/*:ref: zzekspsh_ 14 2 4 4 */
/*:ref: zzekaps_ 14 6 4 4 4 12 4 4 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int zzekif02_(integer *handle, integer *segno);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: zzekmloc_ 14 4 4 4 4 4 */
/*:ref: dasrdi_ 14 4 4 4 4 4 */
/*:ref: zzekaps_ 14 6 4 4 4 12 4 4 */
/*:ref: dasudi_ 14 4 4 4 4 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int zzekiic1_(integer *handle, integer *segdsc, integer *coldsc, char *ckey, integer *recptr, logical *null, ftnlen ckey_len);
/*:ref: failed_ 12 0 */
/*:ref: zzekcnam_ 14 4 4 4 13 124 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: zzeklerc_ 14 9 4 4 4 13 4 12 4 4 124 */
/*:ref: zzektrin_ 14 4 4 4 4 4 */
 
extern int zzekiid1_(integer *handle, integer *segdsc, integer *coldsc, doublereal *dkey, integer *recptr, logical *null);
/*:ref: failed_ 12 0 */
/*:ref: zzekcnam_ 14 4 4 4 13 124 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: zzeklerd_ 14 8 4 4 4 7 4 12 4 4 */
/*:ref: zzektrin_ 14 4 4 4 4 4 */
 
extern int zzekiii1_(integer *handle, integer *segdsc, integer *coldsc, integer *ikey, integer *recptr, logical *null);
/*:ref: failed_ 12 0 */
/*:ref: zzekcnam_ 14 4 4 4 13 124 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: zzekleri_ 14 8 4 4 4 4 4 12 4 4 */
/*:ref: zzektrin_ 14 4 4 4 4 4 */
 
extern integer zzekille_(integer *handle, integer *segdsc, integer *coldsc, integer *nrows, integer *dtype, char *cval, doublereal *dval, integer *ival, ftnlen cval_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzekllec_ 14 7 4 4 4 13 4 4 124 */
/*:ref: zzeklled_ 14 6 4 4 4 7 4 4 */
/*:ref: zzekllei_ 14 6 4 4 4 4 4 4 */
 
extern integer zzekillt_(integer *handle, integer *segdsc, integer *coldsc, integer *nrows, integer *dtype, char *cval, doublereal *dval, integer *ival, ftnlen cval_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzeklltc_ 14 7 4 4 4 13 4 4 124 */
/*:ref: zzeklltd_ 14 6 4 4 4 7 4 4 */
/*:ref: zzekllti_ 14 6 4 4 4 4 4 4 */
 
extern int zzekinqc_(char *value, integer *length, integer *lexbeg, integer *lexend, integer *eqryi, char *eqryc, integer *descr, ftnlen value_len, ftnlen eqryc_len);
/*:ref: zzekreqi_ 14 4 4 13 4 124 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: cleari_ 14 2 4 4 */
/*:ref: zzekweqi_ 14 4 13 4 4 124 */
 
extern int zzekinqn_(doublereal *value, integer *type__, integer *lexbeg, integer *lexend, integer *eqryi, doublereal *eqryd, integer *descr);
/*:ref: zzekreqi_ 14 4 4 13 4 124 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: cleari_ 14 2 4 4 */
/*:ref: zzekweqi_ 14 4 13 4 4 124 */
 
extern int zzekixdl_(integer *handle, integer *segdsc, integer *coldsc, integer *recptr);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: zzekfrx_ 14 5 4 4 4 4 4 */
/*:ref: zzektrdl_ 14 3 4 4 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzekrp2n_ 4 3 4 4 4 */
/*:ref: dashlu_ 14 2 4 4 */
/*:ref: errfnm_ 14 3 13 4 124 */
 
extern int zzekixlk_(integer *handle, integer *coldsc, integer *key, integer *recptr);
/*:ref: zzektrdp_ 14 4 4 4 4 4 */
/*:ref: dasrdi_ 14 4 4 4 4 4 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int zzekjoin_(integer *jbase1, integer *jbase2, integer *njcnst, logical *active, integer *cpidx1, integer *clidx1, integer *elts1, integer *ops, integer *cpidx2, integer *clidx2, integer *elts2, integer *sthan, integer *stsdsc, integer *stdtpt, integer *dtpool, integer *dtdscs, integer *jbase3, integer *nrows);
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzeksrd_ 14 3 4 4 4 */
/*:ref: zzekstop_ 14 1 4 */
/*:ref: zzekspsh_ 14 2 4 4 */
/*:ref: zzeksupd_ 14 3 4 4 4 */
/*:ref: zzekjprp_ 14 23 4 4 4 4 4 4 4 4 4 4 12 4 4 4 4 4 4 4 4 4 4 4 4 */
/*:ref: zzekjnxt_ 14 2 12 4 */
 
extern int zzekjsqz_(integer *jrsbas);
/*:ref: zzeksrd_ 14 3 4 4 4 */
/*:ref: failed_ 12 0 */
/*:ref: zzeksupd_ 14 3 4 4 4 */
 
extern int zzekjsrt_(integer *njrs, integer *ubases, integer *norder, integer *otabs, integer *ocols, integer *oelts, integer *senses, integer *sthan, integer *stsdsc, integer *stdtpt, integer *dtpool, integer *dtdscs, integer *ordbas);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzeksrd_ 14 3 4 4 4 */
/*:ref: zzekvset_ 14 2 4 4 */
/*:ref: zzekvcal_ 14 3 4 4 4 */
/*:ref: lnknxt_ 4 2 4 4 */
/*:ref: zzekrsc_ 14 10 4 4 4 4 4 4 13 12 12 124 */
/*:ref: dashlu_ 14 2 4 4 */
/*:ref: errfnm_ 14 3 13 4 124 */
/*:ref: zzekrsd_ 14 8 4 4 4 4 4 7 12 12 */
/*:ref: zzekrsi_ 14 8 4 4 4 4 4 4 12 12 */
/*:ref: zzekvcmp_ 12 15 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 */
/*:ref: swapi_ 14 2 4 4 */
/*:ref: zzekstop_ 14 1 4 */
/*:ref: zzekspsh_ 14 2 4 4 */
/*:ref: zzeksupd_ 14 3 4 4 4 */
 
extern int zzekjtst_(integer *segvec, integer *jbase1, integer *nt1, integer *rb1, integer *nr1, integer *jbase2, integer *nt2, integer *rb2, integer *nr2, integer *njcnst, logical *active, integer *cpidx1, integer *clidx1, integer *elts1, integer *ops, integer *cpidx2, integer *clidx2, integer *elts2, integer *sthan, integer *stsdsc, integer *stdtpt, integer *dtpool, integer *dtdscs, logical *found, integer *rowvec);
extern int zzekjprp_(integer *segvec, integer *jbase1, integer *nt1, integer *rb1, integer *nr1, integer *jbase2, integer *nt2, integer *rb2, integer *nr2, integer *njcnst, logical *active, integer *cpidx1, integer *clidx1, integer *elts1, integer *ops, integer *cpidx2, integer *clidx2, integer *elts2, integer *sthan, integer *stsdsc, integer *stdtpt, integer *dtpool, integer *dtdscs);
extern int zzekjnxt_(logical *found, integer *rowvec);
/*:ref: chkin_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: return_ 12 0 */
/*:ref: movei_ 14 3 4 4 4 */
/*:ref: lnknxt_ 4 2 4 4 */
/*:ref: zzekstop_ 14 1 4 */
/*:ref: zzekspsh_ 14 2 4 4 */
/*:ref: zzeksrd_ 14 3 4 4 4 */
/*:ref: zzeksupd_ 14 3 4 4 4 */
/*:ref: zzekjsrt_ 14 13 4 4 4 4 4 4 4 4 4 4 4 4 4 */
/*:ref: zzekrcmp_ 12 12 4 4 4 4 4 4 4 4 4 4 4 4 */
/*:ref: zzekvmch_ 12 13 4 12 4 4 4 4 4 4 4 4 4 4 4 */
 
extern int zzekkey_(integer *handle, integer *segdsc, integer *nrows, integer *ncnstr, integer *clidxs, integer *dsclst, integer *ops, integer *dtypes, char *chrbuf, integer *cbegs, integer *cends, doublereal *dvals, integer *ivals, logical *active, integer *key, integer *keydsc, integer *begidx, integer *endidx, logical *found, ftnlen chrbuf_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: ssizei_ 14 2 4 4 */
/*:ref: insrti_ 14 2 4 4 */
/*:ref: cardi_ 4 1 4 */
/*:ref: zzekillt_ 4 9 4 4 4 4 4 13 7 4 124 */
/*:ref: zzekille_ 4 9 4 4 4 4 4 13 7 4 124 */
/*:ref: failed_ 12 0 */
/*:ref: ordi_ 4 2 4 4 */
/*:ref: movei_ 14 3 4 4 4 */
 
extern int zzeklerc_(integer *handle, integer *segdsc, integer *coldsc, char *ckey, integer *recptr, logical *null, integer *prvidx, integer *prvptr, ftnlen ckey_len);
/*:ref: failed_ 12 0 */
/*:ref: zzekcnam_ 14 4 4 4 13 124 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: zzekerc1_ 14 9 4 4 4 13 4 12 4 4 124 */
 
extern int zzeklerd_(integer *handle, integer *segdsc, integer *coldsc, doublereal *dkey, integer *recptr, logical *null, integer *prvidx, integer *prvptr);
/*:ref: failed_ 12 0 */
/*:ref: zzekcnam_ 14 4 4 4 13 124 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: zzekerd1_ 14 8 4 4 4 7 4 12 4 4 */
 
extern int zzekleri_(integer *handle, integer *segdsc, integer *coldsc, integer *ikey, integer *recptr, logical *null, integer *prvidx, integer *prvptr);
/*:ref: failed_ 12 0 */
/*:ref: zzekcnam_ 14 4 4 4 13 124 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: zzekeri1_ 14 8 4 4 4 4 4 12 4 4 */
 
extern int zzekllec_(integer *handle, integer *segdsc, integer *coldsc, char *ckey, integer *prvloc, integer *prvptr, ftnlen ckey_len);
/*:ref: zzekcnam_ 14 4 4 4 13 124 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: zzekixlk_ 14 4 4 4 4 4 */
/*:ref: zzekscmp_ 12 12 4 4 4 4 4 4 4 13 7 4 12 124 */
 
extern int zzeklled_(integer *handle, integer *segdsc, integer *coldsc, doublereal *dkey, integer *prvloc, integer *prvptr);
/*:ref: zzekcnam_ 14 4 4 4 13 124 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: zzekixlk_ 14 4 4 4 4 4 */
/*:ref: zzekscmp_ 12 12 4 4 4 4 4 4 4 13 7 4 12 124 */
 
extern int zzekllei_(integer *handle, integer *segdsc, integer *coldsc, integer *ikey, integer *prvloc, integer *prvptr);
/*:ref: zzekcnam_ 14 4 4 4 13 124 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: zzekixlk_ 14 4 4 4 4 4 */
/*:ref: zzekscmp_ 12 12 4 4 4 4 4 4 4 13 7 4 12 124 */
 
extern int zzeklltc_(integer *handle, integer *segdsc, integer *coldsc, char *ckey, integer *prvloc, integer *prvptr, ftnlen ckey_len);
/*:ref: zzekcnam_ 14 4 4 4 13 124 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: zzekixlk_ 14 4 4 4 4 4 */
/*:ref: zzekscmp_ 12 12 4 4 4 4 4 4 4 13 7 4 12 124 */
 
extern int zzeklltd_(integer *handle, integer *segdsc, integer *coldsc, doublereal *dkey, integer *prvloc, integer *prvptr);
/*:ref: zzekcnam_ 14 4 4 4 13 124 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: zzekixlk_ 14 4 4 4 4 4 */
/*:ref: zzekscmp_ 12 12 4 4 4 4 4 4 4 13 7 4 12 124 */
 
extern int zzekllti_(integer *handle, integer *segdsc, integer *coldsc, integer *ikey, integer *prvloc, integer *prvptr);
/*:ref: zzekcnam_ 14 4 4 4 13 124 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: zzekixlk_ 14 4 4 4 4 4 */
/*:ref: zzekscmp_ 12 12 4 4 4 4 4 4 4 13 7 4 12 124 */
 
extern int zzekmloc_(integer *handle, integer *segno, integer *page, integer *base);
/*:ref: eknseg_ 4 1 4 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzektrbs_ 4 1 4 */
/*:ref: dasrdi_ 14 4 4 4 4 4 */
/*:ref: zzektrdp_ 14 4 4 4 4 4 */
 
extern int zzeknres_(char *query, integer *eqryi, char *eqryc, logical *error, char *errmsg, integer *errptr, ftnlen query_len, ftnlen eqryc_len, ftnlen errmsg_len);
/*:ref: zzekreqi_ 14 4 4 13 4 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzekqtab_ 14 8 4 13 4 13 13 124 124 124 */
/*:ref: repmc_ 14 8 13 13 13 13 124 124 124 124 */
/*:ref: isrchc_ 4 5 13 4 13 124 124 */
/*:ref: ekntab_ 14 1 4 */
/*:ref: ektnam_ 14 3 4 13 124 */
/*:ref: ekccnt_ 14 3 13 4 124 */
/*:ref: zzekcchk_ 14 15 13 4 13 4 13 13 4 12 13 4 124 124 124 124 124 */
/*:ref: zzekweqi_ 14 4 13 4 4 124 */
 
extern int zzeknrml_(char *query, integer *ntoken, integer *lxbegs, integer *lxends, integer *tokens, integer *values, doublereal *numvls, char *chrbuf, integer *chbegs, integer *chends, integer *eqryi, char *eqryc, doublereal *eqryd, logical *error, char *prserr, ftnlen query_len, ftnlen chrbuf_len, ftnlen eqryc_len, ftnlen prserr_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: repmi_ 14 7 13 13 4 13 124 124 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzektloc_ 14 7 4 4 4 4 4 4 12 */
/*:ref: lnkini_ 14 2 4 4 */
/*:ref: lnkan_ 14 2 4 4 */
/*:ref: lnkila_ 14 3 4 4 4 */
/*:ref: zzekinqn_ 14 7 7 4 4 4 4 7 4 */
/*:ref: zzekinqc_ 14 9 13 4 4 4 4 13 4 124 124 */
/*:ref: isrchi_ 4 3 4 4 4 */
/*:ref: repmc_ 14 8 13 13 13 13 124 124 124 124 */
/*:ref: lnkhl_ 4 2 4 4 */
/*:ref: lnkprv_ 4 2 4 4 */
/*:ref: lnknxt_ 4 2 4 4 */
/*:ref: lnkfsl_ 14 3 4 4 4 */
/*:ref: cleari_ 14 2 4 4 */
/*:ref: lnkilb_ 14 3 4 4 4 */
/*:ref: lnknfn_ 4 1 4 */
/*:ref: lnktl_ 4 2 4 4 */
/*:ref: ssizei_ 14 2 4 4 */
/*:ref: insrti_ 14 2 4 4 */
/*:ref: cardi_ 4 1 4 */
/*:ref: appndi_ 14 2 4 4 */
/*:ref: zzekweqi_ 14 4 13 4 4 124 */
 
extern int zzekordc_(char *cvals, logical *nullok, logical *nlflgs, integer *nvals, integer *iorder, ftnlen cvals_len);
/*:ref: swapi_ 14 2 4 4 */
 
extern int zzekordd_(doublereal *dvals, logical *nullok, logical *nlflgs, integer *nvals, integer *iorder);
/*:ref: swapi_ 14 2 4 4 */
 
extern int zzekordi_(integer *ivals, logical *nullok, logical *nlflgs, integer *nvals, integer *iorder);
/*:ref: swapi_ 14 2 4 4 */
 
extern int zzekpage_(integer *handle, integer *type__, integer *addrss, char *stat, integer *p, char *pagec, doublereal *paged, integer *pagei, integer *base, integer *value, ftnlen stat_len, ftnlen pagec_len);
extern int zzekpgin_(integer *handle);
extern int zzekpgan_(integer *handle, integer *type__, integer *p, integer *base);
extern int zzekpgal_(integer *handle, integer *type__, integer *p, integer *base);
extern int zzekpgfr_(integer *handle, integer *type__, integer *p);
extern int zzekpgrc_(integer *handle, integer *p, char *pagec, ftnlen pagec_len);
extern int zzekpgrd_(integer *handle, integer *p, doublereal *paged);
extern int zzekpgri_(integer *handle, integer *p, integer *pagei);
extern int zzekpgwc_(integer *handle, integer *p, char *pagec, ftnlen pagec_len);
extern int zzekpgwd_(integer *handle, integer *p, doublereal *paged);
extern int zzekpgwi_(integer *handle, integer *p, integer *pagei);
extern int zzekpgbs_(integer *type__, integer *p, integer *base);
extern int zzekpgpg_(integer *type__, integer *addrss, integer *p, integer *base);
extern int zzekpgst_(integer *handle, char *stat, integer *value, ftnlen stat_len);
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dassih_ 14 3 4 13 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: daslla_ 14 4 4 4 4 4 */
/*:ref: dashlu_ 14 2 4 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errfnm_ 14 3 13 4 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: fillc_ 14 5 13 4 13 124 124 */
/*:ref: filld_ 14 3 7 4 7 */
/*:ref: filli_ 14 3 4 4 4 */
/*:ref: dasadi_ 14 3 4 4 4 */
/*:ref: dasudi_ 14 4 4 4 4 4 */
/*:ref: zzekpgch_ 14 3 4 13 124 */
/*:ref: dasadc_ 14 6 4 4 4 4 13 124 */
/*:ref: dasrdi_ 14 4 4 4 4 4 */
/*:ref: dasadd_ 14 3 4 4 7 */
/*:ref: dasrdc_ 14 7 4 4 4 4 4 13 124 */
/*:ref: prtdec_ 14 3 13 4 124 */
/*:ref: dasrdd_ 14 4 4 4 4 7 */
/*:ref: prtenc_ 14 3 4 13 124 */
/*:ref: dasudc_ 14 7 4 4 4 4 4 13 124 */
/*:ref: dasudd_ 14 4 4 4 4 7 */
/*:ref: eqstr_ 12 4 13 13 124 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
 
extern int zzekpars_(char *query, integer *ntoken, integer *lxbegs, integer *lxends, integer *tokens, integer *values, doublereal *numvls, char *chrbuf, integer *chbegs, integer *chends, integer *eqryi, char *eqryc, doublereal *eqryd, logical *error, char *prserr, ftnlen query_len, ftnlen chrbuf_len, ftnlen eqryc_len, ftnlen prserr_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: zzekqini_ 14 6 4 4 4 13 7 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzektloc_ 14 7 4 4 4 4 4 4 12 */
/*:ref: zzekinqc_ 14 9 13 4 4 4 4 13 4 124 124 */
/*:ref: appndi_ 14 2 4 4 */
/*:ref: cleari_ 14 2 4 4 */
/*:ref: zzekweqi_ 14 4 13 4 4 124 */
/*:ref: movei_ 14 3 4 4 4 */
/*:ref: cardi_ 4 1 4 */
/*:ref: zzeknrml_ 14 19 13 4 4 4 4 4 7 13 4 4 4 13 7 12 13 124 124 124 124 */
/*:ref: repmi_ 14 7 13 13 4 13 124 124 124 */
/*:ref: repmc_ 14 8 13 13 13 13 124 124 124 124 */
 
extern int zzekpcol_(char *qcol, integer *eqryi, char *eqryc, char *table, char *alias, integer *tabidx, char *column, integer *colidx, logical *error, char *errmsg, ftnlen qcol_len, ftnlen eqryc_len, ftnlen table_len, ftnlen alias_len, ftnlen column_len, ftnlen errmsg_len);
/*:ref: zzekreqi_ 14 4 4 13 4 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzekscan_ 14 17 13 4 4 4 4 4 4 4 7 13 4 4 12 13 124 124 124 */
/*:ref: ucase_ 14 4 13 13 124 124 */
/*:ref: zzekqtab_ 14 8 4 13 4 13 13 124 124 124 */
/*:ref: isrchc_ 4 5 13 4 13 124 124 */
/*:ref: repmc_ 14 8 13 13 13 13 124 124 124 124 */
/*:ref: ekccnt_ 14 3 13 4 124 */
/*:ref: ekcii_ 14 6 13 4 13 4 124 124 */
 
extern int zzekpdec_(char *decl, integer *pardsc, ftnlen decl_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cleari_ 14 2 4 4 */
/*:ref: lparsm_ 14 8 13 13 4 4 13 124 124 124 */
/*:ref: ucase_ 14 4 13 13 124 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: eqstr_ 12 4 13 13 124 124 */
/*:ref: cmprss_ 14 7 13 4 13 13 124 124 124 */
/*:ref: nparsi_ 14 6 13 4 13 4 124 124 */
 
extern int zzekpgch_(integer *handle, char *access, ftnlen access_len);
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dassih_ 14 3 4 13 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dasrdi_ 14 4 4 4 4 4 */
/*:ref: dashlu_ 14 2 4 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errfnm_ 14 3 13 4 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: daslla_ 14 4 4 4 4 4 */
 
extern int zzekqcnj_(integer *eqryi, integer *n, integer *size);
/*:ref: zzekreqi_ 14 4 4 13 4 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
 
extern int zzekqcon_(integer *eqryi, char *eqryc, doublereal *eqryd, integer *n, integer *cnstyp, char *ltname, integer *ltidx, char *lcname, integer *lcidx, integer *opcode, char *rtname, integer *rtidx, char *rcname, integer *rcidx, integer *dtype, integer *cbeg, integer *cend, doublereal *dval, integer *ival, ftnlen eqryc_len, ftnlen ltname_len, ftnlen lcname_len, ftnlen rtname_len, ftnlen rcname_len);
/*:ref: zzekreqi_ 14 4 4 13 4 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
 
extern int zzekqini_(integer *isize, integer *dsize, integer *eqryi, char *eqryc, doublereal *eqryd, ftnlen eqryc_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: ssizei_ 14 2 4 4 */
/*:ref: cleard_ 14 2 4 7 */
/*:ref: appndi_ 14 2 4 4 */
/*:ref: cleari_ 14 2 4 4 */
 
extern int zzekqord_(integer *eqryi, char *eqryc, integer *n, char *table, integer *tabidx, char *column, integer *colidx, integer *sense, ftnlen eqryc_len, ftnlen table_len, ftnlen column_len);
/*:ref: zzekreqi_ 14 4 4 13 4 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
 
extern int zzekqsel_(integer *eqryi, char *eqryc, integer *n, integer *lxbeg, integer *lxend, char *table, integer *tabidx, char *column, integer *colidx, ftnlen eqryc_len, ftnlen table_len, ftnlen column_len);
/*:ref: zzekreqi_ 14 4 4 13 4 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
 
extern int zzekqtab_(integer *eqryi, char *eqryc, integer *n, char *table, char *alias, ftnlen eqryc_len, ftnlen table_len, ftnlen alias_len);
/*:ref: zzekreqi_ 14 4 4 13 4 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
 
extern int zzekrbck_(char *action, integer *handle, integer *segdsc, integer *coldsc, integer *recno, ftnlen action_len);
 
extern logical zzekrcmp_(integer *op, integer *ncols, integer *han1, integer *sgdsc1, integer *cdlst1, integer *row1, integer *elts1, integer *han2, integer *sgdsc2, integer *cdlst2, integer *row2, integer *elts2);
/*:ref: movei_ 14 3 4 4 4 */
/*:ref: zzekecmp_ 4 5 4 4 4 4 4 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int zzekrd01_(integer *handle, integer *segdsc, integer *coldsc, integer *recptr, integer *ival, logical *isnull);
/*:ref: zzekrp2n_ 4 3 4 4 4 */
/*:ref: dashlu_ 14 2 4 4 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: errfnm_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dasrdi_ 14 4 4 4 4 4 */
 
extern int zzekrd02_(integer *handle, integer *segdsc, integer *coldsc, integer *recptr, doublereal *dval, logical *isnull);
/*:ref: dashlu_ 14 2 4 4 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dasrdi_ 14 4 4 4 4 4 */
/*:ref: dasrdd_ 14 4 4 4 4 7 */
/*:ref: zzekrp2n_ 4 3 4 4 4 */
/*:ref: errfnm_ 14 3 13 4 124 */
 
extern int zzekrd03_(integer *handle, integer *segdsc, integer *coldsc, integer *recptr, integer *cvlen, char *cval, logical *isnull, ftnlen cval_len);
/*:ref: zzekrp2n_ 4 3 4 4 4 */
/*:ref: dashlu_ 14 2 4 4 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: errfnm_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dasrdi_ 14 4 4 4 4 4 */
/*:ref: zzekgei_ 14 3 4 4 4 */
/*:ref: zzekpgpg_ 14 4 4 4 4 4 */
/*:ref: dasrdc_ 14 7 4 4 4 4 4 13 124 */
/*:ref: zzekpgbs_ 14 3 4 4 4 */
/*:ref: zzekcnam_ 14 4 4 4 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
 
extern int zzekrd04_(integer *handle, integer *segdsc, integer *coldsc, integer *recptr, integer *beg, integer *end, integer *ivals, logical *isnull, logical *found);
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dasrdi_ 14 4 4 4 4 4 */
/*:ref: zzekpgpg_ 14 4 4 4 4 4 */
/*:ref: zzekpgbs_ 14 3 4 4 4 */
/*:ref: failed_ 12 0 */
/*:ref: zzekgfwd_ 14 4 4 4 4 4 */
/*:ref: zzekrp2n_ 4 3 4 4 4 */
/*:ref: dashlu_ 14 2 4 4 */
/*:ref: errfnm_ 14 3 13 4 124 */
 
extern int zzekrd05_(integer *handle, integer *segdsc, integer *coldsc, integer *recptr, integer *beg, integer *end, doublereal *dvals, logical *isnull, logical *found);
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dasrdi_ 14 4 4 4 4 4 */
/*:ref: dasrdd_ 14 4 4 4 4 7 */
/*:ref: zzekpgpg_ 14 4 4 4 4 4 */
/*:ref: zzekgfwd_ 14 4 4 4 4 4 */
/*:ref: zzekpgbs_ 14 3 4 4 4 */
/*:ref: failed_ 12 0 */
/*:ref: zzekrp2n_ 4 3 4 4 4 */
/*:ref: dashlu_ 14 2 4 4 */
/*:ref: errfnm_ 14 3 13 4 124 */
 
extern int zzekrd06_(integer *handle, integer *segdsc, integer *coldsc, integer *recptr, integer *beg, integer *end, char *cvals, logical *isnull, logical *found, ftnlen cvals_len);
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dashlu_ 14 2 4 4 */
/*:ref: zzekcnam_ 14 4 4 4 13 124 */
/*:ref: zzekrp2n_ 4 3 4 4 4 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: errfnm_ 14 3 13 4 124 */
/*:ref: dasrdi_ 14 4 4 4 4 4 */
/*:ref: zzekgei_ 14 3 4 4 4 */
/*:ref: zzekpgpg_ 14 4 4 4 4 4 */
/*:ref: zzekpgbs_ 14 3 4 4 4 */
/*:ref: failed_ 12 0 */
/*:ref: dasrdc_ 14 7 4 4 4 4 4 13 124 */
 
extern int zzekrd07_(integer *handle, integer *segdsc, integer *coldsc, integer *recptr, integer *ival, logical *isnull);
/*:ref: zzekrp2n_ 4 3 4 4 4 */
/*:ref: dashlu_ 14 2 4 4 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: errfnm_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dasrdi_ 14 4 4 4 4 4 */
/*:ref: dasrdc_ 14 7 4 4 4 4 4 13 124 */
 
extern int zzekrd08_(integer *handle, integer *segdsc, integer *coldsc, integer *recptr, doublereal *dval, logical *isnull);
/*:ref: zzekrp2n_ 4 3 4 4 4 */
/*:ref: dashlu_ 14 2 4 4 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dasrdi_ 14 4 4 4 4 4 */
/*:ref: dasrdc_ 14 7 4 4 4 4 4 13 124 */
/*:ref: dasrdd_ 14 4 4 4 4 7 */
 
extern int zzekrd09_(integer *handle, integer *segdsc, integer *coldsc, integer *recno, integer *cvlen, char *cval, logical *isnull, ftnlen cval_len);
/*:ref: dashlu_ 14 2 4 4 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: errfnm_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzekcnam_ 14 4 4 4 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: dasrdi_ 14 4 4 4 4 4 */
/*:ref: dasrdc_ 14 7 4 4 4 4 4 13 124 */
 
extern int zzekreqi_(integer *eqryi, char *name__, integer *value, ftnlen name_len);
/*:ref: ljust_ 14 4 13 13 124 124 */
/*:ref: ucase_ 14 4 13 13 124 124 */
/*:ref: isrchc_ 4 5 13 4 13 124 124 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern logical zzekrmch_(integer *ncnstr, logical *active, integer *handle, integer *segdsc, integer *cdscrs, integer *row, integer *elts, integer *ops, integer *vtypes, char *chrbuf, integer *cbegs, integer *cends, doublereal *dvals, integer *ivals, ftnlen chrbuf_len);
/*:ref: zzekscmp_ 12 12 4 4 4 4 4 4 4 13 7 4 12 124 */
 
extern integer zzekrp2n_(integer *handle, integer *segno, integer *recptr);
/*:ref: zzeksdsc_ 14 3 4 4 4 */
/*:ref: failed_ 12 0 */
/*:ref: zzektrls_ 4 3 4 4 4 */
/*:ref: dashlu_ 14 2 4 4 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: errfnm_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int zzekrplk_(integer *handle, integer *segdsc, integer *n, integer *recptr);
/*:ref: zzektrdp_ 14 4 4 4 4 4 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int zzekrsc_(integer *handle, integer *segdsc, integer *coldsc, integer *recptr, integer *eltidx, integer *cvlen, char *cval, logical *isnull, logical *found, ftnlen cval_len);
/*:ref: zzekcnam_ 14 4 4 4 13 124 */
/*:ref: zzekrp2n_ 4 3 4 4 4 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dashlu_ 14 2 4 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: errfnm_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzekrd03_ 14 8 4 4 4 4 4 13 12 124 */
/*:ref: zzekrd06_ 14 10 4 4 4 4 4 4 13 12 12 124 */
/*:ref: zzekrd09_ 14 8 4 4 4 4 4 13 12 124 */
 
extern int zzekrsd_(integer *handle, integer *segdsc, integer *coldsc, integer *recptr, integer *eltidx, doublereal *dval, logical *isnull, logical *found);
/*:ref: zzekcnam_ 14 4 4 4 13 124 */
/*:ref: dashlu_ 14 2 4 4 */
/*:ref: zzekrp2n_ 4 3 4 4 4 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: errfnm_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzekrd02_ 14 6 4 4 4 4 7 12 */
/*:ref: zzekrd05_ 14 9 4 4 4 4 4 4 7 12 12 */
/*:ref: zzekrd08_ 14 6 4 4 4 4 7 12 */
 
extern int zzekrsi_(integer *handle, integer *segdsc, integer *coldsc, integer *recptr, integer *eltidx, integer *ival, logical *isnull, logical *found);
/*:ref: zzekcnam_ 14 4 4 4 13 124 */
/*:ref: dashlu_ 14 2 4 4 */
/*:ref: zzekrp2n_ 4 3 4 4 4 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: errfnm_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzekrd01_ 14 6 4 4 4 4 4 12 */
/*:ref: zzekrd04_ 14 9 4 4 4 4 4 4 4 12 12 */
/*:ref: zzekrd07_ 14 6 4 4 4 4 4 12 */
 
extern int zzeksca_(integer *n, integer *beg, integer *end, integer *idata, integer *top);
extern int zzekstop_(integer *top);
extern int zzekspsh_(integer *n, integer *idata);
extern int zzekspop_(integer *n, integer *idata);
extern int zzeksdec_(integer *n);
extern int zzeksupd_(integer *beg, integer *end, integer *idata);
extern int zzeksrd_(integer *beg, integer *end, integer *idata);
extern int zzekscln_(void);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dasops_ 14 1 4 */
/*:ref: failed_ 12 0 */
/*:ref: daslla_ 14 4 4 4 4 4 */
/*:ref: dasudi_ 14 4 4 4 4 4 */
/*:ref: dasadi_ 14 3 4 4 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: dasrdi_ 14 4 4 4 4 4 */
/*:ref: cleari_ 14 2 4 4 */
/*:ref: daswbr_ 14 1 4 */
/*:ref: dasllc_ 14 1 4 */
 
extern int zzekscan_(char *query, integer *maxntk, integer *maxnum, integer *ntoken, integer *tokens, integer *lxbegs, integer *lxends, integer *values, doublereal *numvls, char *chrbuf, integer *chbegs, integer *chends, logical *scnerr, char *errmsg, ftnlen query_len, ftnlen chrbuf_len, ftnlen errmsg_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: ssizei_ 14 2 4 4 */
/*:ref: lxcsid_ 14 5 13 13 4 124 124 */
/*:ref: rtrim_ 4 2 13 124 */
/*:ref: repmi_ 14 7 13 13 4 13 124 124 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: lxqstr_ 14 7 13 13 4 4 4 124 124 */
/*:ref: parsqs_ 14 11 13 13 13 4 12 13 4 124 124 124 124 */
/*:ref: prefix_ 14 5 13 4 13 124 124 */
/*:ref: repmc_ 14 8 13 13 13 13 124 124 124 124 */
/*:ref: lx4num_ 14 5 13 4 4 4 124 */
/*:ref: nparsd_ 14 6 13 7 13 4 124 124 */
/*:ref: beint_ 12 2 13 124 */
/*:ref: lxidnt_ 14 6 4 13 4 4 4 124 */
/*:ref: ucase_ 14 4 13 13 124 124 */
/*:ref: bsrchc_ 4 5 13 4 13 124 124 */
/*:ref: isrchc_ 4 5 13 4 13 124 124 */
/*:ref: frstpc_ 4 2 13 124 */
/*:ref: suffix_ 14 5 13 4 13 124 124 */
 
extern int zzekscdp_(integer *handle, integer *segdsc, integer *coldsc, integer *recptr, integer *datptr);
/*:ref: zzekpgch_ 14 3 4 13 124 */
/*:ref: failed_ 12 0 */
/*:ref: zzekrp2n_ 4 3 4 4 4 */
/*:ref: dashlu_ 14 2 4 4 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: errfnm_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dasudi_ 14 4 4 4 4 4 */
 
extern logical zzekscmp_(integer *op, integer *handle, integer *segdsc, integer *coldsc, integer *row, integer *eltidx, integer *dtype, char *cval, doublereal *dval, integer *ival, logical *null, ftnlen cval_len);
/*:ref: zzekrsc_ 14 10 4 4 4 4 4 4 13 12 12 124 */
/*:ref: zzekrsd_ 14 8 4 4 4 4 4 7 12 12 */
/*:ref: zzekrsi_ 14 8 4 4 4 4 4 4 12 12 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dashlu_ 14 2 4 4 */
/*:ref: errfnm_ 14 3 13 4 124 */
/*:ref: matchi_ 12 8 13 13 13 13 124 124 124 124 */
 
extern int zzeksdsc_(integer *handle, integer *segno, integer *segdsc);
/*:ref: zzekmloc_ 14 4 4 4 4 4 */
/*:ref: dasrdi_ 14 4 4 4 4 4 */
 
extern int zzeksei_(integer *handle, integer *addrss, integer *ival);
/*:ref: prtenc_ 14 3 4 13 124 */
/*:ref: dasudc_ 14 7 4 4 4 4 4 13 124 */
 
extern int zzeksemc_(char *query, integer *eqryi, char *eqryc, logical *error, char *errmsg, integer *errptr, ftnlen query_len, ftnlen eqryc_len, ftnlen errmsg_len);
/*:ref: zzekreqi_ 14 4 4 13 4 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzekqtab_ 14 8 4 13 4 13 13 124 124 124 */
/*:ref: ekcii_ 14 6 13 4 13 4 124 124 */
/*:ref: repmc_ 14 8 13 13 13 13 124 124 124 124 */
/*:ref: repmi_ 14 7 13 13 4 13 124 124 124 */
/*:ref: zzekweqi_ 14 4 13 4 4 124 */
 
extern int zzeksfwd_(integer *handle, integer *type__, integer *p, integer *fward);
/*:ref: zzekpgbs_ 14 3 4 4 4 */
/*:ref: failed_ 12 0 */
/*:ref: zzeksei_ 14 3 4 4 4 */
/*:ref: dasudd_ 14 4 4 4 4 7 */
/*:ref: dasudi_ 14 4 4 4 4 4 */
 
extern int zzeksinf_(integer *handle, integer *segno, char *tabnam, integer *segdsc, char *cnames, integer *cdscrs, ftnlen tabnam_len, ftnlen cnames_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: zzekpgch_ 14 3 4 13 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: eknseg_ 4 1 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: zzekmloc_ 14 4 4 4 4 4 */
/*:ref: dasrdi_ 14 4 4 4 4 4 */
/*:ref: dasrdc_ 14 7 4 4 4 4 4 13 124 */
 
extern int zzekslnk_(integer *handle, integer *type__, integer *p, integer *nlinks);
/*:ref: zzekpgbs_ 14 3 4 4 4 */
/*:ref: failed_ 12 0 */
/*:ref: zzeksei_ 14 3 4 4 4 */
/*:ref: dasudd_ 14 4 4 4 4 7 */
/*:ref: dasudi_ 14 4 4 4 4 4 */
 
extern int zzeksrcp_(integer *handle, integer *recptr, integer *recno);
/*:ref: zzekpgch_ 14 3 4 13 124 */
/*:ref: failed_ 12 0 */
/*:ref: dasudi_ 14 4 4 4 4 4 */
 
extern int zzeksrs_(integer *handle, integer *recptr, integer *status);
/*:ref: zzekpgch_ 14 3 4 13 124 */
/*:ref: failed_ 12 0 */
/*:ref: dasudi_ 14 4 4 4 4 4 */
 
extern integer zzekstyp_(integer *ncols, integer *cdscrs);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern integer zzeksz04_(integer *handle, integer *segdsc, integer *coldsc, integer *recptr);
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dasrdi_ 14 4 4 4 4 4 */
 
extern integer zzeksz05_(integer *handle, integer *segdsc, integer *coldsc, integer *recptr);
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dasrdi_ 14 4 4 4 4 4 */
/*:ref: dasrdd_ 14 4 4 4 4 7 */
 
extern integer zzeksz06_(integer *handle, integer *segdsc, integer *coldsc, integer *recptr);
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dasrdi_ 14 4 4 4 4 4 */
/*:ref: zzekgei_ 14 3 4 4 4 */
 
extern int zzektcnv_(char *timstr, doublereal *et, logical *error, char *errmsg, ftnlen timstr_len, ftnlen errmsg_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cmprss_ 14 7 13 4 13 13 124 124 124 */
/*:ref: ljust_ 14 4 13 13 124 124 */
/*:ref: ucase_ 14 4 13 13 124 124 */
/*:ref: posr_ 4 5 13 13 4 124 124 */
/*:ref: rtrim_ 4 2 13 124 */
/*:ref: scn2id_ 14 4 13 4 12 124 */
/*:ref: repmc_ 14 8 13 13 13 13 124 124 124 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: scpars_ 14 7 4 13 12 13 7 124 124 */
/*:ref: failed_ 12 0 */
/*:ref: suffix_ 14 5 13 4 13 124 124 */
/*:ref: sct2e_ 14 3 4 7 7 */
/*:ref: tpartv_ 14 15 13 7 4 13 13 12 12 12 13 13 124 124 124 124 124 */
/*:ref: str2et_ 14 3 13 7 124 */
 
extern int zzektloc_(integer *tokid, integer *kwcode, integer *ntoken, integer *tokens, integer *values, integer *loc, logical *found);
 
extern int zzektr13_(integer *handle, integer *tree);
/*:ref: zzekpgri_ 14 3 4 4 4 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzekpgal_ 14 4 4 4 4 4 */
/*:ref: cleari_ 14 2 4 4 */
/*:ref: movei_ 14 3 4 4 4 */
/*:ref: zzekpgwi_ 14 3 4 4 4 */
 
extern int zzektr1s_(integer *handle, integer *tree, integer *size, integer *values);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: zzektrsz_ 4 2 4 4 */
/*:ref: dashlu_ 14 2 4 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: errfnm_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzekpgri_ 14 3 4 4 4 */
/*:ref: zzekpgal_ 14 4 4 4 4 4 */
/*:ref: zzekpgwi_ 14 3 4 4 4 */
/*:ref: cleari_ 14 2 4 4 */
/*:ref: zzektrbs_ 4 1 4 */
/*:ref: dasudi_ 14 4 4 4 4 4 */
 
extern int zzektr23_(integer *handle, integer *tree, integer *left, integer *right, integer *parent, integer *pkidx, logical *overfl);
/*:ref: zzekpgri_ 14 3 4 4 4 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzekpgal_ 14 4 4 4 4 4 */
/*:ref: cleari_ 14 2 4 4 */
/*:ref: movei_ 14 3 4 4 4 */
/*:ref: zzektrbs_ 4 1 4 */
/*:ref: dasrdi_ 14 4 4 4 4 4 */
/*:ref: dasudi_ 14 4 4 4 4 4 */
/*:ref: zzekpgwi_ 14 3 4 4 4 */
 
extern int zzektr31_(integer *handle, integer *tree);
/*:ref: zzekpgri_ 14 3 4 4 4 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: movei_ 14 3 4 4 4 */
/*:ref: zzekpgwi_ 14 3 4 4 4 */
/*:ref: zzekpgfr_ 14 3 4 4 4 */
 
extern int zzektr32_(integer *handle, integer *tree, integer *left, integer *middle, integer *right, integer *parent, integer *lpkidx, logical *undrfl);
/*:ref: zzekpgri_ 14 3 4 4 4 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: movei_ 14 3 4 4 4 */
/*:ref: zzektrbs_ 4 1 4 */
/*:ref: dasrdi_ 14 4 4 4 4 4 */
/*:ref: dasudi_ 14 4 4 4 4 4 */
/*:ref: zzekpgwi_ 14 3 4 4 4 */
/*:ref: zzekpgfr_ 14 3 4 4 4 */
 
extern int zzektrap_(integer *handle, integer *tree, integer *value, integer *key);
/*:ref: zzektrsz_ 4 2 4 4 */
/*:ref: zzektrin_ 14 4 4 4 4 4 */
 
extern int zzektrbn_(integer *handle, integer *tree, integer *left, integer *right, integer *parent, integer *pkidx);
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzektrnk_ 4 3 4 4 4 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: zzektrrk_ 14 7 4 4 4 4 4 4 4 */
 
extern integer zzektrbs_(integer *node);
/*:ref: zzekpgbs_ 14 3 4 4 4 */
 
extern int zzektrdl_(integer *handle, integer *tree, integer *key);
/*:ref: zzektrud_ 14 5 4 4 4 4 12 */
/*:ref: failed_ 12 0 */
/*:ref: zzektrlk_ 14 8 4 4 4 4 4 4 4 4 */
/*:ref: zzektrsb_ 14 7 4 4 4 4 4 4 4 */
/*:ref: zzektrnk_ 4 3 4 4 4 */
/*:ref: zzektrpi_ 14 12 4 4 4 4 4 4 4 4 4 4 4 4 */
/*:ref: zzektrrk_ 14 7 4 4 4 4 4 4 4 */
/*:ref: zzektrbn_ 14 6 4 4 4 4 4 4 */
/*:ref: zzektrki_ 14 5 4 4 4 4 4 */
/*:ref: zzektr32_ 14 8 4 4 4 4 4 4 4 12 */
/*:ref: zzektr31_ 14 2 4 4 */
 
extern int zzektrdp_(integer *handle, integer *tree, integer *key, integer *ptr);
/*:ref: zzektrlk_ 14 8 4 4 4 4 4 4 4 4 */
 
extern int zzektres_(char *query, integer *eqryi, char *eqryc, doublereal *eqryd, logical *error, char *errmsg, integer *errptr, ftnlen query_len, ftnlen eqryc_len, ftnlen errmsg_len);
/*:ref: zzekreqi_ 14 4 4 13 4 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzekqtab_ 14 8 4 13 4 13 13 124 124 124 */
/*:ref: ekcii_ 14 6 13 4 13 4 124 124 */
/*:ref: zzektcnv_ 14 6 13 7 12 13 124 124 */
/*:ref: zzekinqn_ 14 7 7 4 4 4 4 7 4 */
/*:ref: movei_ 14 3 4 4 4 */
/*:ref: zzekweqi_ 14 4 13 4 4 124 */
 
extern int zzektrfr_(integer *handle, integer *tree);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: zzekpgri_ 14 3 4 4 4 */
/*:ref: dashlu_ 14 2 4 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: errfnm_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzekpgfr_ 14 3 4 4 4 */
 
extern int zzektrin_(integer *handle, integer *tree, integer *key, integer *value);
/*:ref: zzektrui_ 14 5 4 4 4 4 12 */
/*:ref: failed_ 12 0 */
/*:ref: zzektrlk_ 14 8 4 4 4 4 4 4 4 4 */
/*:ref: zzektrpi_ 14 12 4 4 4 4 4 4 4 4 4 4 4 4 */
/*:ref: zzektrnk_ 4 3 4 4 4 */
/*:ref: zzektrbn_ 14 6 4 4 4 4 4 4 */
/*:ref: zzektrki_ 14 5 4 4 4 4 4 */
/*:ref: zzektr23_ 14 7 4 4 4 4 4 4 12 */
/*:ref: zzektr13_ 14 2 4 4 */
 
extern int zzektrit_(integer *handle, integer *tree);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: zzekpgal_ 14 4 4 4 4 4 */
/*:ref: cleari_ 14 2 4 4 */
/*:ref: zzekpgwi_ 14 3 4 4 4 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int zzektrki_(integer *handle, integer *tree, integer *nodkey, integer *n, integer *key);
/*:ref: zzektrlk_ 14 8 4 4 4 4 4 4 4 4 */
/*:ref: zzektrnk_ 4 3 4 4 4 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzektrbs_ 4 1 4 */
/*:ref: dasrdi_ 14 4 4 4 4 4 */
 
extern int zzektrlk_(integer *handle, integer *tree, integer *key, integer *idx, integer *node, integer *noffst, integer *level, integer *value);
/*:ref: dasham_ 14 3 4 13 124 */
/*:ref: zzekpgri_ 14 3 4 4 4 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dashlu_ 14 2 4 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: errfnm_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: lstlei_ 4 3 4 4 4 */
 
extern integer zzektrls_(integer *handle, integer *tree, integer *ival);
/*:ref: zzektrsz_ 4 2 4 4 */
/*:ref: zzektrdp_ 14 4 4 4 4 4 */
 
extern integer zzektrnk_(integer *handle, integer *tree, integer *node);
/*:ref: zzektrbs_ 4 1 4 */
/*:ref: dasrdi_ 14 4 4 4 4 4 */
 
extern int zzektrpi_(integer *handle, integer *tree, integer *key, integer *parent, integer *pkey, integer *poffst, integer *lpidx, integer *lpkey, integer *lsib, integer *rpidx, integer *rpkey, integer *rsib);
/*:ref: zzekpgri_ 14 3 4 4 4 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dashlu_ 14 2 4 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: errfnm_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: lstlei_ 4 3 4 4 4 */
 
extern int zzektrrk_(integer *handle, integer *tree, integer *left, integer *right, integer *parent, integer *pkidx, integer *nrot);
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzekpgri_ 14 3 4 4 4 */
/*:ref: failed_ 12 0 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: movei_ 14 3 4 4 4 */
/*:ref: zzekpgwi_ 14 3 4 4 4 */
 
extern int zzektrsb_(integer *handle, integer *tree, integer *key, integer *lsib, integer *lkey, integer *rsib, integer *rkey);
/*:ref: zzektrpi_ 14 12 4 4 4 4 4 4 4 4 4 4 4 4 */
/*:ref: failed_ 12 0 */
/*:ref: zzektrbs_ 4 1 4 */
/*:ref: dasrdi_ 14 4 4 4 4 4 */
 
extern integer zzektrsz_(integer *handle, integer *tree);
/*:ref: zzektrbs_ 4 1 4 */
/*:ref: dasrdi_ 14 4 4 4 4 4 */
 
extern int zzektrud_(integer *handle, integer *tree, integer *key, integer *trgkey, logical *undrfl);
/*:ref: zzekpgri_ 14 3 4 4 4 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dashlu_ 14 2 4 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: errfnm_ 14 3 13 4 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzekpgwi_ 14 3 4 4 4 */
/*:ref: zzektrlk_ 14 8 4 4 4 4 4 4 4 4 */
/*:ref: failed_ 12 0 */
/*:ref: zzektrpi_ 14 12 4 4 4 4 4 4 4 4 4 4 4 4 */
/*:ref: sigerr_ 14 2 13 124 */
 
extern int zzektrui_(integer *handle, integer *tree, integer *key, integer *value, logical *overfl);
/*:ref: zzekpgri_ 14 3 4 4 4 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: dashlu_ 14 2 4 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: errfnm_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzekpgwi_ 14 3 4 4 4 */
/*:ref: zzektrlk_ 14 8 4 4 4 4 4 4 4 4 */
/*:ref: failed_ 12 0 */
/*:ref: zzektrpi_ 14 12 4 4 4 4 4 4 4 4 4 4 4 4 */
 
extern int zzekue01_(integer *handle, integer *segdsc, integer *coldsc, integer *recptr, integer *ival, logical *isnull);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: zzekpgch_ 14 3 4 13 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dasrdi_ 14 4 4 4 4 4 */
/*:ref: zzekixdl_ 14 4 4 4 4 4 */
/*:ref: zzekiii1_ 14 6 4 4 4 4 4 12 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: zzekpgpg_ 14 4 4 4 4 4 */
/*:ref: zzekglnk_ 14 4 4 4 4 4 */
/*:ref: zzekslnk_ 14 4 4 4 4 4 */
/*:ref: dasudi_ 14 4 4 4 4 4 */
/*:ref: zzekad01_ 14 6 4 4 4 4 4 12 */
/*:ref: zzekrp2n_ 4 3 4 4 4 */
/*:ref: dashlu_ 14 2 4 4 */
/*:ref: errfnm_ 14 3 13 4 124 */
 
extern int zzekue02_(integer *handle, integer *segdsc, integer *coldsc, integer *recptr, doublereal *dval, logical *isnull);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: zzekpgch_ 14 3 4 13 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dasrdi_ 14 4 4 4 4 4 */
/*:ref: zzekixdl_ 14 4 4 4 4 4 */
/*:ref: zzekiid1_ 14 6 4 4 4 7 4 12 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: zzekpgpg_ 14 4 4 4 4 4 */
/*:ref: zzekglnk_ 14 4 4 4 4 4 */
/*:ref: zzekslnk_ 14 4 4 4 4 4 */
/*:ref: dasudi_ 14 4 4 4 4 4 */
/*:ref: dasudd_ 14 4 4 4 4 7 */
/*:ref: zzekad02_ 14 6 4 4 4 4 7 12 */
/*:ref: zzekrp2n_ 4 3 4 4 4 */
/*:ref: dashlu_ 14 2 4 4 */
/*:ref: errfnm_ 14 3 13 4 124 */
 
extern int zzekue03_(integer *handle, integer *segdsc, integer *coldsc, integer *recptr, char *cval, logical *isnull, ftnlen cval_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: zzekde03_ 14 4 4 4 4 4 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzekad03_ 14 7 4 4 4 4 13 12 124 */
 
extern int zzekue04_(integer *handle, integer *segdsc, integer *coldsc, integer *recptr, integer *nvals, integer *ivals, logical *isnull);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: zzekde04_ 14 4 4 4 4 4 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzekad04_ 14 7 4 4 4 4 4 4 12 */
 
extern int zzekue05_(integer *handle, integer *segdsc, integer *coldsc, integer *recptr, integer *nvals, doublereal *dvals, logical *isnull);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: zzekde05_ 14 4 4 4 4 4 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzekad05_ 14 7 4 4 4 4 4 7 12 */
 
extern int zzekue06_(integer *handle, integer *segdsc, integer *coldsc, integer *recptr, integer *nvals, char *cvals, logical *isnull, ftnlen cvals_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: zzekde06_ 14 4 4 4 4 4 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzekad06_ 14 8 4 4 4 4 4 13 12 124 */
 
extern int zzekvadr_(integer *njrs, integer *bases, integer *rwvidx, integer *rwvbas, integer *sgvbas);
extern int zzekvset_(integer *njrs, integer *bases);
extern int zzekvcal_(integer *rwvidx, integer *rwvbas, integer *sgvbas);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: zzekstop_ 14 1 4 */
/*:ref: zzeksrd_ 14 3 4 4 4 */
/*:ref: cleari_ 14 2 4 4 */
/*:ref: lstlei_ 4 3 4 4 4 */
 
extern logical zzekvcmp_(integer *op, integer *ncols, integer *tabs, integer *cols, integer *elts, integer *senses, integer *sthan, integer *stsdsc, integer *stdtpt, integer *dtpool, integer *dtdscs, integer *sgvec1, integer *rwvec1, integer *sgvec2, integer *rwvec2);
/*:ref: lnknxt_ 4 2 4 4 */
/*:ref: movei_ 14 3 4 4 4 */
/*:ref: zzekecmp_ 4 5 4 4 4 4 4 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern logical zzekvmch_(integer *ncnstr, logical *active, integer *lhans, integer *lsdscs, integer *lcdscs, integer *lrows, integer *lelts, integer *ops, integer *rhans, integer *rsdscs, integer *rcdscs, integer *rrows, integer *relts);
/*:ref: movei_ 14 3 4 4 4 */
/*:ref: zzekecmp_ 4 5 4 4 4 4 4 */
/*:ref: zzekrsc_ 14 10 4 4 4 4 4 4 13 12 12 124 */
/*:ref: dashlu_ 14 2 4 4 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errfnm_ 14 3 13 4 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: matchi_ 12 8 13 13 13 13 124 124 124 124 */
 
extern int zzekweed_(integer *njrs, integer *bases, integer *nrows);
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzekvset_ 14 2 4 4 */
/*:ref: zzeksrd_ 14 3 4 4 4 */
/*:ref: sameai_ 12 3 4 4 4 */
/*:ref: zzeksupd_ 14 3 4 4 4 */
/*:ref: zzekjsqz_ 14 1 4 */
 
extern int zzekweqi_(char *name__, integer *value, integer *eqryi, ftnlen name_len);
/*:ref: ljust_ 14 4 13 13 124 124 */
/*:ref: ucase_ 14 4 13 13 124 124 */
/*:ref: isrchc_ 4 5 13 4 13 124 124 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int zzekwpac_(integer *handle, integer *segdsc, integer *nvals, integer *l, char *cvals, integer *p, integer *base, ftnlen cvals_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: zzekacps_ 14 6 4 4 4 4 4 4 */
/*:ref: zzekpgwc_ 14 4 4 4 13 124 */
/*:ref: zzekslnk_ 14 4 4 4 4 4 */
 
extern int zzekwpai_(integer *handle, integer *segdsc, integer *nvals, integer *ivals, integer *p, integer *base);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: zzekacps_ 14 6 4 4 4 4 4 4 */
/*:ref: cleari_ 14 2 4 4 */
/*:ref: zzekpgwi_ 14 3 4 4 4 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int zzekwpal_(integer *handle, integer *segdsc, integer *nvals, logical *lvals, integer *p, integer *base);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: zzekacps_ 14 6 4 4 4 4 4 4 */
/*:ref: zzekpgwc_ 14 4 4 4 13 124 */
/*:ref: zzekslnk_ 14 4 4 4 4 4 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int zzelvupy_(doublereal *ellips, doublereal *vertex, doublereal *axis, integer *n, doublereal *bounds, logical *found);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: el2cgv_ 14 4 7 7 7 7 */
/*:ref: saelgv_ 14 4 7 7 7 7 */
/*:ref: vzero_ 12 1 7 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: vnorm_ 7 1 7 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: cgv2el_ 14 4 7 7 7 7 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: vsub_ 14 3 7 7 7 */
/*:ref: vsep_ 7 2 7 7 */
/*:ref: pi_ 7 0 */
/*:ref: ucrss_ 14 3 7 7 7 */
/*:ref: repmot_ 14 9 13 13 4 13 13 124 124 124 124 */
/*:ref: vdist_ 7 2 7 7 */
/*:ref: vdot_ 7 2 7 7 */
/*:ref: nvp2pl_ 14 3 7 7 7 */
/*:ref: inrypl_ 14 5 7 7 7 4 7 */
/*:ref: zzwind_ 4 4 7 4 7 7 */
/*:ref: psv2pl_ 14 4 7 7 7 7 */
/*:ref: inelpl_ 14 5 7 7 4 7 7 */
/*:ref: vhat_ 14 2 7 7 */
/*:ref: vlcom_ 14 5 7 7 7 7 7 */
 
extern int zzenut80_(doublereal *et, doublereal *nutxf);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: zzwahr_ 14 2 7 7 */
/*:ref: zzmobliq_ 14 3 7 7 7 */
/*:ref: eul2xf_ 14 5 7 4 4 4 7 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int zzeprc76_(doublereal *et, doublereal *precxf);
/*:ref: jyear_ 7 0 */
/*:ref: rpd_ 7 0 */
/*:ref: eul2xf_ 14 5 7 4 4 4 7 */
 
extern int zzeprcss_(doublereal *et, doublereal *precm);
/*:ref: jyear_ 7 0 */
/*:ref: rpd_ 7 0 */
/*:ref: eul2m_ 14 7 7 7 7 4 4 4 7 */
 
extern int zzfdat_(integer *ncount, char *name__, integer *idcode, integer *center, integer *type__, integer *typid, integer *norder, integer *corder, integer *centrd, ftnlen name_len);
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: irfnam_ 14 3 4 13 124 */
/*:ref: orderc_ 14 4 13 4 4 124 */
/*:ref: orderi_ 14 3 4 4 4 */
 
extern int zzfrmch0_(integer *frame1, integer *frame2, doublereal *et, doublereal *xform);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: frinfo_ 14 5 4 4 4 4 12 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: zzfrmgt0_ 14 5 4 7 7 4 12 */
/*:ref: zzmsxf_ 14 3 7 4 7 */
/*:ref: isrchi_ 4 3 4 4 4 */
/*:ref: frmnam_ 14 3 4 13 124 */
/*:ref: rtrim_ 4 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: invstm_ 14 2 7 7 */
 
extern int zzfrmch1_(integer *frame1, integer *frame2, doublereal *et, doublereal *xform);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: frinfo_ 14 5 4 4 4 4 12 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: zzfrmgt1_ 14 5 4 7 7 4 12 */
/*:ref: zzmsxf_ 14 3 7 4 7 */
/*:ref: isrchi_ 4 3 4 4 4 */
/*:ref: frmnam_ 14 3 4 13 124 */
/*:ref: rtrim_ 4 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: invstm_ 14 2 7 7 */
 
extern int zzfrmgt0_(integer *infrm, doublereal *et, doublereal *xform, integer *outfrm, logical *found);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: frinfo_ 14 5 4 4 4 4 12 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: tisbod_ 14 5 13 4 7 7 124 */
/*:ref: invstm_ 14 2 7 7 */
/*:ref: namfrm_ 14 3 13 4 124 */
/*:ref: irfrot_ 14 3 4 4 7 */
/*:ref: ckfxfm_ 14 5 4 7 7 4 12 */
/*:ref: tkfram_ 14 4 4 7 4 12 */
/*:ref: zzdynfr0_ 14 5 4 4 7 7 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: failed_ 12 0 */
 
extern int zzfrmgt1_(integer *infrm, doublereal *et, doublereal *xform, integer *outfrm, logical *found);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: frinfo_ 14 5 4 4 4 4 12 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: tisbod_ 14 5 13 4 7 7 124 */
/*:ref: invstm_ 14 2 7 7 */
/*:ref: namfrm_ 14 3 13 4 124 */
/*:ref: irfrot_ 14 3 4 4 7 */
/*:ref: ckfxfm_ 14 5 4 7 7 4 12 */
/*:ref: tkfram_ 14 4 4 7 4 12 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: failed_ 12 0 */
 
extern int zzftpchk_(char *string, logical *ftperr, ftnlen string_len);
/*:ref: zzftpstr_ 14 8 13 13 13 13 124 124 124 124 */
/*:ref: zzrbrkst_ 14 10 13 13 13 13 4 12 124 124 124 124 */
/*:ref: rtrim_ 4 2 13 124 */
/*:ref: pos_ 4 5 13 13 4 124 124 */
 
extern int zzftpstr_(char *tstcom, char *lend, char *rend, char *delim, ftnlen tstcom_len, ftnlen lend_len, ftnlen rend_len, ftnlen delim_len);
/*:ref: suffix_ 14 5 13 4 13 124 124 */
 
extern int zzgetbff_(integer *bffid);
 
extern int zzgetelm_(integer *frstyr, char *lines, doublereal *epoch, doublereal *elems, logical *ok, char *error, ftnlen lines_len, ftnlen error_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: rpd_ 7 0 */
/*:ref: twopi_ 7 0 */
/*:ref: repmc_ 14 8 13 13 13 13 124 124 124 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: lastnb_ 4 2 13 124 */
/*:ref: repmi_ 14 7 13 13 4 13 124 124 124 */
/*:ref: nparsi_ 14 6 13 4 13 4 124 124 */
/*:ref: nparsd_ 14 6 13 7 13 4 124 124 */
/*:ref: repmd_ 14 8 13 13 7 4 13 124 124 124 */
/*:ref: ttrans_ 14 5 13 13 7 124 124 */
 
extern int zzgpnm_(integer *namlst, integer *nmpool, char *names, integer *datlst, integer *dppool, doublereal *dpvals, integer *chpool, char *chvals, char *varnam, logical *found, integer *lookat, integer *nameat, ftnlen names_len, ftnlen chvals_len, ftnlen varnam_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: zzhash_ 4 2 13 124 */
/*:ref: lnknfn_ 4 1 4 */
/*:ref: lnkan_ 14 2 4 4 */
/*:ref: lnkila_ 14 3 4 4 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int zzidmap_(integer *bltcod, char *bltnam, ftnlen bltnam_len);
 
extern int zzinssub_(char *in, char *sub, integer *loc, char *out, ftnlen in_len, ftnlen sub_len, ftnlen out_len);
 
extern int zzldker_(char *file, char *nofile, char *filtyp, integer *handle, ftnlen file_len, ftnlen nofile_len, ftnlen filtyp_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: exists_ 12 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: getfat_ 14 6 13 13 13 124 124 124 */
/*:ref: spklef_ 14 3 13 4 124 */
/*:ref: cklpf_ 14 3 13 4 124 */
/*:ref: pcklof_ 14 3 13 4 124 */
/*:ref: tkvrsn_ 14 4 13 13 124 124 */
/*:ref: eklef_ 14 3 13 4 124 */
/*:ref: ldpool_ 14 2 13 124 */
/*:ref: failed_ 12 0 */
/*:ref: zzbodkik_ 14 0 */
 
extern int zzmkpc_(char *pictur, integer *b, integer *e, char *mark, char *pattrn, ftnlen pictur_len, ftnlen mark_len, ftnlen pattrn_len);
/*:ref: lastnb_ 4 2 13 124 */
/*:ref: zzrepsub_ 14 8 13 4 4 13 13 124 124 124 */
 
extern int zzmobliq_(doublereal *et, doublereal *mob, doublereal *dmob);
/*:ref: jyear_ 7 0 */
/*:ref: rpd_ 7 0 */
 
extern int zzmsxf_(doublereal *matrix, integer *n, doublereal *output);
 
extern int zznrddp_(doublereal *ao, doublereal *elems, doublereal *em, doublereal *omgasm, doublereal *omgdot, doublereal *t, doublereal *xinc, doublereal *xll, doublereal *xlldot, doublereal *xn, doublereal *xnodes, doublereal *xnodot, doublereal *xnodp);
extern int zzdpinit_(doublereal *ao, doublereal *xlldot, doublereal *omgdot, doublereal *xnodot, doublereal *xnodp, doublereal *elems);
extern int zzdpsec_(doublereal *xll, doublereal *omgasm, doublereal *xnodes, doublereal *em, doublereal *xinc, doublereal *xn, doublereal *t, doublereal *elems, doublereal *omgdot);
extern int zzdpper_(doublereal *t, doublereal *em, doublereal *xinc, doublereal *omgasm, doublereal *xnodes, doublereal *xll);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: pi_ 7 0 */
/*:ref: twopi_ 7 0 */
/*:ref: j2000_ 7 0 */
/*:ref: spd_ 7 0 */
/*:ref: j1950_ 7 0 */
/*:ref: zzsecprt_ 14 12 4 7 7 7 7 7 7 7 7 7 7 7 */
 
extern integer zzocced_(doublereal *viewpt, doublereal *centr1, doublereal *semax1, doublereal *centr2, doublereal *semax2);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: unorm_ 14 3 7 7 7 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errdp_ 14 3 13 7 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: vequ_ 14 2 7 7 */
/*:ref: vsub_ 14 3 7 7 7 */
/*:ref: vnorm_ 7 1 7 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: isrot_ 12 3 7 7 7 */
/*:ref: det_ 7 1 7 */
/*:ref: vsep_ 7 2 7 7 */
/*:ref: vminus_ 14 2 7 7 */
/*:ref: halfpi_ 7 0 */
/*:ref: xpose_ 14 2 7 7 */
/*:ref: mxv_ 14 3 7 7 7 */
/*:ref: edlimb_ 14 5 7 7 7 7 7 */
/*:ref: el2cgv_ 14 4 7 7 7 7 */
/*:ref: mtxv_ 14 3 7 7 7 */
/*:ref: vadd_ 14 3 7 7 7 */
/*:ref: cleard_ 14 2 4 7 */
/*:ref: mxm_ 14 3 7 7 7 */
/*:ref: saelgv_ 14 4 7 7 7 7 */
/*:ref: cgv2el_ 14 4 7 7 7 7 */
/*:ref: zzasryel_ 14 7 13 7 7 7 7 7 124 */
/*:ref: failed_ 12 0 */
/*:ref: vdist_ 7 2 7 7 */
 
extern integer zzphsh_(char *word, integer *m, integer *m2, ftnlen word_len);
extern integer zzshsh_(integer *m);
extern integer zzhash_(char *word, ftnlen word_len);
extern integer zzhash2_(char *word, integer *m2, ftnlen word_len);
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
 
extern int zzpini_(logical *first, integer *maxvar, integer *maxval, integer *maxlin, char *begdat, char *begtxt, integer *nmpool, integer *dppool, integer *chpool, integer *namlst, integer *datlst, integer *maxagt, integer *mxnote, char *watsym, integer *watptr, char *watval, char *agents, char *active, char *notify, ftnlen begdat_len, ftnlen begtxt_len, ftnlen watsym_len, ftnlen watval_len, ftnlen agents_len, ftnlen active_len, ftnlen notify_len);
/*:ref: chkin_ 14 2 13 124 */
/*:ref: zzshsh_ 4 1 4 */
/*:ref: lnkini_ 14 2 4 4 */
/*:ref: ssizec_ 14 3 4 13 124 */
/*:ref: ssizei_ 14 2 4 4 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int zzplatfm_(char *key, char *value, ftnlen key_len, ftnlen value_len);
/*:ref: ucase_ 14 4 13 13 124 124 */
/*:ref: ljust_ 14 4 13 13 124 124 */
/*:ref: isrchc_ 4 5 13 4 13 124 124 */
 
extern int zzpltchk_(logical *ok);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: zzplatfm_ 14 4 13 13 124 124 */
/*:ref: zzgetbff_ 14 1 4 */
/*:ref: zzddhgsd_ 14 5 13 4 13 124 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int zzprscor_(char *abcorr, logical *attblk, ftnlen abcorr_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: orderc_ 14 4 13 4 4 124 */
/*:ref: reordc_ 14 4 4 4 13 124 */
/*:ref: reordl_ 14 3 4 4 12 */
/*:ref: cmprss_ 14 7 13 4 13 13 124 124 124 */
/*:ref: ucase_ 14 4 13 13 124 124 */
/*:ref: bsrchc_ 4 5 13 4 13 124 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int zzrbrkst_(char *string, char *lftend, char *rgtend, char *substr, integer *length, logical *bkpres, ftnlen string_len, ftnlen lftend_len, ftnlen rgtend_len, ftnlen substr_len);
/*:ref: posr_ 4 5 13 13 4 124 124 */
 
extern int zzrefch0_(integer *frame1, integer *frame2, doublereal *et, doublereal *rotate);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: ident_ 14 1 7 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: frinfo_ 14 5 4 4 4 4 12 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: zzrotgt0_ 14 5 4 7 7 4 12 */
/*:ref: zzrxr_ 14 3 7 4 7 */
/*:ref: isrchi_ 4 3 4 4 4 */
/*:ref: frmnam_ 14 3 4 13 124 */
/*:ref: rtrim_ 4 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: xpose_ 14 2 7 7 */
 
extern int zzrefch1_(integer *frame1, integer *frame2, doublereal *et, doublereal *rotate);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: ident_ 14 1 7 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: frinfo_ 14 5 4 4 4 4 12 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: zzrotgt1_ 14 5 4 7 7 4 12 */
/*:ref: zzrxr_ 14 3 7 4 7 */
/*:ref: isrchi_ 4 3 4 4 4 */
/*:ref: frmnam_ 14 3 4 13 124 */
/*:ref: rtrim_ 4 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: xpose_ 14 2 7 7 */
 
extern int zzrepsub_(char *in, integer *left, integer *right, char *string, char *out, ftnlen in_len, ftnlen string_len, ftnlen out_len);
/*:ref: sumai_ 4 2 4 4 */
 
extern logical zzrept_(char *sub, char *replac, logical *l2r, ftnlen sub_len, ftnlen replac_len);
/*:ref: zzsubt_ 12 5 13 13 12 124 124 */
/*:ref: zzremt_ 12 2 13 124 */
 
extern int zzrotgt0_(integer *infrm, doublereal *et, doublereal *rotate, integer *outfrm, logical *found);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: frinfo_ 14 5 4 4 4 4 12 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: irfrot_ 14 3 4 4 7 */
/*:ref: tipbod_ 14 5 13 4 7 7 124 */
/*:ref: xpose_ 14 2 7 7 */
/*:ref: namfrm_ 14 3 13 4 124 */
/*:ref: failed_ 12 0 */
/*:ref: ckfrot_ 14 5 4 7 7 4 12 */
/*:ref: tkfram_ 14 4 4 7 4 12 */
/*:ref: zzdynrt0_ 14 5 4 4 7 7 4 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
 
extern int zzrotgt1_(integer *infrm, doublereal *et, doublereal *rotate, integer *outfrm, logical *found);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: frinfo_ 14 5 4 4 4 4 12 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: irfrot_ 14 3 4 4 7 */
/*:ref: tipbod_ 14 5 13 4 7 7 124 */
/*:ref: xpose_ 14 2 7 7 */
/*:ref: namfrm_ 14 3 13 4 124 */
/*:ref: failed_ 12 0 */
/*:ref: ckfrot_ 14 5 4 7 7 4 12 */
/*:ref: tkfram_ 14 4 4 7 4 12 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
 
extern int zzrvar_(integer *namlst, integer *nmpool, char *names, integer *datlst, integer *dppool, doublereal *dpvals, integer *chpool, char *chvals, char *varnam, logical *eof, ftnlen names_len, ftnlen chvals_len, ftnlen varnam_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: failed_ 12 0 */
/*:ref: rdkdat_ 14 3 13 12 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: rdklin_ 14 3 13 4 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: rtrim_ 4 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: lastpc_ 4 2 13 124 */
/*:ref: zzhash_ 4 2 13 124 */
/*:ref: lnknfn_ 4 1 4 */
/*:ref: lnkan_ 14 2 4 4 */
/*:ref: lnkila_ 14 3 4 4 4 */
/*:ref: lnkfsl_ 14 3 4 4 4 */
/*:ref: zzcln_ 14 7 4 4 4 4 4 4 4 */
/*:ref: tparse_ 14 5 13 7 13 124 124 */
/*:ref: lastnb_ 4 2 13 124 */
/*:ref: nparsd_ 14 6 13 7 13 4 124 124 */
 
extern int zzrvbf_(char *buffer, integer *bsize, integer *linnum, integer *namlst, integer *nmpool, char *names, integer *datlst, integer *dppool, doublereal *dpvals, integer *chpool, char *chvals, char *varnam, logical *eof, ftnlen buffer_len, ftnlen names_len, ftnlen chvals_len, ftnlen varnam_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: failed_ 12 0 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: rtrim_ 4 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: lastpc_ 4 2 13 124 */
/*:ref: zzhash_ 4 2 13 124 */
/*:ref: lnknfn_ 4 1 4 */
/*:ref: lnkan_ 14 2 4 4 */
/*:ref: lnkila_ 14 3 4 4 4 */
/*:ref: lnkfsl_ 14 3 4 4 4 */
/*:ref: zzcln_ 14 7 4 4 4 4 4 4 4 */
/*:ref: tparse_ 14 5 13 7 13 124 124 */
/*:ref: nparsd_ 14 6 13 7 13 4 124 124 */
 
extern int zzrxr_(doublereal *matrix, integer *n, doublereal *output);
/*:ref: ident_ 14 1 7 */
 
extern logical zzsclk_(integer *ckid, integer *sclkid);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: ssizei_ 14 2 4 4 */
/*:ref: intstr_ 14 3 4 13 124 */
/*:ref: elemi_ 12 2 4 4 */
/*:ref: cvpool_ 14 3 13 12 124 */
/*:ref: cardi_ 4 1 4 */
/*:ref: sizei_ 4 1 4 */
/*:ref: insrti_ 14 2 4 4 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: swpool_ 14 5 13 4 13 124 124 */
/*:ref: dtpool_ 14 6 13 12 4 13 124 124 */
/*:ref: removi_ 14 2 4 4 */
 
extern int zzsecprt_(integer *isynfl, doublereal *dg, doublereal *del, doublereal *xni, doublereal *omegao, doublereal *atime, doublereal *omgdot, doublereal *xli, doublereal *xfact, doublereal *xldot, doublereal *xndot, doublereal *xnddt);
 
extern int zzsizeok_(integer *size, integer *psize, integer *dsize, integer *offset, logical *ok, integer *n);
/*:ref: rmaini_ 14 4 4 4 4 4 */
 
extern int zzspkap0_(integer *targ, doublereal *et, char *ref, doublereal *sobs, char *abcorr, doublereal *starg, doublereal *lt, ftnlen ref_len, ftnlen abcorr_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cmprss_ 14 7 13 4 13 13 124 124 124 */
/*:ref: ucase_ 14 4 13 13 124 124 */
/*:ref: isrchc_ 4 5 13 4 13 124 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: odd_ 12 1 4 */
/*:ref: irfnum_ 14 3 13 4 124 */
/*:ref: zzspksb0_ 14 5 4 7 13 7 124 */
/*:ref: vsubg_ 14 4 7 7 4 7 */
/*:ref: moved_ 14 3 7 4 7 */
/*:ref: vnorm_ 7 1 7 */
/*:ref: clight_ 7 0 */
/*:ref: stlabx_ 14 3 7 7 7 */
/*:ref: vequ_ 14 2 7 7 */
/*:ref: stelab_ 14 3 7 7 7 */
 
extern int zzspkap1_(integer *targ, doublereal *et, char *ref, doublereal *sobs, char *abcorr, doublereal *starg, doublereal *lt, ftnlen ref_len, ftnlen abcorr_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cmprss_ 14 7 13 4 13 13 124 124 124 */
/*:ref: ucase_ 14 4 13 13 124 124 */
/*:ref: isrchc_ 4 5 13 4 13 124 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: odd_ 12 1 4 */
/*:ref: irfnum_ 14 3 13 4 124 */
/*:ref: zzspksb1_ 14 5 4 7 13 7 124 */
/*:ref: vsubg_ 14 4 7 7 4 7 */
/*:ref: moved_ 14 3 7 4 7 */
/*:ref: vnorm_ 7 1 7 */
/*:ref: clight_ 7 0 */
/*:ref: stlabx_ 14 3 7 7 7 */
/*:ref: vequ_ 14 2 7 7 */
/*:ref: stelab_ 14 3 7 7 7 */
 
extern int zzspkez0_(integer *targ, doublereal *et, char *ref, char *abcorr, integer *obs, doublereal *starg, doublereal *lt, ftnlen ref_len, ftnlen abcorr_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: namfrm_ 14 3 13 4 124 */
/*:ref: ltrim_ 4 2 13 124 */
/*:ref: eqchr_ 12 4 13 13 124 124 */
/*:ref: eqstr_ 12 4 13 13 124 124 */
/*:ref: zzspkgo0_ 14 7 4 7 13 4 7 7 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: frinfo_ 14 5 4 4 4 4 12 */
/*:ref: zzspksb0_ 14 5 4 7 13 7 124 */
/*:ref: zzspkap0_ 14 9 4 7 13 7 13 7 7 124 124 */
/*:ref: failed_ 12 0 */
/*:ref: zzfrmch0_ 14 4 4 4 7 7 */
/*:ref: mxvg_ 14 5 7 7 4 4 7 */
 
extern int zzspkez1_(integer *targ, doublereal *et, char *ref, char *abcorr, integer *obs, doublereal *starg, doublereal *lt, ftnlen ref_len, ftnlen abcorr_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: namfrm_ 14 3 13 4 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: frinfo_ 14 5 4 4 4 4 12 */
/*:ref: ltrim_ 4 2 13 124 */
/*:ref: eqchr_ 12 4 13 13 124 124 */
/*:ref: eqstr_ 12 4 13 13 124 124 */
/*:ref: zzspkgo1_ 14 7 4 7 13 4 7 7 124 */
/*:ref: zzspksb1_ 14 5 4 7 13 7 124 */
/*:ref: zzspkap1_ 14 9 4 7 13 7 13 7 7 124 124 */
/*:ref: failed_ 12 0 */
/*:ref: zzfrmch1_ 14 4 4 4 7 7 */
/*:ref: mxvg_ 14 5 7 7 4 4 7 */
 
extern int zzspkgo0_(integer *targ, doublereal *et, char *ref, integer *obs, doublereal *state, doublereal *lt, ftnlen ref_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cleard_ 14 2 4 7 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: irfnum_ 14 3 13 4 124 */
/*:ref: namfrm_ 14 3 13 4 124 */
/*:ref: frstnp_ 4 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: failed_ 12 0 */
/*:ref: spksfs_ 14 7 4 7 4 7 13 12 124 */
/*:ref: spkpvn_ 14 6 4 7 7 4 7 4 */
/*:ref: moved_ 14 3 7 4 7 */
/*:ref: irfrot_ 14 3 4 4 7 */
/*:ref: mxv_ 14 3 7 7 7 */
/*:ref: zzfrmch0_ 14 4 4 4 7 7 */
/*:ref: mxvg_ 14 5 7 7 4 4 7 */
/*:ref: vaddg_ 14 4 7 7 4 7 */
/*:ref: isrchi_ 4 3 4 4 4 */
/*:ref: bodc2n_ 14 4 4 13 12 124 */
/*:ref: prefix_ 14 5 13 4 13 124 124 */
/*:ref: suffix_ 14 5 13 4 13 124 124 */
/*:ref: repmi_ 14 7 13 13 4 13 124 124 124 */
/*:ref: intstr_ 14 3 4 13 124 */
/*:ref: etcal_ 14 3 7 13 124 */
/*:ref: vsubg_ 14 4 7 7 4 7 */
/*:ref: vnorm_ 7 1 7 */
/*:ref: clight_ 7 0 */
 
extern int zzspkgo1_(integer *targ, doublereal *et, char *ref, integer *obs, doublereal *state, doublereal *lt, ftnlen ref_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cleard_ 14 2 4 7 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: irfnum_ 14 3 13 4 124 */
/*:ref: namfrm_ 14 3 13 4 124 */
/*:ref: frstnp_ 4 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: failed_ 12 0 */
/*:ref: spksfs_ 14 7 4 7 4 7 13 12 124 */
/*:ref: spkpvn_ 14 6 4 7 7 4 7 4 */
/*:ref: moved_ 14 3 7 4 7 */
/*:ref: irfrot_ 14 3 4 4 7 */
/*:ref: mxv_ 14 3 7 7 7 */
/*:ref: zzfrmch1_ 14 4 4 4 7 7 */
/*:ref: mxvg_ 14 5 7 7 4 4 7 */
/*:ref: vaddg_ 14 4 7 7 4 7 */
/*:ref: isrchi_ 4 3 4 4 4 */
/*:ref: bodc2n_ 14 4 4 13 12 124 */
/*:ref: prefix_ 14 5 13 4 13 124 124 */
/*:ref: suffix_ 14 5 13 4 13 124 124 */
/*:ref: repmi_ 14 7 13 13 4 13 124 124 124 */
/*:ref: intstr_ 14 3 4 13 124 */
/*:ref: etcal_ 14 3 7 13 124 */
/*:ref: vsubg_ 14 4 7 7 4 7 */
/*:ref: vnorm_ 7 1 7 */
/*:ref: clight_ 7 0 */
 
extern int zzspkgp0_(integer *targ, doublereal *et, char *ref, integer *obs, doublereal *pos, doublereal *lt, ftnlen ref_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cleard_ 14 2 4 7 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: irfnum_ 14 3 13 4 124 */
/*:ref: namfrm_ 14 3 13 4 124 */
/*:ref: frstnp_ 4 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: failed_ 12 0 */
/*:ref: spksfs_ 14 7 4 7 4 7 13 12 124 */
/*:ref: spkpvn_ 14 6 4 7 7 4 7 4 */
/*:ref: moved_ 14 3 7 4 7 */
/*:ref: irfrot_ 14 3 4 4 7 */
/*:ref: mxv_ 14 3 7 7 7 */
/*:ref: zzrefch0_ 14 4 4 4 7 7 */
/*:ref: vadd_ 14 3 7 7 7 */
/*:ref: vequ_ 14 2 7 7 */
/*:ref: isrchi_ 4 3 4 4 4 */
/*:ref: bodc2n_ 14 4 4 13 12 124 */
/*:ref: prefix_ 14 5 13 4 13 124 124 */
/*:ref: suffix_ 14 5 13 4 13 124 124 */
/*:ref: repmi_ 14 7 13 13 4 13 124 124 124 */
/*:ref: intstr_ 14 3 4 13 124 */
/*:ref: etcal_ 14 3 7 13 124 */
/*:ref: vsub_ 14 3 7 7 7 */
/*:ref: vnorm_ 7 1 7 */
/*:ref: clight_ 7 0 */
 
extern int zzspkgp1_(integer *targ, doublereal *et, char *ref, integer *obs, doublereal *pos, doublereal *lt, ftnlen ref_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cleard_ 14 2 4 7 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: irfnum_ 14 3 13 4 124 */
/*:ref: namfrm_ 14 3 13 4 124 */
/*:ref: frstnp_ 4 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: failed_ 12 0 */
/*:ref: spksfs_ 14 7 4 7 4 7 13 12 124 */
/*:ref: spkpvn_ 14 6 4 7 7 4 7 4 */
/*:ref: moved_ 14 3 7 4 7 */
/*:ref: irfrot_ 14 3 4 4 7 */
/*:ref: mxv_ 14 3 7 7 7 */
/*:ref: zzrefch1_ 14 4 4 4 7 7 */
/*:ref: vadd_ 14 3 7 7 7 */
/*:ref: vequ_ 14 2 7 7 */
/*:ref: isrchi_ 4 3 4 4 4 */
/*:ref: bodc2n_ 14 4 4 13 12 124 */
/*:ref: prefix_ 14 5 13 4 13 124 124 */
/*:ref: suffix_ 14 5 13 4 13 124 124 */
/*:ref: repmi_ 14 7 13 13 4 13 124 124 124 */
/*:ref: intstr_ 14 3 4 13 124 */
/*:ref: etcal_ 14 3 7 13 124 */
/*:ref: vsub_ 14 3 7 7 7 */
/*:ref: vnorm_ 7 1 7 */
/*:ref: clight_ 7 0 */
 
extern int zzspkpa0_(integer *targ, doublereal *et, char *ref, doublereal *sobs, char *abcorr, doublereal *ptarg, doublereal *lt, ftnlen ref_len, ftnlen abcorr_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cmprss_ 14 7 13 4 13 13 124 124 124 */
/*:ref: ucase_ 14 4 13 13 124 124 */
/*:ref: isrchc_ 4 5 13 4 13 124 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: odd_ 12 1 4 */
/*:ref: irfnum_ 14 3 13 4 124 */
/*:ref: zzspkgp0_ 14 7 4 7 13 4 7 7 124 */
/*:ref: vsub_ 14 3 7 7 7 */
/*:ref: vequ_ 14 2 7 7 */
/*:ref: vnorm_ 7 1 7 */
/*:ref: clight_ 7 0 */
/*:ref: stlabx_ 14 3 7 7 7 */
/*:ref: stelab_ 14 3 7 7 7 */
 
extern int zzspkpa1_(integer *targ, doublereal *et, char *ref, doublereal *sobs, char *abcorr, doublereal *ptarg, doublereal *lt, ftnlen ref_len, ftnlen abcorr_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: cmprss_ 14 7 13 4 13 13 124 124 124 */
/*:ref: ucase_ 14 4 13 13 124 124 */
/*:ref: isrchc_ 4 5 13 4 13 124 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: odd_ 12 1 4 */
/*:ref: irfnum_ 14 3 13 4 124 */
/*:ref: zzspkgp1_ 14 7 4 7 13 4 7 7 124 */
/*:ref: vsub_ 14 3 7 7 7 */
/*:ref: vequ_ 14 2 7 7 */
/*:ref: vnorm_ 7 1 7 */
/*:ref: clight_ 7 0 */
/*:ref: stlabx_ 14 3 7 7 7 */
/*:ref: stelab_ 14 3 7 7 7 */
 
extern int zzspksb0_(integer *targ, doublereal *et, char *ref, doublereal *starg, ftnlen ref_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: zzspkgo0_ 14 7 4 7 13 4 7 7 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int zzspksb1_(integer *targ, doublereal *et, char *ref, doublereal *starg, ftnlen ref_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: zzspkgo1_ 14 7 4 7 13 4 7 7 124 */
/*:ref: chkout_ 14 2 13 124 */
 
extern int zzspkzp0_(integer *targ, doublereal *et, char *ref, char *abcorr, integer *obs, doublereal *ptarg, doublereal *lt, ftnlen ref_len, ftnlen abcorr_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: namfrm_ 14 3 13 4 124 */
/*:ref: ltrim_ 4 2 13 124 */
/*:ref: eqchr_ 12 4 13 13 124 124 */
/*:ref: eqstr_ 12 4 13 13 124 124 */
/*:ref: zzspkgp0_ 14 7 4 7 13 4 7 7 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: frinfo_ 14 5 4 4 4 4 12 */
/*:ref: zzspksb0_ 14 5 4 7 13 7 124 */
/*:ref: zzspkpa0_ 14 9 4 7 13 7 13 7 7 124 124 */
/*:ref: failed_ 12 0 */
/*:ref: zzrefch0_ 14 4 4 4 7 7 */
/*:ref: mxv_ 14 3 7 7 7 */
 
extern int zzspkzp1_(integer *targ, doublereal *et, char *ref, char *abcorr, integer *obs, doublereal *ptarg, doublereal *lt, ftnlen ref_len, ftnlen abcorr_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: namfrm_ 14 3 13 4 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: frinfo_ 14 5 4 4 4 4 12 */
/*:ref: ltrim_ 4 2 13 124 */
/*:ref: eqchr_ 12 4 13 13 124 124 */
/*:ref: eqstr_ 12 4 13 13 124 124 */
/*:ref: zzspkgp1_ 14 7 4 7 13 4 7 7 124 */
/*:ref: zzspksb1_ 14 5 4 7 13 7 124 */
/*:ref: zzspkpa1_ 14 9 4 7 13 7 13 7 7 124 124 */
/*:ref: failed_ 12 0 */
/*:ref: zzrefch1_ 14 4 4 4 7 7 */
/*:ref: mxv_ 14 3 7 7 7 */
 
extern logical zztime_(char *string, char *transl, char *letter, char *error, char *pic, doublereal *tvec, integer *b, integer *e, logical *l2r, logical *yabbrv, ftnlen string_len, ftnlen transl_len, ftnlen letter_len, ftnlen error_len, ftnlen pic_len);
extern logical zzcmbt_(char *string, char *letter, logical *l2r, ftnlen string_len, ftnlen letter_len);
extern logical zzgrep_(char *string, ftnlen string_len);
extern logical zzispt_(char *string, integer *b, integer *e, ftnlen string_len);
extern logical zzist_(char *letter, ftnlen letter_len);
extern logical zznote_(char *letter, integer *b, integer *e, ftnlen letter_len);
extern logical zzremt_(char *letter, ftnlen letter_len);
extern logical zzsubt_(char *string, char *transl, logical *l2r, ftnlen string_len, ftnlen transl_len);
extern logical zztokns_(char *string, char *error, ftnlen string_len, ftnlen error_len);
extern logical zzunpck_(char *string, logical *yabbrv, doublereal *tvec, integer *e, char *transl, char *pic, char *error, ftnlen string_len, ftnlen transl_len, ftnlen pic_len, ftnlen error_len);
extern logical zzvalt_(char *string, integer *b, integer *e, char *letter, ftnlen string_len, ftnlen letter_len);
/*:ref: pos_ 4 5 13 13 4 124 124 */
/*:ref: posr_ 4 5 13 13 4 124 124 */
/*:ref: zzrepsub_ 14 8 13 4 4 13 13 124 124 124 */
/*:ref: cpos_ 4 5 13 13 4 124 124 */
/*:ref: rtrim_ 4 2 13 124 */
/*:ref: lx4uns_ 14 5 13 4 4 4 124 */
/*:ref: zzinssub_ 14 7 13 13 4 13 124 124 124 */
/*:ref: prefix_ 14 5 13 4 13 124 124 */
/*:ref: repmi_ 14 7 13 13 4 13 124 124 124 */
/*:ref: ucase_ 14 4 13 13 124 124 */
/*:ref: samsbi_ 12 8 13 4 4 13 4 4 124 124 */
/*:ref: samchi_ 12 6 13 4 13 4 124 124 */
/*:ref: suffix_ 14 5 13 4 13 124 124 */
/*:ref: repmc_ 14 8 13 13 13 13 124 124 124 124 */
/*:ref: nparsd_ 14 6 13 7 13 4 124 124 */
/*:ref: isrchc_ 4 5 13 4 13 124 124 */
/*:ref: zzmkpc_ 14 8 13 4 4 13 13 124 124 124 */
/*:ref: nparsi_ 14 6 13 4 13 4 124 124 */
 
extern logical zztpats_(integer *room, integer *nknown, char *known, char *meanng, ftnlen known_len, ftnlen meanng_len);
/*:ref: orderc_ 14 4 13 4 4 124 */
/*:ref: reordc_ 14 4 4 4 13 124 */
 
extern int zztwovxf_(doublereal *axdef, integer *indexa, doublereal *plndef, integer *indexp, doublereal *xform);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: dvhat_ 14 2 7 7 */
/*:ref: ducrss_ 14 3 7 7 7 */
/*:ref: moved_ 14 3 7 4 7 */
/*:ref: cleard_ 14 2 4 7 */
/*:ref: vzero_ 12 1 7 */
 
extern int zzutcpm_(char *string, integer *start, doublereal *hoff, doublereal *moff, integer *last, logical *succes, ftnlen string_len);
/*:ref: lx4uns_ 14 5 13 4 4 4 124 */
/*:ref: nparsd_ 14 6 13 7 13 4 124 124 */
/*:ref: samch_ 12 6 13 4 13 4 124 124 */
 
extern int zzvstrng_(doublereal *x, char *fill, integer *from, integer *to, logical *rnd, integer *expont, char *substr, logical *did, ftnlen fill_len, ftnlen substr_len);
extern int zzvststr_(doublereal *x, char *fill, integer *expont, ftnlen fill_len);
extern int zzvsbstr_(integer *from, integer *to, logical *rnd, char *substr, logical *did, ftnlen substr_len);
/*:ref: dpstr_ 14 4 7 4 13 124 */
 
extern int zzwahr_(doublereal *et, doublereal *dvnut);
/*:ref: pi_ 7 0 */
/*:ref: twopi_ 7 0 */
/*:ref: spd_ 7 0 */
 
extern integer zzwind_(doublereal *plane, integer *n, doublereal *vertcs, doublereal *point);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: pl2nvc_ 14 3 7 7 7 */
/*:ref: vzero_ 12 1 7 */
/*:ref: vdot_ 7 2 7 7 */
/*:ref: vminus_ 14 2 7 7 */
/*:ref: vequ_ 14 2 7 7 */
/*:ref: vsub_ 14 3 7 7 7 */
/*:ref: vperp_ 14 3 7 7 7 */
/*:ref: vsep_ 7 2 7 7 */
/*:ref: ucrss_ 14 3 7 7 7 */
/*:ref: twopi_ 7 0 */
 
extern int zzxlated_(integer *inbff, char *input, integer *space, doublereal *output, ftnlen input_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: zzddhgsd_ 14 5 13 4 13 124 124 */
/*:ref: zzplatfm_ 14 4 13 13 124 124 */
/*:ref: ucase_ 14 4 13 13 124 124 */
/*:ref: isrchc_ 4 5 13 4 13 124 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: intmin_ 4 0 */
/*:ref: errint_ 14 3 13 4 124 */
/*:ref: moved_ 14 3 7 4 7 */
 
extern int zzxlatei_(integer *inbff, char *input, integer *space, integer *output, ftnlen input_len);
/*:ref: return_ 12 0 */
/*:ref: chkin_ 14 2 13 124 */
/*:ref: zzddhgsd_ 14 5 13 4 13 124 124 */
/*:ref: zzplatfm_ 14 4 13 13 124 124 */
/*:ref: ucase_ 14 4 13 13 124 124 */
/*:ref: isrchc_ 4 5 13 4 13 124 124 */
/*:ref: setmsg_ 14 2 13 124 */
/*:ref: errch_ 14 4 13 13 124 124 */
/*:ref: sigerr_ 14 2 13 124 */
/*:ref: chkout_ 14 2 13 124 */
/*:ref: intmin_ 4 0 */
/*:ref: errint_ 14 3 13 4 124 */
 
 
#ifdef __cplusplus
   }
#endif
 
#endif
 
