// scattersim.cpp
//
// Copyright (C) 2007, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//

#include <algorithm>
#include <cmath>
#include <concepts>
#include <csetjmp>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <memory>
#include <numbers>
#include <optional>
#include <string_view>

#include <Eigen/Core>
#include <Eigen/Geometry>
#include <png.h>
#include <zlib.h>

#include <celmath/geomutil.h>
#include <celmath/intersect.h>
#include <celmath/mathlib.h>
#include <celmath/ray.h>
#include <celmath/sphere.h>

using namespace std::string_view_literals;

namespace math = celestia::math;

namespace
{

// Extinction lookup table dimensions
constexpr const unsigned int ExtinctionLUTHeightSteps = 256;
constexpr const unsigned int ExtinctionLUTViewAngleSteps = 512;

// Scattering lookup table dimensions
constexpr const unsigned int ScatteringLUTHeightSteps = 64;
constexpr const unsigned int ScatteringLUTViewAngleSteps = 64;
constexpr const unsigned int ScatteringLUTLightAngleSteps = 64;

enum class LUTUsageType
{
    NoLUT,
    UseExtinctionLUT,
    UseScatteringLUT
};

// Values settable via the command line
struct Options
{
    unsigned int IntegrateScatterSteps{ 20 };
    unsigned int IntegrateDepthSteps{ 20 };
    unsigned int OutputImageWidth{ 600 };
    unsigned int OutputImageHeight{ 450 };
    LUTUsageType LUTUsage{ LUTUsageType::NoLUT };
    bool UseFisheyeCameras{ false };
    double CameraExposure{ 0.0 };
    std::filesystem::path configFilename;
    std::filesystem::path outputImageName{ "out.png" };
};

struct ParameterSet
{
    double rayleighScaleHeight{ 79.94 };
    double rayleighRed{ 0.0 };
    double rayleighGreen{ 0.0 };
    double rayleighBlue{ 0.0 };

    double mieScaleHeight{ 1.2 };
    double mie{ 0.0 };
    double mieAsymmetry{ 0.0 };

    double absorbScaleHeight{ 7.994 };
    double absorbRed{ 0.0 };
    double absorbGreen{ 0.0 };
    double absorbBlue{ 0.0 };

    double radius{ 6378.0 };
    double sunAngularDiameter{ 0.5 }; // degrees

    float surfaceRed{ 0.2f };
    float surfaceGreen{ 0.3f };
    float surfaceBlue{ 0.5f };

    int miePhaseFunction{ 0 };
};

struct Color
{
    Color() = default;
    Color(float _r, float _g, float _b) : r(_r), g(_g), b(_b) {}

    Color exposure(float e) const
    {
#if 0
        float brightness = std::max(r, std::max(g, b));
        float f = 1.0f - static_cast<float>(std::exp(static_cast<double>(-e) * brightness));
        return Color(r * f, g * f, b * f);
#endif
        auto minusE = static_cast<double>(-e);
        return {static_cast<float>(1.0 - std::exp(minusE * r)),
                static_cast<float>(1.0 - std::exp(minusE * g)),
                static_cast<float>(1.0 - std::exp(minusE * b))};
    }

    float r{0.0f};
    float g{0.0f};
    float b{0.0f};
};

Color
operator*(const Color& color, double d)
{
    return {static_cast<float>(color.r * d),
            static_cast<float>(color.g * d),
            static_cast<float>(color.b * d)};
}

Color
operator+(const Color& a, const Color& b)
{
    return {a.r + b.r, a.g + b.g, a.b + b.b};
}

Color
operator*(const Color& a, const Color& b)
{
    return {a.r * b.r, a.g * b.g, a.b * b.b};
}

Color
operator*(const Color& a, const Eigen::Vector3d& v)
{
    return Color(static_cast<float>(a.r * v.x()),
                 static_cast<float>(a.g * v.y()),
                 static_cast<float>(a.b * v.z()));
}

constexpr std::uint8_t
floatToByte(float f)
{
    if (f <= 0.0f)
        return 0;
    if (f >= 1.0f)
        return 255;
    else
        return static_cast<std::uint8_t>(f * 255.99f);
}

enum class CameraType
{
    Planar,
    Spherical,
};

struct Camera
{
    Camera() = default;

    Eigen::ParametrizedLine<double, 3>
    getViewRay(double viewportX, double viewportY) const
    {
        Eigen::Vector3d viewDir;

        if (type == CameraType::Planar)
        {
            double viewPlaneHeight = std::tan(fov / 2.0) * 2 * front;
            viewDir.x() = viewportX * viewPlaneHeight;
            viewDir.y() = viewportY * viewPlaneHeight;
            viewDir.z() = front;
            viewDir.normalize();
        }
        else
        {
            double sphi, cphi;
            math::sincos(-viewportY * fov / 2.0 + std::numbers::pi / 2.0, sphi, cphi);
            double stheta, ctheta;
            math::sincos(viewportX * fov / 2.0 + std::numbers::pi / 2.0, stheta, ctheta);
            viewDir.x() = sphi * ctheta;
            viewDir.y() = cphi;
            viewDir.z() = sphi * stheta;
            viewDir.normalize();
        }

        Eigen::ParametrizedLine<double, 3> viewRay(Eigen::Vector3d::Zero(), viewDir);
        return math::transformRay(viewRay, transform);
    }

    double fov{ std::numbers::pi / 2.0 };
    double front{ 1.0 };
    Eigen::Matrix4d transform{ Eigen::Matrix4d::Identity() };
    CameraType type{ CameraType::Planar };
};

struct Viewport
{
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

struct Light
{
    Eigen::Vector3d direction;
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

using MiePhaseFunction = double (*)(double, double);

double
phaseHenyeyGreenstein_CS(double cosTheta, double g)
{
    // improved HG - phase function -> Rayleigh phase function for
    // g -> 0, -> HG-phase function for g -> 1.
    double g2 = g * g;
    return 1.5 * (1.0 - g2) * (1.0 + math::square(cosTheta)) /((2.0 + g2) * std::sqrt(math::cube(1.0 + g2 - 2.0 * g * cosTheta)));
}

double
phaseHenyeyGreenstein(double cosTheta, double g)
{
    return (1.0 - g * g) / std::sqrt(math::cube(1.0 + g * g - 2.0 * g * cosTheta));
}

// The Schlick phase function is a less computationally expensive than the
// Henyey-Greenstein function, but produces similar results. May be more
// appropriate for a GPU implementation.
constexpr double
phaseSchlick(double cosTheta, double k)
{
    return (1 - math::square(k)) / math::square(1 - k * cosTheta);
}

struct Atmosphere
{
    Atmosphere() :
        miePhaseFunction(&phaseHenyeyGreenstein_CS)
    {
    }

    double calcShellHeight()
    {
        double maxScaleHeight = std::max(rayleighScaleHeight, std::max(mieScaleHeight, absorbScaleHeight));
        return -std::log(0.002) * maxScaleHeight;
    }

    double miePhase(double cosAngle) const
    {
        return miePhaseFunction(cosAngle, mieAsymmetry);
    }

    double rayleighDensity(double h) const;
    double mieDensity(double h) const;
    double absorbDensity(double h) const;

    Eigen::Vector3d computeExtinction(const OpticalDepths&) const;

    double rayleighScaleHeight;
    double mieScaleHeight;
    double absorbScaleHeight;

    Eigen::Vector3d rayleighCoeff;
    Eigen::Vector3d absorbCoeff;
    double mieCoeff;

    double mieAsymmetry;

    MiePhaseFunction miePhaseFunction;
};

class LUT2
{
public:
    LUT2(unsigned int _width, unsigned int _height);

    Eigen::Vector3d getValue(unsigned int x, unsigned int y) const;
    void setValue(unsigned int x, unsigned int y, const Eigen::Vector3d&);
    Eigen::Vector3d lookup(double x, double y) const;
    unsigned int getWidth() const { return width; }
    unsigned int getHeight() const { return height; }

private:
    unsigned int width;
    unsigned int height;
    std::unique_ptr<float[]> values;
};

class LUT3
{
public:
    LUT3(unsigned int _width, unsigned int _height, unsigned int _depth);

    Eigen::Vector4d getValue(unsigned int x, unsigned int y, unsigned int z) const;
    void setValue(unsigned int x, unsigned int y, unsigned int z, const Eigen::Vector4d&);
    Eigen::Vector4d lookup(double x, double y, double z) const;
    unsigned int getWidth() const noexcept { return width; }
    unsigned int getHeight() const noexcept { return height; }
    unsigned int getDepth() const noexcept { return depth; }

private:
    unsigned int width;
    unsigned int height;
    unsigned int depth;
    std::unique_ptr<float[]> values;
};

class Scene
{
public:
    explicit Scene(const Options& _options) : options(_options) {}

    void setParameters(const ParameterSet& params);

    Color raytrace(const Eigen::ParametrizedLine<double, 3>& ray) const;
    Color raytrace_LUT(const Eigen::ParametrizedLine<double, 3>& ray) const;

    Color background;
    Light light;
    math::Sphered planet;
    Color planetColor;
    Color planetColor2;
    Atmosphere atmosphere;

    double atmosphereShellHeight{ 0.0f };

    double sunAngularDiameter{ 0.0f };

    std::unique_ptr<LUT2> extinctionLUT;
    std::unique_ptr<LUT3> scatteringLUT;

    const Options& options;
};

class RGBImage
{
public:
    RGBImage(int w, int h) :
        width(w),
        height(h),
        pixels(std::make_unique<std::uint8_t[]>(w * h * 3))
    {
    }

    void clearRect(const Color& color,
                   unsigned int x,
                   unsigned int y,
                   unsigned int w,
                   unsigned int h)
    {
        std::uint8_t r = floatToByte(color.r);
        std::uint8_t g = floatToByte(color.g);
        std::uint8_t b = floatToByte(color.b);
        for (unsigned int i = y; i < y + h; ++i)
        {
            for (unsigned int j = x; j < x + w; ++j)
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
        if (x < width && y < height)
        {
            unsigned int pix = (x + y * width) * 3;
            pixels[pix + 0] = floatToByte(color.r);
            pixels[pix + 1] = floatToByte(color.g);
            pixels[pix + 2] = floatToByte(color.b);
        }
    }

    unsigned int width;
    unsigned int height;
    std::unique_ptr<std::uint8_t[]> pixels;
};

void
usage()
{
    std::cerr << "Usage: scattersim [options] <config file>\n"
                 "   --lut (or -l)              : accelerate calculation by using a lookup table\n"
                 "   --fisheye (or -f)          : use wide angle cameras on surface\n"
                 "   --exposure <value> (or -e) : set exposure for HDR\n"
                 "   --width <value> (or -w)    : set width of output image\n"
                 "   --height <value> (or -h)   : set height of output image\n"
                 "   --image <filename> (or -i) : set filename of output image\n"
                 "           (default is out.png)\n"
                 "   --depthsteps <value> (or -d)\n"
                 "           set the number of integration steps for depth\n"
                 "   --scattersteps <value> (or -s)\n"
                 "           set the number of integration steps for scattering\n";
}

void
PNGWriteData(png_structp png_ptr, png_bytep data, png_size_t length)
{
    auto fp = static_cast<std::FILE*>(png_get_io_ptr(png_ptr));
    std::fwrite(data, 1, length, fp);
}

bool
WritePNG(const std::filesystem::path& filename, const RGBImage& image)
{
    int rowStride = image.width * 3;

    std::FILE* out;
#ifdef _WIN32
    out = _wfopen(filename.c_str(), L"wb");
#else
    out = std::fopen(filename.c_str(), "wb");
#endif
    if (out == nullptr)
    {
        std::cerr << "Can't open screen capture file " << filename << "\n";
        return false;
    }

    auto* row_pointers = new png_bytep[image.height];
    for (unsigned int i = 0; i < image.height; i++)
        row_pointers[i] = reinterpret_cast<png_bytep>(image.pixels.get() + (rowStride * (image.height - i - 1)));

    png_structp png_ptr;
    png_infop info_ptr;

    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
                                      nullptr, nullptr, nullptr);

    if (png_ptr == nullptr)
    {
        std::cerr << "Screen capture: error allocating png_ptr\n";
        std::fclose(out);
        delete[] row_pointers;
        return false;
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == nullptr)
    {
        std::cerr << "Screen capture: error allocating info_ptr\n";
        std::fclose(out);
        delete[] row_pointers;
        png_destroy_write_struct(&png_ptr, nullptr);
        return false;
    }

    if (setjmp(png_jmpbuf(png_ptr)))
    {
        std::cerr << "Error writing PNG file " << filename << '\n';
        std::fclose(out);
        delete[] row_pointers;
        png_destroy_write_struct(&png_ptr, &info_ptr);
        return false;
    }

    // png_init_io(png_ptr, out);
    png_set_write_fn(png_ptr, out, PNGWriteData, nullptr);

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
    std::fclose(out);

    return true;
}

constexpr float
mapColor(double c)
{
    //return (float) ((5.0 + log(max(0.0, c))) * 1.0);
    return static_cast<float>(c);
}

void
DumpLUT(const LUT3& lut, const std::filesystem::path& filename)
{
    unsigned int xtiles = 8;
    unsigned int ytiles = lut.getDepth() / xtiles;
    unsigned int tileWidth = lut.getHeight();
    unsigned int tileHeight = lut.getWidth();
    //double scale = 1.0 / 1.0;

    RGBImage img(xtiles * tileWidth, xtiles * tileHeight);

    for (unsigned int i = 0; i < ytiles; ++i)
    {
        for (unsigned int j = 0; j < xtiles; ++j)
        {
            unsigned int z = j + xtiles * i;
            for (unsigned int k = 0; k < tileWidth; ++k)
            {
                unsigned int y = k;
                for (unsigned int l = 0; l < tileHeight; ++l)
                {
                    unsigned int x = l;
                    Eigen::Vector4d v = lut.getValue(x, y, z);
                    Color c(mapColor(v.x() * 0.000617f * 4.0 * std::numbers::pi),
                            mapColor(v.y() * 0.00109f * 4.0 * std::numbers::pi),
                            mapColor(v.z() * 0.00195f * 4.0 * std::numbers::pi));
                    img.setPixel(j * tileWidth + k, i * tileHeight + l, c);
                }
            }
        }
    }

    for (unsigned int blah = 0; blah < img.width; blah++)
        img.setPixel(blah, 0, Color(1.0f, 0.0f, 0.0f));

    WritePNG(filename, img);
}

void
DumpLUT(const LUT2& lut, const std::filesystem::path& filename)
{
    RGBImage img(lut.getHeight(), lut.getWidth());

    for (unsigned int i = 0; i < lut.getWidth(); ++i)
    {
        for (unsigned int j = 0; j < lut.getHeight(); ++j)
        {
            Eigen::Vector3d v = lut.getValue(i, j);
            Color c(mapColor(v.x()), mapColor(v.y()), mapColor(v.z()));
            img.setPixel(j, i, c);
        }
    }

    for (unsigned int blah = 0; blah < img.width; blah++)
        img.setPixel(blah, 0, Color(1.0f, 0.0f, 0.0f));

    WritePNG(filename, img);
}

template<typename T>
constexpr T
lerp(double t, const T& v0, const T& v1)
{
    return (1 - t) * v0 + t * v1;
}

template<class T>
constexpr T bilerp(double t, double u,
                   const T& v00, const T& v01,
                   const T& v10, const T& v11)
{
    return lerp(u, lerp(t, v00, v01), lerp(t, v10, v11));
}

template<class T>
constexpr T
trilerp(double t, double u, double v,
        const T& v000, const T& v001,
        const T& v010, const T& v011,
        const T& v100, const T& v101,
        const T& v110, const T& v111)
{
    return lerp(v,
                bilerp(t, u, v000, v001, v010, v011),
                bilerp(t, u, v100, v101, v110, v111));
}

LUT2::LUT2(unsigned int _width, unsigned int _height) :
    width(_width),
    height(_height),
    values(std::make_unique<float[]>(_width * _height * 3))
{
}

Eigen::Vector3d
LUT2::getValue(unsigned int x, unsigned int y) const
{
    unsigned int n = 3 * (x + y * width);

    return {values[n], values[n + 1], values[n + 2]};
}

void
LUT2::setValue(unsigned int x, unsigned int y, const Eigen::Vector3d& v)
{
    unsigned int n = 3 * (x + y * width);
    values[n]     = static_cast<float>(v.x());
    values[n + 1] = static_cast<float>(v.y());
    values[n + 2] = static_cast<float>(v.z());
}

Eigen::Vector3d
LUT2::lookup(double x, double y) const
{
    x = std::clamp(x, 0.0, 0.999999);
    y = std::clamp(y, 0.0, 0.999999);
    auto fx = static_cast<unsigned int>(x * (width - 1));
    auto fy = static_cast<unsigned int>(y * (height - 1));
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
    values(std::make_unique<float[]>(_width * _height * _depth * 4))
{
}

Eigen::Vector4d
LUT3::getValue(unsigned int x, unsigned int y, unsigned int z) const
{
    unsigned int n = 4 * (x + (y + z * height) * width);
    return {values[n], values[n + 1], values[n + 2], values[n + 3]};
}

void
LUT3::setValue(unsigned int x, unsigned int y, unsigned int z, const Eigen::Vector4d& v)
{
    unsigned int n = 4 * (x + (y + z * height) * width);

    values[n]     = static_cast<float>(v.x());
    values[n + 1] = static_cast<float>(v.y());
    values[n + 2] = static_cast<float>(v.z());
    values[n + 3] = static_cast<float>(v.w());
}

Eigen::Vector4d
LUT3::lookup(double x, double y, double z) const
{
    x = std::clamp(x, 0.0, 0.999999);
    y = std::clamp(y, 0.0, 0.999999);
    z = std::clamp(z, 0.0, 0.999999);
    auto fx = static_cast<unsigned int>(x * (width - 1));
    auto fy = static_cast<unsigned int>(y * (height - 1));
    auto fz = static_cast<unsigned int>(z * (depth - 1));
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

template<std::floating_point T>
bool
raySphereIntersect(const Eigen::ParametrizedLine<T, 3>& ray,
                   const math::Sphere<T>& sphere,
                   T& dist0,
                   T& dist1)
{
    using std::sqrt;

    Eigen::Matrix<T, 3, 1> diff = ray.origin() - sphere.center;
    T s = T{ 1 } / math::square(sphere.radius);
    T a = ray.direction().squaredNorm() * s;
    T b = ray.direction().dot(diff) * s;
    T c = diff.dot(diff) * s - T{ 1 };
    T disc = b * b - a * c;
    if (disc < 0.0)
        return false;

    disc = sqrt(disc);
    T sol0 = (-b + disc) / a;
    T sol1 = (-b - disc) / a;

    if (sol0 <= 0.0 && sol1 <= 0.0)
        return false;

    if (sol0 < sol1)
    {
        dist0 = std::max(0.0, sol0);
        dist1 = sol1;
        return true;
    }

    if (sol1 < sol0)
    {
        dist0 = std::max(0.0, sol1);
        dist1 = sol0;
        return true;
    }

    return false;
}

template<std::floating_point T>
bool
raySphereIntersect2(const Eigen::ParametrizedLine<T, 3>& ray,
                    const math::Sphere<T>& sphere,
                    T& dist0,
                    T& dist1)
{
    using std::sqrt;

    Eigen::Matrix<T, 3, 1> diff = ray.origin() - sphere.center;
    T s = T{ 1 } / math::square(sphere.radius);
    T a = ray.direction().squaredNorm() * s;
    T b = ray.direction().dot(diff) * s;
    T c = diff.dot(diff) * s - T{ 1.0 };
    T disc = b * b - a * c;
    if (disc < 0.0)
        return false;

    disc = sqrt(disc);
    T sol0 = (-b + disc) / a;
    T sol1 = (-b - disc) / a;

    if (sol0 < sol1)
    {
        dist0 = sol0;
        dist1 = sol1;
        return true;
    }

    if (sol0 > sol1)
    {
        dist0 = sol1;
        dist1 = sol0;
        return true;
    }

    // One solution to quadratic indicates grazing intersection; treat
    // as no intersection.
    return false;
}

constexpr double
phaseRayleigh(double cosTheta)
{
    return 0.75 * (1.0 + math::square(cosTheta));
}

double
mu2g(double mu)
{
    // mu = <cosTheta>
    // This routine invertes the simple relation of the Cornette-Shanks
    // improved Henyey-Greenstein(HG) phase function:
    // mu = <cosTheta>= 3*g *(g^2 + 4)/(5*(2+g^2))

    double mu2 = mu * mu;
    double x = 0.5555555556 * mu + 0.17146776 * mu * mu2 + std::sqrt(std::max(2.3703704 - 1.3374486 * mu2 + 0.57155921 * mu2 * mu2, 0.0));
    double y = std::cbrt(x);
    return 0.55555555556 * mu - (1.33333333333 - 0.30864198 * mu2) / y + y;
}

// Convert the asymmetry paramter for the Henyey-Greenstein function to
// the approximate equivalent for the Schlick phase function. From Blasi,
// Saec, and Schlick: 1993, "A rendering algorithm for discrete volume
// density objects"
constexpr double
schlick_g2k(double g)
{
    return 1.55 * g - 0.55 * g * g * g;
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
                                    const Eigen::Vector3d& atmStart,
                                    const Eigen::Vector3d& atmEnd)
{
    const unsigned int nSteps = scene.options.IntegrateDepthSteps;

    OpticalDepths depth;
    depth.rayleigh   = 0.0;
    depth.mie        = 0.0;
    depth.absorption = 0.0;

    Eigen::Vector3d dir = atmEnd - atmStart;
    double length = dir.norm();
    double stepDist = length / static_cast<double>(nSteps);
    if (length == 0.0)
        return depth;

    dir = dir * (1.0 / length);
    Eigen::Vector3d samplePoint = atmStart + (0.5 * stepDist * dir);

    for (unsigned int i = 0; i < nSteps; ++i)
    {
        double h = samplePoint.norm() - scene.planet.radius;

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
    return std::exp(std::min(1.0, -h / rayleighScaleHeight));
}

double
Atmosphere::mieDensity(double h) const
{
    return std::exp(std::min(1.0, -h / mieScaleHeight));
}

double
Atmosphere::absorbDensity(double h) const
{
    return std::exp(std::min(1.0, -h / absorbScaleHeight));
}

Eigen::Vector3d
Atmosphere::computeExtinction(const OpticalDepths& depth) const
{
    double x = std::exp(-depth.rayleigh   * rayleighCoeff.x() -
                        depth.mie        * mieCoeff -
                        depth.absorption * absorbCoeff.x());
    double y = std::exp(-depth.rayleigh   * rayleighCoeff.y() -
                        depth.mie        * mieCoeff -
                        depth.absorption * absorbCoeff.y());
    double z = std::exp(-depth.rayleigh   * rayleighCoeff.z() -
                        depth.mie        * mieCoeff -
                        depth.absorption * absorbCoeff.z());

    return {x, y, z};
}

struct InscatteringValues
{
    Eigen::Vector3d rayleighScatter{ Eigen::Vector3d::Zero() };
    Eigen::Vector3d mieScatter{ Eigen::Vector3d::Zero() };
};

InscatteringValues
performInscatteringIntegration(const Scene& scene,
                               const Eigen::Vector3d& atmStart,
                               const Eigen::Vector3d& atmEnd,
                               const Eigen::Vector3d& lightDir)
{
    const unsigned int nSteps = scene.options.IntegrateScatterSteps;

    Eigen::Vector3d dir = atmEnd - atmStart;
    Eigen::Vector3d origin = Eigen::Vector3d::Zero() + (atmStart - scene.planet.center);
    double stepDist = dir.norm() / static_cast<double>(nSteps);
    dir.normalize();

    // Start at the midpoint of the first interval
    Eigen::Vector3d samplePoint = origin + 0.5 * stepDist * dir;

    InscatteringValues result;

    math::Sphered shell(Eigen::Vector3d::Zero(), scene.planet.radius + scene.atmosphereShellHeight);

    for (unsigned int i = 0; i < nSteps; ++i)
    {
        Eigen::ParametrizedLine<double, 3> sunRay(samplePoint, lightDir);
        double sunDist = 0.0;
        testIntersection(sunRay, shell, sunDist);

        // Compute the optical depth along path from sample point to the sun
        OpticalDepths sunDepth = integrateOpticalDepth(scene, samplePoint, sunRay.pointAt(sunDist));
        // Compute the optical depth along the path from the sample point to the eye
        OpticalDepths eyeDepth = integrateOpticalDepth(scene, samplePoint, atmStart);

        // Sum the optical depths to get the depth on the complete path from sun
        // to sample point to eye.
        OpticalDepths totalDepth = sumOpticalDepths(sunDepth, eyeDepth);
        totalDepth.rayleigh *= 4.0 * std::numbers::pi;
        totalDepth.mie      *= 4.0 * std::numbers::pi;

        Eigen::Vector3d extinction = scene.atmosphere.computeExtinction(totalDepth);

        double h = samplePoint.norm() - scene.planet.radius;

        // Add the inscattered light from Rayleigh and Mie scattering particles
        result.rayleighScatter += scene.atmosphere.rayleighDensity(h) * stepDist * extinction;
        result.mieScatter +=      scene.atmosphere.mieDensity(h)      * stepDist * extinction;

        samplePoint += stepDist * dir;
    }

    return result;
}

Eigen::Vector3d
integrateInscattering(const Scene& scene,
                      const Eigen::Vector3d& atmStart,
                      const Eigen::Vector3d& atmEnd)
{
    Eigen::Vector3d lightDir = -scene.light.direction;
    auto scatteringResult = performInscatteringIntegration(scene, atmStart, atmEnd, lightDir);
    double cosSunAngle = lightDir.dot((atmEnd - atmStart).normalized());

    double miePhase = scene.atmosphere.miePhase(cosSunAngle);
    const Eigen::Vector3d& rayleigh = scene.atmosphere.rayleighCoeff;
    return phaseRayleigh(cosSunAngle) * scatteringResult.rayleighScatter.cwiseProduct(rayleigh) +
        miePhase * scatteringResult.mieScatter * scene.atmosphere.mieCoeff;
}

Eigen::Vector4d
integrateInscatteringFactors(const Scene& scene,
                             const Eigen::Vector3d& atmStart,
                             const Eigen::Vector3d& atmEnd,
                             const Eigen::Vector3d& lightDir)
{
    Eigen::Vector4d r = Eigen::Vector4d::Zero();
    r.head<3>() = performInscatteringIntegration(scene, atmStart, atmEnd, lightDir).rayleighScatter;
    return r;
}

/**** Lookup table acceleration of scattering ****/

// Pack a signed value in [-1, 1] into [0, 1]
constexpr double
packSNorm(double sn)
{
    return (sn + 1.0) * 0.5;
}

// Expand an unsigned value in [0, 1] into [-1, 1]
constexpr double
unpackSNorm(double un)
{
    return un * 2.0 - 1.0;
}

std::unique_ptr<LUT2>
buildExtinctionLUT(const Scene& scene)
{
    auto lut = std::make_unique<LUT2>(ExtinctionLUTHeightSteps,
                                      ExtinctionLUTViewAngleSteps);

    //Sphered planet = Sphered(scene.planet.radius);
    math::Sphered shell(scene.planet.radius + scene.atmosphereShellHeight);

    for (unsigned int i = 0; i < ExtinctionLUTHeightSteps; ++i)
    {
        double h = static_cast<double>(i) / static_cast<double>(ExtinctionLUTHeightSteps - 1) *
            scene.atmosphereShellHeight * 0.9999;
        Eigen::Vector3d atmStart = Eigen::Vector3d::Zero() +
            Eigen::Vector3d::UnitX() * (h + scene.planet.radius);

        for (unsigned int j = 0; j < ExtinctionLUTViewAngleSteps; ++j)
        {
            double cosAngle = static_cast<double>(j) / (ExtinctionLUTViewAngleSteps - 1) * 2.0 - 1.0;
            double sinAngle = std::sqrt(1.0 - std::min(1.0, cosAngle * cosAngle));
            Eigen::Vector3d viewDir(cosAngle, sinAngle, 0.0);

            Eigen::ParametrizedLine<double, 3> ray(atmStart, viewDir);
            double dist = 0.0;

            if (!testIntersection(ray, shell, dist))
                dist = 0.0;

            OpticalDepths depth = integrateOpticalDepth(scene, atmStart,
                                                        ray.pointAt(dist));
            depth.rayleigh *= 4.0 * std::numbers::pi;
            depth.mie      *= 4.0 * std::numbers::pi;
            Eigen::Vector3d ext = scene.atmosphere.computeExtinction(depth);

            lut->setValue(i, j, ext.cwiseMax(1.0e-18));
        }
    }

    return lut;
}

Eigen::Vector3d
lookupExtinction(const Scene& scene,
                 const Eigen::Vector3d& atmStart,
                 const Eigen::Vector3d& atmEnd)
{
    Eigen::Vector3d viewDir = atmEnd - atmStart;
    Eigen::Vector3d toCenter = atmStart - Eigen::Vector3d::Zero();
    viewDir.normalize();
    toCenter.normalize();
    double h = (atmStart.norm() - scene.planet.radius) /
        scene.atmosphereShellHeight;
    double cosViewAngle = viewDir.dot(toCenter);

    return scene.extinctionLUT->lookup(h, packSNorm(cosViewAngle));
}

std::unique_ptr<LUT2>
buildOpticalDepthLUT(const Scene& scene)
{
    auto lut = std::make_unique<LUT2>(ExtinctionLUTHeightSteps,
                                      ExtinctionLUTViewAngleSteps);

    //Sphered planet = Sphered(scene.planet.radius);
    math::Sphered shell(scene.planet.radius + scene.atmosphereShellHeight);

    for (unsigned int i = 0; i < ExtinctionLUTHeightSteps; ++i)
    {
        double h = static_cast<double>(i) / static_cast<double>(ExtinctionLUTHeightSteps - 1) *
            scene.atmosphereShellHeight;
        Eigen::Vector3d atmStart = Eigen::Vector3d::Zero() +
            Eigen::Vector3d::UnitX() * (h + scene.planet.radius);

        for (unsigned int j = 0; j < ExtinctionLUTViewAngleSteps; ++j)
        {
            double cosAngle = static_cast<double>(j) / (ExtinctionLUTViewAngleSteps - 1) * 2.0 - 1.0;
            double sinAngle = std::sqrt(1.0 - std::min(1.0, cosAngle * cosAngle));
            Eigen::Vector3d dir(cosAngle, sinAngle, 0.0);

            Eigen::ParametrizedLine<double, 3> ray(atmStart, dir);
            double dist = 0.0;

            if (!testIntersection(ray, shell, dist))
                dist = 0.0;

            OpticalDepths depth = integrateOpticalDepth(scene, atmStart,
                                                        ray.pointAt(dist));
            depth.rayleigh *= 4.0 * std::numbers::pi;
            depth.mie      *= 4.0 * std::numbers::pi;

            lut->setValue(i, j, Eigen::Vector3d(depth.rayleigh, depth.mie, depth.absorption));
        }
    }

    return lut;
}

OpticalDepths
lookupOpticalDepth(const Scene& scene,
                   const Eigen::Vector3d& atmStart,
                   const Eigen::Vector3d& atmEnd)
{
    Eigen::Vector3d dir = atmEnd - atmStart;
    Eigen::Vector3d toCenter = atmStart - Eigen::Vector3d::Zero();
    dir.normalize();
    toCenter.normalize();
    double h = (atmStart.norm() - scene.planet.radius) /
        scene.atmosphereShellHeight;
    double cosViewAngle = dir.dot(toCenter);

    Eigen::Vector3d v = scene.extinctionLUT->lookup(h, (cosViewAngle + 1.0) * 0.5);
    OpticalDepths depth;
    depth.rayleigh = v.x();
    depth.mie = v.y();
    depth.absorption = v.z();

    return depth;
}

Eigen::Vector3d
integrateInscattering_LUT(const Scene& scene,
                          const Eigen::Vector3d& atmStart,
                          const Eigen::Vector3d& atmEnd,
                          const Eigen::Vector3d& eyePt,
                          bool hitPlanet)
{
    const unsigned int nSteps = scene.options.IntegrateScatterSteps;

    double shellHeight = scene.planet.radius + scene.atmosphereShellHeight;
    math::Sphered shell(shellHeight);
    bool eyeInsideAtmosphere = eyePt.norm() < shellHeight;

    Eigen::Vector3d lightDir = -scene.light.direction;

    Eigen::Vector3d origin = eyeInsideAtmosphere ? eyePt : atmStart;
    Eigen::Vector3d viewDir = atmEnd - origin;
    double stepDist = viewDir.norm() / static_cast<double>(nSteps);
    viewDir.normalize();

    // Start at the midpoint of the first interval
    Eigen::Vector3d samplePoint = origin + 0.5 * stepDist * viewDir;

    Eigen::Vector3d rayleighScatter = Eigen::Vector3d::Zero();
    Eigen::Vector3d mieScatter = Eigen::Vector3d::Zero();

    for (unsigned int i = 0; i < nSteps; ++i)
    {
        Eigen::ParametrizedLine<double, 3> sunRay(samplePoint, lightDir);
        double sunDist = 0.0;
        testIntersection(sunRay, shell, sunDist);

        Eigen::Vector3d sunExt = lookupExtinction(scene, samplePoint, sunRay.pointAt(sunDist));
        Eigen::Vector3d eyeExt;
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
            Eigen::Vector3d subExt;
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
            eyeExt = eyeExt.cwiseQuotient(subExt);
        }

        // Compute the extinction along the entire light path from sun to sample point
        // to eye.
        Eigen::Vector3d extinction = sunExt.cwiseProduct(eyeExt);

        double h = samplePoint.norm() - scene.planet.radius;

        // Add the inscattered light from Rayleigh and Mie scattering particles
        rayleighScatter += scene.atmosphere.rayleighDensity(h) * stepDist * extinction;
        mieScatter      += scene.atmosphere.mieDensity(h)      * stepDist * extinction;

        samplePoint += stepDist * viewDir;
    }

    double cosSunAngle = lightDir.dot(viewDir);

    double miePhase = scene.atmosphere.miePhase(cosSunAngle);
    const Eigen::Vector3d& rayleigh = scene.atmosphere.rayleighCoeff;
    return phaseRayleigh(cosSunAngle) * rayleighScatter.cwiseProduct(rayleigh) +
           miePhase * mieScatter * scene.atmosphere.mieCoeff;
}

// Used for building LUT; start point is assumed to be within
// atmosphere.
Eigen::Vector4d
integrateInscatteringFactors_LUT(const Scene& scene,
                                 const Eigen::Vector3d& atmStart,
                                 const Eigen::Vector3d& atmEnd,
                                 const Eigen::Vector3d& lightDir,
                                 bool planetHit)
{
    const unsigned int nSteps = scene.options.IntegrateScatterSteps;

    double shellHeight = scene.planet.radius + scene.atmosphereShellHeight;
    math::Sphered shell(shellHeight);

    Eigen::Vector3d origin = atmStart;
    Eigen::Vector3d viewDir = atmEnd - origin;
    double stepDist = viewDir.norm() / static_cast<double>(nSteps);
    viewDir.normalize();

    // Start at the midpoint of the first interval
    Eigen::Vector3d samplePoint = origin + 0.5 * stepDist * viewDir;

    Eigen::Vector3d rayleighScatter = Eigen::Vector3d::Zero();
    Eigen::Vector3d mieScatter = Eigen::Vector3d::Zero();

    for (unsigned int i = 0; i < nSteps; ++i)
    {
        Eigen::ParametrizedLine<double, 3> sunRay(samplePoint, lightDir);
        double sunDist = 0.0;
        testIntersection(sunRay, shell, sunDist);

        Eigen::Vector3d sunExt = lookupExtinction(scene, samplePoint, sunRay.pointAt(sunDist));
        Eigen::Vector3d eyeExt;
        Eigen::Vector3d subExt;
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
        eyeExt = eyeExt.cwiseQuotient(subExt);

        //Vector3d eyeExt = lookupExtinction(scene, samplePoint, atmStart);

        // Compute the extinction along the entire light path from sun to sample point
        // to eye.
        Eigen::Vector3d extinction = sunExt.cwiseProduct(eyeExt);

        double h = samplePoint.norm() - scene.planet.radius;

        // Add the inscattered light from Rayleigh and Mie scattering particles
        rayleighScatter += scene.atmosphere.rayleighDensity(h) * stepDist * extinction;
        mieScatter      += scene.atmosphere.mieDensity(h) * stepDist * extinction;

        samplePoint += stepDist * viewDir;
    }

    Eigen::Vector4d r = Eigen::Vector4d::Zero();
    r.head<3>() = rayleighScatter;
    return r;
}

std::unique_ptr<LUT3>
buildScatteringLUT(const Scene& scene)
{
    auto lut = std::make_unique<LUT3>(ScatteringLUTHeightSteps,
                                      ScatteringLUTViewAngleSteps,
                                      ScatteringLUTLightAngleSteps);

    math::Sphered shell(scene.planet.radius + scene.atmosphereShellHeight);

    for (unsigned int i = 0; i < ScatteringLUTHeightSteps; ++i)
    {
        double h = static_cast<double>(i) / static_cast<double>(ScatteringLUTHeightSteps - 1) *
            scene.atmosphereShellHeight * 0.9999;
        Eigen::Vector3d atmStart = Eigen::Vector3d::Zero() +
            Eigen::Vector3d::UnitX() * (h + scene.planet.radius);

        for (unsigned int j = 0; j < ScatteringLUTViewAngleSteps; ++j)
        {
            double cosAngle = unpackSNorm(static_cast<double>(j) / (ScatteringLUTViewAngleSteps - 1));
            double sinAngle = std::sqrt(1.0 - std::min(1.0, cosAngle * cosAngle));
            Eigen::Vector3d viewDir(cosAngle, sinAngle, 0.0);

            Eigen::ParametrizedLine<double, 3> viewRay(atmStart, viewDir);
            double dist = 0.0;
            if (!testIntersection(viewRay, shell, dist))
                dist = 0.0;

            Eigen::Vector3d atmEnd = viewRay.pointAt(dist);

            for (unsigned int k = 0; k < ScatteringLUTLightAngleSteps; ++k)
            {
                double cosLightAngle = unpackSNorm(static_cast<double>(k) / (ScatteringLUTLightAngleSteps - 1));
                double sinLightAngle = std::sqrt(1.0 - std::min(1.0, cosLightAngle * cosLightAngle));
                Eigen::Vector3d lightDir(cosLightAngle, sinLightAngle, 0.0);

#if 0
                Eigen::Vector4d inscatter = integrateInscatteringFactors_LUT(scene,
                                                                             atmStart,
                                                                             atmEnd,
                                                                             lightDir,
                                                                             true);
#else
                Eigen::Vector4d inscatter = integrateInscatteringFactors(scene,
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

Eigen::Vector3d
lookupScattering(const Scene& scene,
                 const Eigen::Vector3d& atmStart,
                 const Eigen::Vector3d& atmEnd,
                 const Eigen::Vector3d& lightDir)
{
    Eigen::Vector3d viewDir = atmEnd - atmStart;
    Eigen::Vector3d toCenter = atmStart - Eigen::Vector3d::Zero();
    viewDir.normalize();
    toCenter.normalize();
    double h = (atmStart.norm() - scene.planet.radius) /
        scene.atmosphereShellHeight;
    double cosViewAngle = viewDir.dot(toCenter);
    double cosLightAngle = lightDir.dot(toCenter);

    Eigen::Vector4d v = scene.scatteringLUT->lookup(h,
                                                    packSNorm(cosViewAngle),
                                                    packSNorm(cosLightAngle));

    return v.head<3>();
}

Color
getPlanetColor(const Scene& scene, const Eigen::Vector3d& p)
{
    Eigen::Vector3d n = p - Eigen::Vector3d::Zero();
    n.normalize();

    // Give the planet a checkerboard texture
    double phi = std::atan2(n.z(), n.x());
    double theta = std::asin(n.y());
    auto tx = static_cast<int>(8 + 8 * phi / std::numbers::pi);
    auto ty = static_cast<int>(8 + 8 * theta / std::numbers::pi);

    return ((tx ^ ty) & 0x1) != 0 ? scene.planetColor : scene.planetColor2;
}

Color
Scene::raytrace(const Eigen::ParametrizedLine<double, 3>& ray) const
{
    double dist = 0.0;
    double atmEnter = 0.0;
    double atmExit = 0.0;

    double shellRadius = planet.radius + atmosphereShellHeight;

    Color color = background;
    if (ray.direction().dot(-light.direction) > cos(sunAngularDiameter / 2.0))
        color = light.color;

    if (raySphereIntersect(ray,
                           math::Sphered(planet.center, shellRadius),
                           atmEnter,
                           atmExit))
    {
        Color baseColor = color;
        Eigen::Vector3d atmStart = ray.origin() + atmEnter * ray.direction();
        Eigen::Vector3d atmEnd = ray.origin() + atmExit * ray.direction();

        if (testIntersection(ray, planet, dist))
        {
            Eigen::Vector3d intersectPoint = ray.pointAt(dist);
            Eigen::Vector3d normal = intersectPoint - planet.center;
            normal.normalize();
            Eigen::Vector3d lightDir = -light.direction;
            double diffuse = std::max(0.0, normal.dot(lightDir));

            Eigen::Vector3d surfacePt = Eigen::Vector3d::Zero() + (intersectPoint - planet.center);
            Color planetColor = getPlanetColor(*this, surfacePt);

            math::Sphered shell(shellRadius);

            // Compute ray from surface point to edge of the atmosphere in the direction
            // of the sun.
            Eigen::ParametrizedLine<double, 3> sunRay(surfacePt, lightDir);
            double sunDist = 0.0;
            testIntersection(sunRay, shell, sunDist);

            // Compute color of sunlight filtered by the atmosphere; consider extinction
            // along both the sun-to-surface and surface-to-eye paths.
            OpticalDepths sunDepth = integrateOpticalDepth(*this, surfacePt, sunRay.pointAt(sunDist));
            OpticalDepths eyeDepth = integrateOpticalDepth(*this, atmStart, surfacePt);
            OpticalDepths totalDepth = sumOpticalDepths(sunDepth, eyeDepth);
            totalDepth.rayleigh *= 4.0 * std::numbers::pi;
            totalDepth.mie      *= 4.0 * std::numbers::pi;
            Eigen::Vector3d extinction = atmosphere.computeExtinction(totalDepth);

            // Reflected color of planet surface is:
            //   surface color * sun color * atmospheric extinction
            baseColor = (planetColor * extinction) * light.color * diffuse;

            atmEnd = ray.origin() + dist * ray.direction();
        }

        Eigen::Vector3d inscatter = integrateInscattering(*this, atmStart, atmEnd) * 4.0 * std::numbers::pi;

        return Color(static_cast<float>(inscatter.x()),
                     static_cast<float>(inscatter.y()),
                     static_cast<float>(inscatter.z())) + baseColor;
    }

    return color;
}

Color
Scene::raytrace_LUT(const Eigen::ParametrizedLine<double, 3>& ray) const
{
    double atmEnter = 0.0;
    double atmExit = 0.0;

    double  shellRadius = planet.radius + atmosphereShellHeight;
    math::Sphered shell(shellRadius);
    Eigen::Vector3d eyePt = Eigen::Vector3d::Zero() + (ray.origin() - planet.center);

    Color color = background;
    if (ray.direction().dot(-light.direction) > std::cos(sunAngularDiameter / 2.0))
        color = light.color;

    // Transform ray to model space
    Eigen::ParametrizedLine<double, 3> mray(eyePt, ray.direction());

    bool hit = raySphereIntersect2(mray, shell, atmEnter, atmExit);
    if (hit && atmExit > 0.0)
    {
        Color baseColor = color;

        bool eyeInsideAtmosphere = atmEnter < 0.0;
        Eigen::Vector3d atmStart = mray.origin() + atmEnter * mray.direction();
        Eigen::Vector3d atmEnd = mray.origin() + atmExit * mray.direction();

        double planetEnter = 0.0;
        double planetExit = 0.0;
        hit = raySphereIntersect2(mray,
                                  math::Sphered(planet.radius),
                                  planetEnter,
                                  planetExit);
        if (hit && planetEnter > 0.0)
        {
            Eigen::Vector3d surfacePt = mray.pointAt(planetEnter);

            // Lambert lighting
            Eigen::Vector3d normal = surfacePt - Eigen::Vector3d::Zero();
            normal.normalize();
            Eigen::Vector3d lightDir = -light.direction;
            double diffuse = std::max(0.0, normal.dot(lightDir));

            Color planetColor = getPlanetColor(*this, surfacePt);

            // Compute ray from surface point to edge of the atmosphere in the direction
            // of the sun.
            Eigen::ParametrizedLine<double, 3> sunRay(surfacePt, lightDir);
            double sunDist = 0.0;
            testIntersection(sunRay, shell, sunDist);

            // Compute color of sunlight filtered by the atmosphere; consider extinction
            // along both the sun-to-surface and surface-to-eye paths.
            Eigen::Vector3d sunExt = lookupExtinction(*this, surfacePt, sunRay.pointAt(sunDist));
            Eigen::Vector3d eyeExt = lookupExtinction(*this, surfacePt, atmStart);
            if (eyeInsideAtmosphere)
            {
                Eigen::Vector3d oppExt = lookupExtinction(*this, eyePt, atmStart);
                eyeExt = eyeExt.cwiseQuotient(oppExt);
            }

            Eigen::Vector3d extinction = sunExt.cwiseProduct(eyeExt);

            // Reflected color of planet surface is:
            //   surface color * sun color * atmospheric extinction
            baseColor = (planetColor * extinction) * light.color * diffuse;

            atmEnd = mray.pointAt(planetEnter);
        }

        Eigen::Vector3d inscatter;

        if (options.LUTUsage == LUTUsageType::UseExtinctionLUT)
        {
            bool hitPlanet = hit && planetEnter > 0.0;
            inscatter = integrateInscattering_LUT(*this, atmStart, atmEnd, eyePt, hitPlanet) * 4.0 * std::numbers::pi;
            //if (!hit)
            //inscatter = Vector3d::Zero();
        }
        else if (options.LUTUsage == LUTUsageType::UseScatteringLUT)
        {
            Eigen::Vector3d rayleighScatter;
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
                    atmStart = mray.pointAt(planetEnter);
                    //cout << atmEnter << ", " << planetEnter << ", " << atmExit << "\n";
                    rayleighScatter =
                        lookupScattering(*this, atmStart, atmEnd, -light.direction) -
                        lookupScattering(*this, eyePt, atmEnd, -light.direction);

                    //cout << rayleighScatter.y() << endl;
                    //cout << (atmStart - atmEnd).norm() - (eyePt - atmEnd).norm()
                    //<< ", " << rayleighScatter.y()
                    //<< endl;
                    //rayleighScatter = Vector3d::Zero();
                }
#else
                //rayleighScatter = lookupScattering(*this, atmEnd, atmStart, -light.direction);
#endif
            }
            else
            {
                rayleighScatter = lookupScattering(*this, atmEnd, atmStart, -light.direction);
            }

            const Eigen::Vector3d& rayleigh = atmosphere.rayleighCoeff;
            double cosSunAngle = mray.direction().dot(-light.direction);
            inscatter = phaseRayleigh(cosSunAngle) * rayleighScatter.cwiseProduct(rayleigh);
            inscatter = inscatter * 4.0 * std::numbers::pi;
        }
        else
        {
            // fix GCC warning about possibly uninitialized variable
            inscatter = Eigen::Vector3d::Zero();
        }

        return Color(static_cast<float>(inscatter.x()),
                     static_cast<float>(inscatter.y()),
                     static_cast<float>(inscatter.z())) + baseColor;
    }
    else
    {
        return color;
    }
}

void
render(const Scene& scene,
       const Camera& camera,
       const Viewport& viewport,
       RGBImage& image)
{
    double aspectRatio = static_cast<double>(image.width) / static_cast<double>(image.height);

    if (viewport.x >= image.width || viewport.y >= image.height)
        return;
    unsigned int right = std::min(image.width, viewport.x + viewport.width);
    unsigned int bottom = std::min(image.height, viewport.y + viewport.height);

    std::cout << "Rendering " << viewport.width << "x" << viewport.height << " view\n";
    for (unsigned int i = viewport.y; i < bottom; ++i)
    {
        unsigned int row = i - viewport.y;
        if (row % 50 == 49)
            std::cout << row + 1 << '\n';
        else if (row % 10 == 0)
            std::cout << '.';

        for (unsigned int j = viewport.x; j < right; ++j)
        {
            double viewportX = (static_cast<double>(j - viewport.x) / static_cast<double>(viewport.width - 1) - 0.5) * aspectRatio;
            double viewportY (static_cast<double>(i - viewport.y) / static_cast<double>(viewport.height - 1) - 0.5);

            Eigen::ParametrizedLine<double, 3> viewRay = camera.getViewRay(viewportX, viewportY);

            Color color;
            if (scene.options.LUTUsage != LUTUsageType::NoLUT)
                color = scene.raytrace_LUT(viewRay);
            else
                color = scene.raytrace(viewRay);

            if (scene.options.CameraExposure != 0.0)
                color = color.exposure(static_cast<float>(scene.options.CameraExposure));

            image.setPixel(j, i, color);
        }
    }
    std::cout << "\nComplete\n";
}

Eigen::Vector3d
computeRayleighCoeffs(Eigen::Vector3d wavelengths)
{
    return wavelengths.array().pow(-4.0);
}

void
Scene::setParameters(const ParameterSet& params)
{
    atmosphere.rayleighScaleHeight = params.rayleighScaleHeight;
    atmosphere.rayleighCoeff.x()   = params.rayleighRed;
    atmosphere.rayleighCoeff.y()   = params.rayleighGreen;
    atmosphere.rayleighCoeff.z()   = params.rayleighBlue;

    atmosphere.mieScaleHeight      = params.mieScaleHeight;
    atmosphere.mieCoeff            = params.mie;

    switch (params.miePhaseFunction)
    {
    case 0:
        atmosphere.mieAsymmetry        = mu2g(params.mieAsymmetry);
        atmosphere.miePhaseFunction    = &phaseHenyeyGreenstein_CS;
        break;

    case 1:
        atmosphere.mieAsymmetry        = params.mieAsymmetry;
        atmosphere.miePhaseFunction    = &phaseHenyeyGreenstein;
        break;

    case 2:
        atmosphere.mieAsymmetry        = schlick_g2k(params.mieAsymmetry);
        atmosphere.miePhaseFunction    = &phaseSchlick;
        break;
    }

    atmosphere.absorbScaleHeight   = params.absorbScaleHeight;
    atmosphere.absorbCoeff.x()     = params.absorbRed;
    atmosphere.absorbCoeff.y()     = params.absorbGreen;
    atmosphere.absorbCoeff.z()     = params.absorbBlue;

    atmosphereShellHeight          = atmosphere.calcShellHeight();

    sunAngularDiameter             = math::degToRad(params.sunAngularDiameter);

    planet.radius                  = params.radius;
    planet.center                  = Eigen::Vector3d::Zero();

    planetColor.r                  = params.surfaceRed;
    planetColor.g                  = params.surfaceGreen;
    planetColor.b                  = params.surfaceBlue;

    planetColor2 = planetColor + Color(0.15f, 0.15f, 0.15f);
}

template<typename T>
void
setParameter(std::istream& in, T& value)
{
    T loadValue;
    in >> loadValue;
    if (in.good())
        value = loadValue;
}

void
discardParameter(std::istream& in)
{
    double dummyParameter;
    in >> dummyParameter;
}

bool
LoadParameterSet(ParameterSet& params, const std::filesystem::path& filename)
{
    std::ifstream in(filename);

    if (!in.good())
    {
        std::cerr << "Error opening config file " << filename << '\n';
        return false;
    }

    std::string name;
    while (in.good())
    {
        name.clear();
        in >> name;

        if (name == "MiePhaseFunction"sv)
        {
            std::string strValue;
            in >> strValue;
            if (in.good())
            {
                if (strValue == "HenyeyGreenstein_CS"sv)
                {
                    params.miePhaseFunction = 0;
                }
                else if (strValue == "HenyeyGreenstein"sv)
                {
                    params.miePhaseFunction = 1;
                }
                else if (strValue == "Schlick")
                {
                    params.miePhaseFunction = 2;
                }
            }

            continue;
        }

        if (name == "RayleighScaleHeight"sv)
            setParameter(in, params.rayleighScaleHeight);
        else if (name == "RayleighRed"sv)
            setParameter(in, params.rayleighRed);
        else if (name == "RayleighGreen"sv)
            setParameter(in, params.rayleighGreen);
        else if (name == "RayleighBlue"sv)
            setParameter(in, params.rayleighBlue);
        else if (name == "MieScaleHeight"sv)
            setParameter(in, params.mieScaleHeight);
        else if (name == "Mie"sv)
            setParameter(in, params.mie);
        else if (name == "MieAsymmetry"sv)
            setParameter(in, params.mieAsymmetry);
        else if (name == "AbsorbScaleHeight"sv)
            setParameter(in, params.absorbScaleHeight);
        else if (name == "AbsorbRed"sv)
            setParameter(in, params.absorbRed);
        else if (name == "AbsorbGreen"sv)
            setParameter(in, params.absorbGreen);
        else if (name == "AbsorbBlue"sv)
            setParameter(in, params.absorbBlue);
        else if (name == "Radius"sv)
            setParameter(in, params.radius);
        else if (name == "SunAngularDiameter"sv)
            setParameter(in, params.sunAngularDiameter);
        else if (name == "SurfaceRed"sv)
            setParameter(in, params.surfaceRed);
        else if (name == "SurfaceGreen"sv)
            setParameter(in, params.surfaceGreen);
        else if (name == "SurfaceBlue"sv)
            setParameter(in, params.surfaceBlue);
        else
            discardParameter(in);
    }

    if (in.bad())
    {
        std::cerr << "Error in scene config file " << filename << '\n';
        return false;
    }
    else
    {
        return true;
    }
}

Camera
lookAt(const Eigen::Vector3d& cameraPos,
       const Eigen::Vector3d& targetPos,
       const Eigen::Vector3d& up,
       double fov)
{
    Camera camera;
    camera.fov = math::degToRad(fov);
    camera.front = 1.0;
    Eigen::Matrix4d m(Eigen::Matrix4d::Identity());
    m.topLeftCorner<3, 3>() = math::LookAt(cameraPos, targetPos, up).toRotationMatrix();
    camera.transform = m;

    return camera;
}

std::optional<Options>
parseCommandLine(int argc, char* argv[])
{
    int i = 1;
    int fileCount = 0;

    auto result = std::make_optional<Options>();
    auto& options = *result;

    while (i < argc)
    {
        std::string_view arg = argv[i];
        if (arg.starts_with('-'))
        {
            if (arg == "-l"sv || arg == "--lut"sv)
            {
                options.LUTUsage = LUTUsageType::UseExtinctionLUT;
            }
            else if (arg == "-L"sv || arg == "--LUT"sv)
            {
                options.LUTUsage = LUTUsageType::UseScatteringLUT;
            }
            else if (arg == "-f"sv || arg == "--fisheye"sv)
            {
                options.UseFisheyeCameras = true;
            }
            else if (arg == "-e"sv || arg == "--exposure"sv)
            {
                if (i == argc - 1)
                    return std::nullopt;

                ++i;
                if (std::sscanf(argv[i], " %lf", &options.CameraExposure) != 1)
                    return std::nullopt;
            }
            else if (arg == "-s"sv || arg == "--scattersteps"sv)
            {
                if (i == argc - 1)
                    return std::nullopt;

                ++i;
                if (std::sscanf(argv[i], " %u", &options.IntegrateScatterSteps) != 1)
                    return std::nullopt;
            }
            else if (arg == "-d"sv || arg == "--depthsteps"sv)
            {
                if (i == argc - 1)
                    return std::nullopt;

                ++i;
                if (std::sscanf(argv[i], " %u", &options.IntegrateDepthSteps) != 1)
                    return std::nullopt;
            }
            else if (arg == "-w"sv || arg == "--width"sv)
            {
                if (i == argc - 1)
                    return std::nullopt;

                ++i;
                if (std::sscanf(argv[i], " %u", &options.OutputImageWidth) != 1)
                    return std::nullopt;
            }
            else if (arg == "-h"sv || arg == "--height"sv)
            {
                if (i == argc - 1)
                    return std::nullopt;

                ++i;
                if (std::sscanf(argv[i], " %u", &options.OutputImageHeight) != 1)
                    return std::nullopt;
            }
            else if (arg == "-i"sv || arg == "--image"sv)
            {
                if (i == argc - 1)
                    return std::nullopt;

                ++i;
                options.outputImageName = argv[i];
            }
            else
            {
                return std::nullopt;
            }
        }
        else
        {
            if (fileCount == 0)
            {
                // input filename first
                options.configFilename = arg;
                ++fileCount;
            }
            else
            {
                // more than one filenames on the command line is an error
                return std::nullopt;
            }
        }

        ++i;
    }

    return result;
}

} // end unnamed namespace

int
main(int argc, char* argv[])
{
    auto options = parseCommandLine(argc, argv);
    if (!options.has_value() || options->configFilename.empty())
    {
        usage();
        return EXIT_FAILURE;
    }

    ParameterSet sceneParams;
    if (!LoadParameterSet(sceneParams, options->configFilename))
    {
        return EXIT_FAILURE;
    }

    Scene scene(*options);
    scene.light.color = Color(1, 1, 1);
    scene.light.direction = Eigen::Vector3d::UnitZ();
    scene.light.direction.normalize();

#if 0
    // N0 = 2.668e25  // m^-3
    scene.planet = math::Sphered(Eigen::Vector3d::Zero(), planetRadius);

    scene.atmosphere.rayleighScaleHeight = 79.94;
    scene.atmosphere.mieScaleHeight = 1.2;
    scene.atmosphere.rayleighCoeff =
        computeRayleighCoeffs(Eigen::Vector3d(0.600, 0.520, 0.450)) * 0.000008;
    scene.atmosphereShellHeight = scene.atmosphere.calcShellHeight();

    scene.atmosphere.rayleighCoeff =
        computeRayleighCoeffs(Eigen::Vector3d(0.600, 0.520, 0.450)) * 0.000008;
#endif

    scene.setParameters(sceneParams);

    std::cout << "atmosphere height: " << scene.atmosphereShellHeight << "\n"
                 "attenuation coeffs: " << scene.atmosphere.rayleighCoeff.transpose() * 4 * std::numbers::pi << '\n';

    if (options->LUTUsage != LUTUsageType::NoLUT)
    {
        std::cout << "Building extinction LUT...\n";
        scene.extinctionLUT = buildExtinctionLUT(scene);
        std::cout << "Complete!\n";
        DumpLUT(*scene.extinctionLUT, "extlut.png");
    }

    if (options->LUTUsage == LUTUsageType::UseScatteringLUT)
    {
        std::cout << "Building scattering LUT...\n";
        scene.scatteringLUT = buildScatteringLUT(scene);
        std::cout << "Complete!\n";
        DumpLUT(*scene.scatteringLUT, "lut.png");
    }

    double planetRadius = scene.planet.radius;
    double cameraFarDist = planetRadius * 3;
    double cameraCloseDist = planetRadius * 1.2;

    Camera cameraLowPhase;
    cameraLowPhase.fov = math::degToRad(45.0);
    cameraLowPhase.front = 1.0;
    auto t = math::YRotation(math::degToRad(-20.0)) *
             Eigen::Translation3d(0.0, 0.0, -cameraFarDist);
    cameraLowPhase.transform = t.matrix();

    Camera cameraHighPhase;
    cameraHighPhase.fov = math::degToRad(45.0);
    cameraHighPhase.front = 1.0;
    t = math::YRotation(math::degToRad(-160.0)) *
        Eigen::Translation3d(0.0, 0.0, -cameraFarDist);
    cameraHighPhase.transform = t.matrix();

    Camera cameraClose;
    cameraClose.fov = math::degToRad(45.0);
    cameraClose.front = 1.0;
    t = math::YRotation(math::degToRad(-50.0)) *
        Eigen::Translation3d(0.0, 0.0, -cameraCloseDist) *
        math::XRotation(math::degToRad(-55.0));
    cameraClose.transform = t.matrix();

    Camera cameraSurface;
    cameraSurface.fov = math::degToRad(45.0);
    cameraSurface.front = 1.0;
    t = math::YRotation(math::degToRad(-20.0)) *
        Eigen::Translation3d(0.0, 0.0, -planetRadius * 1.0002) *
        math::XRotation(math::degToRad(-85.0));
    cameraSurface.transform = t.matrix();

    double aspectRatio = static_cast<double>(options->OutputImageWidth) / static_cast<double>(options->OutputImageHeight);
    // Make the horizontal FOV of the fisheye cameras 180 degrees
    double fisheyeFOV = math::degToRad(std::max(180.0, 180.0 / aspectRatio));

    Camera cameraFisheyeMidday;
    cameraFisheyeMidday.fov = fisheyeFOV;
    cameraFisheyeMidday.type = CameraType::Spherical;
    t = math::YRotation(math::degToRad(-20.0)) *
        Eigen::Translation3d(0.0, 0.0, -planetRadius * 1.0002) *
        math::XRotation(math::degToRad(-85.0));
    cameraFisheyeMidday.transform = t.matrix();

    Camera cameraFisheyeSunset;
    cameraFisheyeSunset.fov = math::degToRad(180.0);
    cameraFisheyeSunset.type = CameraType::Spherical;
    t = math::YRotation(math::degToRad(-80.0)) *
        Eigen::Translation3d(0.0, 0.0, -planetRadius * 1.0002) *
        math::XRotation(math::degToRad(-85.0)) *
        math::YRotation(math::degToRad(-90.0)) *
        math::ZRotation(math::degToRad(5.0));
    cameraFisheyeSunset.transform = t.matrix();

    RGBImage image(options->OutputImageWidth, options->OutputImageHeight);

    Viewport topleft (0, 0, image.width / 2, image.height / 2);
    Viewport topright(image.width / 2, 0, image.width / 2, image.height / 2);
    Viewport botleft (0, image.height / 2, image.width / 2, image.height / 2);
    Viewport botright(image.width / 2, image.height / 2, image.width / 2, image.height / 2);
    Viewport tophalf (0, 0, image.width, image.height / 2);
    Viewport bothalf (0, image.height / 2, image.width, image.height / 2);

    image.clear({0.1f, 0.1f, 1.0f});

    if (options->UseFisheyeCameras)
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

    WritePNG(options->outputImageName, image);

    return EXIT_SUCCESS;
}
