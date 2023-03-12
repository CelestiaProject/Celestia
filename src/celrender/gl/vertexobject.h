// vertexobject.h
//
// Copyright (C) 2023-present, Celestia Development Team.
//
// VAO wrapper.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <cstdint>
#include <vector>
#include <celengine/glsupport.h>
#include <celutil/nocreate.h>

namespace celestia::gl
{

class Buffer;

/**
 * @brief VertexObject
 *
 * Wraps an Vertex Array Object if provided by OpenGL or implemnts its functions.
 */

class VertexObject
{
public:
    /**
     * Vertex data type.
     *
     * @see @ref addVertexBuffer()
     */
    enum class DataType
    {
        Byte            = GL_BYTE,
        UnsignedByte    = GL_UNSIGNED_BYTE,
        Short           = GL_SHORT,
        UnsignedShort   = GL_UNSIGNED_SHORT,
        Int             = GL_INT,
        UnsignedInt     = GL_UNSIGNED_INT,
        Half            = GL_HALF_FLOAT,
        Float           = GL_FLOAT,
    };

    /**
     * Index types. Unsigned bytes are not supported as OpenGL ES and WebGL don't provide them.
     *
     * @see @ref setIndexBuffer() @ref isIndexed()
     */
    enum class IndexType
    {
        UnsignedShort   = GL_UNSIGNED_SHORT,
        UnsignedInt     = GL_UNSIGNED_INT,
    };

    /**
     * Primitive type to draw.
     *
     * @see @ref draw() @ref draw(Primitive, int, int)
     */
    enum class Primitive
    {
        Points          = GL_POINTS,
        Lines           = GL_LINES,
        LineStrip       = GL_LINE_STRIP,
        LineLoop        = GL_LINE_LOOP,
        Triangles       = GL_TRIANGLES,
        TriangleStrip   = GL_TRIANGLE_STRIP,
        TriangleFan     = GL_TRIANGLE_FAN,
    };

    /**
     * @brief Construct a new VertexObject object.
     *
     * Create a C++ object but don't create OpenGL objects.
     */
    explicit VertexObject(util::NoCreateT);

    /**
     * @brief Construct a new VertexObject object.
     *
     * Create C++ and OpenGL objects.
     *
     * @param primitive Primitive type to draw.
     *
     * @see @ref Primitive
     */
    explicit VertexObject(Primitive primitive = Primitive::Triangles);

    //! Copying is prohibited.
    VertexObject(const VertexObject&) = delete;

    //! Move constructor.
    VertexObject(VertexObject&&) noexcept;

    //! Destructor.
    ~VertexObject();

    //! Copying is prohibited.
    VertexObject& operator=(const VertexObject&) = delete;

    //! Move operator.
    VertexObject& operator=(VertexObject&&) noexcept;

    /**
     * @brief Return an OpenGL identificator.
     *
     * Return an OpenGL identificator of an underlying object if supported or 0 otherwise.
     */
    GLuint id() const;

    /**
     * @brief Render VertexObject.
     *
     * Render VertexObject using a primitive set by constructor or setPrimitive.
     *
     * @return Reference to self.
     *
     * @see @ref Primitive @ref VertexObject(Primitive)
     */
    VertexObject& draw();

    /**
     * @brief Render VertexObject.
     *
     * Render VertexObject using a default primitive.
     *
     * @param count Number of vertices to draw.
     * @param first First vertex to draw.
     * @return Reference to self.
     *
     * @see @ref Primitive @ref VertexObject(Primitive)
     */
    VertexObject& draw(int count, int first = 0);

    /**
     * @brief Render VertexObject.
     *
     * Render VertexObject using a primitive provided.
     *
     * @param primitive Primitive.
     * @param count Number of vertices to draw.
     * @param first First vertex to draw.
     * @return Reference to self.
     *
     * @see @ref Primitive @ref VertexObject(Primitive)
     */
    VertexObject& draw(Primitive primitive, int count, int first = 0);

    /**
     * @brief Set the primitive.
     *
     * Set the default primitive type to be used by draw().
     *
     * @param primitive New primitive type.
     * @return Reference to self.
     *
     * @see @ref Primitive @ref draw()
     */
    VertexObject& setPrimitive(Primitive primitive);

    /**
     * @brief Return VertexObject's assigned primitive type.
     *
     * @return Primitive
     */
    Primitive primitive() const;

    /**
     * @brief Set the count.
     *
     *  Set the default vertex/index count to be used by draw().
     *
     * @param count New vertices count.
     * @return Reference to self.
     *
     * @see @ref draw()
     */
    VertexObject& setCount(int count);

    /**
     * @brief Return vertex/index count.
     *
     * @return int
     */
    int count() const;

    /**
     * Whether the VertexObject is indexed.
     *
     * @see @ref setIndexBuffer(const Buffer &, std::ptrdiff_t, IndexType)
     */
    bool isIndexed() const;

    /**
     * @brief Define an array of generic vertex attribute data.
     *
     * Define an array of generic vertex attribute data. See documentation for glVertexAttribPointer
     * OpenGL method for more information.
     *
     * @param buffer Buffer with vertex data.
     * @param location Index of the generic vertex attribute.
     * @param elemSize Number of components per generic vertex attribute. Must be 1, 2, 3, or 4.
     * @param type Data type of each component.
     * @param normalized Whether fixed-point data values should be normalized (true) or
     * converted directly as fixed-point values (false).
     * @param stride Offset in bytes between consecutive generic vertex attributes. If stride is 0,
     * the generic vertex attributes are understood to be tightly packed in the array.
     * @param offset Offset of the first component of the first generic vertex attribute in the array.
     * @return Reference to self.
     *
     * @see @ref DataType
     */
    VertexObject& addVertexBuffer(const Buffer &buffer, int location, int elemSize, DataType type, bool normalized = false, int stride = 0, std::ptrdiff_t offset = 0);

    /**
     * @brief Add index buffer.
     *
     * @param buffer Buffer with index data.
     * @param offset Unused.
     * @param type Index type.
     *
     * @see @ref IndexType
     */
    VertexObject& setIndexBuffer(const Buffer &buffer, std::ptrdiff_t /*offset*/, IndexType type);

private:
    void clear() noexcept;
    void enableAttribArrays();
    void disableAttribArrays();
    void bind();
    void unbind();

    struct BufferDesc;

    std::vector<BufferDesc> m_bufferDesc;

    Primitive m_primitive{ Primitive::Triangles };
    int m_count{ 0 };

    GLuint m_id{ 0 };
    GLuint m_idxBufferId{ 0 };

    IndexType m_indexType{ IndexType::UnsignedShort };

    /* additional variables to track the state */
    GLuint m_currBuff{ 0 };
    bool   m_initialized{ false };
};

inline VertexObject::Primitive
VertexObject::primitive() const
{
    return m_primitive;
}

inline int
VertexObject::count() const
{
    return m_count;
}

inline bool
VertexObject::isIndexed() const
{
    return m_idxBufferId != 0;
}

inline GLuint
VertexObject::id() const
{
    return m_id;
}

} // namespace celestia::gl
