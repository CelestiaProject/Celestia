// galaxy.h
//
// Copyright (C) 2001-2009, the Celestia Development Team
// Original version by Chris Laurel, Fridger Schrempp, and Toti
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <cstdint>
#include <cstddef>
#include <string>

#include <Eigen/Core>
#include <Eigen/Geometry>

#include <celcompat/filesystem.h>
#include "deepskyobj.h"


class AssociativeArray;
struct Matrices;
class Renderer;


enum class GalaxyType {
    Irr  =  0,
    S0   =  1,
    Sa   =  2,
    Sb   =  3,
    Sc   =  4,
    SBa  =  5,
    SBb  =  6,
    SBc  =  7,
    E0   =  8,
    E1   =  9,
    E2   = 10,
    E3   = 11,
    E4   = 12,
    E5   = 13,
    E6   = 14,
    E7   = 15,
};


class Galaxy : public DeepSkyObject
{
 public:
    const char* getType() const override;
    void setType(const std::string&) override;
    std::string getDescription() const override;

    float getDetail() const;
    void setDetail(float);

    bool pick(const Eigen::ParametrizedLine<double, 3>& ray,
              double& distanceToPicker,
              double& cosAngleToBoundCenter) const override;
    bool load(const AssociativeArray*, const fs::path&) override;
    void render(const Eigen::Vector3f& offset,
                const Eigen::Quaternionf& viewerOrientation,
                float brightness,
                float pixelSize,
                const Matrices& m,
                Renderer* r) override;

    static void  increaseLightGain();
    static void  decreaseLightGain();
    static float getLightGain();
    static void  setLightGain(float);

    std::uint64_t getRenderMask() const override;
    unsigned int getLabelMask() const override;

    const char* getObjTypeName() const override;

 private:
    void setForm(const fs::path&, const fs::path& = {});
    float getBrightnessCorrection(const Eigen::Vector3f &) const;
    void renderGL3(const Eigen::Vector3f& offset,
                   const Eigen::Quaternionf& viewerOrientation,
                   float brightness,
                   float pixelSize,
                   const Matrices& m,
                   Renderer* r) const;
    void renderGL2(const Eigen::Vector3f& offset,
                   const Eigen::Quaternionf& viewerOrientation,
                   float brightness,
                   float pixelSize,
                   const Matrices& m,
                   Renderer* r) const;

    float       detail{ 1.0f };
    GalaxyType  type{ GalaxyType::Irr };
    std::size_t form{ 0 };

    static float lightGain;
};
