
#include <limits.h>
#include <celutil/debug.h>
#include "astrooctree.h"
#include "star.h"
#include "deepskyobj.h"

using namespace std;
using namespace Eigen;
using namespace celmath;

char* _stack;

size_t OctreeNode::m_maxObjCount = 12;

static void dumpObjects(const OctreeNode *node)
{
    for (const auto &obj : node->getObjects())
        fmt::fprintf(cout, "%f ", obj->getAbsoluteMagnitude());
    fmt::fprintf(cout, "\n");
}

bool OctreeNode::ObjectList::insert(LuminousObject *o)
{
    if (contains(o))
    {
//         fmt::fprintf(cout, "OctreeNode::ObjectList::insert(%i, %f): found duplicate!\n", o->getIndex(), o->getAbsoluteMagnitude());
        return false;
    }
    if (m_brightest == nullptr || o->getAbsoluteMagnitude() < m_brightest->getAbsoluteMagnitude())
        m_brightest = o;
    if (m_faintest == nullptr || o->getAbsoluteMagnitude() > m_faintest->getAbsoluteMagnitude())
        m_faintest = o;
    push_back(o);
    return true;
}

void OctreeNode::ObjectList::remove(LuminousObject *o)
{
//     fmt::fprintf(cout, " ObjectList::remove(%i (%f))\n", o->getIndex(), o->getAbsoluteMagnitude());
//     dump();
    bool ff = false;
    bool fb = false;
    bool found = false;
    if (m_brightest != nullptr && m_brightest->getIndex() == o->getIndex() && m_brightest != o)
    {
        fmt::fprintf(cout, "  Object %i (%f) has the same index but different address than brightest one!\n", o->getIndex(), o->getAbsoluteMagnitude());
        exit(1);
    }
    if (m_faintest != nullptr && m_faintest->getIndex() == o->getIndex() && m_faintest != o)
    {
        fmt::fprintf(cout, "  Object %i (%f) has the same index but different address than faintest one!\n", o->getIndex(), o->getAbsoluteMagnitude());
        exit(1);
    }
    if (o == m_brightest)
    {
        m_brightest = nullptr;
        fb = true;
    }
    if (o == m_faintest)
    {
        m_faintest = nullptr;
        ff = true;
    }
    auto it = begin();
    ObjectList::iterator dit = it;
    float f = -std::numeric_limits<float>::max();
    float b = std::numeric_limits<float>::max();
//     fmt::fprintf(cout, "  Seeking faintest: %i (%f), seeking brightest: %i (%f).\n", ff, f, fb, b);
//     cout << "OctreeNode::ObjectList::erase()\n";
    while(it != end())
    {
//         cout << (*it)->getAbsoluteMagnitude() << endl;
        if (*it == o)
        {
            dit = it;
            found = true;
//             fmt::fprintf(cout, "   Found %i (%f).\n", (*it)->getIndex(), (*it)->getAbsoluteMagnitude());
            if (!ff && !fb)
                break;
        }
        else
        {
            if (ff && (*it)->getAbsoluteMagnitude() > f)
            {
                f = (*it)->getAbsoluteMagnitude();
                m_faintest = *it;
//                 fmt::fprintf(cout, "   Found new faintest %i (%f).\n", (*it)->getIndex(), (*it)->getAbsoluteMagnitude());
            }
            if (fb && (*it)->getAbsoluteMagnitude() < b)
            {
                b = (*it)->getAbsoluteMagnitude();
                m_brightest = *it;
//                 fmt::fprintf(cout, "   Found new brightest %i (%f).\n", (*it)->getIndex(), (*it)->getAbsoluteMagnitude());
            }
        }
        ++it;
    }
//     cout << "Found object: " << found << ", find faintest: " << ff << ", find brightest: " << fb << endl;
    if (found)
    {
//         fmt::fprintf(cout, " Deleting %i (%f)\n", (*dit)->getIndex(), (*dit)->getAbsoluteMagnitude());
        erase(dit);
        if (m_faintest == o)
        {
            fmt::fprintf(cout, " Deleted object %i (%f) is still the faintest one!\n", o->getIndex(), o->getAbsoluteMagnitude());
            dump();
            exit(1);
        }
        if (m_brightest == o)
        {
            fmt::fprintf(cout, " Deleted object %i (%f) is still the brightest one!\n", o->getIndex(), o->getAbsoluteMagnitude());
            dump();
            exit(1);
        }
    }
    else
    {
        fmt::fprintf(cout, " Deleting object %i (%f) not found!\n", o->getIndex(), o->getAbsoluteMagnitude());
        dump();
        exit(1);
    }
//     fmt::fprintf(cout, " Object %i (%f) deleted successfuly.\n", o->getIndex(), o->getAbsoluteMagnitude());
    shrink_to_fit();
}

LuminousObject *OctreeNode::ObjectList::popBrightest()
{
    auto ret = getBrightest();
//     fmt::fprintf(cout, "OctreeNode::ObjectList::popBrightest(): %i (%f)\n", ret->getIndex(), ret->getAbsoluteMagnitude());
    remove(ret);
    return ret;
}

LuminousObject *OctreeNode::ObjectList::popFaintest()
{
    auto ret = getFaintest();
//     fmt::fprintf(cout, "OctreeNode::ObjectList::popFaintest(): %i (%f)\n", ret->getIndex(), ret->getAbsoluteMagnitude());
    remove(ret);
    return ret;
}

bool OctreeNode::ObjectList::contains(const LuminousObject *o) const
{
    for (const auto &it : *this)
        if (it == o)
            return true;
    return false;
}

void OctreeNode::ObjectList::dump() const
{
    fmt::fprintf(cout, "OctreeNode::ObjectList::dump(): %i objects:\n", size());
    if (m_brightest)
        fmt::fprintf(cout, "  brightest: %i (%f)\n", m_brightest->getIndex(), m_brightest->getAbsoluteMagnitude());
    else
        fmt::fprintf(cout, "  no brightest\n");
    if (m_faintest)
        fmt::fprintf(cout, "  faintest: %i (%f)\n", m_faintest->getIndex(), m_faintest->getAbsoluteMagnitude());
    else
        fmt::fprintf(cout, "  no faintest\n");
    for (const auto &it : *this)
        fmt::fprintf(cout, "  %i : %f\n", it->getIndex(), it->getAbsoluteMagnitude());
}

OctreeNode::Children::Children()
{
    for(int i = 0; i < MaxChildren; i++)
        (*this)[i] = nullptr;
}

OctreeNode::Children::~Children()
{
    for (const auto &child : *this)
        delete child;
}

OctreeNode *OctreeNode::Children::getChild(int n, OctreeNode *parent)
{
//     if (!isValid(n))
//         return nullptr;
    if (at(n) != nullptr)
        return at(n);
    if (parent != nullptr)
        return createChild(n, parent);
    return nullptr;
}

OctreeNode *OctreeNode::Children::createChild(int i, OctreeNode *parent)
{
    if (at(i) != nullptr)
    {
        fmt::fprintf(cout, "Creating child at already existing index %i!\n", i);
        exit(1);
    }
    double scale = parent->getScale() / 2;
    Vector3d centerPos = parent->getCenter() + Vector3d(((i & XPos) != 0) ? scale : -scale,
                                        ((i & YPos) != 0) ? scale : -scale,
                                        ((i & ZPos) != 0) ? scale : -scale);
    (*this)[i] = new OctreeNode(centerPos, scale, parent);
    ++m_childrenCount;
    return (*this)[i];
}

bool OctreeNode::Children::deleteChild(int n)
{
    if (at(n) == nullptr)
    {
        fmt::fprintf(cout, "Deleting non-existing child at index %i!\n", n);
        exit(1);
    }
    delete (*this)[n];
    (*this)[n] = nullptr;
    --m_childrenCount;
    return true;
}

OctreeNode::OctreeNode(const Vector3d& cellCenter, double scale, OctreeNode *parent) :
    m_cellCenter(cellCenter),
    m_scale(scale),
    m_parent(parent)
{}

OctreeNode::~OctreeNode()
{
    if (m_children != nullptr)
        delete m_children;
}

bool OctreeNode::add(LuminousObject *obj)
{
//     fmt::fprintf(cout, "OctreeNode::add(%i (%f))\n", obj->getIndex(), obj->getAbsoluteMagnitude());
//     m_objects.dump();
    bool dirty = true;
//     fmt::fprintf(cout, "Adding to node %i object %i with mag %f.\n", this, obj, obj->getAbsoluteMagnitude());
    if (obj == nullptr)
    {
        cerr << "Octree: trying to add null object!!\n";
        return false;
    }
    m_objects.insert(obj);
    obj->setOctreeNode(this);
    setDirty(dirty);
    return true;
}

bool OctreeNode::rm(LuminousObject *obj)
{
//     fmt::fprintf(cout, "OctreeNode::rm(%i (%f))\n", obj->getIndex(), obj->getAbsoluteMagnitude());
    m_objects.remove(obj);
    obj->setOctreeNode(nullptr);
    setDirty(true);
    return true;
}

OctreeNode *OctreeNode::getChild(int i, bool create)
{
//     if (!Children::isValid(i))
//         return nullptr;
    if (m_children == nullptr && create)
        m_children = new Children;
    if (m_children != nullptr)
        return m_children->getChild(i, create ? this : nullptr);
    return nullptr;
}

bool OctreeNode::deleteChild(int n)
{
//     if (!childValid(n))
//         return false;
//     if (m_children->at(n) == nullptr)
//         return false;
    m_children->deleteChild(n);
    if (m_children->getChildrenCount() == 0)
    {
        delete m_children;
        m_children = nullptr;
    }
    return true;
}

bool OctreeNode::insertObject(LuminousObject *obj)
{
//     fmt::fprintf(cout, "OctreeNode::insertObject(%i (%f))\n", obj->getIndex(), obj->getAbsoluteMagnitude());
//     m_objects.dump();
    OctreeNode *node = this;
    while (!node->isInCell(obj->getPosition()))
    {
        if (getParent() != nullptr)
        {
            node = node->getParent();
        }
        else
            return false;
    }
    float mag = obj->getAbsoluteMagnitude();
    while(node->getBrightest() > mag && node->getParent() != nullptr)
    {
        node = node->getParent();
    }
    while(node->getFaintest() < mag && node->m_objects.size() == node->m_maxObjCount)
    {
        node = node->getChild(node->getChildId(obj->getPosition()));
    }
    node->add(obj);
    while(node->m_objects.size() > node->m_maxObjCount)
    {
        LuminousObject *obj = node->popFaintest();
        node = node->getChild(node->getChildId(obj->getPosition()));
        node->add(obj);
    }
    return true;
}

bool OctreeNode::removeObject(LuminousObject *obj)
{
//     fmt::fprintf(cout, "OctreeNode::removeObject(%i (%f))\n", obj->getIndex(), obj->getAbsoluteMagnitude());
    if (!rm(obj))
    {
        fmt::fprintf(clog, "Object %i not found to remove!\n", obj);
        return false;
    }
    OctreeNode *node = this;
    while(node->m_objects.size() < node->m_maxObjCount && node->getChildrenCount() > 0)
    {
//         fmt::fprintf(cout, "Node %i[%i/ %i]->%i\n", node, node->m_objects.size(), node->m_maxObjCount, node->getChildrenCount());
        int i = node->getBrightestChildId();
        if (i < 0)
        {
            fmt::fprintf(clog, "No brightest child of node %i!!\n", node);
            getRoot()->dump(0);
        }
        OctreeNode *child = node->getChild(i);
        if (child == nullptr)
            clog << "Null child!!" << endl;
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
    return (*i1)->getAbsoluteMagnitude() < (*i2)->getAbsoluteMagnitude();
}

float OctreeNode::getBrightest() const
{
    return m_objects.getBrightest() != nullptr ? m_objects.getBrightest()->getAbsoluteMagnitude() : std::numeric_limits<float>::max();
}

float OctreeNode::getFaintest() const
{
    return m_objects.getFaintest() != nullptr ? m_objects.getFaintest()->getAbsoluteMagnitude() : -std::numeric_limits<float>::max();
}

size_t OctreeNode::getChildrenCount() const
{
    return m_children == nullptr ? 0 : m_children->getChildrenCount();
}

int OctreeNode::getBrightestChildId() const
{
    int ret = -1;
    float f = numeric_limits<float>::max();
    if (m_children == nullptr)
        return ret;
    for(int i = 0; i < 8; i++)
    {
        OctreeNode *child = (*m_children)[i];
        if (child != nullptr && child->getBrightest() < f)
        {
            f = child->getBrightest();
            ret = i;
        }
    }
    if (ret < 0 && getChildrenCount() == 0)
    {
        fmt::fprintf(clog, "No brightest child but there should be!\n");
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
    for(ObjectList::const_iterator it = m_objects.begin(); it != m_objects.end(); ++it)
    {
        ObjectList::const_iterator it2 = it;
        ++it2;
        if (!isInCell((*it)->getPosition()))
        {
            Vector3d rpos = (*it)->getPosition() - getCenter();
            fmt::fprintf(
                cout,
                "\nObject nr %u on level %i and scale %f out of cell (rel pos [%f : %f : %f])!\n",
                (*it)->getIndex(),
                level,
                getScale(),
                rpos.x(), rpos.y(), rpos.z()
            );
            if (fatal)
                exit(1);
            nerr = 1;
        }
//         if (it2 != m_objects.end() && (*it)->getAbsoluteMagnitude() > (*it2)->getAbsoluteMagnitude())
//         {
//             fmt::fprintf(cout, "\nObjects on level %i wrongly sorted!\n", level);
//             if (fatal)
//                 exit(1);
//             nerr = 1;
//         }
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
    if (m_children != nullptr)
        for (const auto &child : *m_children)
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
    if (m_children == nullptr)
        return;
    for(const auto &child : *m_children)
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
