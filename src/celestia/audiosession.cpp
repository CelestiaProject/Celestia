#include "audiosession.h"

namespace celestia
{

AudioSession::AudioSession(const fs::path &path, float volume, float pan, bool loop, bool nopause) : m_volume(volume), m_pan(pan), m_loop(loop), m_nopause(nopause)
{
    if (path.is_relative())
        m_path = "sounds" / path;
    else
        m_path = path;
}

void AudioSession::setVolume(float volume)
{
    if (m_volume != volume)
    {
        m_volume = volume;
        updateVolume();
    }
}

void AudioSession::setPan(float pan)
{
    if (m_pan != pan)
    {
        m_pan = pan;
        updatePan();
    }
}

void AudioSession::setLoop(bool loop)
{
    if (m_loop != loop)
    {
        m_loop = loop;
        updateLoop();
    }
}

void AudioSession::setNoPause(bool nopause)
{
    m_nopause = nopause;
}

}
