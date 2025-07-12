// referencemark.h
//
// ReferenceMark base class.
//
// Copyright (C) 2008, the Celestia Development Team
// Initial version by Chris Laurel, claurel@gmail.com
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <string>
#include <string_view>
#include <Eigen/Core>

struct Matrices;

namespace celestia::render
{
class ReferenceMarkRenderer;
}

/*! Reference marks give additional visual information about the
 *  position and orientation of a solar system body. Items such as
 *  axis arrows and planetographic grids are examples of reference
 *  marks in Celestia.
 *
 *  ReferenceMark is an abstract base class. Subclasses must implement
 *  the methods render() and boundingSphereRadius(). They may optionally
 *  override the isOpaque method, which by default returns true. If a
 *  subclass draws translucent geometry but doesn't override isOpaque to
 *  return false, the translucent parts may not be properly depth sorted.
 */
class ReferenceMark
{
 public:
    ReferenceMark() {};
    virtual ~ReferenceMark() {};

    /*! Draw the reference mark geometry at the specified time.
     */
    virtual void render(celestia::render::ReferenceMarkRenderer* refMarkRenderer,
                        const Eigen::Vector3f& position,
                        float discSizeInPixels,
                        double tdb,
                        const Matrices& m) const = 0;

    /*! Return the radius of a bounding sphere (in kilometers) large enough
     *  to contain the reference mark geometry.
     */
    virtual float boundingSphereRadius() const = 0;

    /*! Return true if the reference mark contains no translucent geometry.
     *  The default implementation always returns true (i.e. completely
     *  opaque geometry is assumed.)
     */
    virtual bool isOpaque() const { return true; }

    void setTag(std::string_view tag)
    {
        if (tag.empty() || tag == defaultTag())
            m_tag = std::string{};
        else
            m_tag = tag;
    }

    std::string_view getTag() const
    {
        if (m_tag.empty())
            return defaultTag();
        return m_tag;
    }

protected:
    virtual std::string_view defaultTag() const = 0;

private:
    std::string m_tag;
};
