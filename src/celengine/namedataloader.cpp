
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

        // Iterate through the string for names delimited
        // by ':', and insert them into the star database. Note that
        // db->add() will skip empty names.
        string::size_type startPos = 0;
        while (startPos != string::npos)
        {
            ++startPos;
            string::size_type next = name.find(':', startPos);
            string::size_type length = string::npos;

            if (next != string::npos)
                length = next - startPos;

            m_db->addName(catalogNumber, name.substr(startPos, length));
            startPos = next;
        }
    }

    return !failed;
}
