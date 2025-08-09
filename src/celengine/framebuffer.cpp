// frambuffer.cpp
//
// Copyright (C) 2010-2020, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "framebuffer.h"

FramebufferObject::FramebufferObject(GLuint width, GLuint height, unsigned int attachments) :
    m_width(width),
    m_height(height),
    m_colorTexId(0),
    m_depthTexId(0),
    m_fboId(0),
    m_status(GL_FRAMEBUFFER_UNSUPPORTED)
{
    if (attachments != 0)
    {
        generateFbo(attachments);
    }
}

FramebufferObject::FramebufferObject(FramebufferObject &&other) noexcept:
    m_width(other.m_width),
    m_height(other.m_height),
    m_colorTexId(other.m_colorTexId),
    m_depthTexId(other.m_depthTexId),
    m_fboId(other.m_fboId),
    m_status(other.m_status)
{
    other.m_fboId  = 0;
    other.m_status = GL_FRAMEBUFFER_UNSUPPORTED;
}

FramebufferObject& FramebufferObject::operator=(FramebufferObject &&other) noexcept
{
    m_width        = other.m_width;
    m_height       = other.m_height;
    m_colorTexId   = other.m_colorTexId;
    m_depthTexId   = other.m_depthTexId;
    m_fboId        = other.m_fboId;
    m_status       = other.m_status;

    other.m_fboId  = 0;
    other.m_status = GL_FRAMEBUFFER_UNSUPPORTED;
    return *this;
}

FramebufferObject::~FramebufferObject()
{
    cleanup();
}

bool
FramebufferObject::isValid() const
{
    return m_status == GL_FRAMEBUFFER_COMPLETE;
}

GLuint
FramebufferObject::colorTexture() const
{
    return m_colorTexId;
}

GLuint
FramebufferObject::depthTexture() const
{
    return m_depthTexId;
}

void
FramebufferObject::generateColorTexture()
{
    // Create and bind the texture
    glGenTextures(1, &m_colorTexId);
    glBindTexture(GL_TEXTURE_2D, m_colorTexId);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Clamp to edge
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Set the texture dimensions
    // Do we need to set GL_DEPTH_COMPONENT24 here?
#ifdef GL_ES
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_width, m_height, 0, GL_RGB, GL_HALF_FLOAT_OES, nullptr);
#else
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, m_width, m_height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
#endif

    // Unbind the texture
    glBindTexture(GL_TEXTURE_2D, 0);
}

#ifdef GL_ES
#define CEL_DEPTH_FORMAT GL_UNSIGNED_INT
#else
#define CEL_DEPTH_FORMAT GL_UNSIGNED_BYTE
#endif

void
FramebufferObject::generateDepthTexture()
{
    // Create and bind the texture
    glGenTextures(1, &m_depthTexId);
    glBindTexture(GL_TEXTURE_2D, m_depthTexId);

#ifndef GL_ES
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
#endif

    // Only nearest sampling is appropriate for depth textures
    // But we can use linear to decrease aliasing
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Clamp to edge
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Set the texture dimensions
    // Do we need to set GL_DEPTH_COMPONENT24 here?

    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, m_width, m_height, 0, GL_DEPTH_COMPONENT, CEL_DEPTH_FORMAT, nullptr);

    // Unbind the texture
    glBindTexture(GL_TEXTURE_2D, 0);
}

void
FramebufferObject::generateFbo(unsigned int attachments)
{
    // Create the FBO
    glGenFramebuffers(1, &m_fboId);
    GLint oldFboId;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &oldFboId);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fboId);

#ifndef GL_ES
    glReadBuffer(GL_NONE);
#endif

    if ((attachments & ColorAttachment) != 0)
    {
        generateColorTexture();
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_colorTexId, 0);
        m_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (m_status != GL_FRAMEBUFFER_COMPLETE)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, oldFboId);
            cleanup();
            return;
        }
    }
#ifndef GL_ES
    else
    {
        // Depth-only rendering; no color buffer.
        glDrawBuffer(GL_NONE);
    }
#endif

    if ((attachments & DepthAttachment) != 0)
    {
        generateDepthTexture();
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depthTexId, 0);
        m_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (m_status != GL_FRAMEBUFFER_COMPLETE)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, oldFboId);
            cleanup();
            return;
        }
    }
    else
    {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, 0, 0);
    }

    // Restore default frame buffer
    glBindFramebuffer(GL_FRAMEBUFFER, oldFboId);
}

// Delete all GL objects associated with this framebuffer object
void
FramebufferObject::cleanup()
{
    if (m_fboId != 0)
    {
        glDeleteFramebuffers(1, &m_fboId);
    }

    if (m_colorTexId != 0)
    {
        glDeleteTextures(1, &m_colorTexId);
    }

    if (m_depthTexId != 0)
    {
        glDeleteTextures(1, &m_depthTexId);
    }
}

bool
FramebufferObject::bind()
{
    if (isValid())
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_fboId);
        return true;
    }

    return false;
}

bool
FramebufferObject::unbind(GLint oldfboId)
{
    glBindFramebuffer(GL_FRAMEBUFFER, oldfboId);
    return true;
}
