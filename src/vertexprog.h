// vertexprog.h
//
// Copyright (C) 2001 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _VERTEXPROG_H_
#define _VERTEXPROG_H_

#include "vecmath.h"
#include "color.h"

extern void InitVertexPrograms();
extern void EnableVertexPrograms();
extern void DisableVertexPrograms();
extern void UseVertexProgram(unsigned int);

extern void VertexProgramParameter(unsigned int, const Vec3f&);
extern void VertexProgramParameter(unsigned int, const Point3f&);
extern void VertexProgramParameter(unsigned int, const Color&);

extern unsigned int simpleVP;

#endif // _VERTEXPROG_H_
