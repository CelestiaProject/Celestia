// 3dschunk.h
// 
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _3DSCHUNK_H_
#define _3DSCHUNK_H_

enum M3DChunkType
{
    M3DCHUNK_NULL                = 0x0000,
    M3DCHUNK_VERSION             = 0x0002,
    M3DCHUNK_COLOR_FLOAT         = 0x0010,
    M3DCHUNK_COLOR_24            = 0x0011,
    M3DCHUNK_LIN_COLOR_F         = 0x0013,
    M3DCHUNK_INT_PERCENTAGE      = 0x0030,
    M3DCHUNK_FLOAT_PERCENTAGE    = 0x0031,
    M3DCHUNK_MASTER_SCALE        = 0x0100,

    M3DCHUNK_BACKGROUND_COLOR    = 0x1200,

    M3DCHUNK_MESHDATA            = 0x3d3d,
    M3DCHUNK_MESH_VERSION        = 0x3d3e,

    M3DCHUNK_NAMED_OBJECT        = 0x4000,
    M3DCHUNK_TRIANGLE_MESH       = 0x4100,
    M3DCHUNK_POINT_ARRAY         = 0x4110,
    M3DCHUNK_POINT_FLAG_ARRAY    = 0x4111,
    M3DCHUNK_FACE_ARRAY          = 0x4120,
    M3DCHUNK_MESH_MATERIAL_GROUP = 0x4130,
    M3DCHUNK_MESH_TEXTURE_COORDS = 0x4140,
    M3DCHUNK_MESH_SMOOTH_GROUP   = 0x4150,
    M3DCHUNK_MESH_MATRIX         = 0x4160,
    M3DCHUNK_MAGIC               = 0x4d4d,

    M3DCHUNK_MATERIAL_NAME       = 0xa000,
    M3DCHUNK_MATERIAL_AMBIENT    = 0xa010,
    M3DCHUNK_MATERIAL_DIFFUSE    = 0xa020,
    M3DCHUNK_MATERIAL_SPECULAR   = 0xa030,
    M3DCHUNK_MATERIAL_SHININESS  = 0xa040,
    M3DCHUNK_MATERIAL_SHIN2PCT   = 0xa041,
    M3DCHUNK_MATERIAL_TRANSPARENCY = 0xa050,
    M3DCHUNK_MATERIAL_XPFALL     = 0xa052,
    M3DCHUNK_MATERIAL_REFBLUR    = 0xa053,
    M3DCHUNK_MATERIAL_SELF_ILLUM = 0xa084,
    M3DCHUNK_MATERIAL_WIRESIZE   = 0xa087,
    M3DCHUNK_MATERIAL_XPFALLIN   = 0xa08a,
    M3DCHUNK_MATERIAL_SHADING    = 0xa100,
    M3DCHUNK_MATERIAL_TEXMAP     = 0xa200,
    M3DCHUNK_MATERIAL_MAPNAME    = 0xa300,
    M3DCHUNK_MATERIAL_ENTRY      = 0xafff,

    M3DCHUNK_KFDATA              = 0xb000,
};

#endif // _3DSCHUNK_H_
