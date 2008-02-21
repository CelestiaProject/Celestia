// quaternion.h
// 
// Copyright (C) 2000-2006, Chris Laurel <claurel@shatters.net>
//
// Template-ized quaternion math library.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _QUATERNION_H_
#define _QUATERNION_H_

#include <limits>
#include <celmath/mathlib.h>
#include <celmath/vecmath.h>


template<class T> class Quaternion
{
public:
    inline Quaternion();
    inline Quaternion(const Quaternion<T>&);
    inline Quaternion(T);
    inline Quaternion(const Vector3<T>&);
    inline Quaternion(T, const Vector3<T>&);
    inline Quaternion(T, T, T, T);

    inline Quaternion(const Matrix3<T>&);

    inline Quaternion& operator+=(Quaternion);
    inline Quaternion& operator-=(Quaternion);
    inline Quaternion& operator*=(T);
    Quaternion& operator*=(Quaternion);

    inline Quaternion operator~() const;    // conjugate
    inline Quaternion operator-() const;
    inline Quaternion operator+() const;

    void setAxisAngle(Vector3<T> axis, T angle);

    void getAxisAngle(Vector3<T>& axis, T& angle) const;
    Matrix4<T> toMatrix4() const;
    Matrix3<T> toMatrix3() const;

    static Quaternion<T> slerp(const Quaternion<T>&, const Quaternion<T>&, T);
    static Quaternion<T> vecToVecRotation(const Vector3<T>& v0,
                                          const Vector3<T>& v1);
    static Quaternion<T> matrixToQuaternion(const Matrix3<T>& m);

    void rotate(Vector3<T> axis, T angle);
    void xrotate(T angle);
    void yrotate(T angle);
    void zrotate(T angle);

    bool isPure() const;
    bool isReal() const;
    T normalize();

    static Quaternion<T> xrotation(T);
    static Quaternion<T> yrotation(T);
    static Quaternion<T> zrotation(T);

	static Quaternion<T> lookAt(const Point3<T>& from, const Point3<T>& to, const Vector3<T>& up);

    T w, x, y, z;
};


typedef Quaternion<float> Quatf;
typedef Quaternion<double> Quatd;


template<class T> Quaternion<T>::Quaternion() : w(0), x(0), y(0), z(0)
{
}

template<class T> Quaternion<T>::Quaternion(const Quaternion<T>& q) :
    w(q.w), x(q.x), y(q.y), z(q.z)
{
}

template<class T> Quaternion<T>::Quaternion(T re) :
    w(re), x(0), y(0), z(0)
{
}

// Create a 'pure' quaternion
template<class T> Quaternion<T>::Quaternion(const Vector3<T>& im) :
     w(0), x(im.x), y(im.y), z(im.z)
{
}

template<class T> Quaternion<T>::Quaternion(T re, const Vector3<T>& im) :
     w(re), x(im.x), y(im.y), z(im.z)
{
}

template<class T> Quaternion<T>::Quaternion(T _w, T _x, T _y, T _z) :
    w(_w), x(_x), y(_y), z(_z)
{
}

// Create a quaternion from a rotation matrix
// TODO: purge this from code--it is replaced by the matrixToQuaternion()
// function.
template<class T> Quaternion<T>::Quaternion(const Matrix3<T>& m)
{
    T trace = m[0][0] + m[1][1] + m[2][2];
    T root;

    if (trace >= (T) -1.0 + 1.0e-4f)
    {
        root = (T) sqrt(trace + 1);
        w = (T) 0.5 * root;
        root = (T) 0.5 / root;
        x = (m[2][1] - m[1][2]) * root;
        y = (m[0][2] - m[2][0]) * root;
        z = (m[1][0] - m[0][1]) * root;
    }
    else
    {
        // Identify the largest element of the diagonal
        int i = 0;
        if (m[1][1] > m[i][i])
            i = 1;
        if (m[2][2] > m[i][i])
            i = 2;
        int j = (i == 2) ? 0 : i + 1;
        int k = (j == 2) ? 0 : j + 1;

        root = (T) sqrt(m[i][i] - m[j][j] - m[k][k] + 1);
        T* xyz[3] = { &x, &y, &z };
        *xyz[i] = (T) 0.5 * root;
        root = (T) 0.5 / root;
        w = (m[k][j] - m[j][k]) * root;
        *xyz[j] = (m[j][i] + m[i][j]) * root;
        *xyz[k] = (m[k][i] + m[i][k]) * root;
    }
}

template<class T> Quaternion<T>& Quaternion<T>::operator+=(Quaternion<T> a)
{
    x += a.x; y += a.y; z += a.z; w += a.w;
    return *this;
}

template<class T> Quaternion<T>& Quaternion<T>::operator-=(Quaternion<T> a)
{
    x -= a.x; y -= a.y; z -= a.z; w -= a.w;
    return *this;
}

template<class T> Quaternion<T>& Quaternion<T>::operator*=(Quaternion<T> q)
{
    *this = Quaternion<T>(w * q.w - x * q.x - y * q.y - z * q.z,
                          w * q.x + x * q.w + y * q.z - z * q.y,
                          w * q.y + y * q.w + z * q.x - x * q.z,
                          w * q.z + z * q.w + x * q.y - y * q.x);
                          
    return *this;
}

template<class T> Quaternion<T>& Quaternion<T>::operator*=(T s)
{
    x *= s; y *= s; z *= s; w *= s;
    return *this;
}

// conjugate operator
template<class T> Quaternion<T> Quaternion<T>::operator~() const
{
    return Quaternion<T>(w, -x, -y, -z);
}

template<class T> Quaternion<T> Quaternion<T>::operator-() const
{
    return Quaternion<T>(-w, -x, -y, -z);
}

template<class T> Quaternion<T> Quaternion<T>::operator+() const
{
    return *this;
}


template<class T> Quaternion<T> operator+(Quaternion<T> a, Quaternion<T> b)
{
    return Quaternion<T>(a.w + b.w, a.x + b.x, a.y + b.y, a.z + b.z);
}

template<class T> Quaternion<T> operator-(Quaternion<T> a, Quaternion<T> b)
{
    return Quaternion<T>(a.w - b.w, a.x - b.x, a.y - b.y, a.z - b.z);
}

template<class T> Quaternion<T> operator*(Quaternion<T> a, Quaternion<T> b)
{
    return Quaternion<T>(a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z,
                         a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
                         a.w * b.y + a.y * b.w + a.z * b.x - a.x * b.z,
                         a.w * b.z + a.z * b.w + a.x * b.y - a.y * b.x);
}

template<class T> Quaternion<T> operator*(T s, Quaternion<T> q)
{
    return Quaternion<T>(s * q.w, s * q.x, s * q.y, s * q.z);
}

template<class T> Quaternion<T> operator*(Quaternion<T> q, T s)
{
    return Quaternion<T>(s * q.w, s * q.x, s * q.y, s * q.z);
}

// equivalent to multiplying by the quaternion (0, v)
template<class T> Quaternion<T> operator*(Vector3<T> v, Quaternion<T> q)
{
    return Quaternion<T>(-v.x * q.x - v.y * q.y - v.z * q.z,
                         v.x * q.w + v.y * q.z - v.z * q.y,
                         v.y * q.w + v.z * q.x - v.x * q.z,
                         v.z * q.w + v.x * q.y - v.y * q.x);
}

template<class T> Quaternion<T> operator/(Quaternion<T> q, T s)
{
    return q * (1 / s);
}

template<class T> Quaternion<T> operator/(Quaternion<T> a, Quaternion<T> b)
{
    return a * (~b / abs(b));
}


template<class T> bool operator==(Quaternion<T> a, Quaternion<T> b)
{
    return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
}

template<class T> bool operator!=(Quaternion<T> a, Quaternion<T> b)
{
    return a.x != b.x || a.y != b.y || a.z != b.z || a.w != b.w;
}


// elementary functions
template<class T> Quaternion<T> conjugate(Quaternion<T> q)
{
    return Quaternion<T>(q.w, -q.x, -q.y, -q.z);
}

template<class T> T norm(Quaternion<T> q)
{
    return q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w;
}

template<class T> T abs(Quaternion<T> q)
{
    return (T) sqrt(norm(q));
}

template<class T> Quaternion<T> exp(Quaternion<T> q)
{
    if (q.isReal())
    {
        return Quaternion<T>((T) exp(q.w));
    }
    else
    {
        T l = (T) sqrt(q.x * q.x + q.y * q.y + q.z * q.z);
        T s = (T) sin(l);
        T c = (T) cos(l);
        T e = (T) exp(q.w);
        T t = e * s / l;
        return Quaternion<T>(e * c, t * q.x, t * q.y, t * q.z);
    }
}

template<class T> Quaternion<T> log(Quaternion<T> q)
{
    if (q.isReal())
    {
        if (q.w > 0)
        {
            return Quaternion<T>((T) log(q.w));
        }
        else if (q.w < 0)
        {
            // The log of a negative purely real quaternion has
            // infinitely many values, all of the form (ln(-w), PI * I),
            // where I is any unit vector.  We arbitrarily choose an I
            // of (1, 0, 0) here and whereever else a similar choice is
            // necessary.  Geometrically, the set of roots is a sphere
            // of radius PI centered at ln(-w) on the real axis.
            return Quaternion<T>((T) log(-q.w), (T) PI, 0, 0);
        }
        else
        {
            // error . . . ln(0) not defined
            return Quaternion<T>(0);
        }
    }
    else
    {
        T l = (T) sqrt(q.x * q.x + q.y * q.y + q.z * q.z);
        T r = (T) sqrt(l * l + q.w * q.w);
        T theta = (T) atan2(l, q.w);
        T t = theta / l;
        return Quaternion<T>((T) log(r), t * q.x, t * q.y, t * q.z);
    }
}


template<class T> Quaternion<T> pow(Quaternion<T> q, T s)
{
    return exp(s * log(q));
}


template<class T> Quaternion<T> pow(Quaternion<T> q, Quaternion<T> p)
{
    return exp(p * log(q));
}


template<class T> Quaternion<T> sin(Quaternion<T> q)
{
    if (q.isReal())
    {
        return Quaternion<T>((T) sin(q.w));
    }
    else
    {
        T l = (T) sqrt(q.x * q.x + q.y * q.y + q.z * q.z);
        T m = q.w;
        T s = (T) sin(m);
        T c = (T) cos(m);
        T il = 1 / l;
        T e0 = (T) exp(-l);
        T e1 = (T) exp(l);

        T c0 = (T) -0.5 * e0 * il * c;
        T c1 = (T)  0.5 * e1 * il * c;

        return Quaternion<T>((T) 0.5 * e0 * s, c0 * q.x, c0 * q.y, c0 * q.z) +
            Quaternion<T>((T) 0.5 * e1 * s, c1 * q.x, c1 * q.y, c1 * q.z);
    }
}

template<class T> Quaternion<T> cos(Quaternion<T> q)
{
    if (q.isReal())
    {
        return Quaternion<T>((T) cos(q.w));
    }
    else
    {
        T l = (T) sqrt(q.x * q.x + q.y * q.y + q.z * q.z);
        T m = q.w;
        T s = (T) sin(m);
        T c = (T) cos(m);
        T il = 1 / l;
        T e0 = (T) exp(-l);
        T e1 = (T) exp(l);

        T c0 = (T)  0.5 * e0 * il * s;
        T c1 = (T) -0.5 * e1 * il * s;

        return Quaternion<T>((T) 0.5 * e0 * c, c0 * q.x, c0 * q.y, c0 * q.z) +
            Quaternion<T>((T) 0.5 * e1 * c, c1 * q.x, c1 * q.y, c1 * q.z);
    }
}

template<class T> Quaternion<T> sqrt(Quaternion<T> q)
{
    // In general, the square root of a quaternion has two values, one
    // of which is the negative of the other.  However, any negative purely
    // real quaternion has an infinite number of square roots.
    // This function returns the positive root for positive reals and
    // the root on the positive i axis for negative reals.
    if (q.isReal())
    {
        if (q.w >= 0)
            return Quaternion<T>((T) sqrt(q.w), 0, 0, 0);
        else
            return Quaternion<T>(0, (T) sqrt(-q.w), 0, 0);
    }
    else
    {
        T b = (T) sqrt(q.x * q.x + q.y * q.y + q.z * q.z);
        T r = (T) sqrt(q.w * q.w + b * b);
        if (q.w >= 0)
        {
            T m = (T) sqrt((T) 0.5 * (r + q.w));
            T l = b / (2 * m);
            T t = l / b;
            return Quaternion<T>(m, q.x * t, q.y * t, q.z * t);
        }
        else
        {
            T l = (T) sqrt((T) 0.5 * (r - q.w));
            T m = b / (2 * l);
            T t = l / b;
            return Quaternion<T>(m, q.x * t, q.y * t, q.z * t);
        }
    }
}

template<class T> T real(Quaternion<T> q)
{
    return q.w;
}

template<class T> Vector3<T> imag(Quaternion<T> q)
{
    return Vector3<T>(q.x, q.y, q.z);
}


// Quaternion methods

template<class T> bool Quaternion<T>::isReal() const
{
    return (x == 0 && y == 0 && z == 0);
}

template<class T> bool Quaternion<T>::isPure() const
{
    return w == 0;
}

template<class T> T Quaternion<T>::normalize()
{
    T s = (T) sqrt(w * w + x * x + y * y + z * z);
    T invs = (T) 1 / (T) s;
    x *= invs;
    y *= invs;
    z *= invs;
    w *= invs;

    return s;
}

// Set to the unit quaternion representing an axis angle rotation.  Assume
// that axis is a unit vector
template<class T> void Quaternion<T>::setAxisAngle(Vector3<T> axis, T angle)
{
    T s, c;
    
    Math<T>::sincos(angle * (T) 0.5, s, c);
    x = s * axis.x;
    y = s * axis.y;
    z = s * axis.z;
    w = c;
}


// Assuming that this a unit quaternion, return the in axis/angle form the
// orientation which it represents.
template<class T> void Quaternion<T>::getAxisAngle(Vector3<T>& axis,
                                                   T& angle) const
{
    // The quaternion has the form:
    // w = cos(angle/2), (x y z) = sin(angle/2)*axis

    T magSquared = x * x + y * y + z * z;
    if (magSquared > (T) 1e-10)
    {
        T s =  (T) 1 / (T) sqrt(magSquared);
        axis.x = x * s;
        axis.y = y * s;
        axis.z = z * s;
        if (w <= -1 || w >= 1)
            angle = 0;
        else
            angle = (T) acos(w) * 2;
    }
    else
    {
        // The angle is zero, so we pick an arbitrary unit axis
        axis.x = 1;
        axis.y = 0;
        axis.z = 0;
        angle = 0;
    }
}


// Convert this (assumed to be normalized) quaternion to a rotation matrix
template<class T> Matrix4<T> Quaternion<T>::toMatrix4() const
{
    T wx = w * x * 2;
    T wy = w * y * 2;
    T wz = w * z * 2;
    T xx = x * x * 2;
    T xy = x * y * 2;
    T xz = x * z * 2;
    T yy = y * y * 2;
    T yz = y * z * 2;
    T zz = z * z * 2;

    return Matrix4<T>(Vector4<T>(1 - yy - zz, xy - wz, xz + wy, 0),
                      Vector4<T>(xy + wz, 1 - xx - zz, yz - wx, 0),
                      Vector4<T>(xz - wy, yz + wx, 1 - xx - yy, 0),
                      Vector4<T>(0, 0, 0, 1));
}


// Convert this (assumed to be normalized) quaternion to a rotation matrix
template<class T> Matrix3<T> Quaternion<T>::toMatrix3() const
{
    T wx = w * x * 2;
    T wy = w * y * 2;
    T wz = w * z * 2;
    T xx = x * x * 2;
    T xy = x * y * 2;
    T xz = x * z * 2;
    T yy = y * y * 2;
    T yz = y * z * 2;
    T zz = z * z * 2;

    return Matrix3<T>(Vector3<T>(1 - yy - zz, xy - wz, xz + wy),
                      Vector3<T>(xy + wz, 1 - xx - zz, yz - wx),
                      Vector3<T>(xz - wy, yz + wx, 1 - xx - yy));
}


template<class T> T dot(Quaternion<T> a, Quaternion<T> b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}


/*! Spherical linear interpolation of two unit quaternions. Designed for
 *  interpolating rotations, so shortest path between rotations will be
 *  taken.
 */
template<class T> Quaternion<T> Quaternion<T>::slerp(const Quaternion<T>& q0,
                                                     const Quaternion<T>& q1,
                                                     T t)
{
    const double Nlerp_Threshold = 0.99999;

    T cosAngle = dot(q0, q1);

    // Assuming the quaternions representat rotations, ensure that we interpolate
    // through the shortest path by inverting one of the quaternions if the
    // angle between them is negative.
    Quaternion qstart;
    if (cosAngle < 0)
    {
        qstart = -q0;
        cosAngle  = -cosAngle;
    }
    else
    {
        qstart = q0;
    }

    // Avoid precision troubles when we're near the limit of acos range and
    // perform a linear interpolation followed by a normalize when interpolating
    // very small angles.
    if (cosAngle > (T) Nlerp_Threshold)
    {
        Quaternion<T> q = (1 - t) * qstart + t * q1;
        q.normalize();
        return q;
    }

    // Below code unnecessary since we've already inverted cosAngle if it's negative.
    // It will be necessary if we change slerp to not assume that we want the shortest
    // path between two rotations.
#if 0
    // Because of potential rounding errors, we must clamp c to the domain of acos.
    if (cosAngle < (T) -1.0)
        cosAngle = (T) -1.0;
#endif

    T angle = (T) acos(cosAngle);
    T interpolatedAngle = t * angle;

    // qstart and q2 will form an orthonormal basis in the plane of interpolation.
    Quaternion q2 = q1 - qstart * cosAngle;
    q2.normalize();

    return qstart * (T) cos(interpolatedAngle) + q2 * (T) sin(interpolatedAngle);
#if 0
    T s = (T) sin(angle);
    T is = (T) 1.0 / s;

    return q0 * ((T) sin((1 - t) * angle) * is) +
           q1 * ((T) sin(t * angle) * is);
#endif
}


/*! Return a unit quaternion that representing a rotation that will
 *  rotation v0 to v1 about the axis perpendicular to them both. If the
 *  vectors point in opposite directions, there is no unique axis and
 *  (arbitrarily) a rotation about the x axis will be chosen.
 */
template<class T> Quaternion<T> Quaternion<T>::vecToVecRotation(const Vector3<T>& v0,
                                                                const Vector3<T>& v1)
{
    // We need sine and cosine of half the angle between v0 and v1, so
    // compute the vector halfway between v0 and v1. The cross product of
    // half and v1 gives the imaginary part of the quaternion
    // (axis * sin(angle/2)), and the dot product of half and v1 gives
    // the real part.
    Vector3<T> half = (v0 + v1) * (T) 0.5;

    T hl = half.length();
    if (hl > (T) 0.0)
    {
        half = half / hl; // normalize h

        // The magnitude of rotAxis is the sine of half the angle between
        // v0 and v1.
        Vector3<T> rotAxis = half ^ v1;
        T cosAngle = half * v1;
        return Quaternion<T>(cosAngle, rotAxis.x, rotAxis.y, rotAxis.z);
    }
    else
    {
        // The vectors point in exactly opposite directions, so there is
        // no unique axis of rotation. Rotating v0 180 degrees about any
        // axis will map it to v1; we'll choose the x-axis.
        return Quaternion<T>((T) 0.0, (T) 1.0, (T) 0.0, (T) 0.0);
    }
}


/*! Create a quaternion from a rotation matrix
 */
template<class T> Quaternion<T> Quaternion<T>::matrixToQuaternion(const Matrix3<T>& m)
{
    Quaternion<T> q;
    T trace = m[0][0] + m[1][1] + m[2][2];
    T root;
    T epsilon = std::numeric_limits<T>::epsilon() * (T) 1e3;

    if (trace >= epsilon - 1)
    {
        root = (T) sqrt(trace + 1);
        q.w = (T) 0.5 * root;
        root = (T) 0.5 / root;
        q.x = (m[2][1] - m[1][2]) * root;
        q.y = (m[0][2] - m[2][0]) * root;
        q.z = (m[1][0] - m[0][1]) * root;
    }
    else
    {
        // Set i to the largest element of the diagonal
        int i = 0;
        if (m[1][1] > m[i][i])
            i = 1;
        if (m[2][2] > m[i][i])
            i = 2;
        int j = (i == 2) ? 0 : i + 1;
        int k = (j == 2) ? 0 : j + 1;

        root = (T) sqrt(m[i][i] - m[j][j] - m[k][k] + 1);
        T* xyz[3] = { &q.x, &q.y, &q.z };
        *xyz[i] = (T) 0.5 * root;
        root = (T) 0.5 / root;
        q.w = (m[k][j] - m[j][k]) * root;
        *xyz[j] = (m[j][i] + m[i][j]) * root;
        *xyz[k] = (m[k][i] + m[i][k]) * root;
    }

    return q;
}


/*! Assuming that this is a unit quaternion representing an orientation,
 *  apply a rotation of angle radians about the specfied axis
 */
template<class T> void Quaternion<T>::rotate(Vector3<T> axis, T angle)
{
    Quaternion q;
    q.setAxisAngle(axis, angle);
    *this = q * *this;
}


// Assuming that this is a unit quaternion representing an orientation,
// apply a rotation of angle radians about the x-axis
template<class T> void Quaternion<T>::xrotate(T angle)
{
    T s, c;

    Math<T>::sincos(angle * (T) 0.5, s, c);
    *this = Quaternion<T>(c, s, 0, 0) * *this;
}

// Assuming that this is a unit quaternion representing an orientation,
// apply a rotation of angle radians about the y-axis
template<class T> void Quaternion<T>::yrotate(T angle)
{
    T s, c;

    Math<T>::sincos(angle * (T) 0.5, s, c);
    *this = Quaternion<T>(c, 0, s, 0) * *this;
}

// Assuming that this is a unit quaternion representing an orientation,
// apply a rotation of angle radians about the z-axis
template<class T> void Quaternion<T>::zrotate(T angle)
{
    T s, c;

    Math<T>::sincos(angle * (T) 0.5, s, c);
    *this = Quaternion<T>(c, 0, 0, s) * *this;
}


template<class T> Quaternion<T> Quaternion<T>::xrotation(T angle)
{
    T s, c;
    Math<T>::sincos(angle * (T) 0.5, s, c);
    return Quaternion<T>(c, s, 0, 0);
}

template<class T> Quaternion<T> Quaternion<T>::yrotation(T angle)
{
    T s, c;
    Math<T>::sincos(angle * (T) 0.5, s, c);
    return Quaternion<T>(c, 0, s, 0);
}

template<class T> Quaternion<T> Quaternion<T>::zrotation(T angle)
{
    T s, c;
    Math<T>::sincos(angle * (T) 0.5, s, c);
    return Quaternion<T>(c, 0, 0, s);
}

/*! Determine an orientation that will make the negative z-axis point from
 *  from the observer to the target, with the y-axis pointing in direction
 *  of the component of 'up' that is orthogonal to the z-axis.
 */
template<class T> Quaternion<T> 
Quaternion<T>::lookAt(const Point3<T>& from, const Point3<T>& to, const Vector3<T>& up)
{
    Vector3<T> n = to - from;
    n.normalize();
    Vector3<T> v = n ^ up;
    v.normalize();
    Vector3<T> u = v ^ n;

    return Quaternion<T>::matrixToQuaternion(Matrix3<T>(v, u, -n));
}

#endif // _QUATERNION_H_
