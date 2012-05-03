// glframebuffer.h
//
// Copyright (C) 2004-2010, Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// C++ wrapper for OpenGL framebuffer objects.

#ifndef _GLFRAMEBUFFER_H_
#define _GLFRAMEBUFFER_H_

#include <GL/glew.h>


class GLFrameBufferObject
{
public:
    enum
    {
        ColorAttachment = 0x1,
        DepthAttachment = 0x2
    };
    GLFrameBufferObject(GLuint width, GLuint height, unsigned int attachments);
    ~GLFrameBufferObject();

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

#endif // _GLFRAMEBUFFER_H_
