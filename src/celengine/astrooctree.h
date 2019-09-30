
#pragma once

#include <array>
#include <map>
#include <celmath/frustum.h>
#include "luminobj.h"

class OctreeNode
{
 public:
    const static int MaxChildren = 8;
    typedef std::array<OctreeNode*, MaxChildren> Children;
    typedef std::multimap<float, LuminousObject*> ObjectList;
    static constexpr double MaxScale = 100000000000;
    static constexpr size_t MaxObjectsPerNode = 10;

 protected:
    bool add(LuminousObject*);
    bool rm(LuminousObject*);

    LuminousObject *popBrightest();
    LuminousObject *popFaintest();

    ObjectList::const_iterator objectIterator(const LuminousObject*) const;

    bool createChild(int);
    bool deleteChild(int);

    OctreeNode *m_parent;
    Eigen::Vector3d m_cellCenter;
    ObjectList m_objects;
    Children m_children;
    double m_scale;
    size_t m_childrenCount {0};
    size_t m_maxObjCount {0};
    bool m_dirty { true };
    void setDirty(bool d) { m_dirty = d; }
 public:
    enum
    {
        XPos = 1,
        YPos = 2,
        ZPos = 4,
    };

    OctreeNode(const Eigen::Vector3d& cellCenterPos, double scale, size_t = MaxObjectsPerNode, OctreeNode *parent = nullptr);
    ~OctreeNode();

    double getScale() const { return m_scale; }
    const Eigen::Vector3d& getCenter() const { return m_cellCenter; }

    bool isInFrustum(const celmath::Frustum::PlaneType *planes) const;
    bool isInCell(const Eigen::Vector3d&) const;

    bool insertObject(LuminousObject*);
    bool removeObject(LuminousObject*);

    ObjectList& getObjects() { return m_objects; }
    const ObjectList& getObjects() const { return m_objects; }
    const ObjectList::const_iterator hasObject(const LuminousObject *) const;

    int getChildId(const Eigen::Vector3d&);
    Children& getChildren() { return m_children; }
    const Children& getChildren() const { return m_children; }
    OctreeNode *getChild(int n, bool = true);
    OctreeNode *getChild(const Eigen::Vector3d &pos, bool create = true)
    {
        return getChild(getChildId(pos), create);
    }
    int getBrightestChildId() const;

    float getFaintest() const;
    float getBrightest() const;

    size_t getObjectCount() const { return m_objects.size(); }
    size_t getChildrenCount() const { return m_childrenCount; }
    bool empty() const { return m_objects.empty() && getChildrenCount() == 0; }

    bool isDirty() const { return m_dirty; }
    int check(float max, int, bool);
    void dump(int) const;
    OctreeNode *getParent() { return m_parent; }
    OctreeNode *getRoot();
    const OctreeNode *getParent() const { return m_parent; }
    static bool m_debug;
};
