
#include <iostream>
#include <celmath/mathlib.h>
#include <celmath/frustum.h>
#include <celengine/astrooctree.h>
#include <celengine/processoctree.h>

using namespace std;
using namespace Eigen;

int main()
{
    Frustum::PlaneType planes[5];
    Vector3d pos(0, 0, 0);
    OctreeNode node(Vector3d(0, 0, 0.6), 1);
    bool last_in = false;
    size_t nout = 0;
    for (int i = 0; i < 10000; i++)
    {
        Quaternionf rot(AngleAxisf(i / 5000, Vector3f::UnitX()));
        create5FrustumPlanes(planes, pos, rot, degToRad(90), 1);
        if (node.isInFrustum(planes))
            cout << "+";
        else
            cout << "-";
    }
    cout << endl;

    return 0;
}
