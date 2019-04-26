
#include <cstdlib>
#include <celengine/astrooctree.h>

using namespace std;
using namespace Eigen;

const int n_objs = 12;

int main()
{
    cout << __LINE__ << "\n";
    OctreeNode root(Vector3d(0, 0, 0), 100000000, 2);
    cout << __LINE__ << "\n";
    LuminousObject *objs = new LuminousObject[n_objs];
    cout << __LINE__ << "\n";
    for (int i = 0; i < n_objs; i++)
    {
        objs[i].setAbsoluteMagnitude(i);
        objs[i].setPosition(Vector3d(i * 0.5, 0, 0));
//         cout << i << endl;
        root.insertObject(&objs[i]);
        if (objs[i].getOctreeNode() == nullptr)
        {
            cout << "Inserted object [" << i << "] without octree!!!\n";
            break;
        }
    }
    cout << __LINE__ << "\n";
    for (int i = 0; i < n_objs; i++)
    {
        if (objs[i].getOctreeNode() == nullptr)
        {
            cout << "Object [" << i << "] without octree!!!\n";
            break;
        }
    }
//    root.dump(0);
//    root.normalize();
    cout << __LINE__ << "\n";
    root.check(-1000, 0, false);
    cout << __LINE__ << "\n";
    root.dump(0);
    cout << "Manipulating test\n";
//    root.dump(0);
    objs[2].setAbsoluteMagnitude(-1);
    cout << __LINE__ << "\n";
    objs[n_objs - n_objs/2].setAbsoluteMagnitude(-1);
    objs[1].setPosition(Vector3d(n_objs-1, 1, 0));
    cout << __LINE__ << "\n";
    objs[1].setAbsoluteMagnitude(-1);
    root.dump(0);
    root.check(-1000, 0, false);
    cout << __LINE__ << "\n";
    objs[1].getOctreeNode()->removeObject(&objs[1]);
    cout << __LINE__ << "\n";
    root.dump(0);
    objs[4].getOctreeNode()->removeObject(&objs[4]);
    root.dump(0);
    return 0;
}
