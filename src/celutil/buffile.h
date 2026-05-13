// buffile.h
//
// Copyright (C) 2025, the Celestia Development Team
// Original version by Andrew Tribick
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <memory>
#include <string_view>

namespace celestia::util
{

class BufferedFile
{
public:
    explicit BufferedFile(std::istream& in, std::size_t bufferSize = 4096);

    int next();
    void advance(bool consume) noexcept;
    void consume() noexcept { m_consumed = m_position; }
    void resizeValue(std::size_t length);

    bool error() const noexcept { return m_state == State::Error; }
    std::uint64_t lineNumber() const noexcept { return m_lineNumber; }

    bool has_value() const noexcept { return m_position > m_consumed; }
    std::size_t valueSize() const noexcept { return m_position - m_consumed; }
    std::string_view value() const;

private:
    enum class State
    {
        Normal,
        LF,
        CR,
        Error,
    };

    std::istream* m_stream;
    std::unique_ptr<char[]> m_buffer; //NOSONAR
    std::size_t m_bufferSize;
    std::size_t m_length{ 0 };
    std::size_t m_position{ 0 };
    std::size_t m_consumed{ 0 };
    std::uint64_t m_lineNumber{ 1 };
    State m_state{ State::Normal };
};

} // end namespace celestia::util
