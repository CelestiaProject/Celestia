// visibleregion.h
//
// Visible region reference mark for ellipsoidal bodies.
//
// Copyright (C) 2008, the Celestia Development Team
// Initial version by Chris Laurel, claurel@gmail.com
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <string_view>

#include <Eigen/Core>

#include <celengine/referencemark.h>
#include <celengine/selection.h>
#include <celrender/rendererfwd.h>
#include <celutil/color.h>

class Body;
struct Matrices;

/*! VisibleRegion is a reference mark that shows the outline of
 *  region on the surface of a body in which a specified target
 *  is visible.
 */
class VisibleRegion : public ReferenceMark
{
public:
    VisibleRegion(const Body& body, const Selection& target);
    ~VisibleRegion() = default;

    void render(celestia::render::ReferenceMarkRenderer* refMarkRenderer,
                const Eigen::Vector3f& pos,
                float discSizeInPixels,
                double tdb,
                const Matrices& m) const override;
    float boundingSphereRadius() const override;

    Color color() const;
    void setColor(Color color);
    float opacity() const;
    void setOpacity(float opacity);

protected:
    std::string_view defaultTag() const override;

private:
    const Body& m_body;
    const Selection m_target;
    Color m_color{ 1.0f, 1.0f, 1.0f };
    float m_opacity{ 1.0f };
};
