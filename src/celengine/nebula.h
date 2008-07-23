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

#include <celutil/reshandle.h>
#include <celengine/deepskyobj.h>


class Nebula : public DeepSkyObject
{
 public:
    Nebula();

    virtual const char* getType() const;
    virtual void setType(const std::string&);
    virtual size_t getDescription(char* buf, size_t bufLength) const;

    virtual bool pick(const Ray3d& ray,
                      double& distanceToPicker,
                      double& cosAngleToBoundCenter) const;
    virtual bool load(AssociativeArray*, const std::string&);
    virtual void render(const GLContext& context,
                        const Vec3f& offset,
                        const Quatf& viewerOrientation,
                        float brightness,
                        float pixelSize);

    virtual unsigned int getRenderMask() const;
    virtual unsigned int getLabelMask() const;

    void setGeometry(ResourceHandle);
    ResourceHandle getGeometry() const;
    
    virtual const char* getObjTypeName() const;

 public:
    enum NebulaType
    {
        Emissive           = 0,
        Reflective         = 1,
        Dark               = 2,
        Planetary          = 3,
        Galactic           = 4,
        SupernovaRemnant   = 5,
        Bright_HII_Region  = 6,
        NotDefined         = 7
    };

 private:
    ResourceHandle geometry;
};

#endif // CELENGINE_NEBULA_H_
