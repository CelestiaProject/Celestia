#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "mathlib.h"
#include "perlin.h"

using namespace Eigen;


float bias(float a, float b)
{
    return (float) pow((double) a, log((double) b) / log(0.5));
}

float gain(float a, float b)
{
    float p = (float) (log(1.0 - b) / log(0.5));

    if (a < 0.001f)
	return 0.0f;
    else if (a > 0.999f)
	return 1.0f;

    if (a < 0.5f)
	return (float) pow(2 * a, p) / 2;
    else
	return 1.0f - (float) pow(2.0 * (1.0 - a), (double) p) / 2;
}

float noise(float vec[], int len)
{
    switch (len) {
    case 0:
	return 0.;
    case 1:
	return noise1(vec[0]);
    case 2:
	return noise2(vec);
    default:
	return noise3(vec);
    }
}


float turbulence(float v[], float freq)
{
    float t, vec[3];

    for (t = 0. ; freq >= 1. ; freq /= 2) {
	vec[0] = freq * v[0];
	vec[1] = freq * v[1];
	vec[2] = freq * v[2];
	t += (float) fabs(noise3(vec)) / freq;
    }
    return t;
}


float turbulence(const Vector2f& p, float freq)
{
    float t;
    float vec[2];

    for (t = 0.0f; freq >= 1.0f; freq *= 0.5f)
    {
    vec[0] = freq * p.x();
    vec[1] = freq * p.y();
	t += (float) fabs(noise2(vec)) / freq;
    }

    return t;
}


float turbulence(const Vector3f& p, float freq)
{
    float t;
    float vec[3];

    for (t = 0.0f; freq >= 1.0f; freq *= 0.5f)
    {
    vec[0] = freq * p.x();
    vec[1] = freq * p.y();
    vec[2] = freq * p.z();
	t += (float) fabs(noise3(vec)) / freq;
    }

    return t;
}


float fractalsum(float v[], float freq)
{
    float t;
    float vec[3];

    for (t = 0.0f ; freq >= 1.0f ; freq /= 2.0f) {
	vec[0] = freq * v[0];
	vec[1] = freq * v[1];
	vec[2] = freq * v[2];
	t += noise3(vec) / freq;
    }

    return t;
}


float fractalsum(const Vector2f& p, float freq)
{
    float t;
    float vec[2];

    for (t = 0.0f; freq >= 1.0f; freq *= 0.5f)
    {
    vec[0] = freq * p.x();
    vec[1] = freq * p.y();
	t += noise2(vec) / freq;
    }

    return t;
}


float fractalsum(const Vector3f& p, float freq)
{
    float t;
    float vec[3];

    for (t = 0.0f; freq >= 1.0f; freq *= 0.5f)
    {
    vec[0] = freq * p.x();
    vec[1] = freq * p.y();
    vec[2] = freq * p.z();
	t += noise3(vec) / freq;
    }

    return t;
}


/* noise functions over 1, 2, and 3 dimensions */

#define B 0x100
#define BM 0xff

#define N 0x1000
#define NP 12   /* 2^N */
#define NM 0xfff

static int p[B + B + 2];
static float g3[B + B + 2][3];
static float g2[B + B + 2][2];
static float g1[B + B + 2];

static bool initialized = false;

static void init(void);

#define s_curve(t) ( t * t * (3.0f - 2.0f * t) )

#define setup(i, b0, b1, r0, r1) \
	t = vec[i] + N;\
	b0 = ((int)t) & BM;\
	b1 = (b0+1) & BM;\
	r0 = t - (int)t;\
	r1 = r0 - 1.0f;

float noise1(float arg)
{
    if (!initialized)
        init();

    int bx0, bx1;
    float rx0, rx1, t, u, v, vec[1];

    vec[0] = arg;

    setup(0, bx0, bx1, rx0, rx1);

    u = rx0 * g1[p[bx0]];
    v = rx1 * g1[p[bx1]];

    return Mathf::lerp(s_curve(rx0), u, v);
}

float noise2(float vec[2])
{
	int bx0, bx1, by0, by1, b00, b10, b01, b11;
	float rx0, rx1, ry0, ry1, *q, sx, sy, a, b, t, u, v;
	int i, j;

	if (!initialized)
	    init();

	setup(0, bx0,bx1, rx0,rx1);
	setup(1, by0,by1, ry0,ry1);

	i = p[ bx0 ];
	j = p[ bx1 ];

	b00 = p[ i + by0 ];
	b10 = p[ j + by0 ];
	b01 = p[ i + by1 ];
	b11 = p[ j + by1 ];

	sx = s_curve(rx0);
	sy = s_curve(ry0);

#define at2(rx,ry) ( rx * q[0] + ry * q[1] )

	q = g2[ b00 ] ; u = at2(rx0,ry0);
	q = g2[ b10 ] ; v = at2(rx1,ry0);
	a = Mathf::lerp(sx, u, v);

	q = g2[ b01 ] ; u = at2(rx0,ry1);
	q = g2[ b11 ] ; v = at2(rx1,ry1);
	b = Mathf::lerp(sx, u, v);

	return Mathf::lerp(sy, a, b);
}

float noise3(float vec[3])
{
	if (!initialized)
	    init();

	int bx0, bx1, by0, by1, bz0, bz1, b00, b10, b01, b11;
	float rx0, rx1, ry0, ry1, rz0, rz1, *q, sy, sz, a, b, c, d, t, u, v;
	int i, j;

	setup(0, bx0,bx1, rx0,rx1);
	setup(1, by0,by1, ry0,ry1);
	setup(2, bz0,bz1, rz0,rz1);

	i = p[ bx0 ];
	j = p[ bx1 ];

	b00 = p[ i + by0 ];
	b10 = p[ j + by0 ];
	b01 = p[ i + by1 ];
	b11 = p[ j + by1 ];

	t  = s_curve(rx0);
	sy = s_curve(ry0);
	sz = s_curve(rz0);

#define at3(rx,ry,rz) ( rx * q[0] + ry * q[1] + rz * q[2] )

	q = g3[ b00 + bz0 ] ; u = at3(rx0,ry0,rz0);
	q = g3[ b10 + bz0 ] ; v = at3(rx1,ry0,rz0);
	a = Mathf::lerp(t, u, v);

	q = g3[ b01 + bz0 ] ; u = at3(rx0,ry1,rz0);
	q = g3[ b11 + bz0 ] ; v = at3(rx1,ry1,rz0);
	b = Mathf::lerp(t, u, v);

	c = Mathf::lerp(sy, a, b);

	q = g3[ b00 + bz1 ] ; u = at3(rx0,ry0,rz1);
	q = g3[ b10 + bz1 ] ; v = at3(rx1,ry0,rz1);
	a = Mathf::lerp(t, u, v);

	q = g3[ b01 + bz1 ] ; u = at3(rx0,ry1,rz1);
	q = g3[ b11 + bz1 ] ; v = at3(rx1,ry1,rz1);
	b = Mathf::lerp(t, u, v);

	d = Mathf::lerp(sy, a, b);

	return Mathf::lerp(sz, c, d);
}

static void normalize2(float v[2])
{
    float s = (float) sqrt(v[0] * v[0] + v[1] * v[1]);
    v[0] = v[0] / s;
    v[1] = v[1] / s;
}

static void normalize3(float v[3])
{
    float s = (float) sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    v[0] = v[0] / s;
    v[1] = v[1] / s;
    v[2] = v[2] / s;
}


static void init()
{
    int i, j, k;

    for (i = 0; i < B; i++)
    {
        g1[i]    = Mathf::sfrand();

        g2[i][0] = Mathf::sfrand();
        g2[i][1] = Mathf::sfrand();
        normalize2(g2[i]);

        g3[i][0] = Mathf::sfrand();
        g3[i][1] = Mathf::sfrand();
        g3[i][2] = Mathf::sfrand();
        normalize3(g3[i]);
    }

    // Fill the permutation array with values . . .
    for (i = 0; i < B; i++)
        p[i] = i;

    // . . . and then shuffle it
    for (i = 0; i < B; i++)
    {
        k = p[i];
        j = rand() % B;
        p[i] = p[j];
        p[j] = k;
    }

    // Duplicate values to accelerate table lookups
    for (i = 0; i < B + 2; i++) 
    {
        p[B + i] = p[i];
        g1[B + i]    = g1[i];
        g2[B + i][0] = g2[i][0];
        g2[B + i][1] = g2[i][1];
        g3[B + i][0] = g3[i][0];
        g3[B + i][1] = g3[i][1];
        g3[B + i][2] = g3[i][2];
    }

    initialized = true;
}

