// 3dsmesh.h
// 
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _3DSMESH_H_
#define _3DSMESH_H_

#include <vector>
#include "mesh.h"
#include "trilist.h"
#include "3dsmodel.h"


class Mesh3DS : public Mesh
{
 public:
    Mesh3DS(const M3DScene& scene);
    ~Mesh3DS();

    void render();
    void normalize();

 private:
    typedef std::vector<TriangleList*> TriListVec;
    TriListVec triLists;
};

#endif // _3DSMESH_H_
