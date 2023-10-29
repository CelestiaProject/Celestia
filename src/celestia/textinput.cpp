// textinput.cpp
//
// Copyright (C) 2023, the Celestia Development Team
//
// Split out from celestiacore.h/celestiacore.cpp
// Copyright (C) 2001-2009, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "textinput.h"

#include <cstddef>
#include <cstdint>
#include <cwctype>

#include <celengine/simulation.h>
#include <celutil/utf8.h>

namespace celestia
{

std::string_view
TextInput::getTypedText() const
{
    return m_text;
}

util::array_view<std::string>
TextInput::getCompletion() const
{
    return m_completion;
}

int
TextInput::getCompletionIndex() const
{
    return m_completionIdx;
}

CharEnteredResult
TextInput::charEntered(const Simulation* sim, std::string_view c_p, bool withLocations)
{
    char c = c_p.front();
    if (std::int32_t uc = 0;
        UTF8Decode(c_p, uc) && !(uc <= WCHAR_MAX && std::iswcntrl(static_cast<std::wint_t>(uc))))
    {
        appendText(sim, c_p, withLocations);
    }
    else if (c == '\b')
    {
        m_completionIdx = -1;
        if (!m_text.empty())
        {
#ifdef AUTO_COMPLETION
            do
            {
#endif
                for (;;)
                {
                    auto ch = static_cast<std::byte>(m_text.back());
                    m_text.pop_back();
                    // If the string is empty, or the removed character was
                    // not a UTF-8 continuation byte 0b10xx_xxxx then we're
                    // done.
                    if (m_text.empty() || (ch & std::byte(0xc0)) != std::byte(0x80))
                        break;
                }

                m_completion.clear();
                if (!m_text.empty())
                        sim->getObjectCompletion(m_completion, m_text, withLocations);
#ifdef AUTO_COMPLETION
            } while (!typedText.empty() && typedTextCompletion.size() == 1);
#endif
        }
    }
    else if (c == '\011') // TAB
    {
        if (m_completionIdx + 1 < (int) m_completion.size())
            m_completionIdx++;
        else if ((int) m_completion.size() > 0 && m_completionIdx + 1 == (int) m_completion.size())
            m_completionIdx = 0;

        if (m_completionIdx >= 0)
        {
            auto pos = m_text.rfind('/');
            m_text.resize(pos == std::string::npos ? 0 : (pos + 1));
            m_text.append(m_completion[m_completionIdx]);
        }
    }
    else if (c == '\177') // BackTab
    {
        if (m_completionIdx > 0)
            m_completionIdx--;
        else if (m_completionIdx == 0 || m_completion.size() > 0)
            m_completionIdx = m_completion.size() - 1;

        if (m_completionIdx >= 0)
        {
            auto pos = m_text.rfind('/');
            m_text.resize(pos == std::string::npos ? 0 : (pos + 1));
            m_text.append(m_completion[m_completionIdx]);
        }
    }
    else if (c == '\033') // ESC
    {
        return CharEnteredResult::Cancelled;
    }
    else if (c == '\n' || c == '\r')
    {
        return CharEnteredResult::Finished;
    }

    return CharEnteredResult::Normal;
}

void
TextInput::appendText(const Simulation* sim, std::string_view sv, bool withLocations)
{
    m_text.append(sv);
    m_completion.clear();
    sim->getObjectCompletion(m_completion, m_text, withLocations);
    m_completionIdx = -1;
#ifdef AUTO_COMPLETION
    if (m_completion.size() == 1)
    {
        auto pos = m_text.rfind('/');
        m_text.resize(pos == std::string::npos ? 0 : (pos + 1));
        m_text.append(m_completion.front());
    }
#endif
}

void
TextInput::reset()
{
    m_text.clear();
    m_completion.clear();
    m_completionIdx = -1;
}

} // end namespace celestia
