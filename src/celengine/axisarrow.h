// axisarrow.h
//
// Copyright (C) 2007, Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELENGINE_AXISARROW_H_
#define _CELENGINE_AXISARROW_H_

#include <celmath/quaternion.h>

void RenderAxisArrows(const Quatf& orientation, float scale, float opacity);
void RenderSunDirectionArrow(const Vec3f& direction, float scale, float opacity);
void RenderVelocityArrow(const Vec3f& direction, float scale, float opacity);

#endif // _CELENGINE_AXISARROW_H_



