
#include "Audio3DError.h" 

#define AL_ERR_STR(error) case error: return #error

std::string Audio3D::alErrorToString(ALenum code)
{
    switch (code)
    {
        AL_ERR_STR(AL_NO_ERROR);
        AL_ERR_STR(AL_INVALID_NAME);
        AL_ERR_STR(AL_INVALID_ENUM);
        AL_ERR_STR(AL_INVALID_OPERATION);
        AL_ERR_STR(AL_INVALID_VALUE);
        AL_ERR_STR(AL_OUT_OF_MEMORY);
        default: return "Unknown error code";
    }
}

std::string Audio3D::alcErrorToString(ALenum code)
{
    switch(code)
    {
        AL_ERR_STR(ALC_NO_ERROR);
        AL_ERR_STR(ALC_INVALID_DEVICE);
        AL_ERR_STR(ALC_INVALID_CONTEXT);
        AL_ERR_STR(ALC_INVALID_ENUM);
        AL_ERR_STR(ALC_INVALID_VALUE);
        AL_ERR_STR(ALC_OUT_OF_MEMORY);
        default: return "Unknown error code";
    }
}

#undef AL_ERR_STR
