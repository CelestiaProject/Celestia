// buffile.h
//
// Copyright (C) 2025, the Celestia Development Team
// Original version by Andrew Tribick
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "buffile.h"

#include <cstring>
#include <istream>
#include <string>

#include "utf8.h"

namespace celestia::util
{

BufferedFile::BufferedFile(std::istream& in, std::size_t bufferSize) :
    m_stream(&in),
    m_buffer(std::make_unique<char[]>(bufferSize)),
    m_bufferSize(bufferSize)
{
    SkipUTF8BOM(*m_stream);
}

int
BufferedFile::next()
{
    if (m_state == State::Error)
        return std::char_traits<char>::eof();

    if (m_position >= m_length)
    {
        std::size_t unconsumed = m_length - m_consumed;
        if (unconsumed == m_bufferSize)
        {
            // We have an overlong token, set to error state
            m_state = State::Error;
            return std::char_traits<char>::eof();
        }

        if (unconsumed > 0)
            std::memmove(m_buffer.get(), m_buffer.get() + m_consumed, unconsumed);

        m_stream->read(m_buffer.get() + unconsumed, m_bufferSize - unconsumed); /* Flawfinder: ignore */
        if (m_stream->bad())
        {
            m_state = State::Error;
            return std::char_traits<char>::eof();
        }

        m_length = unconsumed + static_cast<std::size_t>(m_stream->gcount());
        m_position = unconsumed;
        m_consumed = 0;

        if (m_position >= m_length)
            return std::char_traits<char>::eof();
    }

    auto ch = m_buffer[m_position];
    if (ch == '\n')
    {
        if (m_state != State::CR)
            ++m_lineNumber;
        m_state = State::LF;
    }
    else if (ch == '\r')
    {
        if (m_state != State::LF)
            ++m_lineNumber;
        m_state = State::CR;
    }
    else
    {
        m_state = State::Normal;
    }

    return std::char_traits<char>::to_int_type(ch);
}

void
BufferedFile::advance(bool consume) noexcept
{
    ++m_position;
    if (consume)
        m_consumed = m_position;
}

void
BufferedFile::resizeValue(std::size_t length)
{
    if (auto max = valueSize(); length > max)
        length = max;

    m_position = m_consumed + length;
}

std::string_view
BufferedFile::value() const
{
    return m_position > m_consumed
        ? std::string_view(m_buffer.get() + m_consumed, m_position - m_consumed)
        : std::string_view{};
}

} // end namespace celestia::util
