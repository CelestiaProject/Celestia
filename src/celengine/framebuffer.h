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

#include <celutil/flag.h>
#include "glsupport.h"

class FramebufferObject
{
 public:
    enum class Attachment : unsigned int
    {
        None  = 0,
        Color = 0x1,
        Depth = 0x2,
    };

    FramebufferObject() = delete;
    FramebufferObject(GLuint width, GLuint height, Attachment attachments, int samples = 1, bool useFloatColor = false);
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

    // True for MSAA FBOs whose contents must be resolved before sampling.
    bool isMultisample() const { return m_msaaFboId != 0; }

    // Hint that the listed attachments may be discarded. Acts on the current
    // GL_DRAW_FRAMEBUFFER binding.
    void discard(Attachment attachments) const;

 private:
    explicit FramebufferObject(GLuint fboId); // non-owning wrapper; only bind() is valid

    void generateColorTexture();
    void generateDepthTexture();
    void generateFbo(Attachment attachments);
    void generateMSAAFbo(Attachment attachments);
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

ENUM_CLASS_BITWISE_OPS(FramebufferObject::Attachment)
