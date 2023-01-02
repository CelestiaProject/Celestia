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
#include <vector>

#include <Eigen/Core>
#include <Eigen/Geometry>

#include <celcompat/filesystem.h>
#include <celrender/vertexobject.h>
#include "deepskyobj.h"

struct GlobularForm;
struct Matrices;
class Renderer;


class Globular : public DeepSkyObject
{
 public:
    Globular();

    const char* getType() const override;
    void setType(const std::string&) override;
    std::string getDescription() const override;
    float getDetail() const;
    void setDetail(float);
    float getCoreRadius() const;
    void setCoreRadius(float);
    void setConcentration(float);
    float getConcentration() const;
    float getHalfMassRadius() const override;
    unsigned int cSlot(float) const;

    float getBoundingSphereRadius() const override { return tidalRadius; }

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

    std::uint64_t getRenderMask() const override;
    unsigned int getLabelMask() const override;
    const char* getObjTypeName() const override;

 private:
    // Reference values ( = data base averages) of core radius, King concentration
    // and mu25 isophote radius:
    static constexpr float R_c_ref = 0.83f, C_ref = 2.1f, R_mu25 = 40.32f;

    void recomputeTidalRadius();

    float detail{ 1.0f };
    const GlobularForm* form{ nullptr };
    float r_c{ R_c_ref };
    float c{ C_ref };
    float tidalRadius{ 0.0f };

    celestia::render::VertexObject vo{ GL_ARRAY_BUFFER, 0, GL_STATIC_DRAW };
};
