#include <celutil/debug.h>
#include "asterism.h"
#include "name.h"

using namespace std;

uint32_t NameDatabase::getNameCount() const
{
    return nameIndex.size();
}

void NameDatabase::add(const uint32_t catalogNumber, const std::string& name, bool replaceGreek)
{
    if (name.length() != 0)
    {
#ifdef DEBUG
        uint32_t tmp;
        if ((tmp = getCatalogNumberByName(name)) != InvalidCatalogNumber)
            DPRINTF(2,"Duplicated name '%s' on object with catalog numbers: %d and %d\n", name.c_str(), tmp, catalogNumber);
#endif
        // Add the new name
        //nameIndex.insert(NameIndex::value_type(name, catalogNumber));
        std::string fname = ReplaceGreekLetterAbbr(name);

        nameIndex[fname] = catalogNumber;
        numberIndex.insert(NumberIndex::value_type(catalogNumber, fname));
    }
}
void NameDatabase::erase(const uint32_t catalogNumber)
{
    numberIndex.erase(catalogNumber);
}

uint32_t NameDatabase::getCatalogNumberByName(const std::string& name) const
{
    NameIndex::const_iterator iter = nameIndex.find(name);

    if (iter == nameIndex.end())
    {
        iter = nameIndex.find(ReplaceGreekLetterAbbr(name));
        if (iter == nameIndex.end())
            return InvalidCatalogNumber;
    }
    return iter->second;
}

// Return the first name matching the catalog number or end()
// if there are no matching names.  The first name *should* be the
// proper name of the OBJ, if one exists. This requires the
// OBJ name database file to have the proper names listed before
// other designations.  Also, the STL implementation must
// preserve this order when inserting the names into the multimap
// (not certain whether or not this behavior is in the STL spec.
// but it works on the implementations I've tried so far.)
std::string NameDatabase::getNameByCatalogNumber(const uint32_t catalogNumber) const
{
    if (catalogNumber == InvalidCatalogNumber)
        return "";

    NumberIndex::const_iterator iter = numberIndex.lower_bound(catalogNumber);

    if (iter != numberIndex.end() && iter->first == catalogNumber)
        return iter->second;

    return "";
}


// Return the first name matching the catalog number or end()
// if there are no matching names.  The first name *should* be the
// proper name of the OBJ, if one exists. This requires the
// OBJ name database file to have the proper names listed before
// other designations.  Also, the STL implementation must
// preserve this order when inserting the names into the multimap
// (not certain whether or not this behavior is in the STL spec.
// but it works on the implementations I've tried so far.)
NameDatabase::NumberIndex::const_iterator NameDatabase::getFirstNameIter(const uint32_t catalogNumber) const
{
    NumberIndex::const_iterator iter = numberIndex.lower_bound(catalogNumber);

    if (iter == numberIndex.end() || iter->first != catalogNumber)
        return getFinalNameIter();
    else
        return iter;
}

NameDatabase::NumberIndex::const_iterator NameDatabase::getFinalNameIter() const
{
    return numberIndex.end();
}

std::vector<std::string> NameDatabase::getCompletion(const std::string& name, bool greek) const
{
    if (greek)
    {
        auto compList = getGreekCompletion(name);
        compList.push_back(name);
        return getCompletion(compList);
    }

    std::vector<std::string> completion;
    int name_length = UTF8Length(name);

    for (NameIndex::const_iterator iter = nameIndex.begin(); iter != nameIndex.end(); ++iter)
    {
        if (!UTF8StringCompare(iter->first, name, name_length, true))
        {
            completion.push_back(iter->first);
        }
    }
    return completion;
}

std::vector<std::string> NameDatabase::getCompletion(const std::vector<std::string> &list) const
{
    std::vector<std::string> completion;
    for (const auto &n : list)
    {
        for (const auto &nn : getCompletion(n, false))
            completion.emplace_back(nn);
    }
    return completion;
}

uint32_t NameDatabase::findCatalogNumberByName(const std::string& name) const
{
    uint32_t catalogNumber = getCatalogNumberByName(name);
    if (catalogNumber != AstroCatalog::InvalidIndex)
        return catalogNumber;

    std::string priName   = name;
    std::string altName;

    // See if the name is a Bayer or Flamsteed designation
    std::string::size_type pos  = name.find(' ');
    if (pos != 0 && pos != std::string::npos && pos < name.length() - 1)
    {
        std::string prefix(name, 0, pos);
        std::string conName(name, pos + 1, std::string::npos);
        Constellation* con  = Constellation::getConstellation(conName);
        if (con != nullptr)
        {
            char digit  = ' ';
            int len = prefix.length();

            // If the first character of the prefix is a letter
            // and the last character is a digit, we may have
            // something like 'Alpha2 Cen' . . . Extract the digit
            // before trying to match a Greek letter.
            if (len > 2 && isalpha(prefix[0]) && isdigit(prefix[len - 1]))
            {
                --len;
                digit = prefix[len];
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
                    priName = letter + digit + ' ' + con->getAbbreviation();
                }
            }
            else
            {
                // Something other than a Bayer designation
                priName = prefix + ' ' + con->getAbbreviation();
            }
        }
    }

    catalogNumber = getCatalogNumberByName(priName);
    if (catalogNumber != AstroCatalog::InvalidIndex)
        return catalogNumber;

    priName += " A";  // try by appending an A
    catalogNumber = getCatalogNumberByName(priName);
    if (catalogNumber != AstroCatalog::InvalidIndex)
        return catalogNumber;

    // If the first search failed, try using the alternate name
    if (altName.length() != 0)
    {
        catalogNumber = getCatalogNumberByName(altName);
        if (catalogNumber == AstroCatalog::InvalidIndex)
        {
            altName += " A";
            catalogNumber = getCatalogNumberByName(altName);
        }   // Intentional fallthrough.
    }

    return catalogNumber;
}
