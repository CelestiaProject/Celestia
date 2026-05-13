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
    FramebufferObject(GLuint width, GLuint height, unsigned int attachments, int samples = 1, bool useFloatColor = false);
    FramebufferObject(const FramebufferObject&) = delete;
    FramebufferObject(FramebufferObject&&) noexcept;
    FramebufferObject& operator=(const FramebufferObject&) = delete;
    FramebufferObject& operator=(FramebufferObject&&) noexcept;
    ~FramebufferObject();

    // Create a non-owning wrapper around the currently bound GL framebuffer.
    // Only bind() is valid on the resulting object; other methods will assert.
    static FramebufferObject wrapCurrentBinding();

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

    bool useFloatColor() const
    {
        return m_useFloatColor;
    }

    GLuint colorTexture() const;
    GLuint depthTexture() const;

    bool bind();
    bool unbind(GLint oldfboId);
    bool resolve() const;

 private:
    explicit FramebufferObject(GLuint fboId); // non-owning wrapper; only bind() is valid

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
    GLuint m_msaaFboId{ 0 };
    GLuint m_colorRboId{ 0 };
    GLuint m_depthRboId{ 0 };
    int    m_samples;
    bool   m_useFloatColor;
    GLenum m_status;
    bool   m_owned;
};
