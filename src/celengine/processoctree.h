
#pragma once

#include <celmath/frustum.h>
#include <celengine/selection.h>

class Star;
class DeepSkyObject;
class OctreeNode;

constexpr double SQRT3 = 1.732050807568877;

#ifdef OCTREE_DEBUG
struct OctreeProcStats
{
    size_t objects { 0 };
    size_t height { 0 };
    size_t nodes { 0 };
    size_t bads { 0 };
    Selection selection;
    bool selProc;
    bool selNode;
    Eigen::Vector3d obsPos;
    bool selInFrustum;
    float limit;
    float faintest;
    float appFaintest;
    Frustum::PlaneType frustPlanes[5];
    const OctreeNode *lastSelNode;
    bool lastSelNodeInFrustum;
    bool selWrongOrder;
    bool isSelNode(const OctreeNode *) const;
    void reset()
    {
        objects = 0;
        height = 0;
        nodes = 0;
        bads = 0;
        selProc = false;
        selNode = false;
        selInFrustum = false;
        limit = 9999;
        faintest = 1000;
        appFaintest = 1001;
        lastSelNode = nullptr;
        lastSelNodeInFrustum = false;
        selWrongOrder = false;
    }
};
#endif

template<typename T>
class ObjectProcesor
{
 public:
    ObjectProcesor() {};
    virtual ~ObjectProcesor() {};

    virtual void process(T, double distance, float appMag) = 0;
};

typedef ObjectProcesor<const Star*> StarProcesor;
typedef ObjectProcesor<const DeepSkyObject*> DsoProcesor;

void create5FrustumPlanes(Frustum::PlaneType *, const Eigen::Vector3d &, const Eigen::Quaternionf &, float, float);

void processVisibleStars(
    const OctreeNode *node,
    StarProcesor& procesor,
    const Eigen::Vector3d& obsPos,
    const Frustum::PlaneType *frustumPlanes,
#ifdef OCTREE_DEBUG
    float limitFactor,
    OctreeProcStats * = nullptr);
#else
    float limitFactor);
#endif

void processVisibleStars(
    const OctreeNode *node,
    StarProcesor& procesor,
    const Eigen::Vector3d &position,
    const Eigen::Quaternionf &orientation,
    float fovY,
    float aspectRatio,
#ifdef OCTREE_DEBUG
    float limitingFactor,
    OctreeProcStats * = nullptr);
#else
    float limitingFactor);
#endif

void processVisibleDsos(
    const OctreeNode *node,
    DsoProcesor& procesor,
    const Eigen::Vector3d& obsPos,
    const Frustum::PlaneType *frustumPlanes,
#ifdef OCTREE_DEBUG
    float limitFactor,
    OctreeProcStats * = nullptr);
#else
        float limitFactor);
#endif

void processVisibleDsos(
    const OctreeNode *node,
    DsoProcesor& procesor,
    const Eigen::Vector3d &position,
    const Eigen::Quaternionf &orientation,
    float fovY,
    float aspectRatio,
#ifdef OCTREE_DEBUG
    float limitingFactor,
    OctreeProcStats * = nullptr);
#else
    float limitingFactor);
#endif

void processCloseStars(const OctreeNode *node, StarProcesor& procesor, const Eigen::Vector3d& obsPos, double bRadius);
void processCloseDsos(const OctreeNode *node, DsoProcesor& procesor, const Eigen::Vector3d& obsPos, double bRadius);
