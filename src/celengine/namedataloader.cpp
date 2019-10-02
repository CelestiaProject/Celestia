
#include "namedataloader.h"

using namespace std;

bool NameDataLoader::load(istream& in)
{
    bool failed = false;
    string s;

    while (!failed)
    {
        uint32_t catalogNumber = AstroCatalog::InvalidIndex;

        in >> catalogNumber;
        if (in.eof())
            break;
        if (in.bad())
        {
            failed = true;
            break;
        }

        // in.get(); // skip a space (or colon);

        string name;
        getline(in, name);
        if (in.bad())
        {
            failed = true;
            break;
        }

        m_db->addNames(catalogNumber, name);
    }

    return !failed;
}
