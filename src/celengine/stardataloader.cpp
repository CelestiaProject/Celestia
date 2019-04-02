
#include <iostream>
#include <celutil/util.h>
#include <celutil/bytes.h>
#include <fmt/printf.h>
#include "parseobject.h"
#include "stardataloader.h"
#include "astrocat.h"
#include "star.h"
#include "astrodb.h"

#undef NDEBUG

using namespace std;

/*! Load an STC file with star definitions. Each definition has the form:
 *
 *  [disposition] [object type] [catalog number] [name]
 *  {
 *      [properties]
 *  }
 *
 *  Disposition is either Add, Replace, or Modify; Add is the default.
 *  Object type is either Star or Barycenter, with Star the default
 *  It is an error to omit both the catalog number and the name.
 *
 *  The dispositions are slightly more complicated than suggested by
 *  their names. Every star must have an unique catalog number. But
 *  instead of generating an error, Adding a star with a catalog
 *  number that already exists will actually replace that star. Here
 *  are how all of the possibilities are handled:
 *
 *  <name> or <number> already exists:
 *  Add <name>        : new star
 *  Add <number>      : replace star
 *  Replace <name>    : replace star
 *  Replace <number>  : replace star
 *  Modify <name>     : modify star
 *  Modify <number>   : modify star
 *
 *  <name> or <number> doesn't exist:
 *  Add <name>        : new star
 *  Add <number>      : new star
 *  Replace <name>    : new star
 *  Replace <number>  : new star
 *  Modify <name>     : error
 *  Modify <number>   : error
 */
bool StcDataLoader::load(istream &in)
{
    size_t successCount = 0;
    Tokenizer tokenizer(&in);
    Parser parser(&tokenizer);

    bindtextdomain(resourcePath.c_str(), resourcePath.c_str()); // domain name is the same as resource path

    while (tokenizer.nextToken() != Tokenizer::TokenEnd)
    {
        bool isStar = true;

        // Parse the disposition--either Add, Replace, or Modify. The disposition
        // may be omitted. The default value is Add.
        DataDisposition disposition = DataDisposition::Add;
        if (tokenizer.getTokenType() == Tokenizer::TokenName)
        {
            if (tokenizer.getNameValue() == "Modify")
            {
//                 clog << "New entry with disposition: Modify.\n";
                disposition = DataDisposition::Modify;
                tokenizer.nextToken();
            }
            else if (tokenizer.getNameValue() == "Replace")
            {
//                 clog << "New entry with disposition: Replace.\n";
                disposition = DataDisposition::Replace;
                tokenizer.nextToken();
            }
            else if (tokenizer.getNameValue() == "Add")
            {
//                 clog << "New entry with disposition: Add.\n";
                disposition = DataDisposition::Add;
                tokenizer.nextToken();
            }// else clog << "New entry with unrecognized disposition type: " << tokenizer.getNameValue() << endl;
        }// else clog << "New entry with default disposition: Add.\n";

        // Parse the object type--either Star or Barycenter. The object type
        // may be omitted. The default is Star.
        if (tokenizer.getTokenType() == Tokenizer::TokenName)
        {
            if (tokenizer.getNameValue() == "Star")
            {
                isStar = true;
            }
            else if (tokenizer.getNameValue() == "Barycenter")
            {
                isStar = false;
            }
            else
            {
                stcError(tokenizer, "unrecognized object type");
                return false;
            }
            tokenizer.nextToken();
        }

//         clog << " Is star: " << isStar << endl;
        // Parse the catalog number; it may be omitted if a name is supplied.
        AstroCatalog::IndexNumber catalogNumber = AstroCatalog::InvalidIndex;
        if (tokenizer.getTokenType() == Tokenizer::TokenNumber)
        {
            catalogNumber = (AstroCatalog::IndexNumber) tokenizer.getNumberValue();
            tokenizer.nextToken();
        }

        string objName;
        string firstName;
        if (tokenizer.getTokenType() == Tokenizer::TokenString)
        {
            // A star name (or names) is present
            objName    = tokenizer.getStringValue();
            tokenizer.nextToken();
            if (!objName.empty())
            {
                string::size_type next = objName.find(':', 0);
                firstName = objName.substr(0, next);
            }
        }
        // If catalog number is absent, try to find star by name
        if (catalogNumber == AstroCatalog::InvalidIndex)
            catalogNumber = m_db->nameToIndex(firstName);

        bool isNewStar = false;
        bool ok = true;
        Star* star = nullptr;

        switch (disposition)
        {
        case DataDisposition::Add:
            // Automatically generate a catalog number for the star if one isn't
            // supplied.
            if (catalogNumber != AstroCatalog::InvalidIndex)
            {
                star = m_db->getStar(catalogNumber);
                if (star == nullptr)
                {
                    star = new Star();
                    star->setIndex(catalogNumber);
                    if (!m_db->addStar(star))
                    {
                        clog << __FILE__ << "(" << __LINE__ << "): Cannot add star wit nr " << catalogNumber << "!\n";
                        ok = false;
                    }
                    isNewStar = true;
                }
            }
            else
            {
                star = new Star();
                if (!m_db->addStar(star))
                {
                    clog << __FILE__ << "(" << __LINE__ << "): Cannot add new star!\n";
                    ok = false;
                }
                else
                    catalogNumber = star->getIndex();
            }
            break;

        case DataDisposition::Replace:
            if (catalogNumber == AstroCatalog::InvalidIndex)
            {
                if (!firstName.empty())
                {
                    catalogNumber = m_db->nameToIndex(firstName);
                }
            }

            if (catalogNumber == AstroCatalog::InvalidIndex)
            {
                star = new Star();
//                 clog << " Replace: About to add star with nr: " << catalogNumber << endl;
                if (m_db->addStar(star))
                {
                    catalogNumber = star->getIndex();
//                     clog << " Replace: Added star with nr " << catalogNumber << endl;
                }
                else
                {
//                     clog << " Unable to add star with index " << star->getIndex() << endl;
                    delete star;
                    ok = false;
                }
                isNewStar = true;
            }
            else
            {
                star = m_db->getStar(catalogNumber);
                if (star == nullptr)
                {
                    star = new Star();
                    star->setIndex(catalogNumber);
//                     clog << " Replace 2: About to add star with nr: " << catalogNumber << endl;
                    if (!m_db->addStar(star))
                    {
//                         clog << " Unable to add star with index " << star->getIndex() << endl;
                        delete star;
                        ok = false;
                    }// else clog << " Replace 2: Added star with nr " << catalogNumber << endl;
                }
            }
            break;

        case DataDisposition::Modify:
            // If no catalog number was specified, try looking up the star by name
            if (catalogNumber == AstroCatalog::InvalidIndex && !firstName.empty())
            {
                catalogNumber = m_db->nameToIndex(firstName);
            }

            if (catalogNumber == AstroCatalog::InvalidIndex)
            {
                clog << __FILE__ << "(" << __LINE__ << "): No star index to modify.\n";
                ok = false;
                break;
            }
            star = m_db->getStar(catalogNumber);
            if (star == nullptr)
            {
                clog << __FILE__ << "(" << __LINE__ << "): Star nr " << catalogNumber << " not found.\n";
                ok = false;
            }
            break;
        }

//         For backward compatibility: index within HIP numbers range is supposed to be valid HIP number
        if (ok && catalogNumber < HipparcosAstroCatalog::MaxCatalogNumber)
            m_db->addCatalogNumber(catalogNumber, AstroDatabase::Hipparcos, catalogNumber);

        tokenizer.pushBack();

        Value* starDataValue = parser.readValue();
        if (starDataValue == nullptr)
        {
            clog << __FILE__ << "(" << __LINE__ << "): Error reading star.\n";
            return false;
        }

        if (starDataValue->getType() != Value::HashType)
        {
//            clog << " Bad star definition.\n";
            delete starDataValue;
            return false;
        }
        Hash* starData = starDataValue->getHash();

/*        if (isNewStar)
            star = new Star();*/

        if (ok)
        {
//             clog << " About to create star with nr " << star->getIndex() << endl;
            ok = Star::createStar(star, disposition, starData, resourcePath, !isStar, m_db);
            if (!ok)
                clog << __FILE__ << "(" << __LINE__ << "): Creation of star failed!\n";
            star->loadCategories(starData, disposition, resourcePath);
        }// else clog << " Errors while preparing star, skipping creation.\n";

        if (starDataValue != nullptr)
            delete starDataValue;

        if (ok)
        {
            if (!objName.empty())
            {
                m_db->addNames(catalogNumber, objName);
            }
            successCount++;
        }
        else
        {
             clog << "Bad star definition -- will continue parsing file.\n";
        }
//         clog << "End parsing entry.\n";
    }

    clog << "Successfully parsed " << successCount << " stars or barycenters.\n";

    return true;
}

const char StarBinDataLoader::FILE_HEADER[] = "CELSTARS";

bool StarBinDataLoader::load(istream& in)
{
    uint32_t nStarsInFile = 0;

    // Verify that the star database file has a correct header
    {
        int headerLength = strlen(FILE_HEADER);
        char* header = new char[headerLength];
        in.read(header, headerLength);
        if (strncmp(header, FILE_HEADER, headerLength)) {
            fmt::fprintf(cerr, "Bad file header!\n");
            delete[] header;
            return false;
        }
        delete[] header;
    }

    // Verify the version
    {
        uint16_t version;
        in.read((char*) &version, sizeof version);
        LE_TO_CPU_INT16(version, version);
        if (version != 0x0100)
        {
            fmt::fprintf(cerr, "Bad file version!\n");
            return false;
        }
    }

    // Read the star count
    in.read((char *) &nStarsInFile, sizeof nStarsInFile);
    LE_TO_CPU_INT32(nStarsInFile, nStarsInFile);
    if (!in.good())
        return false;

    for (unsigned int i = 0; i < nStarsInFile; i++)
    {
        uint32_t catNo = 0;
        float x = 0.0f, y = 0.0f, z = 0.0f;
        int16_t absMag;
        uint16_t spectralType;

        in.read((char *) &catNo, sizeof catNo);
        LE_TO_CPU_INT32(catNo, catNo);
        in.read((char *) &x, sizeof x);
        LE_TO_CPU_FLOAT(x, x);
        in.read((char *) &y, sizeof y);
        LE_TO_CPU_FLOAT(y, y);
        in.read((char *) &z, sizeof z);
        LE_TO_CPU_FLOAT(z, z);
        in.read((char *) &absMag, sizeof absMag);
        LE_TO_CPU_INT16(absMag, absMag);
        in.read((char *) &spectralType, sizeof spectralType);
        LE_TO_CPU_INT16(spectralType, spectralType);
        if (in.bad())
        break;

        Star *star = new Star();
        star->setPosition(x, y, z);
        star->setAbsoluteMagnitude((float) absMag / 256.0f);

        StarDetails* details = nullptr;
        StellarClass sc;
        if (sc.unpack(spectralType))
            details = StarDetails::GetStarDetails(sc);

        if (details == nullptr)
        {
            fmt::fprintf(cerr, _("Bad spectral type in star database, star #%u\n"), star->getIndex());
            return false;
        }

        star->setDetails(details);
        star->setIndex(catNo);
        if (m_db->addStar(star))
            ;//clog << "BinData: Added star nr " << star->getIndex() << endl;
        else
            clog << "BinData: unable to add star nr " << star->getIndex() << endl;
    }

    if (in.bad())
        return false;

    DPRINTF(0, "StarBinDataLoader::load stars count: %d\n", nStarsInFile);
    fmt::fprintf(clog, _("%d stars in binary database\n"), m_db->getStars().size());

    // Create the temporary list of stars sorted by catalog number; this
    // will be used to lookup stars during file loading. After loading is
    // complete, the stars are sorted into an octree and this list gets
    // replaced.

    return true;
}
