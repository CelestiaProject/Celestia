// mesh.h
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _MESH_H_
#define _MESH_H_


class Mesh
{
 public:
    virtual ~Mesh() {};
    virtual void render() = 0;
    virtual void render(unsigned int attributes) = 0;

    enum {
        Normals    = 0x01,
        Tangents   = 0x02,
        Colors     = 0x04,
        TexCoords0 = 0x08,
        TexCoords1 = 0x10,
    };
};

#endif // _MESH_H_
