// deepskyobj.h
//
// Copyright (C) 2003, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <cstdint>
#include <iosfwd>
#include <string>
#include <string_view>
#include <vector>

#include <Eigen/Core>
#include <Eigen/Geometry>

#include <celcompat/filesystem.h>
#include "astroobj.h"
#include "renderflags.h"

class AssociativeArray;
class Selection;
class Renderer;
struct Matrices;

constexpr inline float DSO_DEFAULT_ABS_MAGNITUDE = -1000.0f;

class Nebula;
class Galaxy;
class Globular;
class OpenCluster;

enum class DeepSkyObjectType
{
    Galaxy,
    Globular,
    Nebula,
    OpenCluster,
};

class DeepSkyObject
{
public:
    DeepSkyObject() = default;
    virtual ~DeepSkyObject() = default;

    Eigen::Vector3d getPosition() const;
    void setPosition(const Eigen::Vector3d&);

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

    float getAbsoluteMagnitude() const;
    void setAbsoluteMagnitude(float);

    const std::string& getInfoURL() const;
    void setInfoURL(std::string&&);

    bool isVisible() const { return visible; }
    void setVisible(bool _visible) { visible = _visible; }
    bool isClickable() const { return clickable; }
    void setClickable(bool _clickable) { clickable = _clickable; }

    virtual DeepSkyObjectType getObjType() const = 0;

    virtual bool pick(const Eigen::ParametrizedLine<double, 3>& ray,
                      double& distanceToPicker,
                      double& cosAngleToBoundCenter) const;
    virtual bool load(const AssociativeArray*, const fs::path& resPath, std::string_view name);

    virtual RenderFlags getRenderMask() const { return RenderFlags::ShowNothing; }
    virtual RenderLabels getLabelMask() const { return RenderLabels::NoLabels; }

    AstroCatalog::IndexNumber getIndex() const { return indexNumber; }
    void setIndex(AstroCatalog::IndexNumber idx) { indexNumber = idx; }

private:
    Eigen::Vector3d position{ Eigen::Vector3d::Zero() };
    Eigen::Quaternionf orientation{ Eigen::Quaternionf::Identity() };
    float        radius{ 1 };
    float        absMag{ DSO_DEFAULT_ABS_MAGNITUDE } ;
    AstroCatalog::IndexNumber indexNumber{ AstroCatalog::InvalidIndex };
    std::string infoURL;

    bool visible { true };
    bool clickable { true };
};
