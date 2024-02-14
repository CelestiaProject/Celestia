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
    enum class AttachmentType
    {
        None,
        Texture,
        Renderbuffer,
    };

    FramebufferObject() = delete;
    FramebufferObject(GLuint width, GLuint height, AttachmentType colorAttachment, AttachmentType depthAttachment);
    FramebufferObject(const FramebufferObject&) = delete;
    FramebufferObject(FramebufferObject&&) noexcept;
    FramebufferObject& operator=(const FramebufferObject&) = delete;
    FramebufferObject& operator=(FramebufferObject&&) noexcept;
    ~FramebufferObject();

    static bool isSupported();
    bool isValid() const;
    GLuint width() const
    {
        return m_width;
    }

    GLuint height() const
    {
        return m_height;
    }

    GLuint colorAttachment() const;
    GLuint depthAttachment() const;

    AttachmentType colorAttachmentType() const;
    AttachmentType depthAttachmentType() const;

    bool bind();
    bool unbind(GLint oldfboId);

 private:
    void generateColorTexture();
    void generateDepthTexture();
    void generateColorRenderbuffer();
    void generateDepthRenderbuffer();

    void generateFbo();
    void cleanup();

 private:
    GLuint m_width;
    GLuint m_height;
    AttachmentType m_colorAttachmentType;
    AttachmentType m_depthAttachmentType;
    GLuint m_colorAttachmentId  { 0 };
    GLuint m_depthAttachmentId  { 0 };
    GLuint m_fboId              { 0 };
    GLenum m_status             { GL_FRAMEBUFFER_UNSUPPORTED };
};

inline bool FramebufferObject::isSupported()
{
#ifdef GL_ES
    return true;
#else
    return celestia::gl::ARB_framebuffer_object;
#endif
}
