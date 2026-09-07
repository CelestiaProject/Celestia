// starpipelineowner.h
//
// Copyright (C) 2026-present, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

namespace celestia::render
{

// A star buffer that can drain its pending vertices on demand.
class StarPipelineFlushable
{
public:
    virtual ~StarPipelineFlushable() = default;

    // Drain any pending vertices and release the "active" claim with
    // the owner.  The owner clears the active pointer before calling
    // finish(), so rendering can reacquire it without recursively
    // flushing the same buffer.
    virtual void finish() = 0;
};

// Tracks the last star buffer to submit a batch, so switching buffers
// first drains its pending vertices.  This is not a GL binding cache:
// other renderers can change programs and uniforms between submissions.
// Every batch must rebind its program and refresh its uniforms, even
// when its buffer is still active here.
class StarPipelineOwner
{
public:
    StarPipelineOwner() = default;
    StarPipelineOwner(const StarPipelineOwner&) = delete;
    StarPipelineOwner& operator=(const StarPipelineOwner&) = delete;

    bool isActive(const StarPipelineFlushable *p) const noexcept
    {
        return m_active == p;
    }

    // Drain whoever is currently active and release its claim.
    void flush()
    {
        if (m_active != nullptr)
        {
            auto *prev = m_active;
            m_active = nullptr;
            prev->finish();
        }
    }

    // Make `p` the active pipeline.  If a different buffer was
    // active, drain it first so its pending vertices are submitted
    // before the GL state changes.
    void setActive(StarPipelineFlushable *p)
    {
        if (m_active == p)
            return;
        if (m_active != nullptr)
        {
            auto *prev = m_active;
            m_active = nullptr;
            prev->finish();
        }
        m_active = p;
    }

    // Forget `p` if it was the active buffer.  Called by a buffer's
    // own finish() after it has drained itself.
    void clearIfActive(const StarPipelineFlushable *p) noexcept
    {
        if (m_active == p)
            m_active = nullptr;
    }

private:
    StarPipelineFlushable *m_active{ nullptr };
};

} // namespace celestia::render
