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
#include <string>

#include <Eigen/Core>
#include <Eigen/Geometry>

#include <celcompat/filesystem.h>
#include "deepskyobj.h"


class AssociativeArray;
struct Matrices;
class Renderer;


class GalacticForm;

class Galaxy : public DeepSkyObject
{
 public:
    const char* getType() const override;
    void setType(const std::string&) override;
    std::string getDescription() const override;
    std::string getCustomTmpName() const;
    void setCustomTmpName(const std::string&);

    float getDetail() const;
    void setDetail(float);

    bool pick(const Eigen::ParametrizedLine<double, 3>& ray,
              double& distanceToPicker,
              double& cosAngleToBoundCenter) const override;
    bool load(AssociativeArray*, const fs::path&) override;
    void render(const Eigen::Vector3f& offset,
                const Eigen::Quaternionf& viewerOrientation,
                float brightness,
                float pixelSize,
                const Matrices& m,
                Renderer* r) override;

    GalacticForm* getForm() const;

    static void  increaseLightGain();
    static void  decreaseLightGain();
    static float getLightGain();
    static void  setLightGain(float);

    std::uint64_t getRenderMask() const override;
    unsigned int getLabelMask() const override;

    const char* getObjTypeName() const override;

 public:
    enum GalaxyType {
        S0   =  0,
        Sa   =  1,
        Sb   =  2,
        Sc   =  3,
        SBa  =  4,
        SBb  =  5,
        SBc  =  6,
        E0   =  7,
        E1   =  8,
        E2   =  9,
        E3   = 10,
        E4   = 11,
        E5   = 12,
        E6   = 13,
        E7   = 14,
        Irr  = 15
    };

 private:
    void renderGalaxyPointSprites(const Eigen::Vector3f& offset,
                                  const Eigen::Quaternionf& viewerOrientation,
                                  float brightness,
                                  float pixelSize,
                                  const Matrices& mvp,
                                  Renderer* r);

    float         detail{ 1.0f };
    std::string*  customTmpName{ nullptr };
    GalaxyType    type{ S0 };
    GalacticForm* form{ nullptr };

    static float  lightGain;
};
