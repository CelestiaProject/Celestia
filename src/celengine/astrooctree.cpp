
#include <limits.h>
#include "astrooctree.h"
#include "star.h"
#include "deepskyobj.h"

using namespace std;
using namespace Eigen;

char* _stack;

static void dumpObjects(const OctreeNode *node)
{
    for (const auto &obj : node->getObjects())
        fmt::fprintf(cout, "%f ", obj.first);
    fmt::fprintf(cout, "\n");
}

OctreeNode::OctreeNode(const Vector3d& cellCenter, double scale, size_t maxObj, OctreeNode *parent) :
    m_cellCenter(cellCenter),
    m_scale(scale),
    m_maxObjCount(maxObj),
    m_parent(parent)
{
    for(int i = 0; i < 8; i++)
        m_children[i] = nullptr;
}

OctreeNode::~OctreeNode()
{
    for (auto &i : m_children)
        if (i != nullptr)
            delete i;
}

bool OctreeNode::add(LuminousObject *obj)
{
    bool dirty = true;
//     fmt::fprintf(cout, "Adding to node %i object %i with mag %f.\n", this, obj, obj->getAbsoluteMagnitude());
    if (obj == nullptr)
        cout << "Null object add!!\n";
    m_objects.insert(make_pair(obj->getAbsoluteMagnitude(), obj));
    obj->setOctreeNode(this);
    setDirty(dirty);
    return true;
}

bool OctreeNode::rm(LuminousObject *obj)
{
    auto i = objectIterator(obj);
    if (i == m_objects.end())
        return false;
    m_objects.erase(i);
    obj->setOctreeNode(nullptr);
    setDirty(true);
    return true;
}

OctreeNode *OctreeNode::getChild(int i, bool create)
{
    if (m_children[i] != nullptr)
        return m_children[i];

    if (!create)
        return nullptr;

    createChild(i);
    return m_children[i];
}

bool OctreeNode::createChild(int i)
{
    if (m_children[i] != nullptr)
    {
// #ifdef OCTREE_DEBUG
        fmt::fprintf(cout, "Trying to create already created node of index %i!\n", i);
// #endif
        return false;
    }
    double scale = m_scale / 2;
    Vector3d centerPos = m_cellCenter + Vector3d(((i & XPos) != 0) ? scale : -scale,
                                        ((i & YPos) != 0) ? scale : -scale,
                                        ((i & ZPos) != 0) ? scale : -scale);
    m_children[i] = new OctreeNode(centerPos, scale, m_maxObjCount, this);
    if (m_children[i] == nullptr)
        fmt::fprintf(cout, "Cannot create new child!\n");
    m_childrenCount++;
    return true;
}

bool OctreeNode::deleteChild(int n)
{
    if (m_children[n] == nullptr)
        return false;
    delete m_children[n];
    m_children[n] = nullptr;
    m_childrenCount--;
    return true;
}

bool OctreeNode::insertObject(LuminousObject *obj)
{
    OctreeNode *node = this;
//     cout << __LINE__ << "\n";
    while (!node->isInCell(obj->getPosition()))
    {
        if (getParent() != nullptr)
        {
            node = node->getParent();
        }
        else
            return false;
    }
//     cout << __LINE__ << "\n";
    float mag = obj->getAbsoluteMagnitude();
//     cout << __LINE__ << "\n";
    while(node->getBrightest() > mag && node->getParent() != nullptr)
    {
        node = node->getParent();
    }
//     cout << __LINE__ << "\n";
    while(node->getFaintest() < mag && node->m_objects.size() == node->m_maxObjCount)
    {
        node = node->getChild(node->getChildId(obj->getPosition()));
    }
//     cout << __LINE__ << "\n";
    node->add(obj);
//     cout << __LINE__ << "\n";
    while(node->m_objects.size() > node->m_maxObjCount)
    {
        LuminousObject *obj = node->popFaintest();
        node = node->getChild(node->getChildId(obj->getPosition()));
        node->add(obj);
    }
//     cout << __LINE__ << "\n";
    return true;
}

bool OctreeNode::removeObject(LuminousObject *obj)
{
    if (!rm(obj))
    {
        fmt::fprintf(cout, "Object %i not found to remove!\n", obj);
        return false;
    }
    OctreeNode *node = this;
    while(node->m_objects.size() < node->m_maxObjCount && node->getChildrenCount() > 0)
    {
//         fmt::fprintf(cout, "Node %i[%i/ %i]->%i\n", node, node->m_objects.size(), node->m_maxObjCount, node->getChildrenCount());
        int i = node->getBrightestChildId();
        if (i < 0)
        {
            fmt::fprintf(cout, "No brightest child of node %i!!\n", node);
            getRoot()->dump(0);
        }
        OctreeNode *child = node->getChild(i);
        if (child == nullptr)
            cout << "Null child!!" << endl;
        LuminousObject *obj = child->popBrightest();
        node->add(obj);
        if (child->getObjectCount() > 0)
        {
//             fmt::fprintf(cout, "Going to child %i.\n", child);
            node = child;
        }
        else
        {
//             fmt::fprintf(cout, "Deleting child nr %i.\n", child);
            node->deleteChild(i);
        }
    }
    return true;
}

int OctreeNode::getChildId(const Eigen::Vector3d &pos)
{
    int child = 0;
    child |= pos.x() < m_cellCenter.x() ? 0 : XPos;
    child |= pos.y() < m_cellCenter.y() ? 0 : YPos;
    child |= pos.z() < m_cellCenter.z() ? 0 : ZPos;

    return child;
}

bool OctreeNode::isInFrustum(const Frustum::PlaneType *planes) const
{
    for (unsigned int i = 0; i < 5; ++i)
    {
        const Frustum::PlaneType& plane = planes[i];

        float r = m_scale * plane.normal().cwiseAbs().sum() * 1.1;
        if (plane.signedDistance(m_cellCenter.cast<float>()) < -r)
            return false;
    }
    return true;
}

bool OctreeNode::isInCell(const Vector3d& pos) const
{
    Vector3d rpos = pos - getCenter();
    double s = getScale();
    if (rpos.x() >= - s && rpos.x() < s &&
        rpos.y() >= - s && rpos.y() < s &&
        rpos.z() >= - s && rpos.z() < s)
        return true;
    return false;
}

static bool pred(const OctreeNode::ObjectList::iterator &i1, const OctreeNode::ObjectList::iterator &i2)
{
    return i1->first < i2->first;
}

OctreeNode::ObjectList::const_iterator OctreeNode::objectIterator(const LuminousObject *obj) const
{
    auto pair = m_objects.equal_range(obj->getAbsoluteMagnitude());
    if (pair.first == m_objects.end())
        return m_objects.end();
    OctreeNode::ObjectList::const_iterator it = pair.first;
    do
    {
        if (it->second == obj)
            return it;
        if(it == pair.second)
            break;
        it++;
    }
    while(it != m_objects.end());
    return m_objects.end();
}

float OctreeNode::getBrightest() const
{
    if (m_objects.empty())
        return numeric_limits<float>::max();
    return m_objects.begin()->first;
}

float OctreeNode::getFaintest() const
{
    if (m_objects.empty())
        return numeric_limits<float>::min();
    return (--m_objects.end())->first;
}

LuminousObject *OctreeNode::popBrightest()
{
    if (m_objects.empty())
        return nullptr;
    auto it = m_objects.begin();
    m_objects.erase(it);
    setDirty(true);
    return it->second;
}

LuminousObject *OctreeNode::popFaintest()
{
    if (m_objects.empty())
        return nullptr;
    auto it = --m_objects.end();
    m_objects.erase(it);
    setDirty(true);
    return it->second;
}

int OctreeNode::getBrightestChildId() const
{
    int ret = -1;
    float f = numeric_limits<float>::max();
    for(int i = 0; i < 8; i++)
    {
        OctreeNode *child = m_children[i];
        if (child != nullptr && child->getBrightest() < f)
        {
            f = child->getBrightest();
            ret = i;
        }
    }
    if (ret < 0 && m_childrenCount == 0)
    {
        fmt::fprintf(cout, "No brightest child but there should be!\n");
    }
    return ret;
}

int OctreeNode::check(float max, int level, bool fatal)
{
    int nerr = 0;
    level++;
    if (m_objects.size() > m_maxObjCount)
    {
        fmt::fprintf(cout, "Objects number exceedes maximum at level %i!\n", level);
        if (fatal)
            exit(1);
        if (nerr == 0)
            nerr = 1;
    }
    if (m_objects.size() < m_maxObjCount && getChildrenCount() > 0)
    {
        fmt::fprintf(cout, "Objects number is less than maximum at level %i!\n", level);
        if (fatal)
            exit(1);
        if (nerr == 0)
            nerr = 1;
    }
    for(const auto &obj : m_objects)
    {
        if (obj.first != obj.second->getAbsoluteMagnitude())
        {
            fmt::fprintf(cout, "Object with mag %f wrongly assigned to mag %f on level %i!\n", obj.second->getAbsoluteMagnitude(), obj.first, level);
            if (fatal)
                exit(1);
            nerr = 1;
        }
    }
    for(ObjectList::const_iterator it = m_objects.begin(); it != m_objects.end(); it++)
    {
        ObjectList::const_iterator it2 = it;
        it2++;
        if (!isInCell(it->second->getPosition()))
        {
            Vector3d rpos = it->second->getPosition() - getCenter();
            fmt::fprintf(
                cout,
                "\nObject nr %i on level %i and scale %f out of cell (rel pos [%f : %f : %f])!\n",
                it->second->getIndex(),
                level,
                getScale(),
                rpos.x(), rpos.y(), rpos.z()
            );
            if (fatal)
                exit(1);
            nerr = 1;
        }
        if (it2 != m_objects.end() && it->first > it2->first)
        {
            fmt::fprintf(cout, "\nObjects on level %i wrongly sorted!\n", level);
            if (fatal)
                exit(1);
            nerr = 1;
        }
    }
    if (getBrightest() < max)
    {
        fmt::fprintf(cout, "\nBrightest brighter than max on level %i (%f < %f) in node %i!\n", level, getBrightest(), max, this);
        dumpObjects(this);
        if (m_parent != nullptr)
        {
            fmt::fprintf(cout, "\nParent %i dump:\n", getParent());
            dumpObjects(m_parent);
        }
        if (fatal)
            exit(1);
        nerr = 1;
    }
    for (const auto &child : m_children)
    {
        if (child != nullptr)
            nerr += child->check(getFaintest(), level, fatal);
    }
    return nerr;
}

void OctreeNode::dump(int level) const
{
    int l = level;
    while(l > 0)
    {
        cout << " ";
        l--;
    }
    cout << this << " ";
    dumpObjects(this);
    for(const auto &child : m_children)
    {
        if (child != nullptr)
            child->dump(level + 1);
    }
}

OctreeNode *OctreeNode::getRoot()
{
    OctreeNode *ret = this;
    while(ret->getParent() != nullptr)
        ret = ret->getParent();
    return ret;
}

bool OctreeNode::m_debug = false;
