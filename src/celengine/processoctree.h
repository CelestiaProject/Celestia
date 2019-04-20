
#include <celmath/frustum.h>

class Star;
class DeepSkyObject;
class OctreeNode;

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

void processVisibleStars(
    const OctreeNode *node,
    StarProcesor& procesor,
    const Eigen::Vector3d& obsPos,
    const Frustum::PlaneType *frustumPlanes,
    float limitFactor);

void processVisibleStars(
    const OctreeNode *node,
    StarProcesor& procesor,
    Eigen::Vector3d position,
    Eigen::Quaternionf orientation,
    float fovY,
    float aspectRatio,
    float limitingFactor);

void processVisibleDsos(
    const OctreeNode *node,
    DsoProcesor& procesor,
    const Eigen::Vector3d& obsPos,
    const Frustum::PlaneType *frustumPlanes,
    float limitFactor);

void processVisibleDsos(
    const OctreeNode *node,
    DsoProcesor& procesor,
    Eigen::Vector3d position,
    Eigen::Quaternionf orientation,
    float fovY,
    float aspectRatio,
    float limitingFactor);

void processCloseStars(const OctreeNode *node, StarProcesor& procesor, const Eigen::Vector3d& obsPos, double bRadius);
void processCloseDsos(const OctreeNode *node, DsoProcesor& procesor, const Eigen::Vector3d& obsPos, double bRadius);
