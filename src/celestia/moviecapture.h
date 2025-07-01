// moviecapture.h
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <filesystem>

class Renderer;

class MovieCapture
{
public:
    MovieCapture(const Renderer *r) : renderer(r) {};
    virtual ~MovieCapture() = default;

    virtual bool start(const std::filesystem::path& filename,
                       int width, int height,
                       float fps) = 0;
    virtual bool end() = 0;
    virtual bool captureFrame() = 0;

    virtual int getFrameCount() const = 0;
    virtual int getWidth() const = 0;
    virtual int getHeight() const = 0;
    virtual float getFrameRate() const = 0;

    virtual void setAspectRatio(int aspectNumerator, int aspectDenominator) = 0;
    virtual void setQuality(float) = 0;
    bool recordingStatus() const { return m_recordingStatus; }
    void recordingStatus(bool started)
    {
        m_recordingStatus = started;
        recordingStatusUpdated(started);
    }

protected:
    const Renderer *renderer{ nullptr };

    virtual void recordingStatusUpdated(bool started) = 0;

private:
    bool m_recordingStatus;
};
