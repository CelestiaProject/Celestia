/* glext.h */

#ifndef _GLEXT_H_
#define _GLEXT_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _WIN32
#define APIENTRY
#endif

/* EXT_rescale_normal defines from <GL/gl.h> */
#ifndef GL_EXT_rescale_normal
#define GL_RESCALE_NORMAL_EXT               0x803A
#endif

/* EXT_texture_edge_clamp defines from <GL/gl.h> */
#ifndef GL_EXT_texture_edge_clamp
#define GL_CLAMP_TO_EDGE_EXT                ((GLenum) 0x812F)
#endif

/* EXT_bgra defines from <GL/gl.h> */
#ifndef GL_EXT_bgra
#define GL_BGR_EXT                          0x80E0
#define GL_BGRA_EXT                         0x80E1
#endif

/* GL_ARB_texture_compression */
#ifndef GL_ARB_texture_compression
#define GL_ARB_texture_compression 1

#define GL_COMPRESSED_ALPHA_ARB           0x84E9
#define GL_COMPRESSED_LUMINANCE_ARB       0x84EA
#define GL_COMPRESSED_LUMINANCE_ALPHA_ARB 0x84EB
#define GL_COMPRESSED_INTENSITY_ARB       0x84EC
#define GL_COMPRESSED_RGB_ARB             0x84ED
#define GL_COMPRESSED_RGBA_ARB            0x84EE
#define GL_TEXTURE_COMPRESSION_HINT_ARB   0x84EF
#define GL_TEXTURE_IMAGE_SIZE_ARB         0x86A0
#define GL_TEXTURE_COMPRESSED_ARB         0x86A1
#define GL_NUM_COMPRESSED_TEXTURE_FORMATS_ARB 0x86A2
#define GL_COMPRESSED_TEXTURE_FORMATS_ARB 0x86A3

typedef void (APIENTRY * PFNGLCOMPRESSEDTEXIMAGE3DARBPROC) (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const GLvoid *data);
typedef void (APIENTRY * PFNGLCOMPRESSEDTEXIMAGE2DARBPROC) (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid *data);
typedef void (APIENTRY * PFNGLCOMPRESSEDTEXIMAGE1DARBPROC) (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const GLvoid *data);
typedef void (APIENTRY * PFNGLCOMPRESSEDTEXSUBIMAGE3DARBPROC) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const GLvoid *data);
typedef void (APIENTRY * PFNGLCOMPRESSEDTEXSUBIMAGE2DARBPROC) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid *data);
typedef void (APIENTRY * PFNGLCOMPRESSEDTEXSUBIMAGE1DARBPROC) (GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const GLvoid *data);
typedef void (APIENTRY * PFNGLGETCOMPRESSEDTEXIMAGEARBPROC) (GLenum target, GLint level, void *img);

extern PFNGLCOMPRESSEDTEXIMAGE3DARBPROC glCompressedTexImage3DARB;
extern PFNGLCOMPRESSEDTEXIMAGE2DARBPROC glCompressedTexImage2DARB;
extern PFNGLCOMPRESSEDTEXIMAGE1DARBPROC glCompressedTexImage1DARB;
extern PFNGLCOMPRESSEDTEXSUBIMAGE3DARBPROC glCompressedTexSubImage3DARB;
extern PFNGLCOMPRESSEDTEXSUBIMAGE2DARBPROC glCompressedTexSubImage2DARB;
extern PFNGLCOMPRESSEDTEXSUBIMAGE1DARBPROC glCompressedTexSubImage1DARB;
#endif

/* GL_EXT_texture_compression_s3tc */
#ifndef GL_EXT_texture_compression_s3tc
#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT   0x83F0
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT  0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT  0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT  0x83F3
#endif

/* ARB_multitexture defines and prototypes from <GL/gl.h> */
#ifndef GL_ARB_multitexture
#define GL_ACTIVE_TEXTURE_ARB               0x84E0
#define GL_CLIENT_ACTIVE_TEXTURE_ARB        0x84E1
#define GL_MAX_TEXTURE_UNITS_ARB            0x84E2
#define GL_TEXTURE0_ARB                     0x84C0
#define GL_TEXTURE1_ARB                     0x84C1
#define GL_TEXTURE2_ARB                     0x84C2
#define GL_TEXTURE3_ARB                     0x84C3
typedef void (APIENTRY * PFNGLMULTITEXCOORD1DARBPROC) (GLenum target, GLdouble s);
typedef void (APIENTRY * PFNGLMULTITEXCOORD1DVARBPROC) (GLenum target, const GLdouble *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD1FARBPROC) (GLenum target, GLfloat s);
typedef void (APIENTRY * PFNGLMULTITEXCOORD1FVARBPROC) (GLenum target, const GLfloat *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD1IARBPROC) (GLenum target, GLint s);
typedef void (APIENTRY * PFNGLMULTITEXCOORD1IVARBPROC) (GLenum target, const GLint *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD1SARBPROC) (GLenum target, GLshort s);
typedef void (APIENTRY * PFNGLMULTITEXCOORD1SVARBPROC) (GLenum target, const GLshort *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD2DARBPROC) (GLenum target, GLdouble s, GLdouble t);
typedef void (APIENTRY * PFNGLMULTITEXCOORD2DVARBPROC) (GLenum target, const GLdouble *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD2FARBPROC) (GLenum target, GLfloat s, GLfloat t);
typedef void (APIENTRY * PFNGLMULTITEXCOORD2FVARBPROC) (GLenum target, const GLfloat *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD2IARBPROC) (GLenum target, GLint s, GLint t);
typedef void (APIENTRY * PFNGLMULTITEXCOORD2IVARBPROC) (GLenum target, const GLint *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD2SARBPROC) (GLenum target, GLshort s, GLshort t);
typedef void (APIENTRY * PFNGLMULTITEXCOORD2SVARBPROC) (GLenum target, const GLshort *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD3DARBPROC) (GLenum target, GLdouble s, GLdouble t, GLdouble r);
typedef void (APIENTRY * PFNGLMULTITEXCOORD3DVARBPROC) (GLenum target, const GLdouble *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD3FARBPROC) (GLenum target, GLfloat s, GLfloat t, GLfloat r);
typedef void (APIENTRY * PFNGLMULTITEXCOORD3FVARBPROC) (GLenum target, const GLfloat *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD3IARBPROC) (GLenum target, GLint s, GLint t, GLint r);
typedef void (APIENTRY * PFNGLMULTITEXCOORD3IVARBPROC) (GLenum target, const GLint *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD3SARBPROC) (GLenum target, GLshort s, GLshort t, GLshort r);
typedef void (APIENTRY * PFNGLMULTITEXCOORD3SVARBPROC) (GLenum target, const GLshort *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD4DARBPROC) (GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q);
typedef void (APIENTRY * PFNGLMULTITEXCOORD4DVARBPROC) (GLenum target, const GLdouble *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD4FARBPROC) (GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q);
typedef void (APIENTRY * PFNGLMULTITEXCOORD4FVARBPROC) (GLenum target, const GLfloat *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD4IARBPROC) (GLenum target, GLint s, GLint t, GLint r, GLint q);
typedef void (APIENTRY * PFNGLMULTITEXCOORD4IVARBPROC) (GLenum target, const GLint *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD4SARBPROC) (GLenum target, GLshort s, GLshort t, GLshort r, GLshort q);
typedef void (APIENTRY * PFNGLMULTITEXCOORD4SVARBPROC) (GLenum target, const GLshort *v);
typedef void (APIENTRY * PFNGLACTIVETEXTUREARBPROC) (GLenum target);
typedef void (APIENTRY * PFNGLCLIENTACTIVETEXTUREARBPROC) (GLenum target);

extern PFNGLMULTITEXCOORD2IARBPROC glMultiTexCoord2iARB;
extern PFNGLMULTITEXCOORD2FARBPROC glMultiTexCoord2fARB;
extern PFNGLMULTITEXCOORD3FARBPROC glMultiTexCoord3fARB;
extern PFNGLMULTITEXCOORD3FVARBPROC glMultiTexCoord3fvARB;
extern PFNGLACTIVETEXTUREARBPROC glActiveTextureARB;
extern PFNGLCLIENTACTIVETEXTUREARBPROC glClientActiveTextureARB;
#endif

/* EXT_texture_cube_map defines from <GL/gl.h> */
#if !(defined( GL_EXT_texture_cube_map) || defined(__glext_h_))
#define GL_NORMAL_MAP_EXT                   ((GLenum) 0x8511)
#define GL_REFLECTION_MAP_EXT               ((GLenum) 0x8512)
#define GL_TEXTURE_CUBE_MAP_EXT             ((GLenum) 0x8513)
#define GL_TEXTURE_BINDING_CUBE_MAP_EXT     ((GLenum) 0x8514)
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X_EXT  ((GLenum) 0x8515)
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_X_EXT  ((GLenum) 0x8516)
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Y_EXT  ((GLenum) 0x8517)
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_EXT  ((GLenum) 0x8518)
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Z_EXT  ((GLenum) 0x8519)
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_EXT  ((GLenum) 0x851A)
#define GL_PROXY_TEXTURE_CUBE_MAP_EXT       ((GLenum) 0x851B)
#define GL_MAX_CUBE_MAP_TEXTURE_SIZE_EXT    ((GLenum) 0x851C)
#endif

/* EXT_separate_specular_color defines from <GL/gl.h> */
#ifndef GL_EXT_separate_specular_color
#define GL_LIGHT_MODEL_COLOR_CONTROL_EXT     0x81F8
#define GL_SINGLE_COLOR_EXT                  0x81F9
#define GL_SEPARATE_SPECULAR_COLOR_EXT       0x81FA
#endif

/* EXT_texture_env_combine defines from <GL/gl.h> */
#ifndef GL_EXT_texture_env_combine

// Accepted by the <params> parameter of TexEnvf, TexEnvi, TexEnvfv,
// and TexEnviv when the <pname> parameter value is TEXTURE_ENV_MODE
#define GL_COMBINE_EXT         0x8570

// Accepted by the <pname> parameter of TexEnvf, TexEnvi, TexEnvfv,
// and TexEnviv when the <target> parameter value is TEXTURE_ENV
#define GL_COMBINE_RGB_EXT     0x8571
#define GL_COMBINE_ALPHA_EXT   0x8572
#define GL_SOURCE0_RGB_EXT     0x8580
#define GL_SOURCE1_RGB_EXT     0x8581
#define GL_SOURCE2_RGB_EXT     0x8582
#define GL_SOURCE0_ALPHA_EXT   0x8588
#define GL_SOURCE1_ALPHA_EXT   0x8589
#define GL_SOURCE2_ALPHA_EXT   0x858A
#define GL_OPERAND0_RGB_EXT    0x8590
#define GL_OPERAND1_RGB_EXT    0x8591
#define GL_OPERAND2_RGB_EXT    0x8592
#define GL_OPERAND0_ALPHA_EXT  0x8598
#define GL_OPERAND1_ALPHA_EXT  0x8599
#define GL_OPERAND2_ALPHA_EXT  0x859A
#define GL_RGB_SCALE_EXT       0x8573

// Accepted by the <params> parameter of TexEnvf, TexEnvi, TexEnvfv,
// and TexEnviv when the <pname> parameter value is COMBINE_RGB_EXT
// or COMBINE_ALPHA_EXT
#define GL_ADD_SIGNED_EXT      0x8574
#define GL_INTERPOLATE_EXT     0x8575

// Accepted by the <params> parameter of TexEnvf, TexEnvi, TexEnvfv,
// and TexEnviv when the <pname> parameter value is SOURCE0_RGB_EXT,
// SOURCE1_RGB_EXT, SOURCE2_RGB_EXT, SOURCE0_ALPHA_EXT,
// SOURCE1_ALPHA_EXT, or SOURCE2_ALPHA_EXT
#define GL_CONSTANT_EXT        0x8576
#define GL_PRIMARY_COLOR_EXT   0x8577
#define GL_PREVIOUS_EXT        0x8578

#endif // GL_EXT_texture_env_combine


#ifndef GL_SGIS_generate_mipmap
#define GL_GENERATE_MIPMAP_SGIS           0x8191
#define GL_GENERATE_MIPMAP_HINT_SGIS      0x8192
#endif


#ifndef GL_EXT_texture_lod_bias
#define GL_MAX_TEXTURE_LOD_BIAS_EXT       0x84FD
#define GL_TEXTURE_FILTER_CONTROL_EXT     0x8500
#define GL_TEXTURE_LOD_BIAS_EXT           0x8501
#endif


/* NV_register_combiners defines and prototypes from <GL/gl.h> */
#ifndef GL_NV_register_combiners
#define GL_REGISTER_COMBINERS_NV            ((GLenum) 0x8522)
#define GL_COMBINER0_NV                     ((GLenum) 0x8550)
#define GL_COMBINER1_NV                     ((GLenum) 0x8551)
#define GL_COMBINER2_NV                     ((GLenum) 0x8552)
#define GL_COMBINER3_NV                     ((GLenum) 0x8553)
#define GL_COMBINER4_NV                     ((GLenum) 0x8554)
#define GL_COMBINER5_NV                     ((GLenum) 0x8555)
#define GL_COMBINER6_NV                     ((GLenum) 0x8556)
#define GL_COMBINER7_NV                     ((GLenum) 0x8557)
#define GL_VARIABLE_A_NV                    ((GLenum) 0x8523)
#define GL_VARIABLE_B_NV                    ((GLenum) 0x8524)
#define GL_VARIABLE_C_NV                    ((GLenum) 0x8525)
#define GL_VARIABLE_D_NV                    ((GLenum) 0x8526)
#define GL_VARIABLE_E_NV                    ((GLenum) 0x8527)
#define GL_VARIABLE_F_NV                    ((GLenum) 0x8528)
#define GL_VARIABLE_G_NV                    ((GLenum) 0x8529)
/*      GL_ZERO */
#define GL_CONSTANT_COLOR0_NV               ((GLenum) 0x852A)
#define GL_CONSTANT_COLOR1_NV               ((GLenum) 0x852B)
/*      GL_FOG */
#define GL_PRIMARY_COLOR_NV                 ((GLenum) 0x852C)
#define GL_SECONDARY_COLOR_NV               ((GLenum) 0x852D)
#define GL_SPARE0_NV                        ((GLenum) 0x852E)
#define GL_SPARE1_NV                        ((GLenum) 0x852F)
/*      GL_TEXTURE0_ARB */
/*      GL_TEXTURE1_ARB */
#define GL_UNSIGNED_IDENTITY_NV             ((GLenum) 0x8536)
#define GL_UNSIGNED_INVERT_NV               ((GLenum) 0x8537)
#define GL_EXPAND_NORMAL_NV                 ((GLenum) 0x8538)
#define GL_EXPAND_NEGATE_NV                 ((GLenum) 0x8539)
#define GL_HALF_BIAS_NORMAL_NV              ((GLenum) 0x853A)
#define GL_HALF_BIAS_NEGATE_NV              ((GLenum) 0x853B)
#define GL_SIGNED_IDENTITY_NV               ((GLenum) 0x853C)
#define GL_SIGNED_NEGATE_NV                 ((GLenum) 0x853D)
#define GL_E_TIMES_F_NV                     ((GLenum) 0x8531)
#define GL_SPARE0_PLUS_SECONDARY_COLOR_NV   ((GLenum) 0x8532)
/*      GL_NONE */
#define GL_SCALE_BY_TWO_NV                  ((GLenum) 0x853E)
#define GL_SCALE_BY_FOUR_NV                 ((GLenum) 0x853F)
#define GL_SCALE_BY_ONE_HALF_NV             ((GLenum) 0x8540)
#define GL_BIAS_BY_NEGATIVE_ONE_HALF_NV     ((GLenum) 0x8541)
#define GL_DISCARD_NV                       ((GLenum) 0x8530)
#define GL_COMBINER_INPUT_NV                ((GLenum) 0x8542)
#define GL_COMBINER_MAPPING_NV              ((GLenum) 0x8543)
#define GL_COMBINER_COMPONENT_USAGE_NV      ((GLenum) 0x8544)
#define GL_COMBINER_AB_DOT_PRODUCT_NV       ((GLenum) 0x8545)
#define GL_COMBINER_CD_DOT_PRODUCT_NV       ((GLenum) 0x8546)
#define GL_COMBINER_MUX_SUM_NV              ((GLenum) 0x8547)
#define GL_COMBINER_SCALE_NV                ((GLenum) 0x8548)
#define GL_COMBINER_BIAS_NV                 ((GLenum) 0x8549)
#define GL_COMBINER_AB_OUTPUT_NV            ((GLenum) 0x854A)
#define GL_COMBINER_CD_OUTPUT_NV            ((GLenum) 0x854B)
#define GL_COMBINER_SUM_OUTPUT_NV           ((GLenum) 0x854C)
#define GL_MAX_GENERAL_COMBINERS_NV         ((GLenum) 0x854D)
#define GL_NUM_GENERAL_COMBINERS_NV         ((GLenum) 0x854E)
#define GL_COLOR_SUM_CLAMP_NV               ((GLenum) 0x854F)

typedef void (APIENTRY * PFNGLCOMBINERPARAMETERFVNVPROC) (GLenum pname, const GLfloat *params);
typedef void (APIENTRY * PFNGLCOMBINERPARAMETERFNVPROC) (GLenum pname, GLfloat param);
typedef void (APIENTRY * PFNGLCOMBINERPARAMETERIVNVPROC) (GLenum pname, const GLint *params);
typedef void (APIENTRY * PFNGLCOMBINERPARAMETERINVPROC) (GLenum pname, GLint param);
typedef void (APIENTRY * PFNGLCOMBINERINPUTNVPROC) (GLenum stage, GLenum portion, GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage);
typedef void (APIENTRY * PFNGLCOMBINEROUTPUTNVPROC) (GLenum stage, GLenum portion, GLenum abOutput, GLenum cdOutput, GLenum sumOutput, GLenum scale, GLenum bias, GLboolean abDotProduct, GLboolean cdDotProduct, GLboolean muxSum);
typedef void (APIENTRY * PFNGLFINALCOMBINERINPUTNVPROC) (GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage);
typedef void (APIENTRY * PFNGLGETCOMBINERINPUTPARAMETERFVNVPROC) (GLenum stage, GLenum portion, GLenum variable, GLenum pname, GLfloat *params);
typedef void (APIENTRY * PFNGLGETCOMBINERINPUTPARAMETERIVNVPROC) (GLenum stage, GLenum portion, GLenum variable, GLenum pname, GLint *params);
typedef void (APIENTRY * PFNGLGETCOMBINEROUTPUTPARAMETERFVNVPROC) (GLenum stage, GLenum portion, GLenum pname, GLfloat *params);
typedef void (APIENTRY * PFNGLGETCOMBINEROUTPUTPARAMETERIVNVPROC) (GLenum stage, GLenum portion, GLenum pname, GLint *params);
typedef void (APIENTRY * PFNGLGETFINALCOMBINERINPUTPARAMETERFVNVPROC) (GLenum variable, GLenum pname, GLfloat *params);
typedef void (APIENTRY * PFNGLGETFINALCOMBINERINPUTPARAMETERIVNVPROC) (GLenum variable, GLenum pname, GLint *params);

/* NV_register_combiners command function pointers */
extern PFNGLCOMBINERPARAMETERFVNVPROC glCombinerParameterfvNV;
extern PFNGLCOMBINERPARAMETERIVNVPROC glCombinerParameterivNV;
extern PFNGLCOMBINERPARAMETERFNVPROC glCombinerParameterfNV;
extern PFNGLCOMBINERPARAMETERINVPROC glCombinerParameteriNV;
extern PFNGLCOMBINERINPUTNVPROC glCombinerInputNV;
extern PFNGLCOMBINEROUTPUTNVPROC glCombinerOutputNV;
extern PFNGLFINALCOMBINERINPUTNVPROC glFinalCombinerInputNV;
extern PFNGLGETCOMBINERINPUTPARAMETERFVNVPROC glGetCombinerInputParameterfvNV;
extern PFNGLGETCOMBINERINPUTPARAMETERIVNVPROC glGetCombinerInputParameterivNV;
extern PFNGLGETCOMBINEROUTPUTPARAMETERFVNVPROC glGetCombinerOutputParameterfvNV;
extern PFNGLGETCOMBINEROUTPUTPARAMETERIVNVPROC glGetCombinerOutputParameterivNV;
extern PFNGLGETFINALCOMBINERINPUTPARAMETERFVNVPROC glGetFinalCombinerInputParameterfvNV;
extern PFNGLGETFINALCOMBINERINPUTPARAMETERIVNVPROC glGetFinalCombinerInputParameterivNV;

#endif


/* NV_register_combiners2 */
#ifndef GL_NV_register_combiners2
#define GL_PER_STAGE_CONSTANTS_NV         0x8535

typedef void (APIENTRY * PFNGLCOMBINERSTAGEPARAMETERFVNVPROC) (GLenum stage, GLenum pname, const GLfloat *params);
typedef void (APIENTRY * PFNGLGETCOMBINERSTAGEPARAMETERFVNVPROC) (GLenum stage, GLenum pname, GLfloat *params);

extern PFNGLCOMBINERSTAGEPARAMETERFVNVPROC glCombinerStageParameterfvNV;
extern PFNGLGETCOMBINERSTAGEPARAMETERFVNVPROC glGetCombinerStageParameterfvNV;

#endif


/* NV_texture_shader */
#ifndef GL_NV_texture_shader
#define GL_NV_texture_shader

#define GL_RGBA_UNSIGNED_DOT_PRODUCT_MAPPING_NV 0x86D9
#define GL_UNSIGNED_INT_S8_S8_8_8_NV      0x86DA
#define GL_UNSIGNED_INT_S8_S8_8_8_REV_NV  0x86DB
#define GL_DSDT_MAG_INTENSITY_NV          0x86DC
#define GL_SHADER_CONSISTENT_NV           0x86DD
#define GL_TEXTURE_SHADER_NV              0x86DE
#define GL_SHADER_OPERATION_NV            0x86DF
#define GL_CULL_MODES_NV                  0x86E0
#define GL_OFFSET_TEXTURE_2D_MATRIX_NV    0x86E1
#define GL_OFFSET_TEXTURE_2D_SCALE_NV     0x86E2
#define GL_OFFSET_TEXTURE_2D_BIAS_NV      0x86E3
#define GL_PREVIOUS_TEXTURE_INPUT_NV      0x86E4
#define GL_CONST_EYE_NV                   0x86E5
#define GL_PASS_THROUGH_NV                0x86E6
#define GL_CULL_FRAGMENT_NV               0x86E7
#define GL_OFFSET_TEXTURE_2D_NV           0x86E8
#define GL_DEPENDENT_AR_TEXTURE_2D_NV     0x86E9
#define GL_DEPENDENT_GB_TEXTURE_2D_NV     0x86EA
#define GL_DOT_PRODUCT_NV                 0x86EC
#define GL_DOT_PRODUCT_DEPTH_REPLACE_NV   0x86ED
#define GL_DOT_PRODUCT_TEXTURE_2D_NV      0x86EE
#define GL_DOT_PRODUCT_TEXTURE_3D_NV      0x86EF
#define GL_DOT_PRODUCT_TEXTURE_CUBE_MAP_NV 0x86F0
#define GL_DOT_PRODUCT_DIFFUSE_CUBE_MAP_NV 0x86F1
#define GL_DOT_PRODUCT_REFLECT_CUBE_MAP_NV 0x86F2
#define GL_DOT_PRODUCT_CONST_EYE_REFLECT_CUBE_MAP_NV 0x86F3
#define GL_HILO_NV                        0x86F4
#define GL_DSDT_NV                        0x86F5
#define GL_DSDT_MAG_NV                    0x86F6
#define GL_DSDT_MAG_VIB_NV                0x86F7
#define GL_HILO16_NV                      0x86F8
#define GL_SIGNED_HILO_NV                 0x86F9
#define GL_SIGNED_HILO16_NV               0x86FA
#define GL_SIGNED_RGBA_NV                 0x86FB
#define GL_SIGNED_RGBA8_NV                0x86FC
#define GL_SIGNED_RGB_NV                  0x86FE
#define GL_SIGNED_RGB8_NV                 0x86FF
#define GL_SIGNED_LUMINANCE_NV            0x8701
#define GL_SIGNED_LUMINANCE8_NV           0x8702
#define GL_SIGNED_LUMINANCE_ALPHA_NV      0x8703
#define GL_SIGNED_LUMINANCE8_ALPHA8_NV    0x8704
#define GL_SIGNED_ALPHA_NV                0x8705
#define GL_SIGNED_ALPHA8_NV               0x8706
#define GL_SIGNED_INTENSITY_NV            0x8707
#define GL_SIGNED_INTENSITY8_NV           0x8708
#define GL_DSDT8_NV                       0x8709
#define GL_DSDT8_MAG8_NV                  0x870A
#define GL_DSDT8_MAG8_INTENSITY8_NV       0x870B
#define GL_SIGNED_RGB_UNSIGNED_ALPHA_NV   0x870C
#define GL_SIGNED_RGB8_UNSIGNED_ALPHA8_NV 0x870D
#define GL_HI_SCALE_NV                    0x870E
#define GL_LO_SCALE_NV                    0x870F
#define GL_DS_SCALE_NV                    0x8710
#define GL_DT_SCALE_NV                    0x8711
#define GL_MAGNITUDE_SCALE_NV             0x8712
#define GL_VIBRANCE_SCALE_NV              0x8713
#define GL_HI_BIAS_NV                     0x8714
#define GL_LO_BIAS_NV                     0x8715
#define GL_DS_BIAS_NV                     0x8716
#define GL_DT_BIAS_NV                     0x8717
#define GL_MAGNITUDE_BIAS_NV              0x8718
#define GL_VIBRANCE_BIAS_NV               0x8719
#define GL_TEXTURE_BORDER_VALUES_NV       0x871A
#define GL_TEXTURE_HI_SIZE_NV             0x871B
#define GL_TEXTURE_LO_SIZE_NV             0x871C
#define GL_TEXTURE_DS_SIZE_NV             0x871D
#define GL_TEXTURE_DT_SIZE_NV             0x871E
#define GL_TEXTURE_MAG_SIZE_NV            0x871F

#endif


/* NV_vertex_program */
#ifndef GL_NV_vertex_program

#define GL_VERTEX_PROGRAM_NV              0x8620
#define GL_VERTEX_STATE_PROGRAM_NV        0x8621
#define GL_ATTRIB_ARRAY_SIZE_NV           0x8623
#define GL_ATTRIB_ARRAY_STRIDE_NV         0x8624
#define GL_ATTRIB_ARRAY_TYPE_NV           0x8625
#define GL_CURRENT_ATTRIB_NV              0x8626
#define GL_PROGRAM_LENGTH_NV              0x8627
#define GL_PROGRAM_STRING_NV              0x8628
#define GL_MODELVIEW_PROJECTION_NV        0x8629
#define GL_IDENTITY_NV                    0x862A
#define GL_INVERSE_NV                     0x862B
#define GL_TRANSPOSE_NV                   0x862C
#define GL_INVERSE_TRANSPOSE_NV           0x862D
#define GL_MAX_TRACK_MATRIX_STACK_DEPTH_NV 0x862E
#define GL_MAX_TRACK_MATRICES_NV          0x862F
#define GL_MATRIX0_NV                     0x8630
#define GL_MATRIX1_NV                     0x8631
#define GL_MATRIX2_NV                     0x8632
#define GL_MATRIX3_NV                     0x8633
#define GL_MATRIX4_NV                     0x8634
#define GL_MATRIX5_NV                     0x8635
#define GL_MATRIX6_NV                     0x8636
#define GL_MATRIX7_NV                     0x8637
#define GL_CURRENT_MATRIX_STACK_DEPTH_NV  0x8640
#define GL_CURRENT_MATRIX_NV              0x8641
#define GL_VERTEX_PROGRAM_POINT_SIZE_NV   0x8642
#define GL_VERTEX_PROGRAM_TWO_SIDE_NV     0x8643
#define GL_PROGRAM_PARAMETER_NV           0x8644
#define GL_ATTRIB_ARRAY_POINTER_NV        0x8645
#define GL_PROGRAM_TARGET_NV              0x8646
#define GL_PROGRAM_RESIDENT_NV            0x8647
#define GL_TRACK_MATRIX_NV                0x8648
#define GL_TRACK_MATRIX_TRANSFORM_NV      0x8649
#define GL_VERTEX_PROGRAM_BINDING_NV      0x864A
#define GL_PROGRAM_ERROR_POSITION_NV      0x864B
#define GL_VERTEX_ATTRIB_ARRAY0_NV        0x8650
#define GL_VERTEX_ATTRIB_ARRAY1_NV        0x8651
#define GL_VERTEX_ATTRIB_ARRAY2_NV        0x8652
#define GL_VERTEX_ATTRIB_ARRAY3_NV        0x8653
#define GL_VERTEX_ATTRIB_ARRAY4_NV        0x8654
#define GL_VERTEX_ATTRIB_ARRAY5_NV        0x8655
#define GL_VERTEX_ATTRIB_ARRAY6_NV        0x8656
#define GL_VERTEX_ATTRIB_ARRAY7_NV        0x8657
#define GL_VERTEX_ATTRIB_ARRAY8_NV        0x8658
#define GL_VERTEX_ATTRIB_ARRAY9_NV        0x8659
#define GL_VERTEX_ATTRIB_ARRAY10_NV       0x865A
#define GL_VERTEX_ATTRIB_ARRAY11_NV       0x865B
#define GL_VERTEX_ATTRIB_ARRAY12_NV       0x865C
#define GL_VERTEX_ATTRIB_ARRAY13_NV       0x865D
#define GL_VERTEX_ATTRIB_ARRAY14_NV       0x865E
#define GL_VERTEX_ATTRIB_ARRAY15_NV       0x865F
#define GL_MAP1_VERTEX_ATTRIB0_4_NV       0x8660
#define GL_MAP1_VERTEX_ATTRIB1_4_NV       0x8661
#define GL_MAP1_VERTEX_ATTRIB2_4_NV       0x8662
#define GL_MAP1_VERTEX_ATTRIB3_4_NV       0x8663
#define GL_MAP1_VERTEX_ATTRIB4_4_NV       0x8664
#define GL_MAP1_VERTEX_ATTRIB5_4_NV       0x8665
#define GL_MAP1_VERTEX_ATTRIB6_4_NV       0x8666
#define GL_MAP1_VERTEX_ATTRIB7_4_NV       0x8667
#define GL_MAP1_VERTEX_ATTRIB8_4_NV       0x8668
#define GL_MAP1_VERTEX_ATTRIB9_4_NV       0x8669
#define GL_MAP1_VERTEX_ATTRIB10_4_NV      0x866A
#define GL_MAP1_VERTEX_ATTRIB11_4_NV      0x866B
#define GL_MAP1_VERTEX_ATTRIB12_4_NV      0x866C
#define GL_MAP1_VERTEX_ATTRIB13_4_NV      0x866D
#define GL_MAP1_VERTEX_ATTRIB14_4_NV      0x866E
#define GL_MAP1_VERTEX_ATTRIB15_4_NV      0x866F
#define GL_MAP2_VERTEX_ATTRIB0_4_NV       0x8670
#define GL_MAP2_VERTEX_ATTRIB1_4_NV       0x8671
#define GL_MAP2_VERTEX_ATTRIB2_4_NV       0x8672
#define GL_MAP2_VERTEX_ATTRIB3_4_NV       0x8673
#define GL_MAP2_VERTEX_ATTRIB4_4_NV       0x8674
#define GL_MAP2_VERTEX_ATTRIB5_4_NV       0x8675
#define GL_MAP2_VERTEX_ATTRIB6_4_NV       0x8676
#define GL_MAP2_VERTEX_ATTRIB7_4_NV       0x8677
#define GL_MAP2_VERTEX_ATTRIB8_4_NV       0x8678
#define GL_MAP2_VERTEX_ATTRIB9_4_NV       0x8679
#define GL_MAP2_VERTEX_ATTRIB10_4_NV      0x867A
#define GL_MAP2_VERTEX_ATTRIB11_4_NV      0x867B
#define GL_MAP2_VERTEX_ATTRIB12_4_NV      0x867C
#define GL_MAP2_VERTEX_ATTRIB13_4_NV      0x867D
#define GL_MAP2_VERTEX_ATTRIB14_4_NV      0x867E
#define GL_MAP2_VERTEX_ATTRIB15_4_NV      0x867F

typedef GLboolean (APIENTRY * PFNGLAREPROGRAMSRESIDENTNVPROC) (GLsizei n, const GLuint *programs, GLboolean *residences);
typedef void (APIENTRY * PFNGLBINDPROGRAMNVPROC) (GLenum target, GLuint id);
typedef void (APIENTRY * PFNGLDELETEPROGRAMSNVPROC) (GLsizei n, const GLuint *programs);
typedef void (APIENTRY * PFNGLEXECUTEPROGRAMNVPROC) (GLenum target, GLuint id, const GLfloat *params);
typedef void (APIENTRY * PFNGLGENPROGRAMSNVPROC) (GLsizei n, GLuint *programs);
typedef void (APIENTRY * PFNGLGETPROGRAMPARAMETERDVNVPROC) (GLenum target, GLuint index, GLenum pname, GLdouble *params);
typedef void (APIENTRY * PFNGLGETPROGRAMPARAMETERFVNVPROC) (GLenum target, GLuint index, GLenum pname, GLfloat *params);
typedef void (APIENTRY * PFNGLGETPROGRAMIVNVPROC) (GLuint id, GLenum pname, GLint *params);
typedef void (APIENTRY * PFNGLGETPROGRAMSTRINGNVPROC) (GLuint id, GLenum pname, GLubyte *program);
typedef void (APIENTRY * PFNGLGETTRACKMATRIXIVNVPROC) (GLenum target, GLuint address, GLenum pname, GLint *params);
typedef void (APIENTRY * PFNGLGETVERTEXATTRIBDVNVPROC) (GLuint index, GLenum pname, GLdouble *params);
typedef void (APIENTRY * PFNGLGETVERTEXATTRIBFVNVPROC) (GLuint index, GLenum pname, GLfloat *params);
typedef void (APIENTRY * PFNGLGETVERTEXATTRIBIVNVPROC) (GLuint index, GLenum pname, GLint *params);
typedef void (APIENTRY * PFNGLGETVERTEXATTRIBPOINTERVNVPROC) (GLuint index, GLenum pname, GLvoid* *pointer);
typedef GLboolean (APIENTRY * PFNGLISPROGRAMNVPROC) (GLuint id);
typedef void (APIENTRY * PFNGLLOADPROGRAMNVPROC) (GLenum target, GLuint id, GLsizei len, const GLubyte *program);
typedef void (APIENTRY * PFNGLPROGRAMPARAMETER4DNVPROC) (GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
typedef void (APIENTRY * PFNGLPROGRAMPARAMETER4DVNVPROC) (GLenum target, GLuint index, const GLdouble *v);
typedef void (APIENTRY * PFNGLPROGRAMPARAMETER4FNVPROC) (GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
typedef void (APIENTRY * PFNGLPROGRAMPARAMETER4FVNVPROC) (GLenum target, GLuint index, const GLfloat *v);
typedef void (APIENTRY * PFNGLPROGRAMPARAMETERS4DVNVPROC) (GLenum target, GLuint index, GLsizei count, const GLdouble *v);
typedef void (APIENTRY * PFNGLPROGRAMPARAMETERS4FVNVPROC) (GLenum target, GLuint index, GLsizei count, const GLfloat *v);
typedef void (APIENTRY * PFNGLREQUESTRESIDENTPROGRAMSNVPROC) (GLsizei n, const GLuint *programs);
typedef void (APIENTRY * PFNGLTRACKMATRIXNVPROC) (GLenum target, GLuint address, GLenum matrix, GLenum transform);
typedef void (APIENTRY * PFNGLVERTEXATTRIBPOINTERNVPROC) (GLuint index, GLint fsize, GLenum type, GLsizei stride, const GLvoid *pointer);
typedef void (APIENTRY * PFNGLVERTEXATTRIB1DNVPROC) (GLuint index, GLdouble x);
typedef void (APIENTRY * PFNGLVERTEXATTRIB1DVNVPROC) (GLuint index, const GLdouble *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIB1FNVPROC) (GLuint index, GLfloat x);
typedef void (APIENTRY * PFNGLVERTEXATTRIB1FVNVPROC) (GLuint index, const GLfloat *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIB1SNVPROC) (GLuint index, GLshort x);
typedef void (APIENTRY * PFNGLVERTEXATTRIB1SVNVPROC) (GLuint index, const GLshort *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIB2DNVPROC) (GLuint index, GLdouble x, GLdouble y);
typedef void (APIENTRY * PFNGLVERTEXATTRIB2DVNVPROC) (GLuint index, const GLdouble *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIB2FNVPROC) (GLuint index, GLfloat x, GLfloat y);
typedef void (APIENTRY * PFNGLVERTEXATTRIB2FVNVPROC) (GLuint index, const GLfloat *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIB2SNVPROC) (GLuint index, GLshort x, GLshort y);
typedef void (APIENTRY * PFNGLVERTEXATTRIB2SVNVPROC) (GLuint index, const GLshort *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIB3DNVPROC) (GLuint index, GLdouble x, GLdouble y, GLdouble z);
typedef void (APIENTRY * PFNGLVERTEXATTRIB3DVNVPROC) (GLuint index, const GLdouble *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIB3FNVPROC) (GLuint index, GLfloat x, GLfloat y, GLfloat z);
typedef void (APIENTRY * PFNGLVERTEXATTRIB3FVNVPROC) (GLuint index, const GLfloat *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIB3SNVPROC) (GLuint index, GLshort x, GLshort y, GLshort z);
typedef void (APIENTRY * PFNGLVERTEXATTRIB3SVNVPROC) (GLuint index, const GLshort *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIB4DNVPROC) (GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
typedef void (APIENTRY * PFNGLVERTEXATTRIB4DVNVPROC) (GLuint index, const GLdouble *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIB4FNVPROC) (GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
typedef void (APIENTRY * PFNGLVERTEXATTRIB4FVNVPROC) (GLuint index, const GLfloat *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIB4SNVPROC) (GLuint index, GLshort x, GLshort y, GLshort z, GLshort w);
typedef void (APIENTRY * PFNGLVERTEXATTRIB4SVNVPROC) (GLuint index, const GLshort *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIB4UBVNVPROC) (GLuint index, const GLubyte *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIBS1DVNVPROC) (GLuint index, GLsizei count, const GLdouble *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIBS1FVNVPROC) (GLuint index, GLsizei count, const GLfloat *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIBS1SVNVPROC) (GLuint index, GLsizei count, const GLshort *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIBS2DVNVPROC) (GLuint index, GLsizei count, const GLdouble *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIBS2FVNVPROC) (GLuint index, GLsizei count, const GLfloat *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIBS2SVNVPROC) (GLuint index, GLsizei count, const GLshort *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIBS3DVNVPROC) (GLuint index, GLsizei count, const GLdouble *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIBS3FVNVPROC) (GLuint index, GLsizei count, const GLfloat *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIBS3SVNVPROC) (GLuint index, GLsizei count, const GLshort *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIBS4DVNVPROC) (GLuint index, GLsizei count, const GLdouble *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIBS4FVNVPROC) (GLuint index, GLsizei count, const GLfloat *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIBS4SVNVPROC) (GLuint index, GLsizei count, const GLshort *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIBS4UBVNVPROC) (GLuint index, GLsizei count, const GLubyte *v);

extern PFNGLAREPROGRAMSRESIDENTNVPROC glAreProgramsResidentNV ;
extern PFNGLBINDPROGRAMNVPROC glBindProgramNV ;
extern PFNGLDELETEPROGRAMSNVPROC glDeleteProgramsNV ;
extern PFNGLEXECUTEPROGRAMNVPROC glExecuteProgramNV ;
extern PFNGLGENPROGRAMSNVPROC glGenProgramsNV ;
extern PFNGLGETPROGRAMPARAMETERDVNVPROC glGetProgramParameterdvNV ;
extern PFNGLGETPROGRAMPARAMETERFVNVPROC glGetProgramParameterfvNV ;
extern PFNGLGETPROGRAMIVNVPROC glGetProgramivNV ;
extern PFNGLGETPROGRAMSTRINGNVPROC glGetProgramStringNV ;
extern PFNGLGETTRACKMATRIXIVNVPROC glGetTrackMatrixivNV ;
extern PFNGLGETVERTEXATTRIBDVNVPROC glGetVertexAttribdvNV ;
extern PFNGLGETVERTEXATTRIBFVNVPROC glGetVertexAttribfvNV ;
extern PFNGLGETVERTEXATTRIBIVNVPROC glGetVertexAttribivNV ;
extern PFNGLGETVERTEXATTRIBPOINTERVNVPROC glGetVertexAttribPointervNV ;
extern PFNGLISPROGRAMNVPROC glIsProgramNV ;
extern PFNGLLOADPROGRAMNVPROC glLoadProgramNV ;
extern PFNGLPROGRAMPARAMETER4DNVPROC glProgramParameter4dNV ;
extern PFNGLPROGRAMPARAMETER4DVNVPROC glProgramParameter4dvNV ;
extern PFNGLPROGRAMPARAMETER4FNVPROC glProgramParameter4fNV ;
extern PFNGLPROGRAMPARAMETER4FVNVPROC glProgramParameter4fvNV ;
extern PFNGLPROGRAMPARAMETERS4DVNVPROC glProgramParameters4dvNV ;
extern PFNGLPROGRAMPARAMETERS4FVNVPROC glProgramParameters4fvNV ;
extern PFNGLREQUESTRESIDENTPROGRAMSNVPROC glRequestResidentProgramsNV ;
extern PFNGLTRACKMATRIXNVPROC glTrackMatrixNV ;
extern PFNGLVERTEXATTRIBPOINTERNVPROC glVertexAttribPointerNV ;
extern PFNGLVERTEXATTRIB1DNVPROC glVertexAttrib1dNV ;
extern PFNGLVERTEXATTRIB1DVNVPROC glVertexAttrib1dvNV ;
extern PFNGLVERTEXATTRIB1FNVPROC glVertexAttrib1fNV ;
extern PFNGLVERTEXATTRIB1FVNVPROC glVertexAttrib1fvNV ;
extern PFNGLVERTEXATTRIB1SNVPROC glVertexAttrib1sNV ;
extern PFNGLVERTEXATTRIB1SVNVPROC glVertexAttrib1svNV ;
extern PFNGLVERTEXATTRIB2DNVPROC glVertexAttrib2dNV ;
extern PFNGLVERTEXATTRIB2DVNVPROC glVertexAttrib2dvNV ;
extern PFNGLVERTEXATTRIB2FNVPROC glVertexAttrib2fNV ;
extern PFNGLVERTEXATTRIB2FVNVPROC glVertexAttrib2fvNV ;
extern PFNGLVERTEXATTRIB2SNVPROC glVertexAttrib2sNV ;
extern PFNGLVERTEXATTRIB2SVNVPROC glVertexAttrib2svNV ;
extern PFNGLVERTEXATTRIB3DNVPROC glVertexAttrib3dNV ;
extern PFNGLVERTEXATTRIB3DVNVPROC glVertexAttrib3dvNV ;
extern PFNGLVERTEXATTRIB3FNVPROC glVertexAttrib3fNV ;
extern PFNGLVERTEXATTRIB3FVNVPROC glVertexAttrib3fvNV ;
extern PFNGLVERTEXATTRIB3SNVPROC glVertexAttrib3sNV ;
extern PFNGLVERTEXATTRIB3SVNVPROC glVertexAttrib3svNV ;
extern PFNGLVERTEXATTRIB4DNVPROC glVertexAttrib4dNV ;
extern PFNGLVERTEXATTRIB4DVNVPROC glVertexAttrib4dvNV ;
extern PFNGLVERTEXATTRIB4FNVPROC glVertexAttrib4fNV ;
extern PFNGLVERTEXATTRIB4FVNVPROC glVertexAttrib4fvNV ;
extern PFNGLVERTEXATTRIB4SNVPROC glVertexAttrib4sNV ;
extern PFNGLVERTEXATTRIB4SVNVPROC glVertexAttrib4svNV ;
extern PFNGLVERTEXATTRIB4UBVNVPROC glVertexAttrib4ubvNV ;
extern PFNGLVERTEXATTRIBS1DVNVPROC glVertexAttribs1dvNV ;
extern PFNGLVERTEXATTRIBS1FVNVPROC glVertexAttribs1fvNV ;
extern PFNGLVERTEXATTRIBS1SVNVPROC glVertexAttribs1svNV ;
extern PFNGLVERTEXATTRIBS2DVNVPROC glVertexAttribs2dvNV ;
extern PFNGLVERTEXATTRIBS2FVNVPROC glVertexAttribs2fvNV ;
extern PFNGLVERTEXATTRIBS2SVNVPROC glVertexAttribs2svNV ;
extern PFNGLVERTEXATTRIBS3DVNVPROC glVertexAttribs3dvNV ;
extern PFNGLVERTEXATTRIBS3FVNVPROC glVertexAttribs3fvNV ;
extern PFNGLVERTEXATTRIBS3SVNVPROC glVertexAttribs3svNV ;
extern PFNGLVERTEXATTRIBS4DVNVPROC glVertexAttribs4dvNV ;
extern PFNGLVERTEXATTRIBS4FVNVPROC glVertexAttribs4fvNV ;
extern PFNGLVERTEXATTRIBS4SVNVPROC glVertexAttribs4svNV ;
extern PFNGLVERTEXATTRIBS4UBVNVPROC glVertexAttribs4ubvNV ;

#endif


/* EXT_paletted_texture defines and prototypes from <GL/gl.h> */
#ifndef GL_EXT_paletted_texture
#define GL_COLOR_INDEX1_EXT               0x80E2
#define GL_COLOR_INDEX2_EXT               0x80E3
#define GL_COLOR_INDEX4_EXT               0x80E4
#define GL_COLOR_INDEX8_EXT               0x80E5
#define GL_COLOR_INDEX12_EXT              0x80E6
#define GL_COLOR_INDEX16_EXT              0x80E7
typedef void (APIENTRY * PFNGLCOLORTABLEEXTPROC) (GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid * table);

/* EXT_paletted_texture command function pointers */
extern PFNGLCOLORTABLEEXTPROC glColorTableEXT;

#endif

/* EXT_blend_minmax defines and prototypes from <GL/gl.h> */
#ifndef GL_EXT_blend_minmax
#define GL_EXT_blend_minmax
#define GL_FUNC_ADD_EXT                  0x8006
#define GL_MIN_EXT                       0x8007
#define GL_MAX_EXT                       0x8008

typedef void (APIENTRY * PFNGLBLENDEQUATIONEXTPROC) (GLenum mode);

extern PFNGLBLENDEQUATIONEXTPROC glBlendEquationEXT;
#endif


/* EXT_blend_subtract defines and prototypes from <GL/gl.h> */
#ifndef GL_EXT_blend_subtract
#define GL_FUNC_SUBTRACT_EXT             0x800A
#define GL_FUNC_REVERSE_SUBTRACT_EXT     0x800B
#endif

/* WGL_EXT_swap_control defines and prototypes from <GL/gl.h> */
#ifndef WGL_EXT_swap_control
typedef int (APIENTRY * PFNWGLSWAPINTERVALEXTPROC) (int);
typedef int (APIENTRY * PFNWGLGETSWAPINTERVALEXTPROC) (void);
#endif

/* OpenGL 1.2 defines and prototypes from <GL/gl.h> */
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE                    0x812F
#endif


/* EXT_texture_env_dot3 */
#ifndef GL_EXT_texture_env_dot3
#define GL_EXT_texture_env_dot3
#define GL_DOT3_RGB_EXT                     0x8740
#define GL_DOT3_RGBA_EXT                    0x8741
#endif

#ifdef _WIN32

/* WGL_EXT_swap_control command function pointers */
extern PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT;
extern PFNWGLGETSWAPINTERVALEXTPROC wglGetSwapIntervalEXT;

#endif

extern bool ExtensionSupported(char *ext);
extern void InitExtRegisterCombiners();
extern void InitExtRegisterCombiners2();
extern void InitExtMultiTexture();
extern void InitExtPalettedTextures();
extern void InitExtSwapControl();
extern void InitExtTextureCompression();
extern void InitExtBlendMinmax();
extern void InitExtVertexProgram();

#ifdef __cplusplus
}
#endif

#endif // _GLEXT_H_
