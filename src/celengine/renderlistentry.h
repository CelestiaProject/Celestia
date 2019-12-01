// renderlistentry.h
//
// Copyright (C) 2001-2019, Celestia Development Team
// Contact: Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <Eigen/Core>

class Star;
class Body;
class ReferenceMark;

struct RenderListEntry
{
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    enum RenderableType
    {
        RenderableStar,
        RenderableBody,
        RenderableCometTail,
        RenderableReferenceMark,
    };

    union
    {
        const Star* star;
        Body* body;
        const ReferenceMark* refMark;
    };

    Eigen::Vector3f position;
    Eigen::Vector3f sun;
    float distance;
    float radius;
    float centerZ;
    float nearZ;
    float farZ;
    float discSizeInPixels;
    float appMag;
    RenderableType renderableType;
    bool isOpaque;
};
