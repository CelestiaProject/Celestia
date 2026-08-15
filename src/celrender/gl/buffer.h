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

#include <atomic>
#include <cstdint>

#include <boost/smart_ptr/intrusive_ptr.hpp>

#include <celengine/glsupport.h>
#include <celutil/array_view.h>
#include <celutil/classops.h>

namespace celestia::gl
{

/**
 * @brief Buffer.
 *
 * Wraps an OpenGL buffer object.
 */
class Buffer : private util::NoMove
{
public:
    using SharedPtr = boost::intrusive_ptr<Buffer>;

    /**
     * @brief Buffer usage.
     *
     * Provides information how frequently buffer object is used.
     * @see @ref setData().
     */
    enum class BufferUsage : GLenum
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
    enum class TargetHint : GLenum
    {
        //! Store vertex attributes.
        Array        = GL_ARRAY_BUFFER,
        //! Store vertex indices.
        ElementArray = GL_ELEMENT_ARRAY_BUFFER,
    };

    //! Destructor.
    ~Buffer();

    //! Return an OpenGL identificator of an underlying buffer.
    GLuint id() const noexcept;

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
    Buffer& setData(util::array_view<void> data, BufferUsage usage = BufferUsage::StaticDraw);

    /**
     * @brief Partially update the Buffer.
     *
     * @param offset Offset in bytes in GPU memory to copy data to.
     * @param data Data.
     * @return Reference to self.
     */
    Buffer& setSubData(GLintptr offset, util::array_view<void> data);

    //! Invalidate buffer data.
    Buffer& invalidateData();

    //! Return target, @see @ref TargetHint.
    TargetHint targetHint() const noexcept;

    //! Bind the default buffer (0) to target. @see @ref TargetHint @ref bind()
    static void unbind(TargetHint target);

    /**
     * @brief Construct a new Buffer object.
     *
     * Create C++ and OpenGL objects.
     *
     * @param targetHint Buffer target.
     *
     * @see @ref TargetHint
     */
    static SharedPtr create(TargetHint targetHint);

    static SharedPtr create(TargetHint  targetHint,
                            GLsizeiptr  size,
                            BufferUsage usage);

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
    static SharedPtr create(TargetHint             targetHint,
                            util::array_view<void> data,
                            BufferUsage            usage = BufferUsage::StaticDraw);

private:
    Buffer(GLuint, TargetHint);

    //! Reset object to initial state
    void clear();
    //! Destroy underlying OpenGL resources
    void destroy() noexcept;

    void setData(const void*, GLsizeiptr, BufferUsage);

    inline friend void
    intrusive_ptr_add_ref(Buffer* p)
    {
        p->m_refCount.fetch_add(1, std::memory_order_relaxed);
    }

    inline friend void
    intrusive_ptr_release(Buffer* p)
    {
        if (p->m_refCount.fetch_sub(1, std::memory_order_acq_rel) == 1)
            delete p; //NOSONAR
    }


    //! Buffer size
    GLsizeiptr m_bufferSize{ 0 };

    //! Buffer Id (OpenGL name)
    GLuint m_id{ 0 };

    //! Buffer target hint, @see @ref TargetHint
    TargetHint m_targetHint{ TargetHint::Array };
    //! Buffer usage hint, @see @ref BufferUsage

    BufferUsage m_usage{ BufferUsage::StaticDraw };

    std::atomic<std::uint32_t> m_refCount{ 1 };
};

inline GLuint
Buffer::id() const noexcept
{
    return m_id;
}

inline Buffer::TargetHint
Buffer::targetHint() const noexcept
{
    return m_targetHint;
}

} // namespace celestia::gl
