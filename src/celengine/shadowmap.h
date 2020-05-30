#ifdef GL_ES
#  ifdef GL_ONLY_SHADOWS
#    undef GL_ONLY_SHADOWS
#  endif
#  define GL_ONLY_SHADOWS 0
#else
#  ifndef GL_ONLY_SHADOWS
#    define GL_ONLY_SHADOWS 1
#  else
#    define GL_ONLY_SHADOWS 0
#  endif
#endif
