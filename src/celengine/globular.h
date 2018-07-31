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


struct GBlob
{
#ifdef __CELVEC__
    Point3f        position;
#else
    Eigen::Vector3f position;
#endif
    unsigned int   colorIndex;
    float          radius_2d;
};

struct GlobularForm
{
    std::vector<GBlob>* gblobs;
#ifdef __CELVEC__
    Vec3f scale;
#else
    Eigen::Vector3f scale;
#endif
};

class Globular : public DeepSkyObject
{
 public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    Globular();
    virtual const char* getType() const;
    virtual void setType(const std::string&);
    virtual size_t getDescription(char* buf, size_t bufLength) const;
    virtual std::string getCustomTmpName() const;
    virtual void setCustomTmpName(const std::string&);
    float getDetail() const;
    void  setDetail(float);
    float getCoreRadius() const;
    void  setCoreRadius(const float);
    void  setConcentration(const float);
    float getConcentration() const;
    float getHalfMassRadius() const;
    unsigned int cSlot(float) const;

    virtual float getBoundingSphereRadius() const { return tidalRadius; }

    virtual bool pick(const Ray3d& ray,
                      double& distanceToPicker,
                      double& cosAngleToBoundCenter) const;
    virtual bool load(AssociativeArray*, const std::string&);
    virtual void render(const GLContext& context,
                        const Eigen::Vector3f& offset,
                        const Eigen::Quaternionf& viewerOrientation,
                        float brightness,
                        float pixelSize);
    virtual void renderGlobularPointSprites(const GLContext& context,
#ifdef __CELVEC__
                                            const Vec3f& offset,
                                            const Quatf& viewerOrientation,
#else
                                            const Eigen::Vector3f& offset,
                                            const Eigen::Quaternionf& viewerOrientation,
#endif
                                            float brightness,
                                            float pixelSize);
    GlobularForm* getForm() const;

    virtual unsigned int getRenderMask() const;
    virtual unsigned int getLabelMask() const;
    virtual const char* getObjTypeName() const;

 private:
    void recomputeTidalRadius();

 private:
    float detail;
    std::string* customTmpName;
    GlobularForm* form;
    float r_c;
    float c;
    float tidalRadius;
};

#endif // _GLOBULAR_H_
