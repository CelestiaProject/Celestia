// scattersim.cpp
//
// Copyright (C) 2007, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//

#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <cmath>
#include <algorithm>
#include <map>
#include <celutil/basictypes.h>
#include <celmath/vecmath.h>
#include <celmath/mathlib.h>
#include <celmath/ray.h>
#include <celmath/sphere.h>
#include <celmath/intersect.h>
#ifdef MACOSX
#include "../../../macosx/png.h"
#else
#include "png.h"
#endif // MACOSX

using namespace std;



// Extinction lookup table dimensions
const unsigned int ExtinctionLUTHeightSteps = 256;
const unsigned int ExtinctionLUTViewAngleSteps = 512;

// Scattering lookup table dimensions
const unsigned int ScatteringLUTHeightSteps = 64;
const unsigned int ScatteringLUTViewAngleSteps = 64;
const unsigned int ScatteringLUTLightAngleSteps = 64;

// Values settable via the command line
static unsigned int IntegrateScatterSteps = 20;
static unsigned int IntegrateDepthSteps = 20;
static unsigned int OutputImageWidth = 600;
static unsigned int OutputImageHeight = 450;
enum LUTUsageType
{
    NoLUT,
    UseExtinctionLUT,
    UseScatteringLUT
};

static LUTUsageType LUTUsage = NoLUT;
static bool UseFisheyeCameras = false;
static double CameraExposure = 0.0;


typedef map<string, double> ParameterSet;


struct Color
{
    Color() : r(0.0f), g(0.0f), b(0.0f) {}
    Color(float _r, float _g, float _b) : r(_r), g(_g), b(_b) {}

    Color exposure(float e) const
    {
#if 0
        float brightness = max(r, max(g, b));
        float f = 1.0f - (float) exp(-e * brightness);
        return Color(r * f, g * f, b * f);
#endif
        return Color((float) (1.0 - exp(-e * r)),
                     (float) (1.0 - exp(-e * g)),
                     (float) (1.0 - exp(-e * b)));
    }

    float r, g, b;
};

Color operator*(const Color& color, double d)
{
    return Color((float) (color.r * d),
                 (float) (color.g * d),
                 (float) (color.b * d));
}

Color operator+(const Color& a, const Color& b)
{
    return Color(a.r + b.r, a.g + b.g, a.b + b.b);
}

Color operator*(const Color& a, const Color& b)
{
    return Color(a.r * b.r, a.g * b.g, a.b * b.b);
}

Color operator*(const Color& a, const Vec3d& v)
{
    return Color((float) (a.r * v.x), (float) (a.g * v.y), (float) (a.b * v.z));
}


static uint8 floatToByte(float f)
{
    if (f <= 0.0f)
        return 0;
    else if (f >= 1.0f)
        return 255;
    else
        return (uint8) (f * 255.99f);
}


class Camera
{
public:
    enum CameraType
    {
        Planar,
        Spherical,
    };

    Camera() :
        fov(PI / 2.0),
        front(1.0),
        transform(Mat4d::identity()),
        type(Planar)
    {
    }

    Ray3d getViewRay(double viewportX, double viewportY) const
    {
        Vec3d viewDir;

        if (type == Planar)
        {
            double viewPlaneHeight = tan(fov / 2.0) * 2 * front;
            viewDir.x = viewportX * viewPlaneHeight;
            viewDir.y = viewportY * viewPlaneHeight;
            viewDir.z = front;
            viewDir.normalize();
        }
        else
        {
            double phi = -viewportY * fov / 2.0 + PI / 2.0;
            double theta = viewportX * fov / 2.0 + PI / 2.0; 
            viewDir.x = sin(phi) * cos(theta);
            viewDir.y = cos(phi);
            viewDir.z = sin(phi) * sin(theta);
            viewDir.normalize();
        }

        Ray3d viewRay(Point3d(0.0, 0.0, 0.0), viewDir);

        return viewRay * transform;
    }

    double fov;
    double front;
    Mat4d transform;
    CameraType type;
};


class Viewport
{
public:
    Viewport(unsigned int _x, unsigned int _y,
             unsigned int _width, unsigned int _height) :
        x(_x), y(_y), width(_width), height(_height)
    {
    }

    unsigned int x;
    unsigned int y;
    unsigned int width;
    unsigned int height;
};


class Light
{
public:
    Vec3d direction;
    Color color;
};


struct OpticalDepths
{
    double rayleigh;
    double mie;
    double absorption;
};

OpticalDepths sumOpticalDepths(OpticalDepths a, OpticalDepths b)
{
    OpticalDepths depths;
    depths.rayleigh   = a.rayleigh + b.rayleigh;
    depths.mie        = a.mie + b.mie;
    depths.absorption = a.absorption + b.absorption;
    return depths;
}


typedef double (*MiePhaseFunction)(double cosTheta, double asymmetry);
double phaseHenyeyGreenstein_CS(double, double);
double phaseHenyeyGreenstein(double, double);
double phaseSchlick(double, double);

class Atmosphere
{
public:
    Atmosphere() :
        miePhaseFunction(phaseHenyeyGreenstein_CS)
    {
    }

    double calcShellHeight()
    {
        double maxScaleHeight = max(rayleighScaleHeight, max(mieScaleHeight, absorbScaleHeight));
        return -log(0.002) * maxScaleHeight;
    }

    double miePhase(double cosAngle) const
    {
        return miePhaseFunction(cosAngle, mieAsymmetry);
    }

    double rayleighDensity(double h) const;
    double mieDensity(double h) const;
    double absorbDensity(double h) const;

    Vec3d computeExtinction(const OpticalDepths&) const;

    double rayleighScaleHeight;
    double mieScaleHeight;
    double absorbScaleHeight;

    Vec3d rayleighCoeff;
    Vec3d absorbCoeff;
    double mieCoeff;

    double mieAsymmetry;

    MiePhaseFunction miePhaseFunction;
};


class LUT2
{
public:
    LUT2(unsigned int w, unsigned int h);
    
    Vec3d getValue(unsigned int x, unsigned int y) const;
    void setValue(unsigned int x, unsigned int y, const Vec3d&);
    Vec3d lookup(double x, double y) const;
    unsigned int getWidth() const { return width; }
    unsigned int getHeight() const { return height; }

private:
    unsigned int width;
    unsigned int height;
    float* values;
};


class LUT3
{
public:
    LUT3(unsigned int w, unsigned int h, unsigned int d);
    
    Vec4d getValue(unsigned int x, unsigned int y, unsigned int z) const;
    void setValue(unsigned int x, unsigned int y, unsigned int z, const Vec4d&);
    Vec4d lookup(double x, double y, double z) const;
    unsigned int getWidth() const { return width; }
    unsigned int getHeight() const { return height; }
    unsigned int getDepth() const { return depth; }

private:
    unsigned int width;
    unsigned int height;
    unsigned int depth;
    float* values;
};


class Scene
{
public:
    Scene() {};
    
    void setParameters(ParameterSet& params);

    Color raytrace(const Ray3d& ray) const;
    Color raytrace_LUT(const Ray3d& ray) const;

    Color background;
    Light light;
    Sphered planet;
    Color planetColor;
    Color planetColor2;
    Atmosphere atmosphere;

    double atmosphereShellHeight;

    double sunAngularDiameter;

    LUT2* extinctionLUT;
    LUT3* scatteringLUT;
};


class RGBImage
{
public:
    RGBImage(int w, int h) :
        width(w),
        height(h),
        pixels(NULL)
    {
        pixels = new uint8[width * height * 3];
    }

    ~RGBImage()
    {
        delete[] pixels;
    }

    void clearRect(const Color& color,
                   unsigned int x,
                   unsigned int y,
                   unsigned int w,
                   unsigned int h)
    {
        
        uint8 r = floatToByte(color.r);
        uint8 g = floatToByte(color.g);
        uint8 b = floatToByte(color.b);
        for (unsigned int i = y; i < y + h; i++)
        {
            for (unsigned int j = x; j < x + w; j++)
            {
                pixels[(i * width + j) * 3    ] = r;
                pixels[(i * width + j) * 3 + 1] = g;
                pixels[(i * width + j) * 3 + 2] = b;
            }
        }
    }

    void clear(const Color& color)
    {
        clearRect(color, 0, 0, width, height);
    }

    void setPixel(unsigned int x, unsigned int y, const Color& color)
    {
        if (x >= 0 && x < width && y >= 0 && y < height)
        {
            unsigned int pix = (x + y * width) * 3;
            pixels[pix + 0] = floatToByte(color.r);
            pixels[pix + 1] = floatToByte(color.g);
            pixels[pix + 2] = floatToByte(color.b);
        }
    }

    unsigned int width;
    unsigned int height;
    unsigned char* pixels;
};


void usage()
{
    cerr << "Usage: scattersim [options] <config file>\n";
    cerr << "   --lut (or -l)              : accelerate calculation by using a lookup table\n";
    cerr << "   --fisheye (or -f)          : use wide angle cameras on surface\n";
    cerr << "   --exposure <value> (or -e) : set exposure for HDR\n";
    cerr << "   --width <value> (or -w)    : set width of output image\n";
    cerr << "   --height <value> (or -h)   : set height of output image\n";
    cerr << "   --image <filename> (or -i) : set filename of output image\n";
    cerr << "           (default is out.png)\n";
    cerr << "   --depthsteps <value> (or -d)\n";
    cerr << "           set the number of integration steps for depth\n";
    cerr << "   --scattersteps <value> (or -s)\n";
    cerr << "           set the number of integration steps for scattering\n";
}


static void PNGWriteData(png_structp png_ptr, png_bytep data, png_size_t length)
{
    FILE* fp = (FILE*) png_get_io_ptr(png_ptr);
    fwrite((void*) data, 1, length, fp);
}


bool WritePNG(const string& filename, const RGBImage& image)
              
{
    int rowStride = image.width * 3;

    FILE* out;
    out = fopen(filename.c_str(), "wb");
    if (out == NULL)
    {
        cerr << "Can't open screen capture file " << filename << "\n";
        return false;
    }

    png_bytep* row_pointers = new png_bytep[image.height];
    for (unsigned int i = 0; i < image.height; i++)
        row_pointers[i] = (png_bytep) &image.pixels[rowStride * (image.height - i - 1)];

    png_structp png_ptr;
    png_infop info_ptr;

    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
                                      NULL, NULL, NULL);

    if (png_ptr == NULL)
    {
        cerr << "Screen capture: error allocating png_ptr\n";
        fclose(out);
        delete[] row_pointers;
        return false;
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL)
    {
        cerr << "Screen capture: error allocating info_ptr\n";
        fclose(out);
        delete[] row_pointers;
        png_destroy_write_struct(&png_ptr, (png_infopp) NULL);
        return false;
    }

    if (setjmp(png_jmpbuf(png_ptr)))
    {
        cerr << "Error writing PNG file " << filename << endl;
        fclose(out);
        delete[] row_pointers;
        png_destroy_write_struct(&png_ptr, &info_ptr);
        return false;
    }

    // png_init_io(png_ptr, out);
    png_set_write_fn(png_ptr, (void*) out, PNGWriteData, NULL);

    png_set_compression_level(png_ptr, Z_BEST_COMPRESSION);
    png_set_IHDR(png_ptr, info_ptr,
                 image.width, image.height,
                 8,
                 PNG_COLOR_TYPE_RGB,
                 PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT,
                 PNG_FILTER_TYPE_DEFAULT);

    png_write_info(png_ptr, info_ptr);
    png_write_image(png_ptr, row_pointers);
    png_write_end(png_ptr, info_ptr);

    // Clean up everything . . .
    png_destroy_write_struct(&png_ptr, &info_ptr);
    delete[] row_pointers;
    fclose(out);

    return true;
}


float mapColor(double c)
{
    //return (float) ((5.0 + log(max(0.0, c))) * 1.0);
    return (float) c;
}


void DumpLUT(const LUT3& lut, const string& filename)
{
    unsigned int xtiles = 8;
    unsigned int ytiles = lut.getDepth() / xtiles;
    unsigned int tileWidth = lut.getHeight();
    unsigned int tileHeight = lut.getWidth();
    double scale = 1.0 / 1.0;

    RGBImage img(xtiles * tileWidth, xtiles * tileHeight);

    for (unsigned int i = 0; i < ytiles; i++)
    {
        for (unsigned int j = 0; j < xtiles; j++)
        {
            unsigned int z = j + xtiles * i;
            for (unsigned int k = 0; k < tileWidth; k++)
            {
                unsigned int y = k;
                for (unsigned int l = 0; l < tileHeight; l++)
                {
                    unsigned int x = l;
                    Vec4d v = lut.getValue(x, y, z);
                    Color c(mapColor(v.x * 0.000617f * 4.0 * PI),
                            mapColor(v.y * 0.00109f * 4.0 * PI),
                            mapColor(v.z * 0.00195f * 4.0 * PI));
                    img.setPixel(j * tileWidth + k, i * tileHeight + l, c);
                }
            }
        }
    }

    for (unsigned int blah = 0; blah < img.width; blah++)
        img.setPixel(blah, 0, Color(1.0f, 0.0f, 0.0f));

    WritePNG(filename, img);
}

void DumpLUT(const LUT2& lut, const string& filename)
{
    RGBImage img(lut.getHeight(), lut.getWidth());

    for (unsigned int i = 0; i < lut.getWidth(); i++)
    {
        for (unsigned int j = 0; j < lut.getHeight(); j++)
        {
            Vec3d v = lut.getValue(i, j);
            Color c(mapColor(v.x), mapColor(v.y), mapColor(v.z));
            img.setPixel(j, i, c);
        }
    }

    for (unsigned int blah = 0; blah < img.width; blah++)
        img.setPixel(blah, 0, Color(1.0f, 0.0f, 0.0f));

    WritePNG(filename, img);
}



template<class T> T lerp(double t, T v0, T v1)
{
    return (1 - t) * v0 + t * v1;
}

template<class T> T bilerp(double t, double u, T v00, T v01, T v10, T v11)
{
    return lerp(u, lerp(t, v00, v01), lerp(t, v10, v11));
}

template<class T> T trilerp(double t, double u, double v,
                            T v000, T v001, T v010, T v011,
                            T v100, T v101, T v110, T v111)
{
    return lerp(v,
                bilerp(t, u, v000, v001, v010, v011),
                bilerp(t, u, v100, v101, v110, v111));
}


LUT2::LUT2(unsigned int _width, unsigned int _height) :
    width(_width),
    height(_height),
    values(NULL)
{
    values = new float[width * height * 3];
}


Vec3d
LUT2::getValue(unsigned int x, unsigned int y) const
{
    unsigned int n = 3 * (x + y * width);

    return Vec3d(values[n], values[n + 1], values[n + 2]);
}


void
LUT2::setValue(unsigned int x, unsigned int y, const Vec3d& v)
{
    unsigned int n = 3 * (x + y * width);
    values[n]     = (float) v.x;
    values[n + 1] = (float) v.y;
    values[n + 2] = (float) v.z;
}


Vec3d
LUT2::lookup(double x, double y) const
{
    x = max(0.0, min(x, 0.999999));
    y = max(0.0, min(y, 0.999999));
    unsigned int fx = (unsigned int) (x * (width - 1));
    unsigned int fy = (unsigned int) (y * (height - 1));
    double t = x * (width - 1) - fx;
    double u = y * (height - 1) - fy;

    return bilerp(t, u,
                  getValue(fx, fy),
                  getValue(fx + 1, fy),
                  getValue(fx, fy + 1),
                  getValue(fx + 1, fy + 1));
}


LUT3::LUT3(unsigned int _width, unsigned int _height, unsigned int _depth) :
    width(_width),
    height(_height),
    depth(_depth),
    values(NULL)
{
    values = new float[width * height * depth * 4];
}


Vec4d
LUT3::getValue(unsigned int x, unsigned int y, unsigned int z) const
{
    unsigned int n = 4 * (x + (y + z * height) * width);

    return Vec4d(values[n], values[n + 1], values[n + 2], values[n + 3]);
}


void
LUT3::setValue(unsigned int x, unsigned int y, unsigned int z, const Vec4d& v)
{
    unsigned int n = 4 * (x + (y + z * height) * width);

    values[n]     = (float) v.x;
    values[n + 1] = (float) v.y;
    values[n + 2] = (float) v.z;
    values[n + 3] = (float) v.w;
}


Vec4d
LUT3::lookup(double x, double y, double z) const
{
    x = max(0.0, min(x, 0.999999));
    y = max(0.0, min(y, 0.999999));
    z = max(0.0, min(z, 0.999999));
    unsigned int fx = (unsigned int) (x * (width - 1));
    unsigned int fy = (unsigned int) (y * (height - 1));
    unsigned int fz = (unsigned int) (z * (depth - 1));
    double t = x * (width - 1) - fx;
    double u = y * (height - 1) - fy;
    double v = z * (depth - 1) - fz;

    return trilerp(t, u, v,
                   getValue(fx,     fy,     fz),
                   getValue(fx + 1, fy,     fz),
                   getValue(fx,     fy + 1, fz),
                   getValue(fx + 1, fy + 1, fz),
                   getValue(fx,     fy,     fz + 1),
                   getValue(fx + 1, fy,     fz + 1),
                   getValue(fx,     fy + 1, fz + 1),
                   getValue(fx + 1, fy + 1, fz + 1));
}


template<class T> bool raySphereIntersect(const Ray3<T>& ray,
                                          const Sphere<T>& sphere,
                                          T& dist0,
                                          T& dist1)
{
    Vector3<T> diff = ray.origin - sphere.center;
    T s = (T) 1.0 / square(sphere.radius);
    T a = ray.direction * ray.direction * s;
    T b = ray.direction * diff * s;
    T c = diff * diff * s - (T) 1.0;
    T disc = b * b - a * c;
    if (disc < 0.0)
        return false;

    disc = (T) sqrt(disc);
    T sol0 = (-b + disc) / a;
    T sol1 = (-b - disc) / a;

    if (sol0 <= 0.0 && sol1 <= 0.0)
    {
        return false;
    }
    else if (sol0 < sol1)
    {
        dist0 = max(0.0, sol0);
        dist1 = sol1;
        return true;
    }
    else if (sol1 < sol0)
    {
        dist0 = max(0.0, sol1);
        dist1 = sol0;
        return true;
    }
    else
    {
        return false;
    }
}


template<class T> bool raySphereIntersect2(const Ray3<T>& ray,
                                           const Sphere<T>& sphere,
                                           T& dist0,
                                           T& dist1)
{
    Vector3<T> diff = ray.origin - sphere.center;
    T s = (T) 1.0 / square(sphere.radius);
    T a = ray.direction * ray.direction * s;
    T b = ray.direction * diff * s;
    T c = diff * diff * s - (T) 1.0;
    T disc = b * b - a * c;
    if (disc < 0.0)
        return false;

    disc = (T) sqrt(disc);
    T sol0 = (-b + disc) / a;
    T sol1 = (-b - disc) / a;

    if (sol0 < sol1)
    {
        dist0 = sol0;
        dist1 = sol1;
        return true;
    }
    else if (sol0 > sol1)
    {
        dist0 = sol1;
        dist1 = sol0;
        return true;
    }
    else
    {
        // One solution to quadratic indicates grazing intersection; treat
        // as no intersection.
        return false;
    }
}



double phaseRayleigh(double cosTheta)
{
    return 0.75 * (1.0 + cosTheta * cosTheta);
}


double phaseHenyeyGreenstein(double cosTheta, double g)
{
    return (1.0 - g * g) / pow(1.0 + g * g - 2.0 * g * cosTheta, 1.5);
}

double mu2g(double mu)
{
    // mu = <cosTheta>
    // This routine invertes the simple relation of the Cornette-Shanks
    // improved Henyey-Greenstein(HG) phase function:
    // mu = <cosTheta>= 3*g *(g^2 + 4)/(5*(2+g^2))

    double mu2 = mu * mu; 
    double x = 0.5555555556 * mu + 0.17146776 * mu * mu2 + sqrt(max(2.3703704 - 1.3374486 * mu2 + 0.57155921 * mu2 * mu2, 0.0));
    double y = pow(x, 0.33333333333);
    return 0.55555555556 * mu - (1.33333333333 - 0.30864198 * mu2) / y + y;
}


double phaseHenyeyGreenstein_CS(double cosTheta, double g)
{ 
    // improved HG - phase function -> Rayleigh phase function for 
    // g -> 0, -> HG-phase function for g -> 1.
    double g2 = g * g;
    return 1.5 * (1.0 - g2) * (1.0 + cosTheta * cosTheta) /((2.0 + g2) * pow(1.0 + g2 - 2.0 * g * cosTheta, 1.5));
}


// Convert the asymmetry paramter for the Henyey-Greenstein function to
// the approximate equivalent for the Schlick phase function. From Blasi,
// Saec, and Schlick: 1993, "A rendering algorithm for discrete volume
// density objects"
double schlick_g2k(double g)
{
    return 1.55 * g - 0.55 * g * g * g;
}


// The Schlick phase function is a less computationally expensive than the
// Henyey-Greenstein function, but produces similar results. May be more
// appropriate for a GPU implementation.
double phaseSchlick(double cosTheta, double k)
{
    return (1 - k * k) / square(1 - k * cosTheta);
}


/*
 * Theory:
 * Atmospheres are assumed to be composed of three different populations of
 * particles: Rayleigh scattering, Mie scattering, and absorbing. The density
 * of each population decreases exponentially with height above the planet
 * surface to a degree determined by a scale height:
 *
 *     density(height) = e^(-height/scaleHeight)
 *
 * Rayleigh scattering is wavelength dependent, with a fixed phase function.
 *
 * Mie scattering is wavelength independent, with a phase function determined
 * by a single parameter g (the asymmetry parameter)
 *
 * Absorption is wavelength dependent
 *
 * The light source is assumed to be at an effectively infinite distance from
 * the planet. This means that for any view ray, the light angle will be
 * constant, and the phase function can thus be pulled out of the inscattering
 * integral to simplify the calculation.
 */


/*** Pure ray marching integration functions; no use of lookup tables ***/


OpticalDepths integrateOpticalDepth(const Scene& scene,
                                    const Point3d& atmStart,
                                    const Point3d& atmEnd)
{
    unsigned int nSteps = IntegrateDepthSteps;

    OpticalDepths depth;
    depth.rayleigh   = 0.0;
    depth.mie        = 0.0;
    depth.absorption = 0.0;

    Vec3d dir = atmEnd - atmStart;
    double length = dir.length();
    double stepDist = length / (double) nSteps;
    if (length == 0.0)
        return depth;

    dir = dir * (1.0 / length);
    Point3d samplePoint = atmStart + (0.5 * stepDist * dir);

    for (unsigned int i = 0; i < nSteps; i++)
    {
        double h = samplePoint.distanceFromOrigin() - scene.planet.radius;

        // Optical depth due to two phenomena:
        //   Outscattering by Rayleigh and Mie scattering particles
        //   Absorption by absorbing particles
        depth.rayleigh   += scene.atmosphere.rayleighDensity(h) * stepDist;
        depth.mie        += scene.atmosphere.mieDensity(h)      * stepDist;
        depth.absorption += scene.atmosphere.absorbDensity(h)   * stepDist;
        
        samplePoint += stepDist * dir;
    }

    return depth;
}


double
Atmosphere::rayleighDensity(double h) const
{
    return exp(min(1.0, -h / rayleighScaleHeight));
}


double
Atmosphere::mieDensity(double h) const
{
    return exp(min(1.0, -h / mieScaleHeight));
}


double
Atmosphere::absorbDensity(double h) const
{
    return exp(min(1.0, -h / absorbScaleHeight));
}


Vec3d
Atmosphere::computeExtinction(const OpticalDepths& depth) const
{
    Vec3d extinction;
    extinction.x = exp(-depth.rayleigh   * rayleighCoeff.x -
                        depth.mie        * mieCoeff -
                        depth.absorption * absorbCoeff.x);
    extinction.y = exp(-depth.rayleigh   * rayleighCoeff.y -
                        depth.mie        * mieCoeff -
                        depth.absorption * absorbCoeff.y);
    extinction.z = exp(-depth.rayleigh   * rayleighCoeff.z -
                        depth.mie        * mieCoeff -
                        depth.absorption * absorbCoeff.z);

    return extinction;
}


Vec3d integrateInscattering(const Scene& scene,
                            const Point3d& atmStart,
                            const Point3d& atmEnd)
{
    const unsigned int nSteps = IntegrateScatterSteps;

    Vec3d dir = atmEnd - atmStart;
    Point3d origin = Point3d(0.0, 0.0, 0.0) + (atmStart - scene.planet.center);
    double stepDist = dir.length() / (double) nSteps;
    dir.normalize();

    // Start at the midpoint of the first interval
    Point3d samplePoint = origin + 0.5 * stepDist * dir;

    Vec3d rayleighScatter(0.0, 0.0, 0.0);
    Vec3d mieScatter(0.0, 0.0, 0.0);

    Vec3d lightDir = -scene.light.direction;
    Sphered shell = Sphered(Point3d(0.0, 0.0, 0.0),
                            scene.planet.radius + scene.atmosphereShellHeight);

    for (unsigned int i = 0; i < nSteps; i++)
    {
        Ray3d sunRay(samplePoint, lightDir);
        double sunDist = 0.0;
        testIntersection(sunRay, shell, sunDist);

        // Compute the optical depth along path from sample point to the sun
        OpticalDepths sunDepth = integrateOpticalDepth(scene, samplePoint, sunRay.point(sunDist));
        // Compute the optical depth along the path from the sample point to the eye
        OpticalDepths eyeDepth = integrateOpticalDepth(scene, samplePoint, atmStart);

        // Sum the optical depths to get the depth on the complete path from sun
        // to sample point to eye.
        OpticalDepths totalDepth = sumOpticalDepths(sunDepth, eyeDepth);
        totalDepth.rayleigh *= 4.0 * PI;
        totalDepth.mie      *= 4.0 * PI;

        Vec3d extinction = scene.atmosphere.computeExtinction(totalDepth);

        double h = samplePoint.distanceFromOrigin() - scene.planet.radius;

        // Add the inscattered light from Rayleigh and Mie scattering particles
        rayleighScatter += scene.atmosphere.rayleighDensity(h) * stepDist * extinction;
        mieScatter +=      scene.atmosphere.mieDensity(h)      * stepDist * extinction;
        
        samplePoint += stepDist * dir;
    }

    double cosSunAngle = lightDir * dir;

    double miePhase = scene.atmosphere.miePhase(cosSunAngle);
    const Vec3d& rayleigh = scene.atmosphere.rayleighCoeff;
    return phaseRayleigh(cosSunAngle) * Vec3d(rayleighScatter.x * rayleigh.x,
                                              rayleighScatter.y * rayleigh.y,
                                              rayleighScatter.z * rayleigh.z) +
        miePhase * mieScatter * scene.atmosphere.mieCoeff;
}


Vec4d integrateInscatteringFactors(const Scene& scene,
                                   const Point3d& atmStart,
                                   const Point3d& atmEnd,
                                   const Vec3d& lightDir)
{
    const unsigned int nSteps = IntegrateScatterSteps;

    Vec3d dir = atmEnd - atmStart;
    Point3d origin = Point3d(0.0, 0.0, 0.0) + (atmStart - scene.planet.center);
    double stepDist = dir.length() / (double) nSteps;
    dir.normalize();

    // Start at the midpoint of the first interval
    Point3d samplePoint = origin + 0.5 * stepDist * dir;

    Vec3d rayleighScatter(0.0, 0.0, 0.0);
    Vec3d mieScatter(0.0, 0.0, 0.0);

    Sphered shell = Sphered(Point3d(0.0, 0.0, 0.0),
                            scene.planet.radius + scene.atmosphereShellHeight);

    for (unsigned int i = 0; i < nSteps; i++)
    {
        Ray3d sunRay(samplePoint, lightDir);
        double sunDist = 0.0;
        testIntersection(sunRay, shell, sunDist);

        // Compute the optical depth along path from sample point to the sun
        OpticalDepths sunDepth = integrateOpticalDepth(scene, samplePoint, sunRay.point(sunDist));
        // Compute the optical depth along the path from the sample point to the eye
        OpticalDepths eyeDepth = integrateOpticalDepth(scene, samplePoint, atmStart);

        // Sum the optical depths to get the depth on the complete path from sun
        // to sample point to eye.
        OpticalDepths totalDepth = sumOpticalDepths(sunDepth, eyeDepth);
        totalDepth.rayleigh *= 4.0 * PI;
        totalDepth.mie      *= 4.0 * PI;

        Vec3d extinction = scene.atmosphere.computeExtinction(totalDepth);

        double h = samplePoint.distanceFromOrigin() - scene.planet.radius;

        // Add the inscattered light from Rayleigh and Mie scattering particles
        rayleighScatter += scene.atmosphere.rayleighDensity(h) * stepDist * extinction;
        mieScatter +=      scene.atmosphere.mieDensity(h)      * stepDist * extinction;
        
        samplePoint += stepDist * dir;
    }

    return Vec4d(rayleighScatter.x,
                 rayleighScatter.y,
                 rayleighScatter.z,
                 0.0);
}



/**** Lookup table acceleration of scattering ****/


// Pack a signed value in [-1, 1] into [0, 1]
double packSNorm(double sn)
{
    return (sn + 1.0) * 0.5;
}


// Expand an unsigned value in [0, 1] into [-1, 1]
double unpackSNorm(double un)
{
    return un * 2.0 - 1.0;
}


LUT2*
buildExtinctionLUT(const Scene& scene)
{
    LUT2* lut = new LUT2(ExtinctionLUTHeightSteps,
                         ExtinctionLUTViewAngleSteps);

    Sphered planet = Sphered(scene.planet.radius);
    Sphered shell = Sphered(scene.planet.radius + scene.atmosphereShellHeight);

    for (unsigned int i = 0; i < ExtinctionLUTHeightSteps; i++)
    {
        double h = (double) i / (double) (ExtinctionLUTHeightSteps - 1) *
            scene.atmosphereShellHeight * 0.9999;
        Point3d atmStart = Point3d(0.0, 0.0, 0.0) +
            Vec3d(1.0, 0.0, 0.0) * (h + scene.planet.radius);

        for (unsigned int j = 0; j < ExtinctionLUTViewAngleSteps; j++)
        {
            double cosAngle = (double) j / (ExtinctionLUTViewAngleSteps - 1) * 2.0 - 1.0;
            double sinAngle = sqrt(1.0 - min(1.0, cosAngle * cosAngle));
            Vec3d viewDir(cosAngle, sinAngle, 0.0);
            
            Ray3d ray(atmStart, viewDir);
            double dist = 0.0;

            if (!testIntersection(ray, shell, dist))
                dist = 0.0;

            OpticalDepths depth = integrateOpticalDepth(scene, atmStart,
                                                        ray.point(dist));
            depth.rayleigh *= 4.0 * PI;
            depth.mie      *= 4.0 * PI;
            Vec3d ext = scene.atmosphere.computeExtinction(depth);
            ext.x = max(ext.x, 1.0e-18);
            ext.y = max(ext.y, 1.0e-18);
            ext.z = max(ext.z, 1.0e-18);

            lut->setValue(i, j, ext);
        }
    }

    return lut;
}


Vec3d
lookupExtinction(const Scene& scene,
                 const Point3d& atmStart,
                 const Point3d& atmEnd)
{
    Vec3d viewDir = atmEnd - atmStart;
    Vec3d toCenter = atmStart - Point3d(0.0, 0.0, 0.0);
    viewDir.normalize();
    toCenter.normalize();
    double h = (atmStart.distanceFromOrigin() - scene.planet.radius) /
        scene.atmosphereShellHeight;
    double cosViewAngle = viewDir * toCenter;

    return scene.extinctionLUT->lookup(h, packSNorm(cosViewAngle));
}



LUT2*
buildOpticalDepthLUT(const Scene& scene)
{
    LUT2* lut = new LUT2(ExtinctionLUTHeightSteps,
                         ExtinctionLUTViewAngleSteps);

    Sphered planet = Sphered(scene.planet.radius);
    Sphered shell = Sphered(scene.planet.radius + scene.atmosphereShellHeight);

    for (unsigned int i = 0; i < ExtinctionLUTHeightSteps; i++)
    {
        double h = (double) i / (double) (ExtinctionLUTHeightSteps - 1) *
            scene.atmosphereShellHeight;
        Point3d atmStart = Point3d(0.0, 0.0, 0.0) +
            Vec3d(1.0, 0.0, 0.0) * (h + scene.planet.radius);

        for (unsigned int j = 0; j < ExtinctionLUTViewAngleSteps; j++)
        {
            double cosAngle = (double) j / (ExtinctionLUTViewAngleSteps - 1) * 2.0 - 1.0;
            double sinAngle = sqrt(1.0 - min(1.0, cosAngle * cosAngle));
            Vec3d dir(cosAngle, sinAngle, 0.0);
            
            Ray3d ray(atmStart, dir);
            double dist = 0.0;

            if (!testIntersection(ray, shell, dist))
                dist = 0.0;

            OpticalDepths depth = integrateOpticalDepth(scene, atmStart,
                                                        ray.point(dist));
            depth.rayleigh *= 4.0 * PI;
            depth.mie      *= 4.0 * PI;

            lut->setValue(i, j, Vec3d(depth.rayleigh, depth.mie, depth.absorption));
        }
    }

    return lut;
}


OpticalDepths
lookupOpticalDepth(const Scene& scene,
                   const Point3d& atmStart,
                   const Point3d& atmEnd)
{
    Vec3d dir = atmEnd - atmStart;
    Vec3d toCenter = atmStart - Point3d(0.0, 0.0, 0.0);
    dir.normalize();
    toCenter.normalize();
    double h = (atmStart.distanceFromOrigin() - scene.planet.radius) /
        scene.atmosphereShellHeight;
    double cosViewAngle = dir * toCenter;

    Vec3d v = scene.extinctionLUT->lookup(h, (cosViewAngle + 1.0) * 0.5);
    OpticalDepths depth;
    depth.rayleigh = v.x;
    depth.mie = v.y;
    depth.absorption = v.z;

    return depth;
}


Vec3d integrateInscattering_LUT(const Scene& scene,
                                const Point3d& atmStart,
                                const Point3d& atmEnd,
                                const Point3d& eyePt,
                                bool hitPlanet)
{
    const unsigned int nSteps = IntegrateScatterSteps;

    double shellHeight = scene.planet.radius + scene.atmosphereShellHeight;
    Sphered shell = Sphered(shellHeight);
    bool eyeInsideAtmosphere = eyePt.distanceFromOrigin() < shellHeight;

    Vec3d lightDir = -scene.light.direction;

    Point3d origin = eyeInsideAtmosphere ? eyePt : atmStart;
    Vec3d viewDir = atmEnd - origin;
    double stepDist = viewDir.length() / (double) nSteps;
    viewDir.normalize();

    // Start at the midpoint of the first interval
    Point3d samplePoint = origin + 0.5 * stepDist * viewDir;

    Vec3d rayleighScatter(0.0, 0.0, 0.0);
    Vec3d mieScatter(0.0, 0.0, 0.0);

    for (unsigned int i = 0; i < nSteps; i++)
    {
        Ray3d sunRay(samplePoint, lightDir);
        double sunDist = 0.0;
        testIntersection(sunRay, shell, sunDist);

        Vec3d sunExt = lookupExtinction(scene, samplePoint, sunRay.point(sunDist));
        Vec3d eyeExt;
        if (!eyeInsideAtmosphere)
        {
            eyeExt = lookupExtinction(scene, samplePoint, atmStart);
        }
        else
        {
            // Eye is inside the atmosphere, so we need to subtract
            // extinction from the part of the light path not traveled.
            // Do this carefully! We want to avoid doing arithmetic with
            // intervals that pass through the planet, since they tend to
            // have values extremely close to zero.
            Vec3d subExt;
            if (hitPlanet)
            {
                eyeExt = lookupExtinction(scene, samplePoint, atmStart);
                subExt = lookupExtinction(scene, eyePt, atmStart);
            }
            else
            {
                eyeExt = lookupExtinction(scene, eyePt, atmEnd);
                subExt = lookupExtinction(scene, samplePoint, atmEnd);
            }

            // Subtract the extinction from the untraversed portion of the
            // light path.
            eyeExt = Vec3d(eyeExt.x / subExt.x,
                           eyeExt.y / subExt.y,
                           eyeExt.z / subExt.z);
        }

        // Compute the extinction along the entire light path from sun to sample point
        // to eye.
        Vec3d extinction(sunExt.x * eyeExt.x, sunExt.y * eyeExt.y, sunExt.z * eyeExt.z);

        double h = samplePoint.distanceFromOrigin() - scene.planet.radius;

        // Add the inscattered light from Rayleigh and Mie scattering particles
        rayleighScatter += scene.atmosphere.rayleighDensity(h) * stepDist * extinction;
        mieScatter      += scene.atmosphere.mieDensity(h)      * stepDist * extinction;
        
        samplePoint += stepDist * viewDir;
    }

    double cosSunAngle = lightDir * viewDir;

    double miePhase = scene.atmosphere.miePhase(cosSunAngle);
    const Vec3d& rayleigh = scene.atmosphere.rayleighCoeff;
    return phaseRayleigh(cosSunAngle) * Vec3d(rayleighScatter.x * rayleigh.x,
                                              rayleighScatter.y * rayleigh.y,
                                              rayleighScatter.z * rayleigh.z) +
        miePhase * mieScatter * scene.atmosphere.mieCoeff;
}


// Used for building LUT; start point is assumed to be within
// atmosphere.
Vec4d integrateInscatteringFactors_LUT(const Scene& scene,
                                       const Point3d& atmStart,
                                       const Point3d& atmEnd,
                                       const Vec3d& lightDir,
                                       bool planetHit)
{
    const unsigned int nSteps = IntegrateScatterSteps;

    double shellHeight = scene.planet.radius + scene.atmosphereShellHeight;
    Sphered shell = Sphered(shellHeight);

    Point3d origin = atmStart;
    Vec3d viewDir = atmEnd - origin;
    double stepDist = viewDir.length() / (double) nSteps;
    viewDir.normalize();

    // Start at the midpoint of the first interval
    Point3d samplePoint = origin + 0.5 * stepDist * viewDir;

    Vec3d rayleighScatter(0.0, 0.0, 0.0);
    Vec3d mieScatter(0.0, 0.0, 0.0);

    for (unsigned int i = 0; i < nSteps; i++)
    {
        Ray3d sunRay(samplePoint, lightDir);
        double sunDist = 0.0;
        testIntersection(sunRay, shell, sunDist);

        Vec3d sunExt = lookupExtinction(scene, samplePoint, sunRay.point(sunDist));
        Vec3d eyeExt;
        Vec3d subExt;
        if (planetHit)
        {
            eyeExt = lookupExtinction(scene, samplePoint, atmEnd);
            subExt = lookupExtinction(scene, atmEnd, atmStart);
        }
        else
        {
            eyeExt = lookupExtinction(scene, atmStart, atmEnd);
            subExt = lookupExtinction(scene, samplePoint, atmEnd);
        }

        // Subtract the extinction from the untraversed portion of the
        // light path.
        eyeExt = Vec3d(eyeExt.x / subExt.x,
                       eyeExt.y / subExt.y,
                       eyeExt.z / subExt.z);

        //Vec3d eyeExt = lookupExtinction(scene, samplePoint, atmStart);


        // Compute the extinction along the entire light path from sun to sample point
        // to eye.
        Vec3d extinction(sunExt.x * eyeExt.x, sunExt.y * eyeExt.y, sunExt.z * eyeExt.z);

        double h = samplePoint.distanceFromOrigin() - scene.planet.radius;

        // Add the inscattered light from Rayleigh and Mie scattering particles
        rayleighScatter += scene.atmosphere.rayleighDensity(h) * stepDist * extinction;
        mieScatter      += scene.atmosphere.mieDensity(h) * stepDist * extinction;
        
        samplePoint += stepDist * viewDir;
    }

    return Vec4d(rayleighScatter.x,
                 rayleighScatter.y,
                 rayleighScatter.z,
                 0.0);
}


LUT3*
buildScatteringLUT(const Scene& scene)
{
    LUT3* lut = new LUT3(ScatteringLUTHeightSteps,
                         ScatteringLUTViewAngleSteps,
                         ScatteringLUTLightAngleSteps);

    Sphered shell = Sphered(scene.planet.radius + scene.atmosphereShellHeight);

    for (unsigned int i = 0; i < ScatteringLUTHeightSteps; i++)
    {
        double h = (double) i / (double) (ScatteringLUTHeightSteps - 1) *
            scene.atmosphereShellHeight * 0.9999;
        Point3d atmStart = Point3d(0.0, 0.0, 0.0) +
            Vec3d(1.0, 0.0, 0.0) * (h + scene.planet.radius);

        for (unsigned int j = 0; j < ScatteringLUTViewAngleSteps; j++)
        {
            double cosAngle = unpackSNorm((double) j / (ScatteringLUTViewAngleSteps - 1));
            double sinAngle = sqrt(1.0 - min(1.0, cosAngle * cosAngle));
            Vec3d viewDir(cosAngle, sinAngle, 0.0);

            Ray3d viewRay(atmStart, viewDir);
            double dist = 0.0;
            if (!testIntersection(viewRay, shell, dist))
                dist = 0.0;

            Point3d atmEnd = viewRay.point(dist);

            for (unsigned int k = 0; k < ScatteringLUTLightAngleSteps; k++)
            {
                double cosLightAngle = unpackSNorm((double) k / (ScatteringLUTLightAngleSteps - 1));
                double sinLightAngle = sqrt(1.0 - min(1.0, cosLightAngle * cosLightAngle));
                Vec3d lightDir(cosLightAngle, sinLightAngle, 0.0);

#if 0                
                Vec4d inscatter = integrateInscatteringFactors_LUT(scene,
                                                                   atmStart,
                                                                   atmEnd,
                                                                   lightDir,
                                                                   true);
#else
                Vec4d inscatter = integrateInscatteringFactors(scene,
                                                               atmStart,
                                                               atmEnd,
                                                               lightDir);
#endif
                lut->setValue(i, j, k, inscatter);
            }
        }
    }

    return lut;
}


Vec3d
lookupScattering(const Scene& scene,
                 const Point3d& atmStart,
                 const Point3d& atmEnd,
                 const Vec3d& lightDir)
{
    Vec3d viewDir = atmEnd - atmStart;
    Vec3d toCenter = atmStart - Point3d(0.0, 0.0, 0.0);
    viewDir.normalize();
    toCenter.normalize();
    double h = (atmStart.distanceFromOrigin() - scene.planet.radius) /
        scene.atmosphereShellHeight;
    double cosViewAngle = viewDir * toCenter;
    double cosLightAngle = lightDir * toCenter;

    Vec4d v = scene.scatteringLUT->lookup(h,
                                          packSNorm(cosViewAngle),
                                          packSNorm(cosLightAngle));

    return Vec3d(v.x, v.y, v.z);
}


Color getPlanetColor(const Scene& scene, const Point3d& p)
{
    Vec3d n = p - Point3d(0.0, 0.0, 0.0);
    n.normalize();

    // Give the planet a checkerboard texture
    double phi = atan2(n.z, n.x);
    double theta = asin(n.y);
    int tx = (int) (8 + 8 * phi / PI);
    int ty = (int) (8 + 8 * theta / PI);

    return ((tx ^ ty) & 0x1) ? scene.planetColor : scene.planetColor2;
}


Color Scene::raytrace(const Ray3d& ray) const
{
    double dist = 0.0;
    double atmEnter = 0.0;
    double atmExit = 0.0;

    double shellRadius = planet.radius + atmosphereShellHeight;

    Color color = background;
    if (ray.direction * -light.direction > cos(sunAngularDiameter / 2.0))
        color = light.color;

    if (raySphereIntersect(ray,
                           Sphered(planet.center, shellRadius),
                           atmEnter,
                           atmExit))
    {
        Color baseColor = color;
        Point3d atmStart = ray.origin + atmEnter * ray.direction;
        Point3d atmEnd = ray.origin + atmExit * ray.direction;

        if (testIntersection(ray, planet, dist))
        {
            Point3d intersectPoint = ray.point(dist);
            Vec3d normal = intersectPoint - planet.center;
            normal.normalize();
            Vec3d lightDir = -light.direction;
            double diffuse = max(0.0, normal * lightDir);
            
            Point3d surfacePt = Point3d(0.0, 0.0, 0.0) + (intersectPoint - planet.center);
            Color planetColor = getPlanetColor(*this, surfacePt);

            Sphered shell = Sphered(shellRadius);

            // Compute ray from surface point to edge of the atmosphere in the direction
            // of the sun.
            Ray3d sunRay(surfacePt, lightDir);
            double sunDist = 0.0;
            testIntersection(sunRay, shell, sunDist);

            // Compute color of sunlight filtered by the atmosphere; consider extinction
            // along both the sun-to-surface and surface-to-eye paths.
            OpticalDepths sunDepth = integrateOpticalDepth(*this, surfacePt, sunRay.point(sunDist));
            OpticalDepths eyeDepth = integrateOpticalDepth(*this, atmStart, surfacePt);
            OpticalDepths totalDepth = sumOpticalDepths(sunDepth, eyeDepth);
            totalDepth.rayleigh *= 4.0 * PI;
            totalDepth.mie      *= 4.0 * PI;
            Vec3d extinction = atmosphere.computeExtinction(totalDepth);

            // Reflected color of planet surface is:
            //   surface color * sun color * atmospheric extinction
            baseColor = (planetColor * extinction) * light.color * diffuse;
            
            atmEnd = ray.origin + dist * ray.direction;
        }

        Vec3d inscatter = integrateInscattering(*this, atmStart, atmEnd) * 4.0 * PI;

        return Color((float) inscatter.x, (float) inscatter.y, (float) inscatter.z) +
            baseColor;
    }
    else
    {
        return color;
    }
}


Color
Scene::raytrace_LUT(const Ray3d& ray) const
{
    double atmEnter = 0.0;
    double atmExit = 0.0;

    double  shellRadius = planet.radius + atmosphereShellHeight;
    Sphered shell(shellRadius);
    Point3d eyePt = Point3d(0.0, 0.0, 0.0) + (ray.origin - planet.center);

    Color color = background;
    if (ray.direction * -light.direction > cos(sunAngularDiameter / 2.0))
        color = light.color;

    // Transform ray to model space
    Ray3d mray(eyePt, ray.direction);

    bool hit = raySphereIntersect2(mray, shell, atmEnter, atmExit);
    if (hit && atmExit > 0.0)
    {
        Color baseColor = color;

        bool eyeInsideAtmosphere = atmEnter < 0.0;
        Point3d atmStart = mray.origin + atmEnter * mray.direction;
        Point3d atmEnd = mray.origin + atmExit * mray.direction;

        double planetEnter = 0.0;
        double planetExit = 0.0;
        hit = raySphereIntersect2(mray,
                                 Sphered(planet.radius),
                                 planetEnter,
                                 planetExit);
        if (hit && planetEnter > 0.0)
        {
            Point3d surfacePt = mray.point(planetEnter);

            // Lambert lighting
            Vec3d normal = surfacePt - Point3d(0, 0, 0);
            normal.normalize();
            Vec3d lightDir = -light.direction;
            double diffuse = max(0.0, normal * lightDir);

            Color planetColor = getPlanetColor(*this, surfacePt);

            // Compute ray from surface point to edge of the atmosphere in the direction
            // of the sun.
            Ray3d sunRay(surfacePt, lightDir);
            double sunDist = 0.0;
            testIntersection(sunRay, shell, sunDist);
            
            // Compute color of sunlight filtered by the atmosphere; consider extinction
            // along both the sun-to-surface and surface-to-eye paths.
            Vec3d sunExt = lookupExtinction(*this, surfacePt, sunRay.point(sunDist));
            Vec3d eyeExt = lookupExtinction(*this, surfacePt, atmStart);
            if (eyeInsideAtmosphere)
            {
                Vec3d oppExt = lookupExtinction(*this, eyePt, atmStart);
                eyeExt = Vec3d(eyeExt.x / oppExt.x,
                               eyeExt.y / oppExt.y,
                               eyeExt.z / oppExt.z);
            }

            Vec3d extinction(sunExt.x * eyeExt.x,
                             sunExt.y * eyeExt.y,
                             sunExt.z * eyeExt.z);

            // Reflected color of planet surface is:
            //   surface color * sun color * atmospheric extinction
            baseColor = (planetColor * extinction) * light.color * diffuse;

            atmEnd = mray.point(planetEnter);
        }

        Vec3d inscatter;

        if (LUTUsage == UseExtinctionLUT)
        {
            bool hitPlanet = hit && planetEnter > 0.0;
            inscatter = integrateInscattering_LUT(*this, atmStart, atmEnd, eyePt, hitPlanet) * 4.0 * PI;
            //if (!hit)
            //inscatter = Vec3d(0.0, 0.0, 0.0);
        }
        else if (LUTUsage == UseScatteringLUT)
        {
            Vec3d rayleighScatter;
            if (eyeInsideAtmosphere)
            {
#if 1
                if (!hit || planetEnter < 0.0)
                {
                    rayleighScatter = 
                        lookupScattering(*this, eyePt, atmEnd, -light.direction);
                }
                else
                {
                    atmEnd = atmStart;
                    atmStart = mray.point(planetEnter);
                    //cout << atmEnter << ", " << planetEnter << ", " << atmExit << "\n";
                    rayleighScatter = 
                        lookupScattering(*this, atmStart, atmEnd, -light.direction) -
                        lookupScattering(*this, eyePt, atmEnd, -light.direction);

                    //cout << rayleighScatter.y << endl;
                    //cout << (atmStart - atmEnd).length() - (eyePt - atmEnd).length()
                    //<< ", " << rayleighScatter.y
                    //<< endl;
                    //rayleighScatter = Vec3d(0.0, 0.0, 0.0);
                }
#else
                //rayleighScatter = lookupScattering(*this, atmEnd, atmStart, -light.direction);
#endif
            }
            else
            {
                rayleighScatter = lookupScattering(*this, atmEnd, atmStart, -light.direction);
            }

            const Vec3d& rayleigh = atmosphere.rayleighCoeff;
            double cosSunAngle = mray.direction * -light.direction;
            inscatter = phaseRayleigh(cosSunAngle) *
                Vec3d(rayleighScatter.x * rayleigh.x,
                      rayleighScatter.y * rayleigh.y,
                      rayleighScatter.z * rayleigh.z);
            inscatter = inscatter * 4.0 * PI;
        }

        return Color((float) inscatter.x, (float) inscatter.y, (float) inscatter.z) +
            baseColor;
    }
    else
    {
        return color;
    }
}


void render(const Scene& scene,
            const Camera& camera,
            const Viewport& viewport,
            RGBImage& image)
{
    double aspectRatio = (double) image.width / (double) image.height;

    if (viewport.x >= image.width || viewport.y >= image.height)
        return;
    unsigned int right = min(image.width, viewport.x + viewport.width);
    unsigned int bottom = min(image.height, viewport.y + viewport.height);

    cout << "Rendering " << viewport.width << "x" << viewport.height << " view" << endl;
    for (unsigned int i = viewport.y; i < bottom; i++)
    {
        unsigned int row = i - viewport.y;
        if (row % 50 == 49)
            cout << row + 1 << endl;
        else if (row % 10 == 0)
            cout << ".";

        for (unsigned int j = viewport.x; j < right; j++)
        {
            double viewportX = ((double) (j - viewport.x) / (double) (viewport.width - 1) - 0.5) * aspectRatio;
            double viewportY ((double) (i - viewport.y) / (double) (viewport.height - 1) - 0.5);

            Ray3d viewRay = camera.getViewRay(viewportX, viewportY);

            Color color;
            if (LUTUsage != NoLUT)
                color = scene.raytrace_LUT(viewRay);
            else
                color = scene.raytrace(viewRay);

            if (CameraExposure != 0.0)
                color = color.exposure((float) CameraExposure);

            image.setPixel(j, i, color);
        }
    }
    cout << endl << "Complete" << endl;
}


Vec3d computeRayleighCoeffs(Vec3d wavelengths)
{
    return Vec3d(pow(wavelengths.x, -4.0),
                 pow(wavelengths.y, -4.0),
                 pow(wavelengths.z, -4.0));
}


void Scene::setParameters(ParameterSet& params)
{
    atmosphere.rayleighScaleHeight = params["RayleighScaleHeight"];
    atmosphere.rayleighCoeff.x     = params["RayleighRed"];
    atmosphere.rayleighCoeff.y     = params["RayleighGreen"];
    atmosphere.rayleighCoeff.z     = params["RayleighBlue"];

    atmosphere.mieScaleHeight      = params["MieScaleHeight"];
    atmosphere.mieCoeff            = params["Mie"];

    double phaseFunc = params["MiePhaseFunction"];
    if (phaseFunc == 0)
    {
        double mu                      = params["MieAsymmetry"];
	atmosphere.mieAsymmetry        = mu2g(mu);
        atmosphere.miePhaseFunction    = phaseHenyeyGreenstein_CS;
    }
    else if (phaseFunc == 1)
    {
        atmosphere.mieAsymmetry        = params["MieAsymmetry"];
        atmosphere.miePhaseFunction    = phaseHenyeyGreenstein;
    }
    else if (phaseFunc == 2)
    {
        double k                       = params["MieAsymmetry"];
        atmosphere.mieAsymmetry        = schlick_g2k(k);
        atmosphere.miePhaseFunction    = phaseSchlick;
    }
    
    atmosphere.absorbScaleHeight   = params["AbsorbScaleHeight"];
    atmosphere.absorbCoeff.x       = params["AbsorbRed"];
    atmosphere.absorbCoeff.y       = params["AbsorbGreen"];
    atmosphere.absorbCoeff.z       = params["AbsorbBlue"];

    atmosphereShellHeight          = atmosphere.calcShellHeight();

    sunAngularDiameter             = degToRad(params["SunAngularDiameter"]);

    planet.radius                  = params["Radius"];
    planet.center                  = Point3d(0.0, 0.0, 0.0);

    planetColor.r                  = (float) params["SurfaceRed"];
    planetColor.g                  = (float) params["SurfaceGreen"];
    planetColor.b                  = (float) params["SurfaceBlue"];

    planetColor2 = planetColor + Color(0.15f, 0.15f, 0.15f);
}


void setSceneDefaults(ParameterSet& params)
{
    params["RayleighScaleHeight"] = 79.94;
    params["RayleighRed"]         = 0.0;
    params["RayleighGreen"]       = 0.0;
    params["RayleighBlue"]        = 0.0;
    
    params["MieScaleHeight"]      = 1.2;
    params["Mie"]                 = 0.0;
    params["MieAsymmetry"]        = 0.0;

    params["AbsorbScaleHeight"]   = 7.994;
    params["AbsorbRed"]           = 0.0;
    params["AbsorbGreen"]         = 0.0;
    params["AbsorbBlue"]          = 0.0;

    params["Radius"]              = 6378.0;

    params["SurfaceRed"]          = 0.2;
    params["SurfaceGreen"]        = 0.3;
    params["SurfaceBlue"]         = 1.0;

    params["SunAngularDiameter"]  = 0.5; // degrees

    params["MiePhaseFunction"]       = 0;
}


bool LoadParameterSet(ParameterSet& params, const string& filename)
{
    ifstream in(filename.c_str());

    if (!in.good())
    {
        cerr << "Error opening config file " << filename << endl;
        return false;
    }

    while (in.good())
    {
        string name;

        in >> name;

        if (name == "MiePhaseFunction")
        {
            string strValue;
            in >> strValue;
            if (strValue == "HenyeyGreenstein_CS")
            {
                params[name] = 0;
            }
            else if (strValue == "HenyeyGreenstein")
            {
                params[name] = 1;
            }
            else if (strValue == "Schlick")
            {
                params[name] = 2;
            }
        }
        else
        {
            double numValue;
            in >> numValue;
            if (in.good())
            {
                params[name] = numValue;
            }
        }
    }

    if (in.bad())
    {
        cerr << "Error in scene config file " << filename << endl;
        return false;
    }
    else
    {
        return true;
    }
}



template<class T> static Matrix4<T> 
lookAt(Point3<T> from, Point3<T> to, Vector3<T> up)
{
    Vector3<T> n = to - from;
    n.normalize();
    Vector3<T> v = n ^ up;
    v.normalize();
    Vector3<T> u = v ^ n;

    Matrix4<T> rot(Vector4<T>(v.x, v.y, v.z, 0.0),
                   Vector4<T>(u.x, u.y, u.z, 0.0),
                   -Vector4<T>(n.x, n.y, n.z, 0.0),
                   Vector4<T>(0.0, 0.0, 0.0, 1.0));

    return Matrix4<T>::translation(from) * rot;
}


Camera lookAt(Point3d cameraPos,
              Point3d targetPos,
              Vec3d up,
              double fov)
{
    Camera camera;
    camera.fov = degToRad(fov);
    camera.front = 1.0;
    camera.transform = lookAt(cameraPos, targetPos, up);

    return camera;
}



string configFilename;
string outputImageName("out.png");

bool parseCommandLine(int argc, char* argv[])
{
    int i = 1;
    int fileCount = 0;

    while (i < argc)
    {
        if (argv[i][0] == '-')
        {
            if (!strcmp(argv[i], "-l") || !strcmp(argv[i], "--lut"))
            {
                LUTUsage = UseExtinctionLUT;
            }
            else if (!strcmp(argv[i], "-L") || !strcmp(argv[i], "--LUT"))
            {
                LUTUsage = UseScatteringLUT;
            }
            else if (!strcmp(argv[i], "-f") || !strcmp(argv[i], "--fisheye"))
            {
                UseFisheyeCameras = true;
            }
            else if (!strcmp(argv[i], "-e") || !strcmp(argv[i], "--exposure"))
            {
                if (i == argc - 1)
                {
                    return false;
                }
                else
                {
                    if (sscanf(argv[i + 1], " %lf", &CameraExposure) != 1)
                        return false;
                    i++;
                }
            }
            else if (!strcmp(argv[i], "-s") || !strcmp(argv[i], "--scattersteps"))
            {
                if (i == argc - 1)
                {
                    return false;
                }
                else
                {
                    if (sscanf(argv[i + 1], " %u", &IntegrateScatterSteps) != 1)
                        return false;
                    i++;
                }
            }
            else if (!strcmp(argv[i], "-d") || !strcmp(argv[i], "--depthsteps"))
            {
                if (i == argc - 1)
                {
                    return false;
                }
                else
                {
                    if (sscanf(argv[i + 1], " %u", &IntegrateDepthSteps) != 1)
                        return false;
                    i++;
                }
            }
            else if (!strcmp(argv[i], "-w") || !strcmp(argv[i], "--width"))
            {
                if (i == argc - 1)
                {
                    return false;
                }
                else
                {
                    if (sscanf(argv[i + 1], " %u", &OutputImageWidth) != 1)
                        return false;
                    i++;
                }
            }
            else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--height"))
            {
                if (i == argc - 1)
                {
                    return false;
                }
                else
                {
                    if (sscanf(argv[i + 1], " %u", &OutputImageHeight) != 1)
                        return false;
                    i++;
                }
            }
            else if (!strcmp(argv[i], "-i") || !strcmp(argv[i], "--image"))
            {
                if (i == argc - 1)
                {
                    return false;
                }
                else
                {
                    outputImageName = string(argv[i + 1]);
                    i++;
                }
            }
            else
            {
                return false;
            }
            i++;
        }
        else
        {
            if (fileCount == 0)
            {
                // input filename first
                configFilename = string(argv[i]);
                fileCount++;
            }
            else
            {
                // more than one filenames on the command line is an error
                return false;
            }
            i++;
        }
    }

    return true;
}


int main(int argc, char* argv[])
{
    bool commandLineOK = parseCommandLine(argc, argv);
    if (!commandLineOK || configFilename.empty())
    {
        usage();
        exit(1);
    }

    ParameterSet sceneParams;
    setSceneDefaults(sceneParams);
    if (!LoadParameterSet(sceneParams, configFilename))
    {
        exit(1);
    }

    Scene scene;
    scene.light.color = Color(1, 1, 1);
    scene.light.direction = Vec3d(0.0, 0.0, 1.0);
    scene.light.direction.normalize();

#if 0
    // N0 = 2.668e25  // m^-3
    scene.planet = Sphered(Point3d(0.0, 0.0, 0.0), planetRadius);

    scene.atmosphere.rayleighScaleHeight = 79.94;
    scene.atmosphere.mieScaleHeight = 1.2;
    scene.atmosphere.rayleighCoeff =
        computeRayleighCoeffs(Vec3d(0.600, 0.520, 0.450)) * 0.000008;
    scene.atmosphereShellHeight = scene.atmosphere.calcShellHeight();

    scene.atmosphere.rayleighCoeff =
        computeRayleighCoeffs(Vec3d(0.600, 0.520, 0.450)) * 0.000008;
#endif

    scene.setParameters(sceneParams);

    cout << "atmosphere height: " << scene.atmosphereShellHeight << "\n";
    cout << "attenuation coeffs: " <<
        scene.atmosphere.rayleighCoeff.x * 4 * PI << ", " <<
        scene.atmosphere.rayleighCoeff.y * 4 * PI << ", " <<
        scene.atmosphere.rayleighCoeff.z * 4 * PI << endl;


    if (LUTUsage != NoLUT)
    {
        cout << "Building extinction LUT...\n";
        scene.extinctionLUT = buildExtinctionLUT(scene);
        cout << "Complete!\n";
        DumpLUT(*scene.extinctionLUT, "extlut.png");
    }

    if (LUTUsage == UseScatteringLUT)
    {
        cout << "Building scattering LUT...\n";
        scene.scatteringLUT = buildScatteringLUT(scene);
        cout << "Complete!\n";
        DumpLUT(*scene.scatteringLUT, "lut.png");
    }

    double planetRadius = scene.planet.radius;
    double cameraFarDist = planetRadius * 3;
    double cameraCloseDist = planetRadius * 1.2;

    Camera cameraLowPhase;
    cameraLowPhase.fov = degToRad(45.0);
    cameraLowPhase.front = 1.0;
    cameraLowPhase.transform =
        Mat4d::translation(Point3d(0.0, 0.0, -cameraFarDist)) *
        Mat4d::yrotation(degToRad(20.0));

    Camera cameraHighPhase;
    cameraHighPhase.fov = degToRad(45.0);
    cameraHighPhase.front = 1.0;
    cameraHighPhase.transform = 
        Mat4d::translation(Point3d(0.0, 0.0, -cameraFarDist)) *
        Mat4d::yrotation(degToRad(160.0));

    Camera cameraClose;
    cameraClose.fov = degToRad(45.0);
    cameraClose.front = 1.0;
    cameraClose.transform = 
        Mat4d::xrotation(degToRad(55.0)) *
        Mat4d::translation(Point3d(0.0, 0.0, -cameraCloseDist)) *
        Mat4d::yrotation(degToRad(50.0));

    Camera cameraSurface;
    cameraSurface.fov = degToRad(45.0);
    cameraSurface.front = 1.0;
    cameraSurface.transform = 
        Mat4d::xrotation(degToRad(85.0)) *
        Mat4d::translation(Point3d(0.0, 0.0, -planetRadius * 1.0002)) *
        Mat4d::yrotation(degToRad(20.0));

    double aspectRatio = (double) OutputImageWidth / (double) OutputImageHeight;
    // Make the horizontal FOV of the fisheye cameras 180 degrees
    double fisheyeFOV = degToRad(max(180.0, 180.0 / aspectRatio));

    Camera cameraFisheyeMidday;
    cameraFisheyeMidday.fov = fisheyeFOV;
    cameraFisheyeMidday.type = Camera::Spherical;
    cameraFisheyeMidday.transform = 
        Mat4d::xrotation(degToRad(85.0)) *
        Mat4d::translation(Point3d(0.0, 0.0, -planetRadius * 1.0002)) *
        Mat4d::yrotation(degToRad(20.0));

    Camera cameraFisheyeSunset;
    cameraFisheyeSunset.fov = degToRad(180.0);
    cameraFisheyeSunset.type = Camera::Spherical;
    cameraFisheyeSunset.transform = 
        Mat4d::zrotation(degToRad(-5.0)) *
        Mat4d::yrotation(degToRad(90.0)) *
        Mat4d::xrotation(degToRad(85.0)) *
        Mat4d::translation(Point3d(0.0, 0.0, -planetRadius * 1.0002)) *
        Mat4d::yrotation(degToRad(80.0));


    RGBImage image(OutputImageWidth, OutputImageHeight);

    Viewport topleft (0, 0, image.width / 2, image.height / 2);
    Viewport topright(image.width / 2, 0, image.width / 2, image.height / 2);
    Viewport botleft (0, image.height / 2, image.width / 2, image.height / 2);
    Viewport botright(image.width / 2, image.height / 2, image.width / 2, image.height / 2);
    Viewport tophalf (0, 0, image.width, image.height / 2);
    Viewport bothalf (0, image.height / 2, image.width, image.height / 2);

    image.clear(Color(0.1f, 0.1f, 1.0f));

    if (UseFisheyeCameras)
    {
        render(scene, cameraFisheyeMidday, tophalf, image);
        render(scene, cameraFisheyeSunset, bothalf, image);
    }
    else
    {
        render(scene, cameraLowPhase, topleft, image);
        render(scene, cameraHighPhase, topright, image);
        render(scene, cameraClose, botleft, image);
        render(scene, cameraSurface, botright, image);
    }

    WritePNG(outputImageName, image);

    exit(0);

}
