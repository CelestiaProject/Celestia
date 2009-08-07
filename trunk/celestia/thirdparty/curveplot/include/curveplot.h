// curveplot.h
//
// Copyright (C) 2009 Chris Laurel <claurel@gmail.com>.
//
// curveplot is a module for rendering curves in OpenGL at high precision. A
// plot is a series of cubic curves. The curves are transformed
// to camera space in software because double precision is absolutely
// required. The cubics are adaptively subdivided based on distance from
// the camera position.
//
// curveplot is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// Alternatively, you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of
// the License, or (at your option) any later version.
//
// curveplot is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License or the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License and a copy of the GNU General Public License along with
// orbitpath. If not, see <http://www.gnu.org/licenses/>.

#include <deque>
#include <Eigen/Geometry>


class HighPrec_Frustum;

class CurvePlotSample
{
public:
    Eigen::Vector3d position;
    double t;
    Eigen::Vector3d velocity;
    double boundingRadius;
};


class CurvePlot
{
 public:
    CurvePlot();

    double duration() const { return m_duration; }
    void setDuration(double duration);

    double startTime() const
    {
        if (m_samples.empty())
            return 0.0;
        else
            return m_samples.front().t;
    }

    double endTime() const
    {
        if (m_samples.empty())
            return 0.0;
        else
            return m_samples.back().t;
    }

    void render(const Eigen::Transform3d& modelview,
                double nearZ,
                double farZ,
                const Eigen::Vector3d viewFrustumPlaneNormals[],
                double subdivisionThreshold) const;
    void render(const Eigen::Transform3d& modelview,
                double nearZ,
                double farZ,
                const Eigen::Vector3d viewFrustumPlaneNormals[],
                double subdivisionThreshold,
                double startTime,
                double endTime) const;

    unsigned int lastUsed() const { return m_lastUsed; }
    void setLastUsed(unsigned int lastUsed) { m_lastUsed = lastUsed; }

    void addSample(const CurvePlotSample& sample);
    void removeSamplesBefore(double t);
    void removeSamplesAfter(double t);

    bool empty() const { return m_samples.empty(); }

 private:
    std::deque<CurvePlotSample> m_samples;
 
    double m_duration;

    unsigned int m_lastUsed;
};

