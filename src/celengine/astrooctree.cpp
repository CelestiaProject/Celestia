
#include "astrooctree.h"
#include "star.h"
#include "deepskyobj.h"

using namespace Eigen;

OctreeNode::OctreeNode(PointType cellCenter, double scale, float starEx, float dsoEx, OctreeNode *parent) :
    m_children(nullptr),
    m_cellCenter(cellCenter),
    m_scale(scale),
    m_starExclusionFactor(starEx),
    m_dsoExclusionFactor(dsoEx),
    m_parent(parent)
{}

OctreeNode::~OctreeNode()
{
    if (m_children != nullptr)
    {
        for (auto &i : *m_children)
            delete i;
        delete m_children;
    }
}

bool OctreeNode::add(Star *star)
{
    m_stars.emplace_back(star);
    return true;
}

bool OctreeNode::add(DeepSkyObject *dso)
{
    m_dsos.emplace_back(dso);
    return true;
}

bool OctreeNode::rm(Star *star)
{
//     size_t s = m_stars.erase(star);
//     return s > 0 ? true : false;
    return false;
}

bool OctreeNode::rm(DeepSkyObject *dso)
{
//     size_t s = m_dsos.erase(dso);
//     return s > 0 ? true : false;
    return false;
}

bool OctreeNode::split()
{
    if (m_children != nullptr)
        return false;
    m_children = new Children;
    double scale = m_scale / 2;
    for (int i = 0; i < 8; ++i)
    {
        PointType centerPos = m_cellCenter;

        centerPos += PointType(((i & XPos) != 0) ? scale : -scale,
                                ((i & YPos) != 0) ? scale : -scale,
                                ((i & ZPos) != 0) ? scale : -scale);

        (*m_children)[i] = new OctreeNode(
            centerPos,
            m_scale / 2,
            starAbsoluteMagnitudeDecay(m_starExclusionFactor),
            dsoAbsoluteMagnitudeDecay(m_dsoExclusionFactor),
            this);
    }
    sortIntoChildNodes();
    return true;
}

bool OctreeNode::collapse()
{/*
    if (m_children == nullptr)
        return false;
    for (auto &child : getChildren())
    {
        child->collapse();
        for (auto &i : child->getStars())
            add(i);
        for (auto &i : child->getDsos())
            add(i);
        delete child;
    }
    delete m_children;
    m_children = nullptr;
    return true;*/
    return false;
}

bool OctreeNode::insertObject(Star *star)
{
    if (limitingFactorPredicate(star, m_starExclusionFactor) || straddlingPredicate(m_cellCenter, star, m_starExclusionFactor))
        add(star);
    else
    {
        if (m_children == nullptr)
        {
            if (getObjectCount() >= SPLIT_THRESHOLD)
                split();
            add(star);
        }
        else
            getChild(star->getPosition().cast<double>())->insertObject(star);
    }
    return true;
}

bool OctreeNode::insertObject(DeepSkyObject *dso)
{
    if (limitingFactorPredicate(dso, m_dsoExclusionFactor) || straddlingPredicate(m_cellCenter, dso, m_dsoExclusionFactor))
        add(dso);
    else
    {
        if (m_children == nullptr)
        {
            if (getObjectCount() >= SPLIT_THRESHOLD)
                split();
            add(dso);
        }
        else
            getChild(dso->getPosition().cast<double>())->insertObject(dso);
    }
    return true;
}

OctreeNode *OctreeNode::getChild(const Eigen::Vector3d &pos)
{
    if (m_children == nullptr)
        return nullptr;

    int child = 0;
    child |= pos.x() < m_cellCenter.x() ? 0 : XPos;
    child |= pos.y() < m_cellCenter.y() ? 0 : YPos;
    child |= pos.z() < m_cellCenter.z() ? 0 : ZPos;

    return (*m_children)[child];
}

OctreeNode *OctreeNode::getChild(int n)
{
    if (m_children == nullptr || n < 0 || n > 7)
        return nullptr;
    return (*m_children)[n];
}

void OctreeNode::sortIntoChildNodes()
{
    unsigned int nKeptInParent = 0;

    for (auto &i : m_stars)
    {
        if (!limitingFactorPredicate(i, m_starExclusionFactor) &&
            !straddlingPredicate(m_cellCenter, i, m_starExclusionFactor))
        {
                getChild(i->getPosition().cast<double>())->add(i);
                rm(i);
        }
    }
    for (auto &i : m_dsos)
    {
        if (!limitingFactorPredicate(i, m_dsoExclusionFactor) &&
            !straddlingPredicate(m_cellCenter, i, m_dsoExclusionFactor))
        {
                getChild(i->getPosition().cast<double>())->add(i);
                rm(i);
        }
    }
}

bool OctreeNode::isInFrustum(const Frustum::PlaneType *planes) const
{
    for (unsigned int i = 0; i < 5; ++i)
    {
        const Frustum::PlaneType& plane = planes[i];

        double r = m_scale * plane.normal().cwiseAbs().sum();
        if (plane.signedDistance(m_cellCenter.cast<float>()) < -r)
            return false;
    }
    return true;
}

bool OctreeNode::limitingFactorPredicate (Star *star, float absMag)
{
    return star->getAbsoluteMagnitude() <= absMag;
}

bool OctreeNode::limitingFactorPredicate (DeepSkyObject *dso, float absMag)
{
    return dso->getAbsoluteMagnitude() <= absMag;
}

bool OctreeNode::straddlingPredicate (const PointType &center, Star *star, float )
{
    float orbitalRadius = star->getOrbitalRadius();
    if (orbitalRadius == 0.0f)
        return false;
    Vector3d starPos = star->getPosition().cast<double>();
    return (starPos - center).cwiseAbs().minCoeff() < orbitalRadius;
}

bool OctreeNode::straddlingPredicate (const PointType &center, DeepSkyObject *dso, float )
{
    float dsoRadius = dso->getBoundingSphereRadius();
    return (dso->getPosition() - center).cwiseAbs().minCoeff() < dsoRadius;
}

double OctreeNode::dsoAbsoluteMagnitudeDecay(double excludingFactor)
{
    return excludingFactor + 0.5f;
}

double OctreeNode::starAbsoluteMagnitudeDecay(double excludingFactor)
{
    return astro::lumToAbsMag(astro::absMagToLum(excludingFactor) / 4.0f);
}

size_t OctreeNode::SPLIT_THRESHOLD;
