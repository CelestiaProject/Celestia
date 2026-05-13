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

#include <cassert>

FramebufferObject::FramebufferObject(GLuint width, GLuint height, unsigned int attachments, int samples, bool useFloatColor) :
    m_width(width),
    m_height(height),
    m_colorTexId(0),
    m_depthTexId(0),
    m_fboId(0),
    m_samples(samples > 1 ? samples : 1),
    m_useFloatColor(useFloatColor),
    m_status(GL_FRAMEBUFFER_UNSUPPORTED),
    m_owned(true)
{
    if (attachments != 0)
    {
        generateFbo(attachments);
    }
}

FramebufferObject::FramebufferObject(GLuint fboId) :
    m_width(0),
    m_height(0),
    m_colorTexId(0),
    m_depthTexId(0),
    m_fboId(fboId),
    m_samples(1),
    m_useFloatColor(false),
    m_status(GL_FRAMEBUFFER_COMPLETE),
    m_owned(false)
{
}

FramebufferObject FramebufferObject::wrapCurrentBinding()
{
    GLint id;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &id);
    return FramebufferObject(static_cast<GLuint>(id));
}

FramebufferObject::FramebufferObject(FramebufferObject &&other) noexcept:
    m_width(other.m_width),
    m_height(other.m_height),
    m_colorTexId(other.m_colorTexId),
    m_depthTexId(other.m_depthTexId),
    m_fboId(other.m_fboId),
    m_msaaFboId(other.m_msaaFboId),
    m_colorRboId(other.m_colorRboId),
    m_depthRboId(other.m_depthRboId),
    m_samples(other.m_samples),
    m_useFloatColor(other.m_useFloatColor),
    m_status(other.m_status),
    m_owned(other.m_owned)
{
    other.m_owned      = false;
    other.m_fboId      = 0;
    other.m_msaaFboId  = 0;
    other.m_colorRboId = 0;
    other.m_depthRboId = 0;
    other.m_status     = GL_FRAMEBUFFER_UNSUPPORTED;
}

FramebufferObject& FramebufferObject::operator=(FramebufferObject &&other) noexcept
{
    m_width         = other.m_width;
    m_height        = other.m_height;
    m_colorTexId    = other.m_colorTexId;
    m_depthTexId    = other.m_depthTexId;
    m_fboId         = other.m_fboId;
    m_msaaFboId     = other.m_msaaFboId;
    m_colorRboId    = other.m_colorRboId;
    m_depthRboId    = other.m_depthRboId;
    m_samples       = other.m_samples;
    m_useFloatColor = other.m_useFloatColor;
    m_status        = other.m_status;
    m_owned         = other.m_owned;

    other.m_owned      = false;
    other.m_fboId      = 0;
    other.m_msaaFboId  = 0;
    other.m_colorRboId = 0;
    other.m_depthRboId = 0;
    other.m_status     = GL_FRAMEBUFFER_UNSUPPORTED;
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
    assert(m_owned && "colorTexture() called on non-owning FBO wrapper");
    return m_colorTexId;
}

GLuint
FramebufferObject::depthTexture() const
{
    assert(m_owned && "depthTexture() called on non-owning FBO wrapper");
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
#ifdef GL_ES
    GLenum format = GL_RGBA;
    GLint internalFormat;
    GLenum type;
    if (celestia::gl::checkVersion(celestia::gl::GLES_3_0))
    {
        internalFormat = m_useFloatColor ? GL_RGBA16F : GL_RGBA8;
        type = m_useFloatColor ? GL_HALF_FLOAT : GL_UNSIGNED_BYTE;
    }
    else
    {
        internalFormat = GL_RGBA;
        type = m_useFloatColor ? GL_HALF_FLOAT_OES : GL_UNSIGNED_BYTE;
    }
#else
    GLint internalFormat = m_useFloatColor ? GL_RGBA16F : GL_RGB8;
    GLenum format = m_useFloatColor ? GL_RGBA : GL_RGB;
    GLenum type = m_useFloatColor ? GL_FLOAT : GL_UNSIGNED_BYTE;
#endif
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_width, m_height, 0, format, type, nullptr);

    // Unbind the texture
    glBindTexture(GL_TEXTURE_2D, 0);
}


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
#ifdef GL_ES
    GLint internalFormat;
    GLenum type;
    if (celestia::gl::checkVersion(celestia::gl::GLES_3_0))
    {
        internalFormat = GL_DEPTH_COMPONENT24;
        type = GL_UNSIGNED_INT;
    }
    else
    {
        internalFormat = GL_DEPTH_COMPONENT;
        type = celestia::gl::OES_depth24 ? GL_UNSIGNED_INT :  GL_UNSIGNED_SHORT;
    }
#else
    GLint internalFormat = GL_DEPTH_COMPONENT;
    GLenum type = GL_UNSIGNED_BYTE;
#endif
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_width, m_height, 0, GL_DEPTH_COMPONENT, type, nullptr);

    // Unbind the texture
    glBindTexture(GL_TEXTURE_2D, 0);
}

void
FramebufferObject::generateFbo(unsigned int attachments)
{
    bool useRenderbufferMSAA = m_samples > 1 && (attachments & ColorAttachment) != 0;

    // Create the texture-based FBO (resolve target for renderbuffer MSAA,
    // or the sole FBO for non-MSAA paths).
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
        if (!useRenderbufferMSAA)
        {
            generateDepthTexture();
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depthTexId, 0);
        }
        // else (renderbuffer MSAA): depth goes into m_msaaFboId; nothing to attach here.

        if (!useRenderbufferMSAA)
        {
            m_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
            if (m_status != GL_FRAMEBUFFER_COMPLETE)
            {
                glBindFramebuffer(GL_FRAMEBUFFER, oldFboId);
                cleanup();
                return;
            }
        }
    }
    else if (!useRenderbufferMSAA)
    {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, 0, 0);
    }

    // Restore previous framebuffer before potentially creating the MSAA FBO.
    glBindFramebuffer(GL_FRAMEBUFFER, oldFboId);

    if (useRenderbufferMSAA)
        generateMSAAFbo(attachments);
}

void
FramebufferObject::generateMSAAFbo(unsigned int attachments)
{
    // Create an MSAA FBO backed by renderbuffers.  The scene is rendered here;
    // resolve() blits the color buffer to the texture-based m_fboId afterward.
    GLint oldFboId;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &oldFboId);

    // Clamp the requested sample count to what the driver actually supports.
    // This avoids the most common reason for glCheckFramebufferStatus failure.
    GLint maxSamples = 1;
    glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);
    if (m_samples > maxSamples)
        m_samples = maxSamples;

    glGenFramebuffers(1, &m_msaaFboId);
    glBindFramebuffer(GL_FRAMEBUFFER, m_msaaFboId);

    if ((attachments & ColorAttachment) != 0)
    {
        glGenRenderbuffers(1, &m_colorRboId);
        glBindRenderbuffer(GL_RENDERBUFFER, m_colorRboId);
#ifdef GL_ES
        GLenum fmt = m_useFloatColor ? GL_RGBA16F : GL_RGBA8;
#else
        GLenum fmt = m_useFloatColor ? GL_RGBA16F : GL_RGB8;
#endif
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, m_samples, fmt, m_width, m_height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_colorRboId);
    }

    if ((attachments & DepthAttachment) != 0)
    {
        glGenRenderbuffers(1, &m_depthRboId);
        glBindRenderbuffer(GL_RENDERBUFFER, m_depthRboId);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, m_samples, GL_DEPTH_COMPONENT24, m_width, m_height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depthRboId);
    }

    m_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glBindFramebuffer(GL_FRAMEBUFFER, oldFboId);

    if (m_status != GL_FRAMEBUFFER_COMPLETE)
    {
        // MSAA FBO creation failed; clean up MSAA resources and fall back to
        // the texture-based FBO without MSAA.
        glDeleteFramebuffers(1, &m_msaaFboId);
        m_msaaFboId = 0;
        if (m_colorRboId != 0)
        {
            glDeleteRenderbuffers(1, &m_colorRboId);
            m_colorRboId = 0;
        }
        if (m_depthRboId != 0)
        {
            glDeleteRenderbuffers(1, &m_depthRboId);
            m_depthRboId = 0;
        }
        m_samples = 1;

        // The texture-based FBO (m_fboId) was created without a depth attachment
        // because depth was supposed to come from the MSAA renderbuffer.  Now that
        // it becomes the sole render target we need to add depth so the scene
        // renders correctly.
        glBindFramebuffer(GL_FRAMEBUFFER, m_fboId);
        if ((attachments & DepthAttachment) != 0)
        {
            generateDepthTexture();
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depthTexId, 0);
        }
        m_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        glBindFramebuffer(GL_FRAMEBUFFER, oldFboId);
    }
}

// Delete all GL objects associated with this framebuffer object
void
FramebufferObject::cleanup()
{
    if (!m_owned)
        return;

    if (m_msaaFboId != 0)
    {
        glDeleteFramebuffers(1, &m_msaaFboId);
        m_msaaFboId = 0;
    }

    if (m_colorRboId != 0)
    {
        glDeleteRenderbuffers(1, &m_colorRboId);
        m_colorRboId = 0;
    }

    if (m_depthRboId != 0)
    {
        glDeleteRenderbuffers(1, &m_depthRboId);
        m_depthRboId = 0;
    }

    if (m_fboId != 0)
    {
        glDeleteFramebuffers(1, &m_fboId);
        m_fboId = 0;
    }

    if (m_colorTexId != 0)
    {
        glDeleteTextures(1, &m_colorTexId);
        m_colorTexId = 0;
    }

    if (m_depthTexId != 0)
    {
        glDeleteTextures(1, &m_depthTexId);
        m_depthTexId = 0;
    }
}

bool
FramebufferObject::bind()
{
    if (isValid())
    {
        // For non-owning wrappers (e.g. the screen framebuffer), bind the stored ID directly.
        // For owned FBOs, prefer the MSAA renderbuffer FBO when available.
        glBindFramebuffer(GL_FRAMEBUFFER, (!m_owned || m_msaaFboId == 0) ? m_fboId : m_msaaFboId);
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

bool
FramebufferObject::resolve() const
{
    assert(m_owned && "resolve() called on non-owning FBO wrapper");
    // No explicit resolve needed for non-MSAA FBOs (m_msaaFboId == 0, m_samples == 1).
    if (m_msaaFboId == 0)
        return true;

    // Desktop GL / GLES3: blit the MSAA color renderbuffer into the resolve texture FBO.
    // GL_READ_FRAMEBUFFER / GL_DRAW_FRAMEBUFFER and glBlitFramebuffer are available on
    // desktop GL (via ARB_framebuffer_object, which is required) and GLES 3.0+.
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_msaaFboId);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_fboId);
    glBlitFramebuffer(0, 0, static_cast<GLint>(m_width), static_cast<GLint>(m_height),
                      0, 0, static_cast<GLint>(m_width), static_cast<GLint>(m_height),
                      GL_COLOR_BUFFER_BIT, GL_NEAREST);
    return true;
}
