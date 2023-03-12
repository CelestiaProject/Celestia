// buffer.h
//
// Copyright (C) 2023-present, Celestia Development Team.
//
// VBO wrapper.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <celengine/glsupport.h>
#include <celutil/array_view.h>
#include <celutil/nocreate.h>

namespace celestia::gl
{
/**
 * @brief Buffer.
 * 
 * Wraps an OpenGL buffer object.
 */
class Buffer
{
public:
    /**
     * @brief Buffer usage.
     * 
     * Provides information how frequently buffer object is used.
     * @see @ref setData().
     */
    enum class BufferUsage
    {
        //! Set data once and use frequently.
        StaticDraw  = GL_STATIC_DRAW,
        //! Set data frequently and use frequently.
        DynamicDraw = GL_DYNAMIC_DRAW,
        //! Set data once and use a few times.
        StreamDraw  = GL_STREAM_DRAW,
    };

    /**
     * @brief Buffer target.
     * 
     * Provides information about buffer object's purpose.
     * 
     * @see @ref Buffer(TargetHint)
     */
    enum class TargetHint
    {
        //! Store vertex attributes.
        Array        = GL_ARRAY_BUFFER,
        //! Store vertex indices.
        ElementArray = GL_ELEMENT_ARRAY_BUFFER,
    };

    /**
     * @brief Construct a new Buffer object.
     * 
     * Create a C++ object but don't create OpenGL objects.
     */
    explicit Buffer(util::NoCreateT);

    /**
     * @brief Construct a new Buffer object.
     * 
     * Create C++ and OpenGL objects.
     * 
     * @param targetHint Buffer target.
     * 
     * @see @ref TargetHint
     */
    explicit Buffer(TargetHint targetHint = TargetHint::Array);

    /**
     * @brief Construct a new Buffer object.
     * 
     * Create C++ and OpenGL objects and upload data.
     * 
     * @param targetHint Buffer target.
     * @param data Data.
     * @param usage Buffer usage.
     * 
     * @see @ref TargetHint @ref BufferUsage
     */
    Buffer(
        TargetHint                   targetHint,
        util::array_view<const void> data,
        BufferUsage                  usage = BufferUsage::StaticDraw);

    //! Copying is prohibited.
    Buffer(const Buffer &) = delete;

    //! Move constructor.
    Buffer(Buffer &&) noexcept;

    //! Destructor.
    ~Buffer();

    //! Copying is prohibited.
    Buffer& operator=(const Buffer&) = delete;

    //! Move operator.
    Buffer& operator=(Buffer&&) noexcept;

    //! Return an OpenGL identificator of an underlying buffer.
    GLuint id() const;

    //! Bind the buffer to use.
    Buffer& bind();

    //! Unbind the buffer (stop using it).
    void unbind() const;

    /**
     * @brief Copy data from a CPU buffer to GPU buffer.
     * 
     * @param data Data.
     * @param usage Buffer usage policy. @see @ref BufferUsage
     * @return Reference to self.
     */
    Buffer& setData(util::array_view<const void> data, BufferUsage usage = BufferUsage::StaticDraw);

    /**
     * @brief Partially update the Buffer.
     * 
     * @param offset Offset in bytes in GPU memory to copy data to.
     * @param data Data.
     * @return Reference to self.
     */
    Buffer& setSubData(GLintptr offset, util::array_view<const void> data);

    //! Invalidate buffer data.
    Buffer& invalidateData();

    //! Set buffer target. @see @ref TargetHint
    Buffer& setTargetHint(TargetHint hint);

    //! Return target, @see @ref TargetHint.
    TargetHint targetHint() const;

    //! Bind the default buffer (0) to target. @see @ref TargetHint @ref bind()
    static void unbind(TargetHint target);

private:
    void clear() noexcept;

    GLsizeiptr  m_bufferSize{ 0 };
    GLuint      m_id{ 0 };
    TargetHint  m_targetHint{ TargetHint::Array };
    BufferUsage m_usage{ BufferUsage::StaticDraw };
};

inline GLuint
Buffer::id() const
{
    return m_id;
}

inline Buffer::TargetHint
Buffer::targetHint() const
{
    return m_targetHint;
}

} // namespace celestia::gl
