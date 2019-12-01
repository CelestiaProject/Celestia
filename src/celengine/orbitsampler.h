// orbitsampler.h
//
// Copyright (C) 2001-2019, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <Eigen/Core>
#include <celephem/orbit.h>
#include "curveplot.h"

class OrbitSampler : public OrbitSampleProc
{
public:
    std::vector<CurvePlotSample> samples;

    OrbitSampler() = default;

    void sample(double t, const Eigen::Vector3d& position, const Eigen::Vector3d& velocity)
    {
        CurvePlotSample samp;
        samp.t = t;
        samp.position = position;
        samp.velocity = velocity;
        samples.push_back(samp);
    }

    void insertForward(CurvePlot* plot)
    {
        for (const auto& sample : samples)
        {
            plot->addSample(sample);
        }
    }

    void insertBackward(CurvePlot* plot)
    {
        for (auto iter = samples.rbegin(); iter != samples.rend(); ++iter)
        {
            plot->addSample(*iter);
        }
    }
};
