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
#include <vector>

FramebufferObject::FramebufferObject(GLuint width, GLuint height, AttachmentType colorAttachment, AttachmentType depthAttachment) :
    m_width(width),
    m_height(height),
    m_colorAttachmentType(colorAttachment),
    m_depthAttachmentType(depthAttachment)
{
    if (isMultisampleResolveSupported())
    {
        GLint attachmentType;
        glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &attachmentType);
        if (attachmentType == GL_RENDERBUFFER)
        {
            GLint rboId;
            glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &rboId);

            GLint oldRboId;
            glGetIntegerv(GL_RENDERBUFFER_BINDING, &oldRboId);
            glBindRenderbuffer(GL_RENDERBUFFER, rboId);
            glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_SAMPLES, &m_sampleCount);
            glBindRenderbuffer(GL_RENDERBUFFER, oldRboId);

            if (m_sampleCount == 1)
                m_sampleCount = 0;
        }
    }

    if (colorAttachment != AttachmentType::None || depthAttachment != AttachmentType::None)
        generateFbo();
}

FramebufferObject::FramebufferObject(FramebufferObject &&other) noexcept:
    m_width(other.m_width),
    m_height(other.m_height),
    m_colorAttachmentType(other.m_colorAttachmentType),
    m_depthAttachmentType(other.m_depthAttachmentType),
    m_colorAttachmentId(other.m_colorAttachmentId),
    m_depthAttachmentId(other.m_depthAttachmentId),
    m_fboId(other.m_fboId),
    m_sampleCount(other.m_sampleCount),
    m_sampleBuffer(other.m_sampleBuffer),
    m_sampleColorBuffer(other.m_sampleColorBuffer),
    m_sampleDepthBuffer(other.m_sampleDepthBuffer),
    m_status(other.m_status)
{
    other.m_fboId  = 0;
    other.m_colorAttachmentId = 0;
    other.m_depthAttachmentId = 0;
    other.m_sampleBuffer = 0;
    other.m_sampleColorBuffer = 0;
    other.m_sampleDepthBuffer = 0;
    other.m_status = GL_FRAMEBUFFER_UNSUPPORTED;
}

FramebufferObject& FramebufferObject::operator=(FramebufferObject &&other) noexcept
{
    m_width                 = other.m_width;
    m_height                = other.m_height;
    m_colorAttachmentType   = other.m_colorAttachmentType;
    m_depthAttachmentType   = other.m_depthAttachmentType;
    m_colorAttachmentId     = other.m_colorAttachmentId;
    m_depthAttachmentId     = other.m_depthAttachmentId;
    m_sampleCount           = other.m_sampleCount;
    m_sampleBuffer          = other.m_sampleBuffer;
    m_sampleColorBuffer     = other.m_sampleColorBuffer;
    m_sampleDepthBuffer     = other.m_sampleDepthBuffer;
    m_fboId                 = other.m_fboId;
    m_status                = other.m_status;

    other.m_fboId  = 0;
    other.m_colorAttachmentId = 0;
    other.m_depthAttachmentId = 0;
    other.m_sampleBuffer = 0;
    other.m_sampleColorBuffer = 0;
    other.m_sampleDepthBuffer = 0;
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
FramebufferObject::colorAttachment() const
{
    return m_colorAttachmentId;
}

GLuint
FramebufferObject::depthAttachment() const
{
    return m_depthAttachmentId;
}

FramebufferObject::AttachmentType
FramebufferObject::colorAttachmentType() const
{
    return m_colorAttachmentType;
}

FramebufferObject::AttachmentType
FramebufferObject::depthAttachmentType() const
{
    return m_depthAttachmentType;
}

void
FramebufferObject::generateColorTexture()
{
    // Create and bind the texture
    glGenTextures(1, &m_colorAttachmentId);
    glBindTexture(GL_TEXTURE_2D, m_colorAttachmentId);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Clamp to edge
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Set the texture dimensions
#ifdef GL_ES
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
#else
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, m_width, m_height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
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
    glGenTextures(1, &m_depthAttachmentId);
    glBindTexture(GL_TEXTURE_2D, m_depthAttachmentId);

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
FramebufferObject::generateColorRenderbuffer()
{
    glGenRenderbuffers(1, &m_colorAttachmentId);
    glBindRenderbuffer(GL_RENDERBUFFER, m_colorAttachmentId);
#ifdef GL_ES
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA, m_width, m_height);
#else
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGB8, m_width, m_height);
#endif
}

void
FramebufferObject::generateDepthRenderbuffer()
{
    glGenRenderbuffers(1, &m_depthAttachmentId);
    glBindRenderbuffer(GL_RENDERBUFFER, m_depthAttachmentId);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, m_width, m_height);
}

void
FramebufferObject::generateSampleColorRenderbuffer()
{
    glGenRenderbuffers(1, &m_sampleColorBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, m_sampleColorBuffer);
#ifdef GL_ES
    if (celestia::gl::checkVersion(celestia::gl::Version::GLES_3))
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, m_sampleCount, GL_RGBA, m_width, m_height);
    else
        glRenderbufferStorageMultisampleAPPLE(GL_RENDERBUFFER, m_sampleCount, GL_RGBA, m_width, m_height);
#else
    glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER, m_sampleCount, GL_RGB8, m_width, m_height);
#endif
}

void
FramebufferObject::generateSampleDepthRenderbuffer()
{
    glGenRenderbuffers(1, &m_sampleDepthBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, m_sampleDepthBuffer);
#ifdef GL_ES
    if (celestia::gl::checkVersion(celestia::gl::Version::GLES_3))
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, m_sampleCount, GL_DEPTH_COMPONENT, m_width, m_height);
    else
        glRenderbufferStorageMultisampleAPPLE(GL_RENDERBUFFER, m_sampleCount, GL_DEPTH_COMPONENT, m_width, m_height);
#else
    glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER, m_sampleCount, GL_DEPTH_COMPONENT, m_width, m_height);
#endif
}

void
FramebufferObject::generateFbo()
{
    GLint oldFboId;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &oldFboId);

    if (m_sampleCount != 0)
    {
        // Create multi sample buffer
        glGenFramebuffers(1, &m_sampleBuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, m_sampleBuffer);

        if (m_colorAttachmentType != AttachmentType::None)
        {
            generateSampleColorRenderbuffer();
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_sampleColorBuffer);

            if (!checkStatus(oldFboId))
                return;
        }

        if (m_depthAttachmentType != AttachmentType::None)
        {
            generateSampleDepthRenderbuffer();
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_sampleDepthBuffer);

            if (!checkStatus(oldFboId))
                return;
        }
    }

    // Create the FBO
    glGenFramebuffers(1, &m_fboId);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fboId);

#ifndef GL_ES
    glReadBuffer(GL_NONE);
#endif

    switch (m_colorAttachmentType)
    {
    case AttachmentType::Texture:
        generateColorTexture();
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_colorAttachmentId, 0);
        break;
    case AttachmentType::Renderbuffer:
        generateColorRenderbuffer();
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_colorAttachmentId);
        break;
#ifdef GL_ES
    default:
        break;
#else
    default:
        // Depth-only rendering; no color buffer.
        glDrawBuffer(GL_NONE);
#endif
    }

    if (m_colorAttachmentType != AttachmentType::None && !checkStatus(oldFboId))
        return;

    switch (m_depthAttachmentType) {
    case AttachmentType::Texture:
        generateDepthTexture();
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depthAttachmentId, 0);
        break;
    case AttachmentType::Renderbuffer:
        generateDepthRenderbuffer();
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depthAttachmentId);
        break;
    default:
        break;
    }

    if (m_depthAttachmentType != AttachmentType::None && !checkStatus(oldFboId))
        return;

    // Restore default frame buffer
    glBindFramebuffer(GL_FRAMEBUFFER, oldFboId);
}

bool
FramebufferObject::checkStatus(GLint oldFboId)
{
    m_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (m_status != GL_FRAMEBUFFER_COMPLETE)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, oldFboId);
        cleanup();
        return false;
    }

    return true;
}

// Delete all GL objects associated with this framebuffer object
void
FramebufferObject::cleanup()
{
    if (m_sampleBuffer != 0)
        glDeleteFramebuffers(1, &m_sampleBuffer);

    if (m_sampleColorBuffer != 0)
        glDeleteRenderbuffers(1, &m_sampleColorBuffer);

    if (m_sampleDepthBuffer != 0)
        glDeleteRenderbuffers(1, &m_sampleDepthBuffer);

    if (m_fboId != 0)
        glDeleteFramebuffers(1, &m_fboId);

    if (m_colorAttachmentId != 0)
    {
        switch (m_colorAttachmentType)
        {
        case AttachmentType::Texture:
            glDeleteTextures(1, &m_colorAttachmentId);
            break;
        case AttachmentType::Renderbuffer:
            glDeleteRenderbuffers(1, &m_colorAttachmentId);
            break;
        default:
            break;
        }
    }

    if (m_depthAttachmentId != 0)
    {
        switch (m_depthAttachmentType)
        {
        case AttachmentType::Texture:
            glDeleteTextures(1, &m_depthAttachmentId);
            break;
        case AttachmentType::Renderbuffer:
            glDeleteRenderbuffers(1, &m_depthAttachmentId);
            break;
        default:
            break;
        }
    }
}

bool
FramebufferObject::bind()
{
    if (isValid())
    {
        GLuint fbo = m_sampleCount != 0 ? m_sampleBuffer : m_fboId;
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        return true;
    }

    return false;
}

bool
FramebufferObject::unbind(GLint oldfboId)
{
    if (m_sampleCount != 0)
    {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, m_sampleBuffer);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_fboId);

        std::vector<GLenum> attachments;
        GLbitfield attachmentBits = 0;

        if (m_colorAttachmentType != AttachmentType::None)
        {
            attachments.push_back(GL_COLOR_ATTACHMENT0);
            attachmentBits |= GL_COLOR_BUFFER_BIT;
        }
        if (m_depthAttachmentType != AttachmentType::None)
        {
            attachments.push_back(GL_DEPTH_COMPONENT);
            attachmentBits |= GL_DEPTH_BUFFER_BIT;
        }

#ifdef GL_ES
        if (celestia::gl::checkVersion(celestia::gl::GLES_3))
        {
            glBlitFramebuffer(0, 0, m_width, m_height, 0, 0, m_width, m_height, attachmentBits, GL_NEAREST);
            glInvalidateFramebuffer(GL_READ_FRAMEBUFFER, attachments.size(), attachments.data());
        }
        else
        {
            glResolveMultisampleFramebufferAPPLE();
            if (celestia::gl::EXT_discard_framebuffer)
                glDiscardFramebufferEXT(GL_READ_FRAMEBUFFER, attachments.size(), attachments.data());
        }
#else
        glBlitFramebufferEXT(0, 0, m_width, m_height, 0, 0, m_width, m_height, attachmentBits, GL_NEAREST);
        if (celestia::gl::ARB_invalidate_subdata)
            glInvalidateFramebuffer(GL_READ_FRAMEBUFFER, attachments.size(), attachments.data());
#endif
    }

    glBindFramebuffer(GL_FRAMEBUFFER, oldfboId);
    return true;
}
