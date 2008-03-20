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

#ifndef _CELENGINE_REFERENCEMARK_H_
#define _CELENGINE_REFERENCEMARK_H_

#include <string>
#include <celmath/vecmath.h>

class Renderer;

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
    virtual void render(Renderer* renderer,
                        const Point3f& position,
                        float discSizeInPixels,
                        double tdb) const = 0;

    /*! Return the radius of a bounding sphere (in kilometers) large enough
     *  to contain the reference mark geometry.
     */
    virtual float boundingSphereRadius() const = 0;

    /*! Return true if the reference mark contains no translucent geometry.
     *  The default implementation always returns true (i.e. completely
     *  opaque geometry is assumed.)
     */
    virtual bool isOpaque() const { return true; }

    void setTag(const std::string& _tag)
    {
        tag = _tag;
    }

    const std::string& getTag() const
    {
        return tag;
    }

private:
    std::string tag;
};

#endif // _CELENGINE_REFERENCEMARK_H_
