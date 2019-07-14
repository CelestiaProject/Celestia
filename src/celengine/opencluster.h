// opencluster.h
//
// Copyright (C) 2003, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef CELENGINE_OPENCLUSTER_H_
#define CELENGINE_OPENCLUSTER_H_

#include <celutil/reshandle.h>
#include <celengine/deepskyobj.h>


class OpenCluster : public DeepSkyObject
{
 public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    OpenCluster() = default;

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
                const Renderer* r = nullptr) override;

    uint64_t getRenderMask() const override;
    unsigned int getLabelMask() const override;

    const char* getObjTypeName() const override;

 public:
    enum ClusterType
    {
        Open          = 0,
        Globular      = 1,
        NotDefined    = 2
    };

 private:
    // TODO: It could be very useful to have a list of stars that are members
    // of the cluster.
};

#endif // CELENGINE_OPENCLUSTER_H_
