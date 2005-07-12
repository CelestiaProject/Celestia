// nebula.h
//
// Copyright (C) 2003, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef CELENGINE_NEBULA_H_
#define CELENGINE_NEBULA_H_

#include <vector>
#include <string>
#include <iostream>
#include <celmath/vecmath.h>
#include <celmath/quaternion.h>
#include <celutil/reshandle.h>
#include <celengine/deepskyobj.h>


class Nebula : public DeepSkyObject
{
 public:
    Nebula();

    virtual bool load(AssociativeArray*, const std::string&);
    virtual void render(const GLContext& context,
                        const Vec3f& offset,
                        const Quatf& viewerOrientation,
                        float brightness,
                        float pixelSize);

    virtual unsigned int getRenderMask();
    virtual unsigned int getLabelMask();

    void setModel(ResourceHandle);
    ResourceHandle getModel() const;

 private:
    ResourceHandle model;
};

#endif // CELENGINE_NEBULA_H_
