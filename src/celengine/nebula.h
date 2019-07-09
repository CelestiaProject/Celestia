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
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    Nebula() = default;

    const char* getType() const override;
    void setType(const std::string&) override;
    std::string getDescription() const override;

    bool pick(const celmath::Ray3d& ray,
              double& distanceToPicker,
              double& cosAngleToBoundCenter) const override;
    bool load(AssociativeArray*, const fs::path&) override;
    void render(const Eigen::Vector3f& offset,
                const Eigen::Quaternionf& viewerOrientation,
                float brightness,
                float pixelSize,
                const Renderer* renderer) const override;

    uint64_t getRenderMask() const override;
    unsigned int getLabelMask() const override;

    void setGeometry(ResourceHandle);
    ResourceHandle getGeometry() const;

    const char* getObjTypeName() const override;

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
    ResourceHandle geometry{ InvalidResource };
};

#endif // CELENGINE_NEBULA_H_
