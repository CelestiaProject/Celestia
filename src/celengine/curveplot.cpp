// curveplot.cpp
//
// Copyright (C) 2009-2010 Chris Laurel <claurel@gmail.com>.
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
// CurvePlot. If not, see <http://www.gnu.org/licenses/>.

#include <algorithm>
#include <cmath>
#include <vector>
#include <celrender/linerenderer.h>

#include "curveplot.h"
#include "render.h"
#include "shadermanager.h"

using celestia::engine::LineRenderer;

namespace
{
constexpr unsigned int VertexBufferCapacity = 4096;
constexpr unsigned int SubdivisionFactor = 8;
constexpr double InvSubdivisionFactor = 1.0 / static_cast<double>(SubdivisionFactor);

// Convert a 3-vector to a 4-vector by adding a zero
inline Eigen::Vector4d
zeroExtend(const Eigen::Vector3d& v)
{
    return Eigen::Vector4d(v.x(), v.y(), v.z(), 0.0);
}


class HighPrec_Frustum
{
public:
    HighPrec_Frustum(double nearZ, double farZ, const Eigen::Vector3d planeNormals[]) :
        m_nearZ(nearZ),
        m_farZ(farZ)
    {
        for (unsigned int i = 0; i < 4; i++)
        {
            m_planeNormals[i] = zeroExtend(planeNormals[i]);
        }
    }

    inline bool cullSphere(const Eigen::Vector3d& center,
                           double radius) const
    {
        return (center.z() - radius > m_nearZ ||
                center.z() + radius < m_farZ  ||
                center.dot(m_planeNormals[0].head(3)) < -radius ||
                center.dot(m_planeNormals[1].head(3)) < -radius ||
                center.dot(m_planeNormals[2].head(3)) < -radius ||
                center.dot(m_planeNormals[3].head(3)) < -radius);
    }

    inline bool cullSphere(const Eigen::Vector4d& center,
                           double radius) const
    {
        return (center.z() - radius > m_nearZ ||
                center.z() + radius < m_farZ  ||
                center.dot(m_planeNormals[0]) < -radius ||
                center.dot(m_planeNormals[1]) < -radius ||
                center.dot(m_planeNormals[2]) < -radius ||
                center.dot(m_planeNormals[3]) < -radius);
    }

    inline double nearZ() const { return m_nearZ; }
    inline double farZ() const { return m_farZ; }

private:
    double m_nearZ;
    double m_farZ;
    Eigen::Vector4d m_planeNormals[4];
};


inline Eigen::Matrix4d
cubicHermiteCoefficients(const Eigen::Vector4d& p0,
                         const Eigen::Vector4d& p1,
                         const Eigen::Vector4d& v0,
                         const Eigen::Vector4d& v1)
{
    Eigen::Matrix4d coeff;
    coeff.col(0) = p0;
    coeff.col(1) = v0;
    coeff.col(2) = 3.0 * (p1 - p0) - (2.0 * v0 + v1);
    coeff.col(3) = 2.0 * (p0 - p1) + (v1 + v0);

    return coeff;
}

Eigen::Matrix4f ModelViewMatrix(Eigen::Matrix4f::Identity());

class HighPrec_VertexBuffer
{
public:
    bool setup(const Renderer& renderer, const Color& color)
    {
        stripLengths.clear();
        currentStripLength = 0;
        this->color = color;
        this->renderer = &renderer;

        if (prog == nullptr)
        {
            prog = renderer.getShaderManager().getShader("orbit");
            if (prog == nullptr) return false;
        }

        if (lr == nullptr)
        {
            lr = new LineRenderer(renderer, 1.0f, LineRenderer::LINE_STRIP, LineRenderer::STREAM_STORAGE);
            lr->setCustomShader(prog);
            lr->setVertexCount(VertexBufferCapacity);
        }
        return true;
    }

    void finish()
    {
    }

    inline void vertex(const Eigen::Vector4d& v, float opacity = 1.0f)
    {
        lr->addVertex(static_cast<float>(v.x()), static_cast<float>(v.y()), static_cast<float>(v.z()), opacity);
        ++currentStripLength;
    }

    inline void begin()
    {
    }

    inline void end()
    {
        stripLengths.push_back(currentStripLength);
        currentStripLength = 0;
    }

    inline void flush()
    {
        if (currentStripLength > 1)
            end();

        Matrices m = { &renderer->getProjectionMatrix(), &ModelViewMatrix };
        unsigned int startIndex = 0;
        for (unsigned int lineCount : stripLengths)
        {
            lr->render(m, color, lineCount, startIndex);
            startIndex += lineCount;
        }
        stripLengths.clear();
        lr->clear();
        currentStripLength = 0;
    }

    void createVertexBuffer()
    {
    }

private:
    unsigned int currentStripLength { 0 };
    std::vector<unsigned int> stripLengths;
    LineRenderer *lr { nullptr };
    const Renderer *renderer { nullptr };
    CelestiaGLProgram *prog { nullptr };
    Color color;
};


class HighPrec_RenderContext
{
public:
    HighPrec_RenderContext(HighPrec_VertexBuffer& vbuf,
                           HighPrec_Frustum& viewFrustum,
                           double subdivisionThreshold) :
        m_vbuf(vbuf),
        m_viewFrustum(viewFrustum),
        m_subdivisionThreshold(subdivisionThreshold)
    {
    }

    ~HighPrec_RenderContext()
    {
        /*
        vbuf.flush();
        vbuf.finish();
        */
    }

    // Return the GL restart status: true if the last segment of the
    // curve was culled and we need to start a new primitive sequence
    // with glBegin().
    bool renderCubic(bool restartCurve,
                     const Eigen::Matrix4d& coeff,
                     double t0, double t1,
                     double curveBoundingRadius,
                     int depth) const
    {
        const double dt = (t1 - t0) * InvSubdivisionFactor;
        double segmentBoundingRadius = curveBoundingRadius * InvSubdivisionFactor;

        Eigen::Vector4d lastP = coeff * Eigen::Vector4d(1.0, t0, t0 * t0, t0 * t0 * t0);

        for (unsigned int i = 1; i <= SubdivisionFactor; i++)
        {
            double t = t0 + dt * i;
            Eigen::Vector4d p = coeff * Eigen::Vector4d(1.0, t, t * t, t * t * t);

            double minDistance = std::max(-m_viewFrustum.nearZ(), std::abs(p.z()) - segmentBoundingRadius);

            if (segmentBoundingRadius >= m_subdivisionThreshold * minDistance)
            {
                if (m_viewFrustum.cullSphere(p, segmentBoundingRadius))
                {
                    if (!restartCurve)
                    {
                        m_vbuf.end();
                        restartCurve = true;
                    }
                }
                else
                {
                    restartCurve = renderCubic(restartCurve,
                                               coeff, t - dt, t,
                                               segmentBoundingRadius,
                                               depth + 1);
                }
            }
            else
            {
                if (restartCurve)
                {
                    m_vbuf.begin();
                    m_vbuf.vertex(lastP);
                    restartCurve = false;
                }
                m_vbuf.vertex(p);
            }
            lastP = p;
        }

        return restartCurve;
    }

    // Return the GL restart status: true if the last segment of the
    // curve was culled and we need to start a new primitive sequence
    // with glBegin().
    bool renderCubicFaded(bool restartCurve,
                          const Eigen::Matrix4d& coeff,
                          double t0, double t1,
                          const Eigen::Vector4f& color,
                          double fadeStart, double fadeRate,
                          double curveBoundingRadius,
                          int depth) const
    {
        const double dt = (t1 - t0) * InvSubdivisionFactor;
        double segmentBoundingRadius = curveBoundingRadius * InvSubdivisionFactor;

        Eigen::Vector4d lastP = coeff * Eigen::Vector4d(1.0, t0, t0 * t0, t0 * t0 * t0);
        double lastOpacity = (t0 - fadeStart) * fadeRate;
        lastOpacity = std::clamp(lastOpacity, 0.0, 1.0);

        for (unsigned int i = 1; i <= SubdivisionFactor; i++)
        {
            double t = t0 + dt * i;
            Eigen::Vector4d p = coeff * Eigen::Vector4d(1.0, t, t * t, t * t * t);
            double opacity = (t - fadeStart) * fadeRate;
            opacity = std::clamp(opacity, 0.0, 1.0);

            double minDistance = std::max(-m_viewFrustum.nearZ(), std::abs(p.z()) - segmentBoundingRadius);

            if (segmentBoundingRadius >= m_subdivisionThreshold * minDistance)
            {
                if (m_viewFrustum.cullSphere(p, segmentBoundingRadius))
                {
                    if (!restartCurve)
                    {
                        m_vbuf.end();
                        restartCurve = true;
                    }
                }
                else
                {
                    restartCurve = renderCubicFaded(restartCurve,
                                                    coeff, t - dt, t,
                                                    color,
                                                    fadeStart, fadeRate,
                                                    segmentBoundingRadius,
                                                    depth + 1);
                }
            }
            else
            {
                if (restartCurve)
                {
                    m_vbuf.begin();
                    m_vbuf.vertex(lastP, static_cast<float>(lastOpacity));
                    restartCurve = false;
                }

                m_vbuf.vertex(p, static_cast<float>(opacity));
            }
            lastP = p;
            lastOpacity = opacity;
        }

        return restartCurve;
    }

private:
    HighPrec_VertexBuffer& m_vbuf;
    HighPrec_Frustum& m_viewFrustum;
    double m_subdivisionThreshold;
};


HighPrec_VertexBuffer vbuf;

} // end unnamed namespace


CurvePlot::CurvePlot(const Renderer &renderer) :
    m_renderer(renderer)
{
}


/** Add a new sample to the path. If the sample time is less than the first time,
  * it is added at the end. If it is greater than the last time, it is appended
  * to the path. The sample is ignored if it has a time in between the first and
  * last times of the path.
  */
void
CurvePlot::addSample(const CurvePlotSample& sample)
{
    bool addToBack = false;

    if (m_samples.empty() || sample.t > m_samples.back().t)
    {
        addToBack = true;
    }
    else if (sample.t < m_samples.front().t)
    {
        addToBack = false;
    }
    else
    {
        // Sample falls within range of current samples; discard it
        return;
    }

    if (addToBack)
        m_samples.push_back(sample);
    else
        m_samples.push_front(sample);

    if (m_samples.size() > 1)
    {
        // Calculate a bounding radius for this segment. No point on the curve will
        // be further from the start point than the bounding radius.
        if (addToBack)
        {
            const CurvePlotSample& lastSample = m_samples[m_samples.size() - 2];
            double dt = sample.t - lastSample.t;
            Eigen::Matrix4d coeff = cubicHermiteCoefficients(
                zeroExtend(lastSample.position),
                zeroExtend(sample.position),
                zeroExtend(lastSample.velocity * dt),
                zeroExtend(sample.velocity * dt));
            Eigen::Vector4d extents = coeff.cwiseAbs() * Eigen::Vector4d(0.0, 1.0, 1.0, 1.0);
            m_samples[m_samples.size() - 1].boundingRadius = extents.norm();
        }
        else
        {
            const CurvePlotSample& nextSample = m_samples[1];
            double dt = nextSample.t - sample.t;
            Eigen::Matrix4d coeff = cubicHermiteCoefficients(
                zeroExtend(sample.position),
                zeroExtend(nextSample.position),
                zeroExtend(sample.velocity * dt),
                zeroExtend(nextSample.velocity * dt));
            Eigen::Vector4d extents = coeff.cwiseAbs() * Eigen::Vector4d(0.0, 1.0, 1.0, 1.0);
            m_samples[1].boundingRadius = extents.norm();
        }
    }
}


/** Remove all samples before the specified time.
  */
void
CurvePlot::removeSamplesBefore(double t)
{
    while (!m_samples.empty() && m_samples.front().t < t)
    {
        m_samples.pop_front();
    }
}


/** Delete all samples after the specified time.
  */
void
CurvePlot::removeSamplesAfter(double t)
{
    while (!m_samples.empty() && m_samples.back().t > t)
    {
        m_samples.pop_back();
    }
}


void
CurvePlot::setDuration(double duration)
{
    m_duration = duration;
}


// Trajectory consists of segments, each of which is a cubic
// polynomial.

/** Draw a piecewise curve with transformation and frustum clipping.
  *
  * @param modelview an affine transformation that will be applied to the curve
  * @param nearZ z coordinate of the near plane
  * @param farZ z coordinate of the far plane
  * @param viewFrustumPlaneNormals array of four normals (top, bottom, left, and right frustum planes)
  * @param subdivisionThreshold the threashhold for subdivision
  */
void
CurvePlot::render(const Eigen::Affine3d& modelview,
                  double nearZ,
                  double farZ,
                  const Eigen::Vector3d viewFrustumPlaneNormals[],
                  double subdivisionThreshold,
                  const Eigen::Vector4f& color) const
{
    // Flag to indicate whether we need to issue a glBegin()
    bool restartCurve = true;

    const Eigen::Vector3d& p0_ = m_samples[0].position;
    const Eigen::Vector3d& v0_ = m_samples[0].velocity;
    Eigen::Vector4d p0 = modelview * Eigen::Vector4d(p0_.x(), p0_.y(), p0_.z(), 1.0);
    Eigen::Vector4d v0 = modelview * Eigen::Vector4d(v0_.x(), v0_.y(), v0_.z(), 0.0);

    HighPrec_Frustum viewFrustum(nearZ, farZ, viewFrustumPlaneNormals);
    HighPrec_RenderContext rc(vbuf, viewFrustum, subdivisionThreshold);

    vbuf.createVertexBuffer();
    if (!vbuf.setup(m_renderer, color)) return;

    for (unsigned int i = 1; i < m_samples.size(); i++)
    {
        // Transform the points into camera space.
        const Eigen::Vector3d& p1_ = m_samples[i].position;
        const Eigen::Vector3d& v1_ = m_samples[i].velocity;
        Eigen::Vector4d p1 = modelview * Eigen::Vector4d(p1_.x(), p1_.y(), p1_.z(), 1.0);
        Eigen::Vector4d v1 = modelview * Eigen::Vector4d(v1_.x(), v1_.y(), v1_.z(), 0.0);

        // O(t) is an approximating function for this segment of
        // the orbit, with 0 <= t <= 1
        // C is the viewer position
        // d(t) = |O(t) - C|, the distance from viewer to the
        // orbit segment.

        double curveBoundingRadius = m_samples[i].boundingRadius;

        // Estimate the minimum possible distance from the
        // curve to the z=0 plane. If the curve is far enough
        // away to be approximated as a straight line, we'll just
        // render it. Otherwise, it should be a performance win
        // to do a sphere-frustum cull test before subdividing and
        // rendering segment.
        double minDistance = std::abs(p0.z()) - curveBoundingRadius;

        // Render close segments as splines with adaptive subdivision. The
        // subdivisions eliminates kinks between line segments and also
        // prevents clipping precision problems that occur when a
        // very long line is rendered with a relatively small view
        // volume.
        if (curveBoundingRadius >= subdivisionThreshold * minDistance)
        {
            // Skip rendering this section if it lies outside the view
            // frustum.
            if (viewFrustum.cullSphere(p0, curveBoundingRadius))
            {
                if (!restartCurve)
                {
                    vbuf.end();
                    restartCurve = true;
                }
            }
            else
            {
                double dt = m_samples[i].t - m_samples[i - 1].t;
                Eigen::Matrix4d coeff = cubicHermiteCoefficients(p0, p1, v0 * dt, v1 * dt);

                restartCurve = rc.renderCubic(restartCurve, coeff, 0.0, 1.0, curveBoundingRadius, 1);
            }
        }
        else
        {
            // Apparent size of curve is small enough that we can approximate
            // it as a line.

            // Simple cull test--just check the far plane
            if (p0.z() + curveBoundingRadius < farZ)
            {
                if (!restartCurve)
                {
                    vbuf.end();
                    restartCurve = true;
                }
            }
            else
            {
                if (restartCurve)
                {
                    vbuf.begin();
                    vbuf.vertex(p0);
                    restartCurve = false;
                }
                vbuf.vertex(p1);
            }
        }

        p0 = p1;
        v0 = v1;
    }

    if (!restartCurve)
    {
        vbuf.end();
    }

    vbuf.flush();
    vbuf.finish();
}


/** Draw some range of a piecewise curve with transformation and frustum clipping.
  *
  * @param modelview an affine transformation that will be applied to the curve
  * @param nearZ z coordinate of the near plane
  * @param farZ z coordinate of the far plane
  * @param viewFrustumPlaneNormals array of four normals (top, bottom, left, and right frustum planes)
  * @param subdivisionThreshold the threashhold for subdivision
  * @param startTime the beginning of the time interval
  * @param endTime the end of the time interval
  */
void
CurvePlot::render(const Eigen::Affine3d& modelview,
                  double nearZ,
                  double farZ,
                  const Eigen::Vector3d viewFrustumPlaneNormals[],
                  double subdivisionThreshold,
                  double startTime,
                  double endTime,
                  const Eigen::Vector4f& color) const
{
    // Flag to indicate whether we need to issue a glBegin()
    bool restartCurve = true;

    if (m_samples.empty() || endTime <= m_samples.front().t || startTime >= m_samples.back().t)
        return;

    // Linear search for the first sample
    unsigned int startSample = 0;
    while (startSample < m_samples.size() - 1 && startTime > m_samples[startSample].t)
        startSample++;

    // Start at the first sample with time <= startTime
    if (startSample > 0)
        startSample--;

    const Eigen::Vector3d& p0_ = m_samples[startSample].position;
    const Eigen::Vector3d& v0_ = m_samples[startSample].velocity;
    Eigen::Vector4d p0 = modelview * Eigen::Vector4d(p0_.x(), p0_.y(), p0_.z(), 1.0);
    Eigen::Vector4d v0 = modelview * Eigen::Vector4d(v0_.x(), v0_.y(), v0_.z(), 0.0);

    HighPrec_Frustum viewFrustum(nearZ, farZ, viewFrustumPlaneNormals);
    HighPrec_RenderContext rc(vbuf, viewFrustum, subdivisionThreshold);

    vbuf.createVertexBuffer();
    if (!vbuf.setup(m_renderer, color)) return;

    bool firstSegment = true;
    bool lastSegment = false;

    for (unsigned int i = startSample + 1; i < m_samples.size() && !lastSegment; i++)
    {
        // Transform the points into camera space.
        const Eigen::Vector3d& p1_ = m_samples[i].position;
        const Eigen::Vector3d& v1_ = m_samples[i].velocity;
        Eigen::Vector4d p1 = modelview * Eigen::Vector4d(p1_.x(), p1_.y(), p1_.z(), 1.0);
        Eigen::Vector4d v1 = modelview * Eigen::Vector4d(v1_.x(), v1_.y(), v1_.z(), 0.0);

        if (endTime <= m_samples[i].t)
        {
            lastSegment = true;
        }

        // O(t) is an approximating function for this segment of
        // the orbit, with 0 <= t <= 1
        // C is the viewer position
        // d(t) = |O(t) - C|, the distance from viewer to the
        // orbit segment.

        double curveBoundingRadius = m_samples[i].boundingRadius;

        // Estimate the minimum possible distance from the
        // curve to the z=0 plane. If the curve is far enough
        // away to be approximated as a straight line, we'll just
        // render it. Otherwise, it should be a performance win
        // to do a sphere-frustum cull test before subdividing and
        // rendering segment.
        double minDistance = abs(p0.z()) - curveBoundingRadius;

        // Render close segments as splines with adaptive subdivision. The
        // subdivisions eliminates kinks between line segments and also
        // prevents clipping precision problems that occur when a
        // very long line is rendered with a relatively small view
        // volume.
        if (curveBoundingRadius >= subdivisionThreshold * minDistance || lastSegment || firstSegment)
        {
            // Skip rendering this section if it lies outside the view
            // frustum.
            if (viewFrustum.cullSphere(p0, curveBoundingRadius))
            {
                if (!restartCurve)
                {
                    vbuf.end();
                    restartCurve = true;
                }
            }
            else
            {
                double dt = m_samples[i].t - m_samples[i - 1].t;
                double t0 = 0.0;
                double t1 = 1.0;

                if (firstSegment)
                {
                    t0 = (startTime - m_samples[i - 1].t) / dt;
                    t0 = std::clamp(t0, 0.0, 1.0);
                    firstSegment = false;
                }

                if (lastSegment)
                {
                    t1 = (endTime - m_samples[i - 1].t) / dt;
                }

                Eigen::Matrix4d coeff = cubicHermiteCoefficients(p0, p1, v0 * dt, v1 * dt);
                restartCurve = rc.renderCubic(restartCurve, coeff, t0, t1, curveBoundingRadius, 1);
            }
        }
        else
        {
            // Apparent size of curve is small enough that we can approximate
            // it as a line.

            // Simple cull test--just check the far plane. This is required because
            // apparent clipping precision limitations can cause a GPU to draw lines
            // that lie completely beyond the far plane.
            if (p0.z() + curveBoundingRadius < farZ)
            {
                if (!restartCurve)
                {
                    vbuf.end();
                    restartCurve = true;
                }
            }
            else
            {
                if (restartCurve)
                {
                    vbuf.begin();
                    vbuf.vertex(p0);
                    restartCurve = false;
                }
                vbuf.vertex(p1);
            }
        }

        p0 = p1;
        v0 = v1;
    }

    if (!restartCurve)
    {
        vbuf.end();
    }

    vbuf.flush();
    vbuf.finish();
}


/** Draw a piecewise cubic curve with transformation and frustum clipping. Only
  * the part of the curve between startTime and endTime will be drawn. Additionally,
  * the curve is drawn with a fade effect. The curve is at full opacity at fadeStartTime
  * and completely transparent at fadeEndTime. fadeStartTime may be greater than
  * fadeEndTime--this just means that the fade direction will be reversed.
  *
  * @param modelview an affine transformation that will be applied to the curve
  * @param nearZ z coordinate of the near plane
  * @param farZ z coordinate of the far plane
  * @param viewFrustumPlaneNormals array of four normals (top, bottom, left, and right frustum planes)
  * @param subdivisionThreshold the threashhold for subdivision
  * @param startTime the beginning of the time interval
  * @param endTime the end of the time interval
  * @param fadeStartTime points on the curve before this time are drawn with full opacity
  * @param fadeEndTime points on the curve after this time are not drawn
  */
void
CurvePlot::renderFaded(const Eigen::Affine3d& modelview,
                       double nearZ,
                       double farZ,
                       const Eigen::Vector3d viewFrustumPlaneNormals[],
                       double subdivisionThreshold,
                       double startTime,
                       double endTime,
                       const Eigen::Vector4f& color,
                       double fadeStartTime,
                       double fadeEndTime) const
{
    // Flag to indicate whether we need to issue a glBegin()
    bool restartCurve = true;

    if (m_samples.empty() || endTime <= m_samples.front().t || startTime >= m_samples.back().t)
        return;

    // Linear search for the first sample
    unsigned int startSample = 0;
    while (startSample < m_samples.size() - 1 && startTime > m_samples[startSample].t)
        startSample++;

    // Start at the first sample with time <= startTime
    if (startSample > 0)
        startSample--;

    double fadeDuration = fadeEndTime - fadeStartTime;
    double fadeRate = 1.0 / fadeDuration;

    const Eigen::Vector3d& p0_ = m_samples[startSample].position;
    const Eigen::Vector3d& v0_ = m_samples[startSample].velocity;
    Eigen::Vector4d p0 = modelview * Eigen::Vector4d(p0_.x(), p0_.y(), p0_.z(), 1.0);
    Eigen::Vector4d v0 = modelview * Eigen::Vector4d(v0_.x(), v0_.y(), v0_.z(), 0.0);
    double opacity0 = (m_samples[startSample].t - fadeStartTime) * fadeRate;
    opacity0 = std::clamp(opacity0, 0.0, 1.0);

    HighPrec_Frustum viewFrustum(nearZ, farZ, viewFrustumPlaneNormals);
    HighPrec_RenderContext rc(vbuf, viewFrustum, subdivisionThreshold);

    vbuf.createVertexBuffer();
    if (!vbuf.setup(m_renderer, color)) return;

    bool firstSegment = true;
    bool lastSegment = false;

    for (unsigned int i = startSample + 1; i < m_samples.size() && !lastSegment; i++)
    {
        // Transform the points into camera space.
        const Eigen::Vector3d& p1_ = m_samples[i].position;
        const Eigen::Vector3d& v1_ = m_samples[i].velocity;
        Eigen::Vector4d p1 = modelview * Eigen::Vector4d(p1_.x(), p1_.y(), p1_.z(), 1.0);
        Eigen::Vector4d v1 = modelview * Eigen::Vector4d(v1_.x(), v1_.y(), v1_.z(), 0.0);
        double opacity1 = (m_samples[i].t - fadeStartTime) * fadeRate;
        opacity1 = std::clamp(opacity1, 0.0, 1.0);

        if (endTime <= m_samples[i].t)
        {
            lastSegment = true;
        }

        // O(t) is an approximating function for this segment of
        // the orbit, with 0 <= t <= 1
        // C is the viewer position
        // d(t) = |O(t) - C|, the distance from viewer to the
        // orbit segment.

        double curveBoundingRadius = m_samples[i].boundingRadius;

        // Estimate the minimum possible distance from the
        // curve to the z=0 plane. If the curve is far enough
        // away to be approximated as a straight line, we'll just
        // render it. Otherwise, it should be a performance win
        // to do a sphere-frustum cull test before subdividing and
        // rendering segment.
        double minDistance = std::abs(p0.z()) - curveBoundingRadius;

        // Render close segments as splines with adaptive subdivision. The
        // subdivisions eliminates kinks between line segments and also
        // prevents clipping precision problems that occur when a
        // very long line is rendered with a relatively small view
        // volume.
        if (curveBoundingRadius >= subdivisionThreshold * minDistance || lastSegment || firstSegment)
        {
            // Skip rendering this section if it lies outside the view
            // frustum.
            if (viewFrustum.cullSphere(p0, curveBoundingRadius))
            {
                if (!restartCurve)
                {
                    vbuf.end();
                    restartCurve = true;
                }
            }
            else
            {
                double dt = m_samples[i].t - m_samples[i - 1].t;
                double t0 = 0.0;
                double t1 = 1.0;

                if (firstSegment)
                {
                    t0 = (startTime - m_samples[i - 1].t) / dt;
                    t0 = std::clamp(t0, 0.0, 1.0);
                    firstSegment = false;
                }

                if (lastSegment)
                {
                    t1 = (endTime - m_samples[i - 1].t) / dt;
                }

                Eigen::Matrix4d coeff = cubicHermiteCoefficients(p0, p1, v0 * dt, v1 * dt);
                restartCurve = rc.renderCubicFaded(restartCurve, coeff,
                                                   t0, t1,
                                                   color,
                                                   (fadeStartTime - m_samples[i - 1].t) / dt, fadeRate * dt,
                                                   curveBoundingRadius, 1);
            }
        }
        else
        {
            // Apparent size of curve is small enough that we can approximate
            // it as a line.

            // Simple cull test--just check the far plane. This is required because
            // apparent clipping precision limitations can cause a GPU to draw lines
            // that lie completely beyond the far plane.
            if (p0.z() + curveBoundingRadius < farZ)
            {
                if (!restartCurve)
                {
                    vbuf.end();
                    restartCurve = true;
                }
            }
            else
            {
                if (restartCurve)
                {
                    vbuf.begin();
                    vbuf.vertex(p0, static_cast<float>(opacity0));
                    restartCurve = false;
                }
                vbuf.vertex(p1, static_cast<float>(opacity1));
            }
        }

        p0 = p1;
        v0 = v1;
        opacity0 = opacity1;
    }

    if (!restartCurve)
    {
        vbuf.end();
    }

    vbuf.flush();
    vbuf.finish();
}
