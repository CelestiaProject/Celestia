
#include "luminobj.h"
#include "astrooctree.h"

void LuminousObject::setAbsoluteMagnitude(float mag)
{
    OctreeNode *node = getOctreeNode();
    if (node != nullptr)
    {
        node->removeObject(this);
        m_absMag = mag;
        node->insertObject(this);
    }
    else
        m_absMag = mag;
}

void LuminousObject::setPosition(const Eigen::Vector3d pos)
{
    OctreeNode *node = getOctreeNode();
    if (node != nullptr)
    {
        node->removeObject(this);
        m_position = pos;
        node->insertObject(this);
    }
    else
        m_position = pos;
}
