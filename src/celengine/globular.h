// qlobular.h
//
// Copyright (C) 2008, Celestia Development Team
// Initial implementation by Dr. Fridger Schrempp <fridger.schrempp@desy.de>
//
// Simulation of globular clusters, theoretical framework by
// Ivan King, Astron. J. 67 (1962) 471; ibid. 71 (1966) 64
//
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

struct Matrices;
class Renderer;


class Globular : public DeepSkyObject
{
public:

   // min/max c-values of globular cluster data
   static constexpr float MinC = 0.50f;
   static constexpr float MaxC = 2.58f;
   static constexpr int GlobularBuckets = 8;
   static constexpr float BinWidth = (MaxC - MinC) / static_cast<float>(GlobularBuckets) + 0.02f;

    Globular();

    const char* getType() const override;
    void setType(const std::string&) override;
    std::string getDescription() const override;
    float getHalfMassRadius() const override;

    float getDetail() const;
    void setDetail(float);

    float getBoundingSphereRadius() const override { return tidalRadius; }

    bool pick(const Eigen::ParametrizedLine<double, 3>& ray,
              double& distanceToPicker,
              double& cosAngleToBoundCenter) const override;
    bool load(const AssociativeArray*, const fs::path&) override;

    std::uint64_t getRenderMask() const override;
    unsigned int getLabelMask() const override;
    const char* getObjTypeName() const override;

    int getFormId() const;

private:
    // Reference values ( = data base averages) of core radius, King concentration
    static constexpr float R_c_ref = 0.83f;
    static constexpr float C_ref = 2.1f;
    // mu25 isophote radius is not used: R_mu25 = 40.32f

    void recomputeTidalRadius();

    float detail{ 1.0f };
    float r_c{ R_c_ref };
    float c{ C_ref };
    float tidalRadius{ 0.0f };
    int formIndex{ -1 };
};
