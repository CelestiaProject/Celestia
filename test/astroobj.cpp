
#include <celengine/astroobj.h>

using namespace std;

void objName(AstroObject *o)
{
    cout << "Has name: " << o->hasName() << endl;
    cout << "Has localized name: " << o->hasLocalizedName() << endl;
    cout << o->getName().str() << endl;
    cout << o->getLocalizedName().str() << endl;
}

void dump(NameInfo *i)
{
    cout << i->getCanon().str() << endl;
    cout << i->getLocalized().str() << endl;
    cout << i->getCanon().ptr() << endl;
    cout << i->getLocalized().ptr() << endl;
}

int main()
{
    AstroObject obj;
    obj.addName(string("Some name"), string(""), true, false);
    objName(&obj);
    objName(&obj);
}
