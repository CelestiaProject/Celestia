/* glext.h */

#ifndef _GLEXT_H_
#define _GLEXT_H_

#ifndef _WIN32
#define APIENTRY
#endif

/* EXT_rescale_normal defines from <GL/gl.h> */
#ifndef GL_EXT_rescale_normal
#define GL_RESCALE_NORMAL_EXT               0x803A
#endif

/* EXT_texture_edge_clamp defines from <GL/gl.h> */
#ifndef GL_EXT_texture_edge_clamp
#define GL_CLAMP_TO_EDGE_EXT                0x812F
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

#if 0
extern void APIENTRY glCompressedTexImage3DARB (GLenum, GLint, GLenum, GLsizei, GLsizei, GLsizei, GLint, GLsizei, const GLvoid *);
extern void APIENTRY glCompressedTexImage2DARB (GLenum, GLint, GLenum, GLsizei, GLsizei, GLint, GLsizei, const GLvoid *);
extern void APIENTRY glCompressedTexImage1DARB (GLenum, GLint, GLenum, GLsizei, GLint, GLsizei, const GLvoid *);
extern void APIENTRY glCompressedTexSubImage3DARB (GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLsizei, const GLvoid *);
extern void APIENTRY glCompressedTexSubImage2DARB (GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLsizei, const GLvoid *);
extern void APIENTRY glCompressedTexSubImage1DARB (GLenum, GLint, GLint, GLsizei, GLenum, GLsizei, const GLvoid *);
extern void APIENTRY glGetCompressedTexImageARB (GLenum, GLint, void *);
#endif /* GL_GLEXT_PROTOTYPES */
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
#ifndef GL_EXT_texture_cube_map
#define GL_NORMAL_MAP_EXT                   0x8511
#define GL_REFLECTION_MAP_EXT               0x8512
#define GL_TEXTURE_CUBE_MAP_EXT             0x8513
#define GL_TEXTURE_BINDING_CUBE_MAP_EXT     0x8514
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X_EXT  0x8515
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_X_EXT  0x8516
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Y_EXT  0x8517
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_EXT  0x8518
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Z_EXT  0x8519
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_EXT  0x851A
#define GL_PROXY_TEXTURE_CUBE_MAP_EXT       0x851B
#define GL_MAX_CUBE_MAP_TEXTURE_SIZE_EXT    0x851C
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


/* NV_register_combiners defines and prototypes from <GL/gl.h> */
#ifndef GL_NV_register_combiners
#define GL_REGISTER_COMBINERS_NV            0x8522
#define GL_COMBINER0_NV                     0x8550
#define GL_COMBINER1_NV                     0x8551
#define GL_COMBINER2_NV                     0x8552
#define GL_COMBINER3_NV                     0x8553
#define GL_COMBINER4_NV                     0x8554
#define GL_COMBINER5_NV                     0x8555
#define GL_COMBINER6_NV                     0x8556
#define GL_COMBINER7_NV                     0x8557
#define GL_VARIABLE_A_NV                    0x8523
#define GL_VARIABLE_B_NV                    0x8524
#define GL_VARIABLE_C_NV                    0x8525
#define GL_VARIABLE_D_NV                    0x8526
#define GL_VARIABLE_E_NV                    0x8527
#define GL_VARIABLE_F_NV                    0x8528
#define GL_VARIABLE_G_NV                    0x8529
/*      GL_ZERO */
#define GL_CONSTANT_COLOR0_NV               0x852A
#define GL_CONSTANT_COLOR1_NV               0x852B
/*      GL_FOG */
#define GL_PRIMARY_COLOR_NV                 0x852C
#define GL_SECONDARY_COLOR_NV               0x852D
#define GL_SPARE0_NV                        0x852E
#define GL_SPARE1_NV                        0x852F
/*      GL_TEXTURE0_ARB */
/*      GL_TEXTURE1_ARB */
#define GL_UNSIGNED_IDENTITY_NV             0x8536
#define GL_UNSIGNED_INVERT_NV               0x8537
#define GL_EXPAND_NORMAL_NV                 0x8538
#define GL_EXPAND_NEGATE_NV                 0x8539
#define GL_HALF_BIAS_NORMAL_NV              0x853A
#define GL_HALF_BIAS_NEGATE_NV              0x853B
#define GL_SIGNED_IDENTITY_NV               0x853C
#define GL_SIGNED_NEGATE_NV                 0x853D
#define GL_E_TIMES_F_NV                     0x8531
#define GL_SPARE0_PLUS_SECONDARY_COLOR_NV   0x8532
/*      GL_NONE */
#define GL_SCALE_BY_TWO_NV                  0x853E
#define GL_SCALE_BY_FOUR_NV                 0x853F
#define GL_SCALE_BY_ONE_HALF_NV             0x8540
#define GL_BIAS_BY_NEGATIVE_ONE_HALF_NV     0x8541
#define GL_DISCARD_NV                       0x8530
#define GL_COMBINER_INPUT_NV                0x8542
#define GL_COMBINER_MAPPING_NV              0x8543
#define GL_COMBINER_COMPONENT_USAGE_NV      0x8544
#define GL_COMBINER_AB_DOT_PRODUCT_NV       0x8545
#define GL_COMBINER_CD_DOT_PRODUCT_NV       0x8546
#define GL_COMBINER_MUX_SUM_NV              0x8547
#define GL_COMBINER_SCALE_NV                0x8548
#define GL_COMBINER_BIAS_NV                 0x8549
#define GL_COMBINER_AB_OUTPUT_NV            0x854A
#define GL_COMBINER_CD_OUTPUT_NV            0x854B
#define GL_COMBINER_SUM_OUTPUT_NV           0x854C
#define GL_MAX_GENERAL_COMBINERS_NV         0x854D
#define GL_NUM_GENERAL_COMBINERS_NV         0x854E
#define GL_COLOR_SUM_CLAMP_NV               0x854F

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

/* WGL_EXT_swap_control defines and prototypes from <GL/gl.h> */
#ifndef WGL_EXT_swap_control
typedef int (APIENTRY * PFNWGLSWAPINTERVALEXTPROC) (int);
typedef int (APIENTRY * PFNWGLGETSWAPINTERVALEXTPROC) (void);
#endif

/* OpenGL 1.2 defines and prototypes from <GL/gl.h> */
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE                    0x812F
#endif

#ifdef _WIN32

/* WGL_EXT_swap_control command function pointers */
extern PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT;
extern PFNWGLGETSWAPINTERVALEXTPROC wglGetSwapIntervalEXT;

#endif

extern bool ExtensionSupported(char *ext);
extern void InitExtRegisterCombiners();
extern void InitExtMultiTexture();
extern void InitExtPalettedTextures();
extern void InitExtSwapControl();
extern void InitExtTextureCompression();

#endif // _GLEXT_H_
