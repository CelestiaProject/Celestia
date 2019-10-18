
#pragma once

#include <array>
// #include <map>
#include <deque>
#include <celmath/frustum.h>
#include "luminobj.h"

class OctreeNode
{
 public:
    const static int MaxChildren = 8;
//     typedef std::array<OctreeNode*, MaxChildren> Children;
//     typedef std::multimap<float, LuminousObject*> ObjectList;
    static constexpr double MaxScale = 100000000000;
    static constexpr size_t MaxObjectsPerNode = 10;

    class ObjectList : public std::deque<LuminousObject*>
    {
        LuminousObject *m_brightest { nullptr };
        LuminousObject *m_faintest { nullptr };
     public:
        LuminousObject *getFaintest() const { return m_faintest; }
        LuminousObject *getBrightest() const { return m_brightest; }
        bool insert(LuminousObject *);
        void remove(LuminousObject *);
        LuminousObject *popBrightest();
        LuminousObject *popFaintest();
        bool contains(const LuminousObject *) const;
        void dump() const;
    };
    class Children : public array<OctreeNode*, MaxChildren>
    {
        size_t m_childrenCount { 0 };
     public:
        static bool isValid(int n) { return n >= 0 && n < OctreeNode::MaxChildren; }
        size_t getChildrenCount() const { return m_childrenCount; }
        OctreeNode *getChild(int n, OctreeNode * = nullptr);
        OctreeNode *createChild(int, OctreeNode *);
        bool deleteChild(int);
        Children();
        ~Children();
    };
 protected:
    bool add(LuminousObject*);
    bool rm(LuminousObject*);

    LuminousObject *popBrightest() { return m_objects.popBrightest(); }
    LuminousObject *popFaintest() { return m_objects.popFaintest(); }

//     ObjectList::const_iterator objectIterator(const LuminousObject*) const;

//     bool createChild(int);
    bool deleteChild(int);

    OctreeNode *m_parent;
    Eigen::Vector3d m_cellCenter;
    ObjectList m_objects;
    Children *m_children { nullptr };
    double m_scale;
    static size_t m_maxObjCount;
    bool m_dirty { true };
    void setDirty(bool d) { m_dirty = d; }
    static size_t m_nodesNumber;
 public:
    enum
    {
        XPos = 1,
        YPos = 2,
        ZPos = 4,
    };

    OctreeNode(const Eigen::Vector3d& cellCenterPos, double scale, OctreeNode *parent = nullptr);
    ~OctreeNode();

    double getScale() const { return m_scale; }
    const Eigen::Vector3d& getCenter() const { return m_cellCenter; }

    static size_t getMaxObjectCount() { return m_maxObjCount; }

    bool isInFrustum(const celmath::Frustum::PlaneType *planes) const;
    bool isInCell(const Eigen::Vector3d&) const;

    bool insertObject(LuminousObject*);
    bool removeObject(LuminousObject*);

    ObjectList& getObjects() { return m_objects; }
    const ObjectList& getObjects() const { return m_objects; }
    const ObjectList::const_iterator hasObject(const LuminousObject *) const;

    int getChildId(const Eigen::Vector3d&);
    Children *getChildren() { return m_children; }
    const Children *getChildren() const { return m_children; }
    OctreeNode *getChild(int n, bool create = true);
    OctreeNode *getChild(const Eigen::Vector3d &pos, bool create = true)
    {
        return getChild(getChildId(pos), create);
    }
    int getBrightestChildId() const;

    float getFaintest() const;
    float getBrightest() const;

    size_t getObjectCount() const { return m_objects.size(); }
    size_t getChildrenCount() const;
    bool empty() const { return m_objects.empty() && getChildrenCount() == 0; }

    bool isDirty() const { return m_dirty; }
    int check(float max, int, bool);
    void dump(int) const;
    OctreeNode *getParent() { return m_parent; }
    OctreeNode *getRoot();
    const OctreeNode *getParent() const { return m_parent; }
    static bool m_debug;
    static size_t getNodesNumber() { return m_nodesNumber; }
};
