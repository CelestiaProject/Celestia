
#pragma once

#include <array>
#include <celmath/frustum.h>
#include "astroobj.h"

class OctreeNode
{
 public:
    typedef Eigen::Matrix<double, 3, 1> PointType;
    typedef std::array<OctreeNode*, 8> Children;

 protected:
    bool limitingFactorPredicate(Star*, float);
    bool limitingFactorPredicate(DeepSkyObject*, float);
    bool straddlingPredicate (const PointType&, Star*, float);
    bool straddlingPredicate (const PointType&, DeepSkyObject*, float);

    bool add(Star*);
    bool add(DeepSkyObject*);
    bool rm(Star*);
    bool rm(DeepSkyObject*);

    bool split();
    bool collapse();

    void sortIntoChildNodes();
    OctreeNode *getChild(const Eigen::Vector3d&);

    double dsoAbsoluteMagnitudeDecay(double excludingFactor);
    double starAbsoluteMagnitudeDecay(double excludingFactor);

    OctreeNode *m_parent;
    PointType m_cellCenter;
    std::vector<Star*> m_stars;
    std::vector<DeepSkyObject*> m_dsos;
    Children *m_children;
    double m_starExclusionFactor;
    double m_dsoExclusionFactor;
    double m_scale;

    static size_t SPLIT_THRESHOLD;
 public:
    enum
    {
        XPos = 1,
        YPos = 2,
        ZPos = 4,
    };

    OctreeNode(PointType cellCenterPos, double scale, float starExclusionFactor, float dsoExclusionFactor, OctreeNode *parent = nullptr);
    ~OctreeNode();

    double getScale() const { return m_scale; }
    PointType getCenter() const { return m_cellCenter; }
    float getStarExclusionFactor() const { return m_starExclusionFactor; }
    float getDsoExclusionFactor() const { return m_dsoExclusionFactor; }
    bool isInFrustum(const Frustum::PlaneType *planes) const;

    bool insertObject(Star*);
    bool insertObject(DeepSkyObject*);
    bool removeObject(Star*);
    bool removeObject(DeepSkyObject*);

    std::vector<Star*>& getStars() { return m_stars; }
    const std::vector<Star*>& getStars() const { return m_stars; }
    std::vector<DeepSkyObject*>& getDsos() { return m_dsos; }
    const std::vector<DeepSkyObject*>& getDsos() const { return m_dsos; }

    bool hasChildren() const { return m_children != nullptr; }
    Children& getChildren() { return *m_children; }
    const Children& getChildren() const { return *m_children; }
    OctreeNode *getChild(int n);

    size_t getObjectCount() const { return m_stars.size() + m_dsos.size(); }
};
