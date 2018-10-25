#ifndef __CELESTIA__AUDIO_3D__SOURCE__H__ 
#define __CELESTIA__AUDIO_3D__SOURCE__H__ 

#include <Eigen/Core>
#include <AL/al.h>

namespace Audio3D
{

    class Source
    {
        ALuint _srcId;

    public:
        static ALuint newSource()
        {
            ALuint id;
            alGenSources(1, &id);
            return id;
        }

        Source() { _srcId = newSource(); }

        ~Source()
        {
            alDeleteSources(1, &_srcId);
        }

        ALuint internalId() { return _srcId; }

        bool isValid() { return alIsSource(_srcId); }

// Set float property

        void setFloatProperty(ALenum param, ALfloat val)
        {
            alSourcef(_srcId, param, val);
        }

        void setPitch(ALfloat v)
        {
            setFloatProperty(AL_PITCH, v);
        }

        void setGain(ALfloat v)
        {
            setFloatProperty(AL_GAIN, v);
        }

        void setMinGain(ALfloat v)
        {
            setFloatProperty(AL_MIN_GAIN, v);
        }

        void setMaxGain(ALfloat v)
        {
            setFloatProperty(AL_MAX_GAIN, v);
        }

        void setMaxDistance(ALfloat v)
        {
            setFloatProperty(AL_MAX_DISTANCE, v);
        }

        void setRolloff(ALfloat v)
        {
            setFloatProperty(AL_ROLLOFF_FACTOR, v);
        }

        void setConeOuterGain(ALfloat v)
        {
            setFloatProperty(AL_CONE_OUTER_GAIN, v);
        }

        void setConeInnerAngle(ALfloat v)
        {
            setFloatProperty(AL_CONE_INNER_ANGLE, v);
        }

        void setConeOuterAngle(ALfloat v)
        {
            setFloatProperty(AL_CONE_OUTER_ANGLE, v);
        }

        void setRefDistance(ALfloat v)
        {
            setFloatProperty(AL_REFERENCE_DISTANCE, v);
        }

// Set 3f property

        void set3FloatProperty(ALenum param, ALfloat v1, ALfloat v2, ALfloat v3)
        {
            alSource3f(_srcId, param, v1, v2, v3);
        }

        void setPosition(ALfloat v1, ALfloat v2, ALfloat v3)
        {
            set3FloatProperty(AL_POSITION, v1, v2, v3);
        }

        void setVelocity(ALfloat v1, ALfloat v2, ALfloat v3)
        {
            set3FloatProperty(AL_VELOCITY, v1, v2, v3);
        }

        void setDirection(ALfloat v1, ALfloat v2, ALfloat v3)
        {
            set3FloatProperty(AL_DIRECTION, v1, v2, v3);
        }

// Set vf property

        void setVFloatProperty(ALenum param, ALfloat *v)
        {
            alSourcefv(_srcId, param, v);
        }

        void setPosition(ALfloat *v)
        {
            setVFloatProperty(AL_POSITION, v);
        }

        void setVelocity(ALfloat *v)
        {
            setVFloatProperty(AL_POSITION, v);
        }

        void setDirection(ALfloat *v)
        {
            setVFloatProperty(AL_DIRECTION, v);
        }

        void setVector3DProperty(ALenum param, const Eigen::Vector3d &v)
        {
            set3FloatProperty(param, v[0], v[1], v[3]);
        }

        void setPosition(const Eigen::Vector3d &v)
        {
            setPosition(v[0], v[1], v[2]);
        }

        void setVelocity(const Eigen::Vector3d &v)
        {
            setVelocity(v[0], v[1], v[2]);
        }

        void setDirection(const Eigen::Vector3d &v)
        {
            setDirection(v[0], v[1], v[2]);
        }

// Set integer property

        void setIntProperty(ALenum param, ALint v)
        {
            alSourcei(_srcId, param, v);
        }

        void setRelative(bool v)
        {
            setIntProperty(AL_SOURCE_RELATIVE, v ? AL_TRUE : AL_FALSE);
        }

        void setLooping(bool v)
        {
            setIntProperty(AL_LOOPING, v ? AL_TRUE : AL_FALSE);
        }

        void setBuffer(ALuint v)
        {
            setIntProperty(AL_BUFFER, v);
        }

        void setBuffer(const Buffer &b) {
            setBuffer(b.internalId());
        }

// Get float property

        ALfloat floatProperty(ALenum param) const
        {
            ALfloat ret;
            alGetSourcef(_srcId, param, &ret);
            return ret;
        }

        ALfloat pitch() const
        {
            return floatProperty(AL_PITCH);
        }

        ALfloat gain() const
        {
            return floatProperty(AL_GAIN);
        }

        ALfloat minGain() const
        {
            return floatProperty(AL_MIN_GAIN);
        }

        ALfloat maxGain() const
        {
            return floatProperty(AL_MAX_GAIN);
        }

        ALfloat maxDistance() const
        {
            return floatProperty(AL_MAX_DISTANCE);
        }

        ALfloat rolloff() const
        {
            return floatProperty(AL_ROLLOFF_FACTOR);
        }

        ALfloat coneOuterGain() const
        {
            return floatProperty(AL_CONE_OUTER_GAIN);
        }

        ALfloat coneInnerAngle() const
        {
            return floatProperty(AL_CONE_INNER_ANGLE);
        }

        ALfloat coneOuterAngle() const
        {
            return floatProperty(AL_CONE_OUTER_ANGLE);
        }

        ALfloat refDistance() const
        {
            return floatProperty(AL_REFERENCE_DISTANCE);
        }

// get integer property

        ALint intProperty(ALenum param) const
        {
            ALint ret;
            alGetSourcei(_srcId, param, &ret);
            return ret;
        }

        bool looping()
        {
            return intProperty(AL_LOOPING) > 0;
        }

        bool relative()
        {
            return intProperty(AL_SOURCE_RELATIVE) > 0;
        }

// get float vector property

        Eigen::Vector3d vectorProperty(ALenum param) const
        {
            ALfloat v1, v2, v3;
            alGetSource3f(_srcId, param, &v1, &v2, &v3);
            return Eigen::Vector3d(v1, v2, v3);
        }

        Eigen::Vector3d position() const
        {
            return vectorProperty(AL_POSITION);
        }

        Eigen::Vector3d velocity() const
        {
            return vectorProperty(AL_VELOCITY);
        }

        Eigen::Vector3d direction() const
        {
            return vectorProperty(AL_DIRECTION);
        }

// State transitions

        void play() { alSourcePlay(_srcId); }
        void pause() { alSourcePause(_srcId); }
        void stop() { alSourceStop(_srcId); }
        void queueBuffer(ALuint v) { alSourceQueueBuffers(_srcId, 1, &v); }
        void queueBuffer(const Buffer &b) { queueBuffer(b.internalId()); }
        void unqueueBuffer(ALuint v) { alSourceUnqueueBuffers(_srcId, 1, &v); }
        void unqueueBuffer(const Buffer &b) { unqueueBuffer(b.internalId()); }
    };

}

#endif
