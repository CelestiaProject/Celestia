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

#ifndef _GLOBULAR_H_
#define _GLOBULAR_H_

#include <Eigen/Geometry>
#include <celengine/deepskyobj.h>
#include <celengine/vertexobject.h>


struct GBlob
{
    Eigen::Vector3f position;
    unsigned int   colorIndex;
    float          radius_2d;
};

struct GlobularForm
{
    std::vector<GBlob>* gblobs;
    Eigen::Vector3f scale;
};

class Globular : public DeepSkyObject
{
 public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    Globular();

    const char* getType() const override;
    void setType(const std::string&) override;
    std::string getDescription() const override;
    std::string getCustomTmpName() const;
    void setCustomTmpName(const std::string&);
    float getDetail() const;
    void  setDetail(float);
    float getCoreRadius() const;
    void  setCoreRadius(const float);
    void  setConcentration(const float);
    float getConcentration() const;
    float getHalfMassRadius() const;
    unsigned int cSlot(float) const;

    float getBoundingSphereRadius() const override { return tidalRadius; }

    bool pick(const celmath::Ray3d& ray,
              double& distanceToPicker,
              double& cosAngleToBoundCenter) const override;
    bool load(AssociativeArray*, const fs::path&) override;
    void render(const Eigen::Vector3f& offset,
                const Eigen::Quaternionf& viewerOrientation,
                float brightness,
                float pixelSize,
                const Renderer* r = nullptr) const override;

    GlobularForm* getForm() const;

    uint64_t getRenderMask() const override;
    unsigned int getLabelMask() const override;
    const char* getObjTypeName() const override;

 private:
    void renderGlobularPointSprites(const Eigen::Vector3f& offset,
                                    const Eigen::Quaternionf& viewerOrientation,
                                    float brightness,
                                    float pixelSize,
                                    const Renderer* r) const;

   // Reference values ( = data base averages) of core radius, King concentration
   // and mu25 isophote radius:
    static constexpr float R_c_ref = 0.83f, C_ref = 2.1f, R_mu25 = 40.32f;

    void recomputeTidalRadius();

    float         detail{ 1.0f };
    std::string*  customTmpName{ nullptr };
    GlobularForm* form{ nullptr };
    float         r_c{ R_c_ref };
    float         c{ C_ref };
    float         tidalRadius{ 0.0f };

    celgl::VertexObject vo{ GL_ARRAY_BUFFER, 0, GL_STATIC_DRAW };
};

#endif // _GLOBULAR_H_
