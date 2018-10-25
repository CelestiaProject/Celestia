#ifndef __CELESTIA__AUDIO_3D__MANAGER__H__ 
#define __CELESTIA__AUDIO_3D__MANAGER__H__ 

#include <AL/al.h>
#include <vector>
#include <string>

namespace Audio3D
{

    class Manager {
    public:
        Manager(int *, char **);

        static ALint intProperty(ALenum param)
        {
            return alGetInteger(param);
        }

        static ALfloat floatProperty(ALenum param)
        {
            return alGetFloat(param);
        }

        static ALdouble doubleProperty(ALenum param)
        {
            return alGetDouble(param);
        }

        static ALfloat dopplerFactor()
        {
            return floatProperty(AL_DOPPLER_FACTOR);
        }

        static ALfloat soundSpeed()
        {
            return floatProperty(AL_SPEED_OF_SOUND);
        }

        static ALint distanceModel()
        {
            return alGetInteger(AL_DISTANCE_MODEL);
        }

        static const ALchar *stringProperty(ALenum param)
        {
            return alGetString(param);
        }

        static const ALchar *vendor()
        {
            return stringProperty(AL_VENDOR);
        }

        static const ALchar *version()
        {
            return stringProperty(AL_VERSION);
        }

        static const ALchar *renderer()
        {
            return stringProperty(AL_RENDERER);
        }

        static const ALchar *extensions()
        {
            return stringProperty(AL_EXTENSIONS);
        }

        static void setModel(ALenum param)
        {
            alDistanceModel(param);
        }

        static void setDopplerFactor(ALfloat v)
        {
            alDopplerFactor(v);
        }

        static void setSoundSpeed(ALfloat v)
        {
            alSpeedOfSound(v);
        }

        static std::vector<std::string> devices();
    };

}

#endif
