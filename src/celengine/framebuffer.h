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
    FramebufferObject(GLuint width, GLuint height, unsigned int attachments);
    FramebufferObject(const FramebufferObject&) = delete;
    FramebufferObject(FramebufferObject&&);
    FramebufferObject& operator=(const FramebufferObject&) = delete;
    FramebufferObject& operator=(FramebufferObject&&);
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

    GLuint colorTexture() const;
    GLuint depthTexture() const;

    bool bind();
    bool unbind();

 private:
    void generateColorTexture();
    void generateDepthTexture();
    void generateFbo(unsigned int attachments);
    void cleanup();

 private:
    GLuint m_width;
    GLuint m_height;
    GLuint m_colorTexId;
    GLuint m_depthTexId;
    GLuint m_fboId;
    GLenum m_status;
};
