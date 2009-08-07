// curveplot.cpp
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
// CurvePlot. If not, see <http://www.gnu.org/licenses/>.

#define DEBUG_ADAPTIVE_SPLINE 0
#if DEBUG_ADAPTIVE_SPLINE
#define USE_VERTEX_BUFFER 0
#else
#define USE_VERTEX_BUFFER 0
#endif

#include "curveplot.h"
#include "GL/glew.h"
#include <vector>
#include <iostream>

using namespace std;
using namespace Eigen;

static const unsigned int SubdivisionFactor = 8;
static const double InvSubdivisionFactor = 1.0 / (double) SubdivisionFactor;



#if DEBUG_ADAPTIVE_SPLINE
static float SplineColors[10][3] = {
    { 0, 0, 1 },
    { 0, 1, 1 },
    { 0, 1, 0 },
    { 1, 1, 0 },
    { 1, 0, 0 },
    { 1, 0, 1 },
    { 0.5f, 0.5f, 1.0f },
    { 0.5f, 1.0f, 1.0f },
    { 0.5f, 1.0f, 0.5f },
    { 1.0f, 1.0f, 0.5f },
};

static unsigned int SegmentCounts[32];
#endif

#ifndef EIGEN_VECTORIZE
// Vectorization should be enabled for improved performance.
#endif

// Convert a 3-vector to a 4-vector by adding a zero
static inline Vector4d zeroExtend(const Vector3d& v)
{
    return Vector4d(v.x(), v.y(), v.z(), 0.0);
}


class HighPrec_Frustum
{
public:
    HighPrec_Frustum(double nearZ, double farZ, const Vector3d planeNormals[]) :
        m_nearZ(nearZ),
        m_farZ(farZ)
    {
        for (unsigned int i = 0; i < 4; i++)
        {
            m_planeNormals[i] = zeroExtend(planeNormals[i]);
        }
    }      

    inline bool cullSphere(const Vector3d& center,
                           double radius) const
    {
        return (center.z() - radius > m_nearZ ||
                center.z() + radius < m_farZ  ||
                center.dot(m_planeNormals[0].start<3>()) < -radius ||
                center.dot(m_planeNormals[1].start<3>()) < -radius ||
                center.dot(m_planeNormals[2].start<3>()) < -radius ||
                center.dot(m_planeNormals[3].start<3>()) < -radius);
    }

    inline bool cullSphere(const Vector4d& center,
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
    Vector4d m_planeNormals[4];
};


static inline Matrix4d cubicHermiteCoefficients(const Vector4d& p0,
                                                const Vector4d& p1,
                                                const Vector4d& v0,
                                                const Vector4d& v1)
{
    Matrix4d coeff;
    coeff.col(0) = p0;
    coeff.col(1) = v0;
    coeff.col(2) = 3.0 * (p1 - p0) - (2.0 * v0 + v1);
    coeff.col(3) = 2.0 * (p0 - p1) + (v1 + v0);

    return coeff;
}


// Test a point to see if it lies within the frustum defined by
// planes z=nearZ, z=farZ, and the four side planes with specified
// normals.
static inline bool frustumCull(const Vector4d& curvePoint,
                               double curveBoundingRadius,
                               double nearZ, double farZ,
                               const Vector4d viewFrustumPlaneNormals[])
{
    return (curvePoint.z() - curveBoundingRadius > nearZ ||
            curvePoint.z() + curveBoundingRadius < farZ  ||
            curvePoint.dot(viewFrustumPlaneNormals[0]) < -curveBoundingRadius ||
            curvePoint.dot(viewFrustumPlaneNormals[1]) < -curveBoundingRadius ||
            curvePoint.dot(viewFrustumPlaneNormals[2]) < -curveBoundingRadius ||
            curvePoint.dot(viewFrustumPlaneNormals[3]) < -curveBoundingRadius);
}


class HighPrec_VertexBuffer
{
public:
    HighPrec_VertexBuffer() :
        currentPosition(0),
        capacity(4096),
        data(NULL),
        vbobj(0),
        currentStripLength(0)
    {
        data = new Vector4f[capacity];
    }

    void setup()
    {
#if USE_VERTEX_BUFFER
        if (vbobj)
        {
            glx::glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbobj);
        }

        glEnableClientState(GL_VERTEX_ARRAY);

        mapBuffer();

        Vector4f* vertexBase = vbobj ? (Vector4f*) NULL : data;
        glVertexPointer(3, GL_FLOAT, sizeof(Vector4f), vertexBase);

        stripLengths.clear();
        currentStripLength = 0;
        currentPosition = 0;
#endif
    }

    void finish()
    {
        unmapBuffer();

#if USE_VERTEX_BUFFER
        if (vbobj)
        {
            glDisableClientState(GL_VERTEX_ARRAY);
            glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
        }
#endif
    }

    inline void vertex(const Vector3d& v)
    {
#if USE_VERTEX_BUFFER
        data[currentPosition++].segment<3>(0) = v.cast<float>();
        ++currentStripLength;
        if (currentPosition == capacity)
        {
            flush();

            data[0].segment<3>(0) = v.cast<float>();
            currentPosition = 1;
            currentStripLength = 1;
        }
#else
        glVertex3dv(v.data());
#endif
    }

    inline void vertex(const Vector4d& v)
    {
#if USE_VERTEX_BUFFER
        data[currentPosition++] = v.cast<float>();
        ++currentStripLength;
        if (currentPosition == capacity)
        {
            flush();

            data[0] = v.cast<float>();
            currentPosition = 1;
            currentStripLength = 1;
        }
#else
        glVertex3dv(v.data());
#endif
    }

    inline void begin()
    {
#if !USE_VERTEX_BUFFER
        glBegin(GL_LINE_STRIP);
#endif
    }

    inline void end()
    {
#if USE_VERTEX_BUFFER
        stripLengths.push_back(currentStripLength);
        currentStripLength = 0;
#else
        glEnd();
#endif
    }

    inline void flush()
    {
#if USE_VERTEX_BUFFER
        if (currentPosition > 0)
        {
            unmapBuffer();

            // Finish the current line strip
            if (currentStripLength > 1)
                end();

            unsigned int startIndex = 0;
            for (vector<unsigned int>::const_iterator iter = stripLengths.begin(); iter != stripLengths.end(); ++iter)
            {
                glDrawArrays(GL_LINE_STRIP, startIndex, *iter);
                startIndex += *iter;
            }

            mapBuffer();

            currentPosition = 0;
            stripLengths.clear();
        }

        currentStripLength = 0;
#endif
    }

    void createVertexBuffer()
    {
#if USE_VERTEX_BUFFER
        if (!vbobj)
        {
            glx::glGenBuffersARB(1, &vbobj);
            glx::glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbobj);
            glx::glBufferDataARB(GL_ARRAY_BUFFER_ARB,
                                 capacity * sizeof(Vector4f),
                                 NULL,
                                 GL_STREAM_DRAW_ARB);
        }
#endif
    }

    void mapBuffer()
    {
        if (vbobj)
        {
            // Calling glBufferDataARB() with NULL before mapping the buffer
            // is a hint to OpenGL that previous contents of vertex buffer will
            // be discarded and overwritten. It enables renaming in the driver,
            // hopefully resulting in performance gains.
            glBufferDataARB(GL_ARRAY_BUFFER_ARB,
                            capacity * sizeof(Vector4f),
                            NULL,
                            GL_STREAM_DRAW_ARB);

            data = reinterpret_cast<Vector4f*>(glMapBufferARB(GL_ARRAY_BUFFER_ARB, GL_WRITE_ONLY_ARB));
        }
    }

    void unmapBuffer()
    {
#if USE_VERTEX_BUFFER
        if (vbobj)
        {
            glUnmapBufferARB(GL_ARRAY_BUFFER_ARB);
            data = NULL;
        }
#endif
    }
        
private:
    unsigned int currentPosition;
    unsigned int capacity;
    Vector4f* data;
    GLuint vbobj;
    unsigned int currentStripLength;
    vector<unsigned int> stripLengths;
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
                     const Matrix4d& coeff,
                     double t0, double t1,
                     double curveBoundingRadius,
                     int depth) const
    {
        const double dt = (t1 - t0) * InvSubdivisionFactor;
        double segmentBoundingRadius = curveBoundingRadius * InvSubdivisionFactor;

#if DEBUG_ADAPTIVE_SPLINE
        {
            int c = depth % 10;
            glColor4f(SplineColors[c][0], SplineColors[c][1], SplineColors[c][2], 1.0f);
            ++SegmentCounts[depth];
        }
#endif

        Vector4d lastP = coeff * Vector4d(1.0, t0, t0 * t0, t0 * t0 * t0);

        for (unsigned int i = 1; i <= SubdivisionFactor; i++)
        {
            double t = t0 + dt * i;
            Vector4d p = coeff * Vector4d(1.0, t, t * t, t * t * t);

            double minDistance = max(-m_viewFrustum.nearZ(), abs(p.z()) - segmentBoundingRadius);

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
#if DEBUG_ADAPTIVE_SPLINE
                {
                    int c = depth % 10;
                    glColor4f(SplineColors[c][0], SplineColors[c][1], SplineColors[c][2], i % 2 ? 0.25f : 1.0f);
                }
#endif

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

private:
    HighPrec_VertexBuffer& m_vbuf;
    HighPrec_Frustum& m_viewFrustum;
    double m_subdivisionThreshold;
};



static HighPrec_VertexBuffer vbuf;


CurvePlot::CurvePlot()
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
            Matrix4d coeff = cubicHermiteCoefficients(zeroExtend(lastSample.position),
                                                      zeroExtend(sample.position),
                                                      zeroExtend(lastSample.velocity * dt),
                                                      zeroExtend(sample.velocity * dt));
            Vector4d extents = coeff.cwise().abs() * Vector4d(0.0, 1.0, 1.0, 1.0);
            m_samples[m_samples.size() - 1].boundingRadius = extents.norm();
        }
        else
        {
            const CurvePlotSample& nextSample = m_samples[1];
            double dt = nextSample.t - sample.t;
            Matrix4d coeff = cubicHermiteCoefficients(zeroExtend(sample.position),
                                                      zeroExtend(nextSample.position),
                                                      zeroExtend(sample.velocity * dt),
                                                      zeroExtend(nextSample.velocity * dt));
            Vector4d extents = coeff.cwise().abs() * Vector4d(0.0, 1.0, 1.0, 1.0);
            m_samples[1].boundingRadius = extents.norm();
        }
    }
}


void
CurvePlot::removeSamplesBefore(double t)
{
    while (!m_samples.empty() && m_samples.front().t < t)
    {
        m_samples.pop_front();
    }
}


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

void
CurvePlot::render(const Transform3d& modelview,
                  double nearZ,
                  double farZ,
                  const Vector3d viewFrustumPlaneNormals[],
                  double subdivisionThreshold) const
{
    // Flag to indicate whether we need to issue a glBegin()
    bool restartCurve = true;

    const Vector3d& p0_ = m_samples[0].position;
    const Vector3d& v0_ = m_samples[0].velocity;
    Vector4d p0 = modelview * Vector4d(p0_.x(), p0_.y(), p0_.z(), 1.0);
    Vector4d v0 = modelview * Vector4d(v0_.x(), v0_.y(), v0_.z(), 0.0);

    HighPrec_Frustum viewFrustum(nearZ, farZ, viewFrustumPlaneNormals);
    HighPrec_RenderContext rc(vbuf, viewFrustum, subdivisionThreshold);

#if DEBUG_ADAPTIVE_SPLINE
    for (unsigned int i = 0; i < sizeof(SegmentCounts) / sizeof(SegmentCounts[0]); i++)
        SegmentCounts[i] = 0;
#endif

    vbuf.createVertexBuffer();
    vbuf.setup();

    clog << "size: " << m_samples.size() << endl;
    for (unsigned int i = 1; i < m_samples.size(); i++)
    {
        // Transform the points into camera space.
        const Vector3d& p1_ = m_samples[i].position;
        const Vector3d& v1_ = m_samples[i].velocity;
        Vector4d p1 = modelview * Vector4d(p1_.x(), p1_.y(), p1_.z(), 1.0);
        Vector4d v1 = modelview * Vector4d(v1_.x(), v1_.y(), v1_.z(), 0.0);

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
        if (curveBoundingRadius >= subdivisionThreshold * minDistance)
        {
#if DEBUG_ADAPTIVE_SPLINE
            ++SegmentCounts[0];
#endif
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
                Matrix4d coeff = cubicHermiteCoefficients(p0, p1, v0 * dt, v1 * dt);

                restartCurve = rc.renderCubic(restartCurve, coeff, 0.0, 1.0, curveBoundingRadius, 1);
            }
        }
        else
        {
#if DEBUG_ADAPTIVE_SPLINE
            glColor4f(SplineColors[0][0], SplineColors[0][1], SplineColors[0][2], 1.0f);
#endif

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

#if DEBUG_ADAPTIVE_SPLINE3
    for (unsigned int i = 0; SegmentCounts[i] != 0 || i < 3; i++)
    {
        clog << i << ":" << SegmentCounts[i] << ", ";
    }
    clog << endl;
#endif
}


void
CurvePlot::render(const Transform3d& modelview,
                  double nearZ,
                  double farZ,
                  const Vector3d viewFrustumPlaneNormals[],
                  double subdivisionThreshold,
                  double startTime,
                  double endTime) const
{
    // Flag to indicate whether we need to issue a glBegin()
    bool restartCurve = true;

    if (m_samples.empty() || endTime <= m_samples.front().t || startTime >= m_samples.back().t)
        return;

    // Linear search for the first sample
    unsigned int startSample = 0;
    while (startSample < m_samples.size() - 1 && startTime < m_samples[startSample].t)
        startSample++;

    // Start at the first sample with time <= startTime
    if (startSample > 0)
        startSample--;

    const Vector3d& p0_ = m_samples[startSample].position;
    const Vector3d& v0_ = m_samples[startSample].velocity;
    Vector4d p0 = modelview * Vector4d(p0_.x(), p0_.y(), p0_.z(), 1.0);
    Vector4d v0 = modelview * Vector4d(v0_.x(), v0_.y(), v0_.z(), 0.0);

    HighPrec_Frustum viewFrustum(nearZ, farZ, viewFrustumPlaneNormals);
    HighPrec_RenderContext rc(vbuf, viewFrustum, subdivisionThreshold);

    vbuf.createVertexBuffer();
    vbuf.setup();

    bool firstSegment = true;
    bool lastSegment = false;

    for (unsigned int i = startSample + 1; i < m_samples.size() && !lastSegment; i++)
    {
        // Transform the points into camera space.
        const Vector3d& p1_ = m_samples[i].position;
        const Vector3d& v1_ = m_samples[i].velocity;
        Vector4d p1 = modelview * Vector4d(p1_.x(), p1_.y(), p1_.z(), 1.0);
        Vector4d v1 = modelview * Vector4d(v1_.x(), v1_.y(), v1_.z(), 0.0);

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
                    t0 = std::max(0.0, std::min(1.0, t0));
                    firstSegment = false;
                }

                if (lastSegment)
                {
                    t1 = (endTime - m_samples[i - 1].t) / dt;
                }

                Matrix4d coeff = cubicHermiteCoefficients(p0, p1, v0 * dt, v1 * dt);
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
