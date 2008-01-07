//
// C++ Implementation: starname
//
// Description:
//
//
// Author: Toti <root@totibox>, (C) 2005
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include <celengine/constellation.h>
#include <celengine/starname.h>

using namespace std;


uint32 StarNameDatabase::findCatalogNumberByName(const string& name) const
{
    string priName   = name;
    string altName;

    // See if the name is a Bayer or Flamsteed designation
    string::size_type pos  = name.find(' ');
    if (pos != 0 && pos != string::npos && pos < name.length() - 1)
    {
        string prefix(name, 0, pos);
        string conName(name, pos + 1, string::npos);
        Constellation* con  = Constellation::getConstellation(conName);
        if (con != NULL)
        {
            char digit  = ' ';
            int len     = prefix.length();

            // If the first character of the prefix is a letter
            // and the last character is a digit, we may have
            // something like 'Alpha2 Cen' . . . Extract the digit
            // before trying to match a Greek letter.
            if (len > 2 && isalpha(prefix[0]) && isdigit(prefix[len - 1]))
            {
                --len;
                digit   = prefix[len];
            }

            // We have a valid constellation as the last part
            // of the name.  Next, we see if the first part of
            // the name is a greek letter.
            const string& letter = Greek::canonicalAbbreviation(string(prefix, 0, len));
            if (letter != "")
            {
                // Matched . . . this is a Bayer designation
                if (digit == ' ')
                {
                    priName  = letter + ' ' + con->getAbbreviation();
                    // If 'let con' doesn't match, try using
                    // 'let1 con' instead.
                    altName  = letter + '1' + ' ' + con->getAbbreviation();
                }
                else
                {
                    priName  = letter + digit + ' ' + con->getAbbreviation();
                }
            }
            else
            {
                // Something other than a Bayer designation
                priName  = prefix + ' ' + con->getAbbreviation();
            }
        }
    }

    uint32 catalogNumber   = getCatalogNumberByName(priName);
    if (catalogNumber != Star::InvalidCatalogNumber)
        return catalogNumber;

    priName        += " A";  // try by appending an A
    catalogNumber   = getCatalogNumberByName(priName);
    if (catalogNumber != Star::InvalidCatalogNumber)
        return catalogNumber;

    // If the first search failed, try using the alternate name
    if (altName.length() != 0)
    {
        catalogNumber   = getCatalogNumberByName(altName);
        if (catalogNumber == Star::InvalidCatalogNumber)
        {
            altName        += " A";
            catalogNumber   = getCatalogNumberByName(altName);
        }   // Intentional fallthrough.
    }

    return catalogNumber;
}


StarNameDatabase* StarNameDatabase::readNames(istream& in)
{
    StarNameDatabase* db = new StarNameDatabase();
    bool failed = false;
    string s;

    while (!failed)
    {
        uint32 catalogNumber = Star::InvalidCatalogNumber;

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

            db->add(catalogNumber, name.substr(startPos, length));
            startPos = next;
        }
    }

    if (failed)
    {
        delete db;
        return NULL;
    }
    else
    {
        return db;
    }
}
