#ifndef __CELESTIA__AUDIO_3D__LISTENER__H__ 
#define __CELESTIA__AUDIO_3D__LISTENER__H__

#include <AL/al.h>
#include <Eigen/Core>

namespace Audio3D
{

    class Listener
    {
    public:
        struct Orientation
        {
            Eigen::Vector3d at;
            Eigen::Vector3d up;
        };

        void static setGain(ALfloat v)
        {
            alListenerf(AL_GAIN, v);
        }

        void static set3FloatProperty(ALenum param, ALfloat v1, ALfloat v2, ALfloat v3)
        {
            alListener3f(param, v1, v2, v3);
        }

        void static setPosition(ALfloat v1, ALfloat v2, ALfloat v3)
        {
            set3FloatProperty(AL_POSITION, v1, v2, v3);
        }

        void static setVelocity(ALfloat v1, ALfloat v2, ALfloat v3)
        {
            set3FloatProperty(AL_VELOCITY, v1, v2, v3);
        }

        void static setPosition(const ALfloat *v)
        {
            setPosition(v[0], v[1], v[3]);
        }

        void static setVelocity(const ALfloat *v)
        {
            setVelocity(v[0], v[1], v[3]);
        }

        void static setPosition(const Eigen::Vector3d &v)
        {
            setPosition(v[0], v[1], v[2]);
        }

        void static setVelocity(const Eigen::Vector3d &v)
        {
            setVelocity(v[0], v[1], v[2]);
        }

        void static setOrientation(const ALfloat *v)
        {
            alListenerfv(AL_ORIENTATION, v);
        }

        void static setOrientation(ALfloat v1, ALfloat v2, ALfloat v3, ALfloat v4, ALfloat v5, ALfloat v6)
        {
            ALfloat v[6];
            v[0] = v1;
            v[1] = v2;
            v[2] = v3;
            v[3] = v4;
            v[4] = v5;
            v[5] = v6;
            setOrientation(v);
        }

        void static setOrientation(const Eigen::Vector3d &at, const Eigen::Vector3d &up)
        {
            setOrientation(at[0], at[1], at[2], up[0], up[1], up[2]);
        }

        void static setOrientation(const Orientation &o) {
            setOrientation(o.at, o.up);
        }

        ALfloat static gain()
        {
            ALfloat v;
            alGetListenerf(AL_GAIN, &v);
            return v;
        }

        Eigen::Vector3d static vectorProperty(ALenum param)
        {
            ALfloat v[3];
            alGetListenerfv(param, v);
            return Eigen::Vector3d(v[0], v[1], v[2]);
        }

        Eigen::Vector3d static position()
        {
            return vectorProperty(AL_POSITION);
        }

        Eigen::Vector3d static velocity()
        {
            return vectorProperty(AL_VELOCITY);
        }

        Orientation static orientation()
        {
            Orientation o;
            ALfloat v[6];
            alListenerfv(AL_ORIENTATION, v);
            o.at = Eigen::Vector3d(v[0], v[1], v[2]);
            o.up = Eigen::Vector3d(v[3], v[4], v[5]);
            return o;
        }
    };

}

#endif
