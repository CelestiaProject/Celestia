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

#include <vector>
#include <string>
#include <iostream>
#include <celmath/vecmath.h>
#include <celmath/quaternion.h>
#include <celutil/reshandle.h>
#include <celengine/deepskyobj.h>


class OpenCluster : public DeepSkyObject
{
 public:
    OpenCluster();

    virtual bool load(AssociativeArray*);
    virtual void render(const Vec3f& offset,
                        const Quatf& viewerOrientation,
                        float brightness,
                        float pixelSize);

 private:
    // TODO: It could be very useful to have a list of stars that are members
    // of the cluster.
};

#endif // CELENGINE_OPENCLUSTER_H_
