// framebuffer.h
//
// Copyright (C) 2010-2020, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include "glsupport.h"

class FramebufferObject
{
 public:
    enum
    {
        ColorAttachment = 0x1,
        DepthAttachment = 0x2
    };
    FramebufferObject() = delete;
    FramebufferObject(GLuint width, GLuint height, unsigned int attachments, int samples = 1);
    FramebufferObject(const FramebufferObject&) = delete;
    FramebufferObject(FramebufferObject&&) noexcept;
    FramebufferObject& operator=(const FramebufferObject&) = delete;
    FramebufferObject& operator=(FramebufferObject&&) noexcept;
    ~FramebufferObject();

    bool isValid() const;
    GLuint width() const
    {
        return m_width;
    }

    GLuint height() const
    {
        return m_height;
    }

    int samples() const
    {
        return m_samples;
    }

    GLuint colorTexture() const;
    GLuint depthTexture() const;

    bool bind();
    bool unbind(GLint oldfboId);
    bool resolve();

 private:
    void generateColorTexture();
    void generateDepthTexture();
    void generateFbo(unsigned int attachments);
    void generateMSAAFbo(unsigned int attachments);
    void cleanup();

 private:
    GLuint m_width;
    GLuint m_height;
    GLuint m_colorTexId;
    GLuint m_depthTexId;
    GLuint m_fboId;
    GLuint m_msaaFboId;
    GLuint m_colorRboId;
    GLuint m_depthRboId;
    int    m_samples;
    GLenum m_status;
};
