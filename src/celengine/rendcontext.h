// rendcontext.h
//
// Copyright (C) 2004, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELENGINE_RENDCONTEXT_H_
#define _CELENGINE_RENDCONTEXT_H_

#include "mesh.h"

class RenderContext
{
 public:
    RenderContext(const Mesh::Material*);
    RenderContext();

    void makeCurrent();
    void setMaterial(const Mesh::Material*);
    void lock();
    void unlock();

 private:
    const Mesh::Material* material;
    bool specularOn;
    bool blendOn;
    bool locked;
};

#endif // _CELENGINE_RENDCONTEXT_H_

