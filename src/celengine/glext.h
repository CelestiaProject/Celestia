/* glext.h */

#ifndef _CELENGINE_GLEXT_H_
#define _CELENGINE_GLEXT_H_

#include <cstddef>

#ifndef APIENTRY
#define APIENTRY
#endif

#ifndef GL_ARB_shader_objects
/* GL types for handling shader object handles and characters */
typedef char GLcharARB;		/* native character */
typedef unsigned int GLhandleARB;	/* shader object handle */
#endif

/* EXT_rescale_normal defines from <GL/gl.h> */
#ifndef GL_EXT_rescale_normal
#define GL_EXT_rescale_normal 1
#define GL_RESCALE_NORMAL_EXT               0x803A
#endif

/* EXT_texture_edge_clamp defines from <GL/gl.h> */
#ifndef GL_EXT_texture_edge_clamp
#define GL_EXT_texture_edge_clamp 1
#define GL_CLAMP_TO_EDGE_EXT                ((GLenum) 0x812F)
#endif

/* EXT_texture_edge_clamp defines from <GL/gl.h> */
#ifndef GL_ARB_texture_border_clamp
#define GL_ARB_texture_border_clamp 1
#define GL_CLAMP_TO_BORDER_ARB              ((GLenum) 0x812D)
#endif

/* EXT_bgra defines from <GL/gl.h> */
#ifndef GL_EXT_bgra
#define GL_EXT_bgra 1
#define GL_BGR_EXT                          0x80E0
#define GL_BGRA_EXT                         0x80E1
#endif

/* ARB_multisample defines from <GL/gl.h> */
#ifndef GL_ARB_multisample
#define GL_MULTISAMPLE_ARB                0x809D
#define GL_SAMPLE_ALPHA_TO_COVERAGE_ARB   0x809E
#define GL_SAMPLE_ALPHA_TO_ONE_ARB        0x809F
#define GL_SAMPLE_COVERAGE_ARB            0x80A0
#define GL_SAMPLE_BUFFERS_ARB             0x80A8
#define GL_SAMPLES_ARB                    0x80A9
#define GL_SAMPLE_COVERAGE_VALUE_ARB      0x80AA
#define GL_SAMPLE_COVERAGE_INVERT_ARB     0x80AB
#define GL_MULTISAMPLE_BIT_ARB            0x20000000
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
#endif

namespace glx
{
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
};


/* GL_EXT_texture_compression_s3tc */
#ifndef GL_EXT_texture_compression_s3tc
#define GL_EXT_texture_compression_s3tc 1
#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT   0x83F0
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT  0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT  0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT  0x83F3
#endif

/* ARB_multitexture defines and prototypes from <GL/gl.h> */
#ifndef GL_ARB_multitexture
#define GL_ARB_multitexture 1
#define GL_ACTIVE_TEXTURE_ARB               0x84E0
#define GL_CLIENT_ACTIVE_TEXTURE_ARB        0x84E1
#define GL_MAX_TEXTURE_UNITS_ARB            0x84E2
#define GL_TEXTURE0_ARB                     0x84C0
#define GL_TEXTURE1_ARB                     0x84C1
#define GL_TEXTURE2_ARB                     0x84C2
#define GL_TEXTURE3_ARB                     0x84C3
#endif

namespace glx
{
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
};

/* ARB_texture_cube_map defines from <GL/gl.h> */
#if !(defined( GL_ARB_texture_cube_map) || defined(__glext_h_))
#define GL_NORMAL_MAP_ARB                   ((GLenum) 0x8511)
#define GL_REFLECTION_MAP_ARB               ((GLenum) 0x8512)
#define GL_TEXTURE_CUBE_MAP_ARB             ((GLenum) 0x8513)
#define GL_TEXTURE_BINDING_CUBE_MAP_ARB     ((GLenum) 0x8514)
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB  ((GLenum) 0x8515)
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB  ((GLenum) 0x8516)
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB  ((GLenum) 0x8517)
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB  ((GLenum) 0x8518)
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB  ((GLenum) 0x8519)
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB  ((GLenum) 0x851A)
#define GL_PROXY_TEXTURE_CUBE_MAP_ARB       ((GLenum) 0x851B)
#define GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB    ((GLenum) 0x851C)
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
#define GL_NV_register_combiners 1
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
#endif

namespace glx
{
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
};


/* NV_register_combiners2 */
#ifndef GL_NV_register_combiners2
#define GL_PER_STAGE_CONSTANTS_NV         0x8535
#endif

namespace glx
{
    typedef void (APIENTRY * PFNGLCOMBINERSTAGEPARAMETERFVNVPROC) (GLenum stage, GLenum pname, const GLfloat *params);
    typedef void (APIENTRY * PFNGLGETCOMBINERSTAGEPARAMETERFVNVPROC) (GLenum stage, GLenum pname, GLfloat *params);

    extern PFNGLCOMBINERSTAGEPARAMETERFVNVPROC glCombinerStageParameterfvNV;
    extern PFNGLGETCOMBINERSTAGEPARAMETERFVNVPROC glGetCombinerStageParameterfvNV;
};


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
#if !defined(GL_NV_vertex_program)||defined(TARGET_OS_MAC)

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

#endif // !defined(GL_NV_VERTEX_PROGRAM) || defined(TARGET_OS_MAC)

#ifndef GL_VERTEX_ATTRIB_ARRAY6_NV
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
#endif

namespace glx
{
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
    typedef void (APIENTRY * PFNGLPROGRAMPARAMETERS4DVNVPROC) (GLenum target, GLuint index, GLuint count, const GLdouble *v);
    typedef void (APIENTRY * PFNGLPROGRAMPARAMETERS4FVNVPROC) (GLenum target, GLuint index, GLuint count, const GLfloat *v);
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
};


/* EXT_paletted_texture defines and prototypes from <GL/gl.h> */
#ifndef GL_EXT_paletted_texture
#define GL_COLOR_INDEX1_EXT               0x80E2
#define GL_COLOR_INDEX2_EXT               0x80E3
#define GL_COLOR_INDEX4_EXT               0x80E4
#define GL_COLOR_INDEX8_EXT               0x80E5
#define GL_COLOR_INDEX12_EXT              0x80E6
#define GL_COLOR_INDEX16_EXT              0x80E7
#endif

namespace glx
{
    typedef void (APIENTRY * PFNGLCOLORTABLEEXTPROC) (GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid * table);
    extern PFNGLCOLORTABLEEXTPROC glColorTableEXT;
};

/* EXT_blend_minmax defines and prototypes from <GL/gl.h> */
#ifndef GL_EXT_blend_minmax
#define GL_FUNC_ADD_EXT                  0x8006
#define GL_MIN_EXT                       0x8007
#define GL_MAX_EXT                       0x8008

#endif

namespace glx
{
    typedef void (APIENTRY * PFNGLBLENDEQUATIONEXTPROC) (GLenum mode);
    extern PFNGLBLENDEQUATIONEXTPROC glBlendEquationEXT;
};


/* EXT_blend_subtract defines and prototypes from <GL/gl.h> */
#ifndef GL_EXT_blend_subtract
#define GL_FUNC_SUBTRACT_EXT             0x800A
#define GL_FUNC_REVERSE_SUBTRACT_EXT     0x800B
#endif

/* WGL_EXT_swap_control defines and prototypes from <GL/gl.h> */
namespace glx
{
    typedef int (APIENTRY * PFNWGLSWAPINTERVALEXTPROC) (int);
    typedef int (APIENTRY * PFNWGLGETSWAPINTERVALEXTPROC) (void);
    extern PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT;
    extern PFNWGLGETSWAPINTERVALEXTPROC wglGetSwapIntervalEXT;
};

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


#ifndef GL_ARB_texture_env_dot3
#define GL_ARB_texture_env_dot3
#define GL_DOT3_RGB_ARB                   0x86AE
#define GL_DOT3_RGBA_ARB                  0x86AF
#endif


/* ARB_vertex_program */
#ifndef GL_ARB_vertex_program
#define GL_ARB_vertex_program 1
#define GL_VERTEX_PROGRAM_ARB                            0x8620
#define GL_VERTEX_PROGRAM_POINT_SIZE_ARB                 0x8642
#define GL_VERTEX_PROGRAM_TWO_SIDE_ARB                   0x8643
#define GL_PROGRAM_FORMAT_ASCII_ARB                      0x8875
#define GL_VERTEX_ATTRIB_ARRAY_ENABLED_ARB               0x8622
#define GL_VERTEX_ATTRIB_ARRAY_SIZE_ARB                  0x8623
#define GL_VERTEX_ATTRIB_ARRAY_STRIDE_ARB                0x8624
#define GL_VERTEX_ATTRIB_ARRAY_TYPE_ARB                  0x8625
#define GL_VERTEX_ATTRIB_ARRAY_NORMALIZED_ARB            0x886A
#define GL_CURRENT_VERTEX_ATTRIB_ARB                     0x8626
#define GL_VERTEX_ATTRIB_ARRAY_POINTER_ARB               0x8645
#define GL_PROGRAM_LENGTH_ARB                            0x8627
#define GL_PROGRAM_FORMAT_ARB                            0x8876
#define GL_PROGRAM_NAME_ARB                              0x8677
#define GL_PROGRAM_INSTRUCTIONS_ARB                      0x88A0
#define GL_MAX_PROGRAM_INSTRUCTIONS_ARB                  0x88A1
#define GL_PROGRAM_NATIVE_INSTRUCTIONS_ARB               0x88A2
#define GL_MAX_PROGRAM_NATIVE_INSTRUCTIONS_ARB           0x88A3
#define GL_PROGRAM_TEMPORARIES_ARB                       0x88A4
#define GL_MAX_PROGRAM_TEMPORARIES_ARB                   0x88A5
#define GL_PROGRAM_NATIVE_TEMPORARIES_ARB                0x88A6
#define GL_MAX_PROGRAM_NATIVE_TEMPORARIES_ARB            0x88A7
#define GL_PROGRAM_PARAMETERS_ARB                        0x88A8
#define GL_MAX_PROGRAM_PARAMETERS_ARB                    0x88A9
#define GL_PROGRAM_NATIVE_PARAMETERS_ARB                 0x88AA
#define GL_MAX_PROGRAM_NATIVE_PARAMETERS_ARB             0x88AB
#define GL_PROGRAM_ATTRIBS_ARB                           0x88AC
#define GL_MAX_PROGRAM_ATTRIBS_ARB                       0x88AD
#define GL_PROGRAM_NATIVE_ATTRIBS_ARB                    0x88AE
#define GL_MAX_PROGRAM_NATIVE_ATTRIBS_ARB                0x88AF
#define GL_PROGRAM_ADDRESS_REGISTERS_ARB                 0x88B0
#define GL_MAX_PROGRAM_ADDRESS_REGISTERS_ARB             0x88B1
#define GL_PROGRAM_NATIVE_ADDRESS_REGISTERS_ARB          0x88B2
#define GL_MAX_PROGRAM_NATIVE_ADDRESS_REGISTERS_ARB      0x88B3
#define GL_MAX_PROGRAM_LOCAL_PARAMETERS_ARB              0x88B4
#define GL_MAX_PROGRAM_ENV_PARAMETERS_ARB                0x88B5
#define GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB               0x88B6
#define GL_PROGRAM_STRING_ARB                            0x8628
#define GL_PROGRAM_ERROR_POSITION_ARB                    0x864B
#define GL_CURRENT_MATRIX_ARB                            0x8641
#define GL_TRANSPOSE_CURRENT_MATRIX_ARB                  0x88B7
#define GL_CURRENT_MATRIX_STACK_DEPTH_ARB                0x8640
#define GL_MAX_VERTEX_ATTRIBS_ARB                        0x8869
#define GL_MAX_PROGRAM_MATRICES_ARB                      0x862F
#define GL_MAX_PROGRAM_MATRIX_STACK_DEPTH_ARB            0x862E
#define GL_PROGRAM_ERROR_STRING_ARB                      0x8874

#define GL_MATRIX0_ARB                                   0x88C0
#define GL_MATRIX1_ARB                                   0x88C1
#define GL_MATRIX2_ARB                                   0x88C2
#define GL_MATRIX3_ARB                                   0x88C3
#define GL_MATRIX4_ARB                                   0x88C4
#define GL_MATRIX5_ARB                                   0x88C5
#define GL_MATRIX6_ARB                                   0x88C6
#define GL_MATRIX7_ARB                                   0x88C7
#define GL_MATRIX8_ARB                                   0x88C8
#define GL_MATRIX9_ARB                                   0x88C9
#define GL_MATRIX10_ARB                                  0x88CA
#define GL_MATRIX11_ARB                                  0x88CB
#define GL_MATRIX12_ARB                                  0x88CC
#define GL_MATRIX13_ARB                                  0x88CD
#define GL_MATRIX14_ARB                                  0x88CE
#define GL_MATRIX15_ARB                                  0x88CF
#define GL_MATRIX16_ARB                                  0x88D0
#define GL_MATRIX17_ARB                                  0x88D1
#define GL_MATRIX18_ARB                                  0x88D2
#define GL_MATRIX19_ARB                                  0x88D3
#define GL_MATRIX20_ARB                                  0x88D4
#define GL_MATRIX21_ARB                                  0x88D5
#define GL_MATRIX22_ARB                                  0x88D6
#define GL_MATRIX23_ARB                                  0x88D7
#define GL_MATRIX24_ARB                                  0x88D8
#define GL_MATRIX25_ARB                                  0x88D9
#define GL_MATRIX26_ARB                                  0x88DA
#define GL_MATRIX27_ARB                                  0x88DB
#define GL_MATRIX28_ARB                                  0x88DC
#define GL_MATRIX29_ARB                                  0x88DD
#define GL_MATRIX30_ARB                                  0x88DE
#define GL_MATRIX31_ARB                                  0x88DF
#endif /* GL_ARB_vertex_program */

namespace glx
{
    typedef void (APIENTRY * PFNGLVERTEXATTRIB1SARBPROC) (GLuint index, GLshort x);
    typedef void (APIENTRY * PFNGLVERTEXATTRIB1FARBPROC) (GLuint index, GLfloat x);
    typedef void (APIENTRY * PFNGLVERTEXATTRIB1DARBPROC) (GLuint index, GLdouble x);
    typedef void (APIENTRY * PFNGLVERTEXATTRIB2SARBPROC) (GLuint index, GLshort x, GLshort y);
    typedef void (APIENTRY * PFNGLVERTEXATTRIB2FARBPROC) (GLuint index, GLfloat x, GLfloat y);
    typedef void (APIENTRY * PFNGLVERTEXATTRIB2DARBPROC) (GLuint index, GLdouble x, GLdouble y);
    typedef void (APIENTRY * PFNGLVERTEXATTRIB3SARBPROC) (GLuint index, GLshort x, GLshort y, GLshort z);
    typedef void (APIENTRY * PFNGLVERTEXATTRIB3FARBPROC) (GLuint index, GLfloat x, GLfloat y, GLfloat z);
    typedef void (APIENTRY * PFNGLVERTEXATTRIB3DARBPROC) (GLuint index, GLdouble x, GLdouble y, GLdouble z);
    typedef void (APIENTRY * PFNGLVERTEXATTRIB4SARBPROC) (GLuint index, GLshort x, GLshort y, GLshort z, GLshort w);
    typedef void (APIENTRY * PFNGLVERTEXATTRIB4FARBPROC) (GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
    typedef void (APIENTRY * PFNGLVERTEXATTRIB4DARBPROC) (GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
    typedef void (APIENTRY * PFNGLVERTEXATTRIB4NUBARBPROC) (GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w);
    typedef void (APIENTRY * PFNGLVERTEXATTRIB1SVARBPROC) (GLuint index, const GLshort *v);
    typedef void (APIENTRY * PFNGLVERTEXATTRIB1FVARBPROC) (GLuint index, const GLfloat *v);
    typedef void (APIENTRY * PFNGLVERTEXATTRIB1DVARBPROC) (GLuint index, const GLdouble *v);
    typedef void (APIENTRY * PFNGLVERTEXATTRIB2SVARBPROC) (GLuint index, const GLshort *v);
    typedef void (APIENTRY * PFNGLVERTEXATTRIB2FVARBPROC) (GLuint index, const GLfloat *v);
    typedef void (APIENTRY * PFNGLVERTEXATTRIB2DVARBPROC) (GLuint index, const GLdouble *v);
    typedef void (APIENTRY * PFNGLVERTEXATTRIB3SVARBPROC) (GLuint index, const GLshort *v);
    typedef void (APIENTRY * PFNGLVERTEXATTRIB3FVARBPROC) (GLuint index, const GLfloat *v);
    typedef void (APIENTRY * PFNGLVERTEXATTRIB3DVARBPROC) (GLuint index, const GLdouble *v);
    typedef void (APIENTRY * PFNGLVERTEXATTRIB4BVARBPROC) (GLuint index, const GLbyte *v);
    typedef void (APIENTRY * PFNGLVERTEXATTRIB4SVARBPROC) (GLuint index, const GLshort *v);
    typedef void (APIENTRY * PFNGLVERTEXATTRIB4IVARBPROC) (GLuint index, const GLint *v);
    typedef void (APIENTRY * PFNGLVERTEXATTRIB4UBVARBPROC) (GLuint index, const GLubyte *v);
    typedef void (APIENTRY * PFNGLVERTEXATTRIB4USVARBPROC) (GLuint index, const GLushort *v);
    typedef void (APIENTRY * PFNGLVERTEXATTRIB4UIVARBPROC) (GLuint index, const GLuint *v);
    typedef void (APIENTRY * PFNGLVERTEXATTRIB4FVARBPROC) (GLuint index, const GLfloat *v);
    typedef void (APIENTRY * PFNGLVERTEXATTRIB4DVARBPROC) (GLuint index, const GLdouble *v);
    typedef void (APIENTRY * PFNGLVERTEXATTRIB4NBVARBPROC) (GLuint index, const GLbyte *v);
    typedef void (APIENTRY * PFNGLVERTEXATTRIB4NSVARBPROC) (GLuint index, const GLshort *v);
    typedef void (APIENTRY * PFNGLVERTEXATTRIB4NIVARBPROC) (GLuint index, const GLint *v);
    typedef void (APIENTRY * PFNGLVERTEXATTRIB4NUBVARBPROC) (GLuint index, const GLubyte *v);
    typedef void (APIENTRY * PFNGLVERTEXATTRIB4NUSVARBPROC) (GLuint index, const GLushort *v);
    typedef void (APIENTRY * PFNGLVERTEXATTRIB4NUIVARBPROC) (GLuint index, const GLuint *v);
    typedef void (APIENTRY * PFNGLVERTEXATTRIBPOINTERARBPROC) (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer);
    typedef void (APIENTRY * PFNGLENABLEVERTEXATTRIBARRAYARBPROC) (GLuint index);
    typedef void (APIENTRY * PFNGLDISABLEVERTEXATTRIBARRAYARBPROC) (GLuint index);
    typedef void (APIENTRY * PFNGLPROGRAMSTRINGARBPROC) (GLenum target, GLenum format, GLsizei len, const void *string); 
    typedef void (APIENTRY * PFNGLBINDPROGRAMARBPROC) (GLenum target, GLuint program);
    typedef void (APIENTRY * PFNGLDELETEPROGRAMSARBPROC) (GLsizei n, const GLuint *programs);
    typedef void (APIENTRY * PFNGLGENPROGRAMSARBPROC) (GLsizei n, GLuint *programs);
    typedef void (APIENTRY * PFNGLPROGRAMENVPARAMETER4DARBPROC) (GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
    typedef void (APIENTRY * PFNGLPROGRAMENVPARAMETER4DVARBPROC) (GLenum target, GLuint index, const GLdouble *params);
    typedef void (APIENTRY * PFNGLPROGRAMENVPARAMETER4FARBPROC) (GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
    typedef void (APIENTRY * PFNGLPROGRAMENVPARAMETER4FVARBPROC) (GLenum target, GLuint index, const GLfloat *params);
    typedef void (APIENTRY * PFNGLPROGRAMLOCALPARAMETER4DARBPROC) (GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
    typedef void (APIENTRY * PFNGLPROGRAMLOCALPARAMETER4DVARBPROC) (GLenum target, GLuint index, const GLdouble *params);
    typedef void (APIENTRY * PFNGLPROGRAMLOCALPARAMETER4FARBPROC) (GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
    typedef void (APIENTRY * PFNGLPROGRAMLOCALPARAMETER4FVARBPROC) (GLenum target, GLuint index, const GLfloat *params);
    typedef void (APIENTRY * PFNGLGETPROGRAMENVPARAMETERDVARBPROC) (GLenum target, GLuint index, GLdouble *params);
    typedef void (APIENTRY * PFNGLGETPROGRAMENVPARAMETERFVARBPROC) (GLenum target, GLuint index, GLfloat *params);
    typedef void (APIENTRY * PFNGLGETPROGRAMLOCALPARAMETERDVARBPROC) (GLenum target, GLuint index, GLdouble *params);
    typedef void (APIENTRY * PFNGLGETPROGRAMLOCALPARAMETERFVARBPROC) (GLenum target, GLuint index, GLfloat *params);
    typedef void (APIENTRY * PFNGLGETPROGRAMIVARBPROC) (GLenum target, GLenum pname, GLint *params);
    typedef void (APIENTRY * PFNGLGETPROGRAMSTRINGARBPROC) (GLenum target, GLenum pname, void *string);
    typedef void (APIENTRY * PFNGLGETVERTEXATTRIBDVARBPROC) (GLuint index, GLenum pname, GLdouble *params);
    typedef void (APIENTRY * PFNGLGETVERTEXATTRIBFVARBPROC) (GLuint index, GLenum pname, GLfloat *params);
    typedef void (APIENTRY * PFNGLGETVERTEXATTRIBIVARBPROC) (GLuint index, GLenum pname, GLint *params);
    typedef void (APIENTRY * PFNGLGETVERTEXATTRIBPOINTERVARBPROC) (GLuint index, GLenum pname, void **pointer);
    typedef GLboolean (APIENTRY * PFNGLISPROGRAMARBPROC) (GLuint program);

    extern PFNGLBINDPROGRAMARBPROC glBindProgramARB;
    extern PFNGLDELETEPROGRAMSARBPROC glDeleteProgramsARB;
    extern PFNGLGENPROGRAMSARBPROC glGenProgramsARB;
    extern PFNGLISPROGRAMARBPROC glIsProgramARB;

    extern PFNGLVERTEXATTRIB1SARBPROC glVertexAttrib1sARB;
    extern PFNGLVERTEXATTRIB1FARBPROC glVertexAttrib1fARB;
    extern PFNGLVERTEXATTRIB1DARBPROC glVertexAttrib1dARB;
    extern PFNGLVERTEXATTRIB2SARBPROC glVertexAttrib2sARB;
    extern PFNGLVERTEXATTRIB2FARBPROC glVertexAttrib2fARB;
    extern PFNGLVERTEXATTRIB2DARBPROC glVertexAttrib2dARB;
    extern PFNGLVERTEXATTRIB3SARBPROC glVertexAttrib3sARB;
    extern PFNGLVERTEXATTRIB3FARBPROC glVertexAttrib3fARB;
    extern PFNGLVERTEXATTRIB3DARBPROC glVertexAttrib3dARB;
    extern PFNGLVERTEXATTRIB4SARBPROC glVertexAttrib4sARB;
    extern PFNGLVERTEXATTRIB4FARBPROC glVertexAttrib4fARB;
    extern PFNGLVERTEXATTRIB4DARBPROC glVertexAttrib4dARB;
    extern PFNGLVERTEXATTRIB4NUBARBPROC glVertexAttrib4NubARB;
    extern PFNGLVERTEXATTRIB1SVARBPROC glVertexAttrib1svARB;
    extern PFNGLVERTEXATTRIB1FVARBPROC glVertexAttrib1fvARB;
    extern PFNGLVERTEXATTRIB1DVARBPROC glVertexAttrib1dvARB;
    extern PFNGLVERTEXATTRIB2SVARBPROC glVertexAttrib2svARB;
    extern PFNGLVERTEXATTRIB2FVARBPROC glVertexAttrib2fvARB;
    extern PFNGLVERTEXATTRIB2DVARBPROC glVertexAttrib2dvARB;
    extern PFNGLVERTEXATTRIB3SVARBPROC glVertexAttrib3svARB;
    extern PFNGLVERTEXATTRIB3FVARBPROC glVertexAttrib3fvARB;
    extern PFNGLVERTEXATTRIB3DVARBPROC glVertexAttrib3dvARB;
    extern PFNGLVERTEXATTRIB4BVARBPROC glVertexAttrib4bvARB;
    extern PFNGLVERTEXATTRIB4SVARBPROC glVertexAttrib4svARB;
    extern PFNGLVERTEXATTRIB4IVARBPROC glVertexAttrib4ivARB;
    extern PFNGLVERTEXATTRIB4UBVARBPROC glVertexAttrib4ubvARB;
    extern PFNGLVERTEXATTRIB4USVARBPROC glVertexAttrib4usvARB;
    extern PFNGLVERTEXATTRIB4UIVARBPROC glVertexAttrib4uivARB;
    extern PFNGLVERTEXATTRIB4FVARBPROC glVertexAttrib4fvARB;
    extern PFNGLVERTEXATTRIB4DVARBPROC glVertexAttrib4dvARB;
    extern PFNGLVERTEXATTRIB4NBVARBPROC glVertexAttrib4NbvARB;
    extern PFNGLVERTEXATTRIB4NSVARBPROC glVertexAttrib4NsvARB;
    extern PFNGLVERTEXATTRIB4NIVARBPROC glVertexAttrib4NivARB;
    extern PFNGLVERTEXATTRIB4NUBVARBPROC glVertexAttrib4NubvARB;
    extern PFNGLVERTEXATTRIB4NUSVARBPROC glVertexAttrib4NusvARB;
    extern PFNGLVERTEXATTRIB4NUIVARBPROC glVertexAttrib4NuivARB;

    extern PFNGLVERTEXATTRIBPOINTERARBPROC glVertexAttribPointerARB;

    extern PFNGLENABLEVERTEXATTRIBARRAYARBPROC glEnableVertexAttribArrayARB;
    extern PFNGLDISABLEVERTEXATTRIBARRAYARBPROC glDisableVertexAttribArrayARB;

    extern PFNGLGETVERTEXATTRIBDVARBPROC glGetVertexAttribdvARB;
    extern PFNGLGETVERTEXATTRIBFVARBPROC glGetVertexAttribfvARB;
    extern PFNGLGETVERTEXATTRIBIVARBPROC glGetVertexAttribivARB;
    extern PFNGLGETVERTEXATTRIBPOINTERVARBPROC glGetVertexAttribPointervARB;

    extern PFNGLPROGRAMENVPARAMETER4DARBPROC glProgramEnvParameter4dARB;
    extern PFNGLPROGRAMENVPARAMETER4DVARBPROC glProgramEnvParameter4dvARB;
    extern PFNGLPROGRAMENVPARAMETER4FARBPROC glProgramEnvParameter4fARB;
    extern PFNGLPROGRAMENVPARAMETER4FVARBPROC glProgramEnvParameter4fvARB;
    extern PFNGLPROGRAMLOCALPARAMETER4DARBPROC glProgramLocalParameter4dARB;
    extern PFNGLPROGRAMLOCALPARAMETER4DVARBPROC glProgramLocalParameter4dvARB;
    extern PFNGLPROGRAMLOCALPARAMETER4FARBPROC glProgramLocalParameter4fARB;
    extern PFNGLPROGRAMLOCALPARAMETER4FVARBPROC glProgramLocalParameter4fvARB;

    extern PFNGLGETPROGRAMENVPARAMETERDVARBPROC glGetProgramEnvParameterdvARB;
    extern PFNGLGETPROGRAMENVPARAMETERFVARBPROC glGetProgramEnvParameterfvARB;
    extern PFNGLGETPROGRAMLOCALPARAMETERDVARBPROC glGetProgramLocalParameterdvARB;
    extern PFNGLGETPROGRAMLOCALPARAMETERFVARBPROC glGetProgramLocalParameterfvARB;

    extern PFNGLPROGRAMSTRINGARBPROC glProgramStringARB;
    extern PFNGLGETPROGRAMSTRINGARBPROC glGetProgramStringARB;

    extern PFNGLGETPROGRAMIVARBPROC glGetProgramivARB;
};


/* GL_NV_fragment_program */
#ifndef GL_NV_fragment_program
#define GL_NV_fragment_program 1
#define GL_MAX_FRAGMENT_PROGRAM_LOCAL_PARAMETERS_NV 0x8868
#define GL_FRAGMENT_PROGRAM_NV            0x8870
#define GL_MAX_TEXTURE_COORDS_NV          0x8871
#define GL_MAX_TEXTURE_IMAGE_UNITS_NV     0x8872
#define GL_FRAGMENT_PROGRAM_BINDING_NV    0x8873
#define GL_PROGRAM_ERROR_STRING_NV        0x8874
#endif /* GL_NV_fragment_program */

#ifndef GL_ARB_vertex_shader
#define GL_VERTEX_SHADER_ARB              0x8B31
#define GL_MAX_VERTEX_UNIFORM_COMPONENTS_ARB 0x8B4A
#define GL_MAX_VARYING_FLOATS_ARB         0x8B4B
#define GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS_ARB 0x8B4C
#define GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS_ARB 0x8B4D
#define GL_OBJECT_ACTIVE_ATTRIBUTES_ARB   0x8B89
#define GL_OBJECT_ACTIVE_ATTRIBUTE_MAX_LENGTH_ARB 0x8B8A
#endif

namespace glx
{
    typedef void (APIENTRY* PFNGLBINDATTRIBLOCATIONARBPROC) (GLhandleARB programObj, GLuint index, const GLcharARB *name);
    typedef void (APIENTRY* PFNGLGETACTIVEATTRIBARBPROC) (GLhandleARB programObj, GLuint index, GLsizei maxLength, GLsizei *length, GLint *size, GLenum *type, GLcharARB *name);
    typedef GLint (APIENTRY* PFNGLGETATTRIBLOCATIONARBPROC) (GLhandleARB programObj, const GLcharARB *name);

    extern PFNGLBINDATTRIBLOCATIONARBPROC glBindAttribLocationARB;
    extern PFNGLGETACTIVEATTRIBARBPROC glGetActiveAttribARB;
    extern PFNGLGETATTRIBLOCATIONARBPROC glGetAttribLocationARB;
};


#ifndef GL_ARB_point_sprite
#define GL_ARB_point_sprite 1
#define GL_POINT_SPRITE_ARB               0x8861
#define GL_COORD_REPLACE_ARB              0x8862
#define GL_SMOOTH_POINT_SIZE_RANGE        0x0B12
#define GL_ALIASED_POINT_SIZE_RANGE       0x846D
#endif


#ifndef GL_ARB_fragment_shader
#define GL_ARB_fragment_shader 1
#define GL_FRAGMENT_SHADER_ARB            0x8B30
#define GL_MAX_FRAGMENT_UNIFORM_COMPONENTS_ARB 0x8B49
#endif

#ifndef GL_ARB_shading_language_100
#define GL_ARB_shading_language_100 1
#define GL_SHADING_LANGUAGE_VERSION_ARB   0x8B8C
#endif

namespace glx
{
    typedef void (APIENTRY * PFNGLPROGRAMNAMEDPARAMETER4FNVPROC)    (GLuint id, GLsizei len, const GLubyte *name, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
    typedef void (APIENTRY * PFNGLPROGRAMNAMEDPARAMETER4DNVPROC)    (GLuint id, GLsizei len, const GLubyte *name, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
    typedef void (APIENTRY * PFNGLPROGRAMNAMEDPARAMETER4FVNVPROC)   (GLuint id, GLsizei len, const GLubyte *name, const GLfloat v[]);
    typedef void (APIENTRY * PFNGLPROGRAMNAMEDPARAMETER4DVNVPROC)   (GLuint id, GLsizei len, const GLubyte *name, const GLdouble v[]);
    typedef void (APIENTRY * PFNGLGETPROGRAMNAMEDPARAMETERFVNVPROC) (GLuint id, GLsizei len, const GLubyte *name, GLfloat *params);
    typedef void (APIENTRY * PFNGLGETPROGRAMNAMEDPARAMETERDVNVPROC) (GLuint id, GLsizei len, const GLubyte *name, GLdouble *params);

    typedef void (APIENTRY * PFNGLPROGRAMLOCALPARAMETER4FNVPROC)    (GLenum target, GLuint id, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
    typedef void (APIENTRY * PFNGLPROGRAMLOCALPARAMETER4DNVPROC)    (GLenum target, GLuint id, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
    typedef void (APIENTRY * PFNGLPROGRAMLOCALPARAMETER4FVNVPROC)   (GLenum target, GLuint id, const GLfloat v[]);
    typedef void (APIENTRY * PFNGLPROGRAMLOCALPARAMETER4DVNVPROC)   (GLenum target, GLuint id, const GLdouble v[]);
    typedef void (APIENTRY * PFNGLGETPROGRAMLOCALPARAMETERFVNVPROC) (GLenum target, GLuint id, GLfloat *params);
    typedef void (APIENTRY * PFNGLGETPROGRAMLOCALPARAMETERDVNVPROC) (GLenum target, GLuint id, GLdouble *params);


    extern PFNGLPROGRAMNAMEDPARAMETER4FNVPROC glProgramNamedParameter4fNV;
    extern PFNGLPROGRAMNAMEDPARAMETER4DNVPROC glProgramNamedParameter4dNV;
    extern PFNGLPROGRAMNAMEDPARAMETER4FVNVPROC glProgramNamedParameter4fvNV;
    extern PFNGLPROGRAMNAMEDPARAMETER4DVNVPROC glProgramNamedParameter4dvNV;
    extern PFNGLGETPROGRAMNAMEDPARAMETERFVNVPROC glGetProgramNamedParameterfvNV;
    extern PFNGLGETPROGRAMNAMEDPARAMETERDVNVPROC glGetProgramNamedParameterdvNV;

    extern PFNGLPROGRAMLOCALPARAMETER4FNVPROC glProgramLocalParameter4fNV;
    extern PFNGLPROGRAMLOCALPARAMETER4DNVPROC glProgramLocalParameter4dNV;
    extern PFNGLPROGRAMLOCALPARAMETER4FVNVPROC glProgramLocalParameter4fvNV;
    extern PFNGLPROGRAMLOCALPARAMETER4DVNVPROC glProgramLocalParameter4dvNV;
    extern PFNGLGETPROGRAMLOCALPARAMETERFVNVPROC glGetProgramLocalParameterfvNV;
    extern PFNGLGETPROGRAMLOCALPARAMETERDVNVPROC glGetProgramLocalParameterdvNV;
};


#ifndef GL_ARB_fragment_program
#define GL_ARB_fragment_program 1
// All ARB_fragment_program entry points are shared with ARB_vertex_program.

#define GL_FRAGMENT_PROGRAM_ARB           0x8804
#define GL_PROGRAM_ALU_INSTRUCTIONS_ARB   0x8805
#define GL_PROGRAM_TEX_INSTRUCTIONS_ARB   0x8806
#define GL_PROGRAM_TEX_INDIRECTIONS_ARB   0x8807
#define GL_PROGRAM_NATIVE_ALU_INSTRUCTIONS_ARB 0x8808
#define GL_PROGRAM_NATIVE_TEX_INSTRUCTIONS_ARB 0x8809
#define GL_PROGRAM_NATIVE_TEX_INDIRECTIONS_ARB 0x880A
#define GL_MAX_PROGRAM_ALU_INSTRUCTIONS_ARB 0x880B
#define GL_MAX_PROGRAM_TEX_INSTRUCTIONS_ARB 0x880C
#define GL_MAX_PROGRAM_TEX_INDIRECTIONS_ARB 0x880D
#define GL_MAX_PROGRAM_NATIVE_ALU_INSTRUCTIONS_ARB 0x880E
#define GL_MAX_PROGRAM_NATIVE_TEX_INSTRUCTIONS_ARB 0x880F
#define GL_MAX_PROGRAM_NATIVE_TEX_INDIRECTIONS_ARB 0x8810
#define GL_MAX_TEXTURE_COORDS_ARB         0x8871
#define GL_MAX_TEXTURE_IMAGE_UNITS_ARB    0x8872
#endif


#ifndef GL_ARB_shader_objects
#define GL_ARB_shader_objects 1
#define GL_PROGRAM_OBJECT_ARB             0x8B40
#define GL_SHADER_OBJECT_ARB              0x8B48
#define GL_OBJECT_TYPE_ARB                0x8B4E
#define GL_OBJECT_SUBTYPE_ARB             0x8B4F
#define GL_FLOAT_VEC2_ARB                 0x8B50
#define GL_FLOAT_VEC3_ARB                 0x8B51
#define GL_FLOAT_VEC4_ARB                 0x8B52
#define GL_INT_VEC2_ARB                   0x8B53
#define GL_INT_VEC3_ARB                   0x8B54
#define GL_INT_VEC4_ARB                   0x8B55
#define GL_BOOL_ARB                       0x8B56
#define GL_BOOL_VEC2_ARB                  0x8B57
#define GL_BOOL_VEC3_ARB                  0x8B58
#define GL_BOOL_VEC4_ARB                  0x8B59
#define GL_FLOAT_MAT2_ARB                 0x8B5A
#define GL_FLOAT_MAT3_ARB                 0x8B5B
#define GL_FLOAT_MAT4_ARB                 0x8B5C
#define GL_OBJECT_DELETE_STATUS_ARB       0x8B80
#define GL_OBJECT_COMPILE_STATUS_ARB      0x8B81
#define GL_OBJECT_LINK_STATUS_ARB         0x8B82
#define GL_OBJECT_VALIDATE_STATUS_ARB     0x8B83
#define GL_OBJECT_INFO_LOG_LENGTH_ARB     0x8B84
#define GL_OBJECT_ATTACHED_OBJECTS_ARB    0x8B85
#define GL_OBJECT_ACTIVE_UNIFORMS_ARB     0x8B86
#define GL_OBJECT_ACTIVE_UNIFORM_MAX_LENGTH_ARB 0x8B87
#define GL_OBJECT_SHADER_SOURCE_LENGTH_ARB 0x8B88

typedef char GLcharARB;		       // native character
typedef unsigned int GLhandleARB;      // shader object handle
#endif


namespace glx
{
    typedef void (APIENTRY* PFNGLDELETEOBJECTARBPROC) (GLhandleARB obj);
    typedef GLhandleARB (APIENTRY* PFNGLGETHANDLEARBPROC) (GLenum pname);
    typedef void (APIENTRY* PFNGLDETACHOBJECTARBPROC) (GLhandleARB containerObj, GLhandleARB attachedObj);
    typedef GLhandleARB (APIENTRY* PFNGLCREATESHADEROBJECTARBPROC) (GLenum shaderType);
    typedef void (APIENTRY* PFNGLSHADERSOURCEARBPROC) (GLhandleARB shaderObj, GLsizei count, const GLcharARB* *string, const GLint *length);
    typedef void (APIENTRY* PFNGLCOMPILESHADERARBPROC) (GLhandleARB shaderObj);
    typedef GLhandleARB (APIENTRY* PFNGLCREATEPROGRAMOBJECTARBPROC) (void);
    typedef void (APIENTRY* PFNGLATTACHOBJECTARBPROC) (GLhandleARB containerObj, GLhandleARB obj);
    typedef void (APIENTRY* PFNGLLINKPROGRAMARBPROC) (GLhandleARB programObj);
    typedef void (APIENTRY* PFNGLUSEPROGRAMOBJECTARBPROC) (GLhandleARB programObj);
    typedef void (APIENTRY* PFNGLVALIDATEPROGRAMARBPROC) (GLhandleARB programObj);
    typedef void (APIENTRY* PFNGLUNIFORM1FARBPROC) (GLint location, GLfloat v0);
    typedef void (APIENTRY* PFNGLUNIFORM2FARBPROC) (GLint location, GLfloat v0, GLfloat v1);
    typedef void (APIENTRY* PFNGLUNIFORM3FARBPROC) (GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
    typedef void (APIENTRY* PFNGLUNIFORM4FARBPROC) (GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
    typedef void (APIENTRY* PFNGLUNIFORM1IARBPROC) (GLint location, GLint v0);
    typedef void (APIENTRY* PFNGLUNIFORM2IARBPROC) (GLint location, GLint v0, GLint v1);
    typedef void (APIENTRY* PFNGLUNIFORM3IARBPROC) (GLint location, GLint v0, GLint v1, GLint v2);
    typedef void (APIENTRY* PFNGLUNIFORM4IARBPROC) (GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
    typedef void (APIENTRY* PFNGLUNIFORM1FVARBPROC) (GLint location, GLsizei count, const GLfloat *value);
    typedef void (APIENTRY* PFNGLUNIFORM2FVARBPROC) (GLint location, GLsizei count, const GLfloat *value);
    typedef void (APIENTRY* PFNGLUNIFORM3FVARBPROC) (GLint location, GLsizei count, const GLfloat *value);
    typedef void (APIENTRY* PFNGLUNIFORM4FVARBPROC) (GLint location, GLsizei count, const GLfloat *value);
    typedef void (APIENTRY* PFNGLUNIFORM1IVARBPROC) (GLint location, GLsizei count, const GLint *value);
    typedef void (APIENTRY* PFNGLUNIFORM2IVARBPROC) (GLint location, GLsizei count, const GLint *value);
    typedef void (APIENTRY* PFNGLUNIFORM3IVARBPROC) (GLint location, GLsizei count, const GLint *value);
    typedef void (APIENTRY* PFNGLUNIFORM4IVARBPROC) (GLint location, GLsizei count, const GLint *value);
    typedef void (APIENTRY* PFNGLUNIFORMMATRIX2FVARBPROC) (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
    typedef void (APIENTRY* PFNGLUNIFORMMATRIX3FVARBPROC) (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
    typedef void (APIENTRY* PFNGLUNIFORMMATRIX4FVARBPROC) (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
    typedef void (APIENTRY* PFNGLGETOBJECTPARAMETERFVARBPROC) (GLhandleARB obj, GLenum pname, GLfloat *params);
    typedef void (APIENTRY* PFNGLGETOBJECTPARAMETERIVARBPROC) (GLhandleARB obj, GLenum pname, GLint *params);
    typedef void (APIENTRY* PFNGLGETINFOLOGARBPROC) (GLhandleARB obj, GLsizei maxLength, GLsizei *length, GLcharARB *infoLog);
    typedef void (APIENTRY* PFNGLGETATTACHEDOBJECTSARBPROC) (GLhandleARB containerObj, GLsizei maxCount, GLsizei *count, GLhandleARB *obj);
    typedef GLint (APIENTRY* PFNGLGETUNIFORMLOCATIONARBPROC) (GLhandleARB programObj, const GLcharARB *name);
    typedef void (APIENTRY* PFNGLGETACTIVEUNIFORMARBPROC) (GLhandleARB programObj, GLuint index, GLsizei maxLength, GLsizei *length, GLint *size, GLenum *type, GLcharARB *name);
    typedef void (APIENTRY* PFNGLGETUNIFORMFVARBPROC) (GLhandleARB programObj, GLint location, GLfloat *params);
    typedef void (APIENTRY* PFNGLGETUNIFORMIVARBPROC) (GLhandleARB programObj, GLint location, GLint *params);
    typedef void (APIENTRY* PFNGLGETSHADERSOURCEARBPROC) (GLhandleARB obj, GLsizei maxLength, GLsizei *length, GLcharARB *source);

    extern PFNGLDELETEOBJECTARBPROC glDeleteObjectARB;
    extern PFNGLGETHANDLEARBPROC glGetHandleARB;
    extern PFNGLDETACHOBJECTARBPROC glDetachObjectARB;
    extern PFNGLCREATESHADEROBJECTARBPROC glCreateShaderObjectARB;
    extern PFNGLSHADERSOURCEARBPROC glShaderSourceARB;
    extern PFNGLCOMPILESHADERARBPROC glCompileShaderARB;
    extern PFNGLCREATEPROGRAMOBJECTARBPROC glCreateProgramObjectARB;
    extern PFNGLATTACHOBJECTARBPROC glAttachObjectARB;
    extern PFNGLLINKPROGRAMARBPROC glLinkProgramARB;
    extern PFNGLUSEPROGRAMOBJECTARBPROC glUseProgramObjectARB;
    extern PFNGLVALIDATEPROGRAMARBPROC glValidateProgramARB;
    extern PFNGLUNIFORM1FARBPROC glUniform1fARB;
    extern PFNGLUNIFORM2FARBPROC glUniform2fARB;
    extern PFNGLUNIFORM3FARBPROC glUniform3fARB;
    extern PFNGLUNIFORM4FARBPROC glUniform4fARB;
    extern PFNGLUNIFORM1IARBPROC glUniform1iARB;
    extern PFNGLUNIFORM2IARBPROC glUniform2iARB;
    extern PFNGLUNIFORM3IARBPROC glUniform3iARB;
    extern PFNGLUNIFORM4IARBPROC glUniform4iARB;
    extern PFNGLUNIFORM1FVARBPROC glUniform1fvARB;
    extern PFNGLUNIFORM2FVARBPROC glUniform2fvARB;
    extern PFNGLUNIFORM3FVARBPROC glUniform3fvARB;
    extern PFNGLUNIFORM4FVARBPROC glUniform4fvARB;
    extern PFNGLUNIFORM1IVARBPROC glUniform1ivARB;
    extern PFNGLUNIFORM2IVARBPROC glUniform2ivARB;
    extern PFNGLUNIFORM3IVARBPROC glUniform3ivARB;
    extern PFNGLUNIFORM4IVARBPROC glUniform4ivARB;
    extern PFNGLUNIFORMMATRIX2FVARBPROC glUniformMatrix2fvARB;
    extern PFNGLUNIFORMMATRIX3FVARBPROC glUniformMatrix3fvARB;
    extern PFNGLUNIFORMMATRIX4FVARBPROC glUniformMatrix4fvARB;
    extern PFNGLGETOBJECTPARAMETERFVARBPROC glGetObjectParameterfvARB;
    extern PFNGLGETOBJECTPARAMETERIVARBPROC glGetObjectParameterivARB;
    extern PFNGLGETINFOLOGARBPROC glGetInfoLogARB;
    extern PFNGLGETATTACHEDOBJECTSARBPROC glGetAttachedObjectsARB;
    extern PFNGLGETUNIFORMLOCATIONARBPROC glGetUniformLocationARB;
    extern PFNGLGETACTIVEUNIFORMARBPROC glGetActiveUniformARB;
    extern PFNGLGETUNIFORMFVARBPROC glGetUniformfvARB;
    extern PFNGLGETUNIFORMIVARBPROC glGetUniformivARB;
    extern PFNGLGETSHADERSOURCEARBPROC glGetShaderSourceARB;
};


#ifndef GL_VERSION_1_5
/* GL types for handling large vertex buffer objects */
typedef ptrdiff_t GLintptr;
typedef ptrdiff_t GLsizeiptr;
#endif

#ifndef GL_ARB_vertex_buffer_object
/* GL types for handling large vertex buffer objects */
typedef ptrdiff_t GLintptrARB;
typedef ptrdiff_t GLsizeiptrARB;
#endif

#ifndef GL_ARB_vertex_buffer_object
#define GL_BUFFER_SIZE_ARB                0x8764
#define GL_BUFFER_USAGE_ARB               0x8765
#define GL_ARRAY_BUFFER_ARB               0x8892
#define GL_ELEMENT_ARRAY_BUFFER_ARB       0x8893
#define GL_ARRAY_BUFFER_BINDING_ARB       0x8894
#define GL_ELEMENT_ARRAY_BUFFER_BINDING_ARB 0x8895
#define GL_VERTEX_ARRAY_BUFFER_BINDING_ARB 0x8896
#define GL_NORMAL_ARRAY_BUFFER_BINDING_ARB 0x8897
#define GL_COLOR_ARRAY_BUFFER_BINDING_ARB 0x8898
#define GL_INDEX_ARRAY_BUFFER_BINDING_ARB 0x8899
#define GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING_ARB 0x889A
#define GL_EDGE_FLAG_ARRAY_BUFFER_BINDING_ARB 0x889B
#define GL_SECONDARY_COLOR_ARRAY_BUFFER_BINDING_ARB 0x889C
#define GL_FOG_COORDINATE_ARRAY_BUFFER_BINDING_ARB 0x889D
#define GL_WEIGHT_ARRAY_BUFFER_BINDING_ARB 0x889E
#define GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING_ARB 0x889F
#define GL_READ_ONLY_ARB                  0x88B8
#define GL_WRITE_ONLY_ARB                 0x88B9
#define GL_READ_WRITE_ARB                 0x88BA
#define GL_BUFFER_ACCESS_ARB              0x88BB
#define GL_BUFFER_MAPPED_ARB              0x88BC
#define GL_BUFFER_MAP_POINTER_ARB         0x88BD
#define GL_STREAM_DRAW_ARB                0x88E0
#define GL_STREAM_READ_ARB                0x88E1
#define GL_STREAM_COPY_ARB                0x88E2
#define GL_STATIC_DRAW_ARB                0x88E4
#define GL_STATIC_READ_ARB                0x88E5
#define GL_STATIC_COPY_ARB                0x88E6
#define GL_DYNAMIC_DRAW_ARB               0x88E8
#define GL_DYNAMIC_READ_ARB               0x88E9
#define GL_DYNAMIC_COPY_ARB               0x88EA
#endif

namespace glx
{
    typedef void (APIENTRY* PFNGLBINDBUFFERARBPROC) (GLenum target, GLuint buffer);
    typedef void (APIENTRY* PFNGLDELETEBUFFERSARBPROC) (GLsizei n, const GLuint *buffers);
    typedef void (APIENTRY* PFNGLGENBUFFERSARBPROC) (GLsizei n, GLuint *buffers);
    typedef GLboolean (APIENTRY* PFNGLISBUFFERARBPROC) (GLuint buffer);
    typedef void (APIENTRY* PFNGLBUFFERDATAARBPROC) (GLenum target, GLsizeiptrARB size, const GLvoid *data, GLenum usage);
    typedef void (APIENTRY* PFNGLBUFFERSUBDATAARBPROC) (GLenum target, GLintptrARB offset, GLsizeiptrARB size, const GLvoid *data);
    typedef void (APIENTRY* PFNGLGETBUFFERSUBDATAARBPROC) (GLenum target, GLintptrARB offset, GLsizeiptrARB size, GLvoid *data);
    typedef GLvoid* (APIENTRY* PFNGLMAPBUFFERARBPROC) (GLenum target, GLenum access);
    typedef GLboolean (APIENTRY* PFNGLUNMAPBUFFERARBPROC) (GLenum target);
    typedef void (APIENTRY* PFNGLGETBUFFERPARAMETERIVARBPROC) (GLenum target, GLenum pname, GLint *params);
    typedef void (APIENTRY* PFNGLGETBUFFERPOINTERVARBPROC) (GLenum target, GLenum pname, GLvoid* *params);

    extern PFNGLBINDBUFFERARBPROC glBindBufferARB;
    extern PFNGLDELETEBUFFERSARBPROC glDeleteBuffersARB;
    extern PFNGLGENBUFFERSARBPROC glGenBuffersARB;
    extern PFNGLISBUFFERARBPROC glIsBufferARB;
    extern PFNGLBUFFERDATAARBPROC glBufferDataARB;
    extern PFNGLBUFFERSUBDATAARBPROC glBufferSubDataARB;
    extern PFNGLGETBUFFERSUBDATAARBPROC glGetBufferSubDataARB;
    extern PFNGLMAPBUFFERARBPROC glMapBufferARB;
    extern PFNGLUNMAPBUFFERARBPROC glUnmapBufferARB;
    extern PFNGLGETBUFFERPARAMETERIVARBPROC glGetBufferParameterivARB;
    extern PFNGLGETBUFFERPOINTERVARBPROC glGetBufferPointervARB;
};


/* GL_ARB_pixel_buffer_object */
#ifndef GL_ARB_pixel_buffer_object
#define GL_ARB_pixel_buffer_object        1
#define GL_PIXEL_PACK_BUFFER_ARB          0x88EB
#define GL_PIXEL_UNPACK_BUFFER_ARB        0x88EC
#define GL_PIXEL_PACK_BUFFER_BINDING_ARB  0x88ED
#define GL_PIXEL_UNPACK_BUFFER_BINDING_ARB 0x88EF
#endif


/* GL_ARB_color_buffer_float */
#ifndef GL_ARB_color_buffer_float
#define GL_ARB_color_buffer_float         1
#define GL_RGBA_FLOAT_MODE_ARB            0x8820
#define GL_CLAMP_VERTEX_COLOR_ARB         0x891A
#define GL_CLAMP_FRAGMENT_COLOR_ARB       0x891B
#define GL_CLAMP_READ_COLOR_ARB           0x891C
#define GL_FIXED_ONLY_ARB                 0x891D
#endif

namespace glx
{
    typedef void (APIENTRY* PFNGLCLAMPCOLORARBPROC) (GLenum target, GLenum clamp);
    extern PFNGLCLAMPCOLORARBPROC glClampColorARB;
};


/* GL_ARB_texture_float */
#ifndef GL_ARB_texture_float
#define GL_ARB_texture_float              1
#define GL_TEXTURE_RED_TYPE_ARB           0x8C10
#define GL_TEXTURE_GREEN_TYPE_ARB         0x8C11
#define GL_TEXTURE_BLUE_TYPE_ARB          0x8C12
#define GL_TEXTURE_ALPHA_TYPE_ARB         0x8C13
#define GL_TEXTURE_LUMINANCE_TYPE_ARB     0x8C14
#define GL_TEXTURE_INTENSITY_TYPE_ARB     0x8C15
#define GL_TEXTURE_DEPTH_TYPE_ARB         0x8C16
#define GL_UNSIGNED_NORMALIZED_ARB        0x8C17
#define GL_RGBA32F_ARB                    0x8814
#define GL_RGB32F_ARB                     0x8815
#define GL_ALPHA32F_ARB                   0x8816
#define GL_INTENSITY32F_ARB               0x8817
#define GL_LUMINANCE32F_ARB               0x8818
#define GL_LUMINANCE_ALPHA32F_ARB         0x8819
#define GL_RGBA16F_ARB                    0x881A
#define GL_RGB16F_ARB                     0x881B
#define GL_ALPHA16F_ARB                   0x881C
#define GL_INTENSITY16F_ARB               0x881D
#define GL_LUMINANCE16F_ARB               0x881E
#define GL_LUMINANCE_ALPHA16F_ARB         0x881F
#endif


#ifndef GL_ARB_half_float_pixel
#define GL_ARB_half_float_pixel           1
#define GL_HALF_FLOAT_ARB                 0x140B
#endif


/* WGL_ARB_pixel_format_float */
#ifndef WGL_ARB_pixel_format_float
#define WGL_ARB_pixel_format_float        1
#define WGL_TYPE_RGBA_FLOAT_ARB           0x21A0
#endif


#ifndef GL_ARB_depth_texture
#define GL_ARB_depth_texture              1
#define GL_DEPTH_COMPONENT16_ARB          0x81A5
#define GL_DEPTH_COMPONENT24_ARB          0x81A6
#define GL_DEPTH_COMPONENT32_ARB          0x81A7
#define GL_TEXTURE_DEPTH_SIZE_ARB         0x884A
#define GL_DEPTH_TEXTURE_MODE_ARB         0x884B
#endif


/* SGI Video Sync from glxext.h, for refresh-rate syncing. */
namespace glx
{
    typedef int ( * PFNGLXGETVIDEOSYNCSGIPROC) (unsigned int *count);
    typedef int ( * PFNGLXWAITVIDEOSYNCSGIPROC) (int divisor, int remainder, unsigned int *count);
    typedef int ( * PFNGLXGETREFRESHRATESGIPROC) (unsigned int *);

    extern PFNGLXGETVIDEOSYNCSGIPROC glXGetVideoSyncSGI;
    extern PFNGLXWAITVIDEOSYNCSGIPROC glXWaitVideoSyncSGI;
    extern PFNGLXGETREFRESHRATESGIPROC glXGetRefreshRateSGI;
};


#ifndef GL_EXT_framebuffer_object
#define GL_EXT_framebuffer_object         1

#define GL_INVALID_FRAMEBUFFER_OPERATION_EXT 0x0506
#define GL_MAX_RENDERBUFFER_SIZE_EXT      0x84E8
#define GL_FRAMEBUFFER_BINDING_EXT        0x8CA6
#define GL_RENDERBUFFER_BINDING_EXT       0x8CA7
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE_EXT 0x8CD0
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_EXT 0x8CD1
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL_EXT 0x8CD2
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE_EXT 0x8CD3
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_3D_ZOFFSET_EXT 0x8CD4
#define GL_FRAMEBUFFER_COMPLETE_EXT       0x8CD5
#define GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT 0x8CD6
#define GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT 0x8CD7
#define GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT 0x8CD9
#define GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT 0x8CDA
#define GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT 0x8CDB
#define GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT 0x8CDC
#define GL_FRAMEBUFFER_UNSUPPORTED_EXT    0x8CDD
#define GL_MAX_COLOR_ATTACHMENTS_EXT      0x8CDF
#define GL_COLOR_ATTACHMENT0_EXT          0x8CE0
#define GL_COLOR_ATTACHMENT1_EXT          0x8CE1
#define GL_COLOR_ATTACHMENT2_EXT          0x8CE2
#define GL_COLOR_ATTACHMENT3_EXT          0x8CE3
#define GL_COLOR_ATTACHMENT4_EXT          0x8CE4
#define GL_COLOR_ATTACHMENT5_EXT          0x8CE5
#define GL_COLOR_ATTACHMENT6_EXT          0x8CE6
#define GL_COLOR_ATTACHMENT7_EXT          0x8CE7
#define GL_COLOR_ATTACHMENT8_EXT          0x8CE8
#define GL_COLOR_ATTACHMENT9_EXT          0x8CE9
#define GL_COLOR_ATTACHMENT10_EXT         0x8CEA
#define GL_COLOR_ATTACHMENT11_EXT         0x8CEB
#define GL_COLOR_ATTACHMENT12_EXT         0x8CEC
#define GL_COLOR_ATTACHMENT13_EXT         0x8CED
#define GL_COLOR_ATTACHMENT14_EXT         0x8CEE
#define GL_COLOR_ATTACHMENT15_EXT         0x8CEF
#define GL_DEPTH_ATTACHMENT_EXT           0x8D00
#define GL_STENCIL_ATTACHMENT_EXT         0x8D20
#define GL_FRAMEBUFFER_EXT                0x8D40
#define GL_RENDERBUFFER_EXT               0x8D41
#define GL_RENDERBUFFER_WIDTH_EXT         0x8D42
#define GL_RENDERBUFFER_HEIGHT_EXT        0x8D43
#define GL_RENDERBUFFER_INTERNAL_FORMAT_EXT 0x8D44
#define GL_STENCIL_INDEX1_EXT             0x8D46
#define GL_STENCIL_INDEX4_EXT             0x8D47
#define GL_STENCIL_INDEX8_EXT             0x8D48
#define GL_STENCIL_INDEX16_EXT            0x8D49
#define GL_RENDERBUFFER_RED_SIZE_EXT      0x8D50
#define GL_RENDERBUFFER_GREEN_SIZE_EXT    0x8D51
#define GL_RENDERBUFFER_BLUE_SIZE_EXT     0x8D52
#define GL_RENDERBUFFER_ALPHA_SIZE_EXT    0x8D53
#define GL_RENDERBUFFER_DEPTH_SIZE_EXT    0x8D54
#define GL_RENDERBUFFER_STENCIL_SIZE_EXT  0x8D55
#endif

namespace glx
{
typedef GLboolean (APIENTRY* PFNGLISRENDERBUFFEREXTPROC) (GLuint renderbuffer);
typedef void (APIENTRY* PFNGLBINDRENDERBUFFEREXTPROC) (GLenum target, GLuint renderbuffer);
typedef void (APIENTRY* PFNGLDELETERENDERBUFFERSEXTPROC) (GLsizei n, const GLuint *renderbuffers);
typedef void (APIENTRY* PFNGLGENRENDERBUFFERSEXTPROC) (GLsizei n, GLuint *renderbuffers);
typedef void (APIENTRY* PFNGLRENDERBUFFERSTORAGEEXTPROC) (GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
typedef void (APIENTRY* PFNGLGETRENDERBUFFERPARAMETERIVEXTPROC) (GLenum target, GLenum pname, GLint *params);
typedef GLboolean (APIENTRY* PFNGLISFRAMEBUFFEREXTPROC) (GLuint framebuffer);
typedef void (APIENTRY* PFNGLBINDFRAMEBUFFEREXTPROC) (GLenum target, GLuint framebuffer);
typedef void (APIENTRY* PFNGLDELETEFRAMEBUFFERSEXTPROC) (GLsizei n, const GLuint *framebuffers);
typedef void (APIENTRY* PFNGLGENFRAMEBUFFERSEXTPROC) (GLsizei n, GLuint *framebuffers);
typedef GLenum (APIENTRY* PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC) (GLenum target);
typedef void (APIENTRY* PFNGLFRAMEBUFFERTEXTURE1DEXTPROC) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
typedef void (APIENTRY* PFNGLFRAMEBUFFERTEXTURE2DEXTPROC) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
typedef void (APIENTRY* PFNGLFRAMEBUFFERTEXTURE3DEXTPROC) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset);
typedef void (APIENTRY* PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC) (GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
typedef void (APIENTRY* PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC) (GLenum target, GLenum attachment, GLenum pname, GLint *params);
typedef void (APIENTRY* PFNGLGENERATEMIPMAPEXTPROC) (GLenum target);

extern PFNGLISRENDERBUFFEREXTPROC glIsRenderbufferEXT;
extern PFNGLBINDRENDERBUFFEREXTPROC glBindRenderbufferEXT;
extern PFNGLDELETERENDERBUFFERSEXTPROC glDeleteRenderbuffersEXT;
extern PFNGLGENRENDERBUFFERSEXTPROC glGenRenderbuffersEXT;
extern PFNGLRENDERBUFFERSTORAGEEXTPROC glRenderbufferStorageEXT;
extern PFNGLGETRENDERBUFFERPARAMETERIVEXTPROC glGetRenderbufferParameterivEXT;
extern PFNGLISFRAMEBUFFEREXTPROC glIsFramebufferEXT;
extern PFNGLBINDFRAMEBUFFEREXTPROC glBindFramebufferEXT;
extern PFNGLDELETEFRAMEBUFFERSEXTPROC glDeleteFramebuffersEXT;
extern PFNGLGENFRAMEBUFFERSEXTPROC glGenFramebuffersEXT;
extern PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC glCheckFramebufferStatusEXT;
extern PFNGLFRAMEBUFFERTEXTURE1DEXTPROC glFramebufferTexture1DEXT;
extern PFNGLFRAMEBUFFERTEXTURE2DEXTPROC glFramebufferTexture2DEXT;
extern PFNGLFRAMEBUFFERTEXTURE3DEXTPROC glFramebufferTexture3DEXT;
extern PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC glFramebufferRenderbufferEXT;
extern PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC glGetFramebufferAttachmentParameterivEXT;
extern PFNGLGENERATEMIPMAPEXTPROC glGenerateMipmapEXT;
};

extern void InitExtension(const char* ext);
extern bool ExtensionSupported(const char *ext);



#endif // _CELENGINE_GLEXT_H_
