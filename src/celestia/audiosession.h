#pragma once

#include <celcompat/filesystem.h>

namespace celestia
{

class AudioSession
{
 public:
    explicit AudioSession(const fs::path &path) : filePath(path) {};

    AudioSession() = delete;
    AudioSession(const AudioSession&) = delete;
    AudioSession(AudioSession&&)      = delete;
    AudioSession &operator=(const AudioSession&) = delete;
    AudioSession &operator=(AudioSession&&) = delete;

    virtual ~AudioSession() = default;
    virtual bool play(double seconds = -1.0) = 0;
    virtual bool isPlaying() const = 0;
    virtual void stop() = 0;
    virtual bool seek(double seconds) = 0;

 protected:
    fs::path path() const { return filePath; };

 private:
    fs::path filePath;
};

}
