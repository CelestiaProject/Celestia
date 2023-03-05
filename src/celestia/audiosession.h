#pragma once

#include <celcompat/filesystem.h>

namespace celestia
{

constexpr inline float minAudioPan = -1.0f;
constexpr inline float maxAudioPan = 1.0f;
constexpr inline float defaultAudioPan = 0.0f;
constexpr inline float minAudioVolume = 0.0f;
constexpr inline float maxAudioVolume = 1.0f;
constexpr inline float defaultAudioVolume = 1.0f;
constexpr inline int minAudioChannel = 0;
constexpr inline int defaultAudioChannel = 0;

class AudioSession
{
 public:
    AudioSession(const fs::path &path, float volume, float pan, bool loop, bool nopause);

    AudioSession() = delete;
    AudioSession(const AudioSession&) = delete;
    AudioSession(AudioSession&&)      = delete;
    AudioSession &operator=(const AudioSession&) = delete;
    AudioSession &operator=(AudioSession&&) = delete;

    virtual ~AudioSession() = default;
    virtual bool play(double startTime = -1.0) = 0;
    virtual bool isPlaying() const = 0;
    virtual void stop() = 0;
    virtual bool seek(double seconds) = 0;

    void setVolume(float volume);
    void setPan(float pan);
    void setLoop(bool loop);
    void setNoPause(bool nopause);

    bool nopause() const { return m_nopause; }

 protected:
    fs::path path() const { return m_path; }
    float volume() const { return m_volume; }
    float pan() const { return m_pan; }
    bool loop() const { return m_loop; }

    virtual void updateVolume() = 0;
    virtual void updatePan() = 0;
    virtual void updateLoop() = 0;

 private:
    fs::path m_path;
    float m_volume;
    float m_pan;
    bool m_loop;
    bool m_nopause;
};

}
