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
#include "png.h"

using namespace std;


const unsigned int IntegrateScatterSteps = 20;
const unsigned int IntegrateDepthSteps = 20;


typedef map<string, double> ParameterSet;


struct Color
{
    Color() : r(0.0f), g(0.0f), b(0.0f) {}
    Color(float _r, float _g, float _b) : r(_r), g(_g), b(_b) {}

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
    double fov;
    double front;
    Mat4d transform;
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


class Atmosphere
{
public:
    double calcShellHeight()
    {
        double maxScaleHeight = max(rayleighScaleHeight, max(mieScaleHeight, absorbScaleHeight));
        return -log(0.002) * maxScaleHeight;
    }

    Vec3d computeExtinction(const OpticalDepths&) const;

    double rayleighScaleHeight;
    double mieScaleHeight;
    double absorbScaleHeight;

    Vec3d rayleighCoeff;
    Vec3d absorbCoeff;
    double mieCoeff;

    double mieAsymmetry;
};


class Scene
{
public:
    Scene() {};
    
    void setParameters(ParameterSet& params);

    Color background;
    Light light;
    Sphered planet;
    Color planetColor;
    Color planetColor2;
    Atmosphere atmosphere;

    double atmosphereShellHeight;
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


double phaseRayleigh(double cosTheta)
{
    return 0.75 * (1.0 + cosTheta * cosTheta);
}


double phaseHenyeyGreenstein(double cosTheta, double g)
{
    return (1.0 - g * g) / pow(1.0 + g * g - 2.0 * g * cosTheta, 1.5);
}


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
 * Absorbing is wavelength dependent
 *
 */


OpticalDepths integrateOpticalDepth(const Scene& scene,
                                    const Point3d& atmStart,
                                    const Point3d& atmEnd)
{
    const unsigned int nSteps = IntegrateDepthSteps;

    Vec3d dir = atmEnd - atmStart;
    double stepDist = dir.length() / (double) nSteps;
    dir.normalize();
    Point3d samplePoint = atmStart + (0.5 * stepDist * dir);

    OpticalDepths depth;
    depth.rayleigh   = 0.0;
    depth.mie        = 0.0;
    depth.absorption = 0.0;

    for (unsigned int i = 0; i < nSteps; i++)
    {
        double h = samplePoint.distanceFromOrigin() - scene.planet.radius;

        // Optical depth due to two phenomena:
        //   Outscattering by Rayleigh and Mie scattering particles
        //   Absorption by absorbing particles
        depth.rayleigh   += exp(-h / scene.atmosphere.rayleighScaleHeight) * stepDist;
        depth.mie        += exp(-h / scene.atmosphere.mieScaleHeight)      * stepDist;
        depth.absorption += exp(-h / scene.atmosphere.absorbScaleHeight)   * stepDist;
        
        samplePoint += stepDist * dir;
    }

    return depth;
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
        rayleighScatter +=
            exp(-h / scene.atmosphere.rayleighScaleHeight) * stepDist * extinction;
        mieScatter +=
            exp(-h / scene.atmosphere.mieScaleHeight) * stepDist * extinction;
        
        samplePoint += stepDist * dir;
    }

    double cosSunAngle = lightDir * dir;

    const Vec3d& rayleigh = scene.atmosphere.rayleighCoeff;
    return phaseRayleigh(cosSunAngle) * Vec3d(rayleighScatter.x * rayleigh.x,
                                              rayleighScatter.y * rayleigh.y,
                                              rayleighScatter.z * rayleigh.z) +
        phaseHenyeyGreenstein(cosSunAngle, scene.atmosphere.mieAsymmetry) *
        mieScatter * scene.atmosphere.mieCoeff;
}


Color raytrace(const Scene& scene, const Ray3d& ray)
{
    double dist = 0.0;
    double atmEnter = 0.0;
    double atmExit = 0.0;

    if (raySphereIntersect(ray,
                           Sphered(scene.planet.center,
                                   scene.planet.radius + scene.atmosphereShellHeight),
                           atmEnter,
                           atmExit))
    {
        Color baseColor = scene.background;
        Point3d atmStart = ray.origin + atmEnter * ray.direction;
        Point3d atmEnd = ray.origin + atmExit * ray.direction;

        if (testIntersection(ray, scene.planet, dist))
        {
            Point3d intersectPoint = ray.point(dist);
            Vec3d normal = intersectPoint - scene.planet.center;
            normal.normalize();
            Vec3d lightDir = -scene.light.direction;
            double diffuse = max(0.0, normal * lightDir);
            
            // Give the planet a checkerboard texture
            Vec3d spherePoint = normal;
            double phi = atan2(normal.z, normal.x);
            double theta = asin(normal.y);
            int tx = (int) (8 + 8 * phi / PI);
            int ty = (int) (8 + 8 * theta / PI);
            Color planetColor = ((tx ^ ty) & 0x1) ? scene.planetColor : scene.planetColor2;

            Point3d surfacePt = Point3d(0.0, 0.0, 0.0) + (intersectPoint - scene.planet.center);
            Sphered shell = Sphered(Point3d(0.0, 0.0, 0.0),
                                    scene.planet.radius + scene.atmosphereShellHeight);

            // Compute ray from surface point to edge of the atmosphere in the direction
            // of the sun.
            Ray3d sunRay(surfacePt, lightDir);
            double sunDist = 0.0;
            testIntersection(sunRay, shell, sunDist);

            // Compute color of sunlight filtered by the atmosphere; consider extinction
            // along both the sun-to-surface and surface-to-eye paths.
            OpticalDepths sunDepth = integrateOpticalDepth(scene, surfacePt, sunRay.point(sunDist));
            OpticalDepths eyeDepth = integrateOpticalDepth(scene, atmStart, surfacePt);
            OpticalDepths totalDepth = sumOpticalDepths(sunDepth, eyeDepth);
            totalDepth.rayleigh *= 4.0 * PI;
            totalDepth.mie      *= 4.0 * PI;
            Vec3d extinction = scene.atmosphere.computeExtinction(totalDepth);

            // Reflected color of planet surface is:
            //   surface color * sun color * atmospheric extinction
            baseColor = (planetColor * extinction) * scene.light.color * diffuse;
            
            atmEnd = ray.origin + dist * ray.direction;
        }

        Vec3d inscatter = integrateInscattering(scene, atmStart, atmEnd) * 4.0 * PI;

        return Color((float) inscatter.x, (float) inscatter.y, (float) inscatter.z) +
            baseColor;
    }
    else
    {
        return scene.background;
    }
}


void render(const Scene& scene,
            const Camera& camera,
            const Viewport& viewport,
            RGBImage& image)
{
    double aspectRatio = (double) image.width / (double) image.height;
    double viewPlaneHeight = tan(camera.fov / 2.0) * 2 * camera.front;
    double viewPlaneWidth = viewPlaneHeight * aspectRatio;

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
            Vec3d viewDir;
            viewDir.x = ((double) (j - viewport.x) / (double) (viewport.width - 1) - 0.5) * viewPlaneWidth;
            viewDir.y = ((double) (i - viewport.y) / (double) (viewport.height - 1) - 0.5) * viewPlaneHeight;
            viewDir.z = camera.front;
            viewDir.normalize();

            Ray3d viewRay(Point3d(0.0, 0.0, 0.0), viewDir);
            viewRay = viewRay * camera.transform;

            Color color = raytrace(scene, viewRay);
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
    atmosphere.mieAsymmetry        = params["MieAsymmetry"];

    atmosphere.absorbScaleHeight   = params["AbsorbScaleHeight"];
    atmosphere.absorbCoeff.x       = params["AbsorbRed"];
    atmosphere.absorbCoeff.y       = params["AbsorbGreen"];
    atmosphere.absorbCoeff.z       = params["AbsorbBlue"];

    atmosphereShellHeight          = atmosphere.calcShellHeight();

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
        double value;

        in >> name;
        in >> value;
        if (in.good())
        {
            params[name] = value;
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


void main(int argc, char* argv[])
{
    if (argc != 2)
    {
        cerr << "Usage: scattersim <config file>\n";
        exit(1);
    }

    ParameterSet sceneParams;
    setSceneDefaults(sceneParams);
    if (!LoadParameterSet(sceneParams, argv[1]))
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
        Mat4d::yrotation(degToRad(110.0));

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

    RGBImage image(600, 450);

    Viewport topleft (0, 0, image.width / 2, image.height / 2);
    Viewport topright(image.width / 2, 0, image.width / 2, image.height / 2);
    Viewport botleft (0, image.height / 2, image.width / 2, image.height / 2);
    Viewport botright(image.width / 2, image.height / 2, image.width / 2, image.height / 2);

    image.clear(Color(0.1f, 0.1f, 1.0f));
    render(scene, cameraLowPhase, topleft, image);
    render(scene, cameraHighPhase, topright, image);
    render(scene, cameraClose, botleft, image);
    render(scene, cameraSurface, botright, image);

    WritePNG("out.png", image);

    exit(0);
}
