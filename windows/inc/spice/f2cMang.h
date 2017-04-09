/*

-Header_File f2cMang.h ( f2c external symbol mangling )

-Abstract

   Define macros that mangle the external symbols in the f2c F77 and I77
   libraries.
      
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

   This header supports linking CSPICE into executables that
   also link in objects compiled from Fortran, in particular
   ones that perform Fortran I/O.  To enable this odd mix,
   one defines the  preprocessor flag

      MIX_C_AND_FORTRAN

   This macro is undefined by default, since the action it invokes
   is usually not desirable.  When the flag is defined, this header
   defines macros that mangle the f2c library external symbols:
   the symbol

      xxx

   gets mapped to 

      xxx_f2c

   This mangling prevents name collisions between the f2c 
   implementations of the F77 and I77 library routines and those 
   in the corresponding Fortran libraries on a host system.
   
   The set of external symbols defined in the f2c libraries can
   be determined by combining objects from both F77 and I77 into
   a single Unix archive libarary, then running the Unix utility
   nm on the that archive.  If available, an nm option that selects
   only external symbols should be invoked.

-Literature_References

   None.

-Author_and_Institution

   N.J. Bachman       (JPL)
   
-Restrictions

   1) It is recommended that use of the features implemented by this
      header be avoided if at all possible.  There are robustness and
      portability problems associated with linking Fortran and C objects
      together in one executable.

   2) When f2c external symbol name mangling is invoked, objects
      derived from C code translated from Fortran by f2c won't
      link against CSPICE any longer, if these objects reference
      the standard f2c external symbols.

   3) The features implemented by this header have been tested only
      under the Sun Solaris GCC, Sun Solaris native ANSI C, and
      PC/Linux/gcc environments.

-Version

   -CSPICE Version 2.0.1, 07-MAR-2009 (NJB)

       Restrictions header section was updated to note successful
       testing on the PC/Linux/gcc platform.

   -CSPICE Version 2.0.0, 19-DEC-2001 (NJB)

*/


   /*
   Define masking macros for f2c external symbols.
   */
   #ifdef  MIX_C_AND_FORTRAN

      /*
      Define the macros only once, if they need to be defined.
      */
      #ifndef F2C_MANGLING_DONE

         #define F77_aloc  F77_aloc_f2c 
         #define F_err  F_err_f2c 
         #define L_len  L_len_f2c 
         #define abort_  abort__f2c 
         #define b_char  b_char_f2c 
         #define c_abs  c_abs_f2c 
         #define c_cos  c_cos_f2c 
         #define c_dfe  c_dfe_f2c 
         #define c_div  c_div_f2c 
         #define c_due  c_due_f2c 
         #define c_exp  c_exp_f2c 
         #define c_le  c_le_f2c 
         #define c_log  c_log_f2c 
         #define c_sfe  c_sfe_f2c 
         #define c_si  c_si_f2c 
         #define c_sin  c_sin_f2c 
         #define c_sqrt  c_sqrt_f2c 
         #define c_sue  c_sue_f2c 
         #define d_abs  d_abs_f2c 
         #define d_acos  d_acos_f2c 
         #define d_asin  d_asin_f2c 
         #define d_atan  d_atan_f2c 
         #define d_atn2  d_atn2_f2c 
         #define d_cnjg  d_cnjg_f2c 
         #define d_cos  d_cos_f2c 
         #define d_cosh  d_cosh_f2c 
         #define d_dim  d_dim_f2c 
         #define d_exp  d_exp_f2c 
         #define d_imag  d_imag_f2c 
         #define d_int  d_int_f2c 
         #define d_lg10  d_lg10_f2c 
         #define d_log  d_log_f2c 
         #define d_mod  d_mod_f2c 
         #define d_nint  d_nint_f2c 
         #define d_prod  d_prod_f2c 
         #define d_sign  d_sign_f2c 
         #define d_sin  d_sin_f2c 
         #define d_sinh  d_sinh_f2c 
         #define d_sqrt  d_sqrt_f2c 
         #define d_tan  d_tan_f2c 
         #define d_tanh  d_tanh_f2c 
         #define derf_  derf__f2c 
         #define derfc_  derfc__f2c 
         #define do_fio  do_fio_f2c 
         #define do_lio  do_lio_f2c 
         #define do_ud  do_ud_f2c 
         #define do_uio  do_uio_f2c 
         #define do_us  do_us_f2c 
         #define dtime_  dtime__f2c 
         #define e_rdfe  e_rdfe_f2c 
         #define e_rdue  e_rdue_f2c 
         #define e_rsfe  e_rsfe_f2c 
         #define e_rsfi  e_rsfi_f2c 
         #define e_rsle  e_rsle_f2c 
         #define e_rsli  e_rsli_f2c 
         #define e_rsue  e_rsue_f2c 
         #define e_wdfe  e_wdfe_f2c 
         #define e_wdue  e_wdue_f2c 
         #define e_wsfe  e_wsfe_f2c 
         #define e_wsfi  e_wsfi_f2c 
         #define e_wsle  e_wsle_f2c 
         #define e_wsli  e_wsli_f2c 
         #define e_wsue  e_wsue_f2c 
         #define ef1asc_  ef1asc__f2c 
         #define ef1cmc_  ef1cmc__f2c 
         #define en_fio  en_fio_f2c 
         #define erf_  erf__f2c 
         #define erfc_  erfc__f2c 
         #define err__fl  err__fl_f2c 
         #define etime_  etime__f2c 
         #define exit_  exit__f2c 
         #define f__Aquote  f__Aquote_f2c 
         #define f__buflen  f__buflen_f2c 
         #define f__cabs  f__cabs_f2c 
         #define f__canseek  f__canseek_f2c 
         #define f__cblank  f__cblank_f2c 
         #define f__cf  f__cf_f2c 
         #define f__cnt  f__cnt_f2c 
         #define f__cp  f__cp_f2c 
         #define f__cplus  f__cplus_f2c 
         #define f__cursor  f__cursor_f2c 
         #define f__curunit  f__curunit_f2c 
         #define f__doed  f__doed_f2c 
         #define f__doend  f__doend_f2c 
         #define f__doned  f__doned_f2c 
         #define f__donewrec  f__donewrec_f2c 
         #define f__dorevert  f__dorevert_f2c 
         #define f__elist  f__elist_f2c 
         #define f__external  f__external_f2c 
         #define f__fatal  f__fatal_f2c 
         #define f__fmtbuf  f__fmtbuf_f2c 
         #define f__formatted  f__formatted_f2c 
         #define f__getn  f__getn_f2c 
         #define f__hiwater  f__hiwater_f2c 
         #define f__icend  f__icend_f2c 
         #define f__icnum  f__icnum_f2c 
         #define f__icptr  f__icptr_f2c 
         #define f__icvt  f__icvt_f2c 
         #define f__init  f__init_f2c 
         #define f__inode  f__inode_f2c 
         #define f__lchar  f__lchar_f2c 
         #define f__lcount  f__lcount_f2c 
         #define f__lioproc  f__lioproc_f2c 
         #define f__lquit  f__lquit_f2c 
         #define f__ltab  f__ltab_f2c 
         #define f__ltype  f__ltype_f2c 
         #define f__lx  f__lx_f2c 
         #define f__ly  f__ly_f2c 
         #define f__nonl  f__nonl_f2c 
         #define f__nowreading  f__nowreading_f2c 
         #define f__nowwriting  f__nowwriting_f2c 
         #define f__parenlvl  f__parenlvl_f2c 
         #define f__pc  f__pc_f2c 
         #define f__putbuf  f__putbuf_f2c 
         #define f__putn  f__putn_f2c 
         #define f__r_mode  f__r_mode_f2c 
         #define f__reading  f__reading_f2c 
         #define f__reclen  f__reclen_f2c 
         #define f__recloc  f__recloc_f2c 
         #define f__recpos  f__recpos_f2c 
         #define f__ret  f__ret_f2c 
         #define f__revloc  f__revloc_f2c 
         #define f__rp  f__rp_f2c 
         #define f__scale  f__scale_f2c 
         #define f__sequential  f__sequential_f2c 
         #define f__svic  f__svic_f2c 
         #define f__typesize  f__typesize_f2c 
         #define f__units  f__units_f2c 
         #define f__w_mode  f__w_mode_f2c 
         #define f__workdone  f__workdone_f2c 
         #define f_back  f_back_f2c 
         #define f_clos  f_clos_f2c 
         #define f_end  f_end_f2c 
         #define f_exit  f_exit_f2c 
         #define f_init  f_init_f2c 
         #define f_inqu  f_inqu_f2c 
         #define f_open  f_open_f2c 
         #define f_rew  f_rew_f2c 
         #define fk_open  fk_open_f2c 
         #define flush_  flush__f2c 
         #define fmt_bg  fmt_bg_f2c 
         #define fseek_  fseek__f2c 
         #define ftell_  ftell__f2c 
         #define g_char  g_char_f2c 
         #define getenv_  getenv__f2c 
         #define h_abs  h_abs_f2c 
         #define h_dim  h_dim_f2c 
         #define h_dnnt  h_dnnt_f2c 
         #define h_indx  h_indx_f2c 
         #define h_len  h_len_f2c 
         #define h_mod  h_mod_f2c 
         #define h_nint  h_nint_f2c 
         #define h_sign  h_sign_f2c 
         #define hl_ge  hl_ge_f2c 
         #define hl_gt  hl_gt_f2c 
         #define hl_le  hl_le_f2c 
         #define hl_lt  hl_lt_f2c 
         #define i_abs  i_abs_f2c 
         #define i_dim  i_dim_f2c 
         #define i_dnnt  i_dnnt_f2c 
         #define i_indx  i_indx_f2c 
         #define i_len  i_len_f2c 
         #define i_mod  i_mod_f2c 
         #define i_nint  i_nint_f2c 
         #define i_sign  i_sign_f2c 
         #define iw_rev  iw_rev_f2c 
         #define l_eof  l_eof_f2c 
         #define l_ge  l_ge_f2c 
         #define l_getc  l_getc_f2c 
         #define l_gt  l_gt_f2c 
         #define l_le  l_le_f2c 
         #define l_lt  l_lt_f2c 
         #define l_read  l_read_f2c 
         #define l_ungetc  l_ungetc_f2c 
         #define l_write  l_write_f2c 
         #define lbit_bits  lbit_bits_f2c 
         #define lbit_cshift  lbit_cshift_f2c 
         #define lbit_shift  lbit_shift_f2c 
         #define mk_hashtab  mk_hashtab_f2c 
         #define nml_read  nml_read_f2c 
         #define pars_f  pars_f_f2c 
         #define pow_ci  pow_ci_f2c 
         #define pow_dd  pow_dd_f2c 
         #define pow_di  pow_di_f2c 
         #define pow_hh  pow_hh_f2c 
         #define pow_ii  pow_ii_f2c 
         #define pow_ri  pow_ri_f2c 
         #define pow_zi  pow_zi_f2c 
         #define pow_zz  pow_zz_f2c 
         #define r_abs  r_abs_f2c 
         #define r_acos  r_acos_f2c 
         #define r_asin  r_asin_f2c 
         #define r_atan  r_atan_f2c 
         #define r_atn2  r_atn2_f2c 
         #define r_cnjg  r_cnjg_f2c 
         #define r_cos  r_cos_f2c 
         #define r_cosh  r_cosh_f2c 
         #define r_dim  r_dim_f2c 
         #define r_exp  r_exp_f2c 
         #define r_imag  r_imag_f2c 
         #define r_int  r_int_f2c 
         #define r_lg10  r_lg10_f2c 
         #define r_log  r_log_f2c 
         #define r_mod  r_mod_f2c 
         #define r_nint  r_nint_f2c 
         #define r_sign  r_sign_f2c 
         #define r_sin  r_sin_f2c 
         #define r_sinh  r_sinh_f2c 
         #define r_sqrt  r_sqrt_f2c 
         #define r_tan  r_tan_f2c 
         #define r_tanh  r_tanh_f2c 
         #define rd_ed  rd_ed_f2c 
         #define rd_ned  rd_ned_f2c 
         #define s_cat  s_cat_f2c 
         #define s_cmp  s_cmp_f2c 
         #define s_copy  s_copy_f2c 
         #define s_paus  s_paus_f2c 
         #define s_rdfe  s_rdfe_f2c 
         #define s_rdue  s_rdue_f2c 
         #define s_rnge  s_rnge_f2c 
         #define s_rsfe  s_rsfe_f2c 
         #define s_rsfi  s_rsfi_f2c 
         #define s_rsle  s_rsle_f2c 
         #define s_rsli  s_rsli_f2c 
         #define s_rsne  s_rsne_f2c 
         #define s_rsni  s_rsni_f2c 
         #define s_rsue  s_rsue_f2c 
         #define s_stop  s_stop_f2c 
         #define s_wdfe  s_wdfe_f2c 
         #define s_wdue  s_wdue_f2c 
         #define s_wsfe  s_wsfe_f2c 
         #define s_wsfi  s_wsfi_f2c 
         #define s_wsle  s_wsle_f2c 
         #define s_wsli  s_wsli_f2c 
         #define s_wsne  s_wsne_f2c 
         #define s_wsni  s_wsni_f2c 
         #define s_wsue  s_wsue_f2c 
         #define sig_die  sig_die_f2c 
         #define signal_  signal__f2c 
         #define system_  system__f2c 
         #define t_getc  t_getc_f2c 
         #define t_runc  t_runc_f2c 
         #define w_ed  w_ed_f2c 
         #define w_ned  w_ned_f2c 
         #define wrt_E  wrt_E_f2c 
         #define wrt_F  wrt_F_f2c 
         #define wrt_L  wrt_L_f2c 
         #define x_endp  x_endp_f2c 
         #define x_getc  x_getc_f2c 
         #define x_putc  x_putc_f2c 
         #define x_rev  x_rev_f2c 
         #define x_rsne  x_rsne_f2c 
         #define x_wSL  x_wSL_f2c 
         #define x_wsne  x_wsne_f2c 
         #define xrd_SL  xrd_SL_f2c 
         #define y_getc  y_getc_f2c 
         #define y_rsk  y_rsk_f2c 
         #define z_abs  z_abs_f2c 
         #define z_cos  z_cos_f2c 
         #define z_div  z_div_f2c 
         #define z_exp  z_exp_f2c 
         #define z_getc  z_getc_f2c 
         #define z_log  z_log_f2c 
         #define z_putc  z_putc_f2c 
         #define z_rnew  z_rnew_f2c 
         #define z_sin  z_sin_f2c 
         #define z_sqrt  z_sqrt_f2c 
         #define z_wnew  z_wnew_f2c 

         #define F2C_MANGLING_DONE

      #endif


   #endif

