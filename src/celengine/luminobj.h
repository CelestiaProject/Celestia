
#pragma once

#include <Eigen/Core>
#include "astroobj.h"

class OctreeNode;

class LuminousObject : public AstroObject
{
 protected:
    float m_absMag;
    Eigen::Vector3d m_position { Eigen::Vector3d::Zero() };
    OctreeNode *m_octreeNode { nullptr };
 public:
    float getAbsoluteMagnitude() const { return m_absMag; }
    void setAbsoluteMagnitude(float _mag);
    Eigen::Vector3d getPosition() const { return m_position; }
    void setPosition(const Eigen::Vector3d _pos);
    void setPosition(double x, double y, double z) { setPosition(Eigen::Vector3d(x, y, z)); }
    void setOctreeNode(OctreeNode *n) { m_octreeNode = n; }
    OctreeNode *getOctreeNode() { return m_octreeNode; }
    const OctreeNode *getOctreeNode() const { return m_octreeNode; }
};
