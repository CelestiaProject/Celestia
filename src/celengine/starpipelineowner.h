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

// Anything that owns transient GL pipeline state (a bound program,
// VAO, uniforms) for star rendering and can be drained on demand.
class StarPipelineFlushable
{
public:
    virtual ~StarPipelineFlushable() = default;

    // Drain any pending vertices and release the "active" claim with
    // the owner.  Implementations should not call setActive() from
    // here; the owner clears the active pointer itself before
    // delegating to finish() to keep reentry simple.
    virtual void finish() = 0;
};

// Tracks which star-pipeline buffer last bound its GL program/VAO,
// so the next buffer that wants to bind can first flush whoever was
// holding the pipeline.  Replaces the per-class `static current`
// pointers in PointStarVertexBuffer / PsfStarVertexBuffer, which only
// interlocked within their own class and could be left stale when
// the other class bound a different program behind their back.
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

    // Drain whoever is currently active.  Use before binding a
    // foreign GL program (e.g. a one-shot draw outside the star
    // pipeline) so that buffer's pending vertices are submitted under
    // its own program and its "active" claim is released — otherwise
    // it would later early-return from makeCurrent() while the
    // foreign program is still bound and draw garbage.
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
