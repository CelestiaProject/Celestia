// deepskyobj.h
//
// Copyright (C) 2003, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELENGINE_DEEPSKYOBJ_H_
#define _CELENGINE_DEEPSKYOBJ_H_

#include <vector>
#include <string>
#include <iostream>
#include <celmath/ray.h>
#include <celengine/luminobj.h>
#ifdef USE_GLCONTEXT
#include <celengine/glcontext.h>
#endif
#include <celengine/parser.h>
#include <celcompat/filesystem.h>
#include <Eigen/Core>
#include <Eigen/Geometry>

class Selection;
class Renderer;

constexpr const float DSO_DEFAULT_ABS_MAGNITUDE = -1000.0f;

class Nebula;
class Galaxy;
class Globular;
class OpenCluster;

class DeepSkyObject : public LuminousObject
{
 public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    virtual Selection toSelection();
    DeepSkyObject()
    {
        setAbsoluteMagnitude(DSO_DEFAULT_ABS_MAGNITUDE);
    }
    virtual ~DeepSkyObject() = default;

    static void hsv2rgb( float*, float*, float*, float, float, float);

    virtual const char* getType() const = 0;
    virtual void setType(const std::string&) = 0;
    virtual std::string getDescription() const;

    Eigen::Quaternionf getOrientation() const;
    void setOrientation(const Eigen::Quaternionf&);

    /*! Return the radius of a bounding sphere large enough to contain the object.
     *  For correct rendering, all of the geometry must fit within this sphere radius.
     *  DSO subclasses an alternate radius that more closely matches the conventional
     *  astronomical definition for the size of the object (e.g. mu25 isophote radius.)
     */
    virtual float getBoundingSphereRadius() const { return radius; }

    /*! Return the radius of the object. This radius will be displayed in the UI and
     *  should match the conventional astronomical definition of the object size.
     */
    float getRadius() const { return radius; }
    void setRadius(float r);
    virtual float getHalfMassRadius() const { return radius; }

    const std::string& getInfoURL() const;
    void setInfoURL(const std::string&);

    bool isVisible() const { return visible; }
    void setVisible(bool _visible) { visible = _visible; }
    bool isClickable() const { return clickable; }
    void setClickable(bool _clickable) { clickable = _clickable; }


    virtual const char* getObjTypeName() const = 0;

    virtual bool pick(const celmath::Ray3d& ray,
                      double& distanceToPicker,
                      double& cosAngleToBoundCenter) const = 0;
    virtual bool load(AssociativeArray*, const fs::path& resPath);
    virtual void render(const Eigen::Vector3f& offset,
                        const Eigen::Quaternionf& viewerOrientation,
                        float brightness,
                        float pixelSize,
                        const Renderer* = nullptr) const = 0;

    virtual uint64_t getRenderMask() const { return 0; }
    virtual unsigned int getLabelMask() const { return 0; }

 private:
    Eigen::Quaternionf orientation{ Eigen::Quaternionf::Identity() };
    float        radius{ 1 };
    std::string infoURL;

    bool visible { true };
    bool clickable { true };
};

typedef std::vector<DeepSkyObject*> DeepSkyCatalog;
int LoadDeepSkyObjects(DeepSkyCatalog&, std::istream& in,
                       const std::string& path);


#endif // _CELENGINE_DEEPSKYOBJ_H_
