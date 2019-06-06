#include <celutil/debug.h>
#include "asterism.h"
#include "constellation.h"
#include "namedb.h"

using namespace std;

uint32_t NameDatabase::getNameCount() const
{
    return m_nameIndex.size();
}

bool NameDatabase::add(NameInfo &info, bool overwrite)
{
    if (!overwrite && (info.getCanon().empty() || m_nameIndex.find(info.getCanon()) != m_nameIndex.end()))
        return false;
    m_nameIndex[info.getCanon()] = info;
    if (!info.getLocalized().empty())
        m_localizedIndex[info.getLocalized()] = info;
    return true;
}

void NameDatabase::erase(const Name& name)
{
    NameMap::iterator it = m_nameIndex.find(name);
    if (it == m_nameIndex.end())
        return;
    m_localizedIndex.erase(it->second.getLocalized());
    m_nameIndex.erase(name);
}

NameInfo *NameDatabase::getNameInfo(const Name& name, bool greek) const
{
    NameMap::const_iterator iter = m_nameIndex.find(name);

    if (iter != m_nameIndex.end())
        iter->second;
    if (greek)
    {
        string fname = ReplaceGreekLetterAbbr(name.str());
        iter = m_nameIndex.find(fname);
        if (iter != m_nameIndex.end())
            return (NameInfo*)&iter->second;
    }
    return nullptr;
}

AstroObject *NameDatabase::getObjectByName(const Name& name, bool greek) const
{
    NameInfo *info = getNameInfo(name, greek);
    if (info == nullptr)
        return nullptr;
    return info->getObject();
}

std::vector<Name> NameDatabase::getCompletion(const std::string& name, bool greek) const
{
    if (greek)
    {
        auto compList = getGreekCompletion(name);
        compList.push_back(name);
        return getCompletion(compList);
    }

    std::vector<Name> completion;
    string fname;
    if (greek)
        fname = ReplaceGreekLetterAbbr(name);
    else fname = name;
    int name_length = UTF8Length(fname);

    for (NameMap::const_iterator iter = m_nameIndex.begin(); iter != m_nameIndex.end(); ++iter)
    {
        if (!UTF8StringCompare(iter->first.str(), fname, name_length, true))
        {
            completion.push_back(iter->first);
        }
    }
    return completion;
}

std::vector<Name> NameDatabase::getCompletion(const std::vector<string> &list) const
{
    std::vector<Name> completion;
    for (const auto &n : list)
    {
        for (const auto &nn : getCompletion(n, false))
            completion.emplace_back(nn);
    }
    return completion;
}

AstroObject *NameDatabase::findObjectByName(const Name& name, bool greek) const
{
    AstroObject *ret = nullptr;

    std::string priName = name.str();
    std::string altName;

    // See if the name is a Bayer or Flamsteed designation
    std::string::size_type pos  = name.str().find(' ');
    if (pos != 0 && pos != std::string::npos && pos < name.length() - 1)
    {
        std::string prefix(name.str(), 0, pos);
        std::string conName(name.str(), pos + 1, std::string::npos);
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

    ret = getObjectByName(priName, greek);
    if (ret != nullptr)
        return ret;

    priName += " A";  // try by appending an A
    ret = getObjectByName(priName, greek);
    if (ret != nullptr)
        return ret;

    // If the first search failed, try using the alternate name
    if (altName.length() != 0)
    {
        ret = getObjectByName(altName, greek);
        if (ret == nullptr)
        {
            altName += " A";
            ret = getObjectByName(altName, greek);
        }   // Intentional fallthrough.
    }

    return ret;
}
