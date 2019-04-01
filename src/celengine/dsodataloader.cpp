
#include "dsodataloader.h"
#include "astrocat.h"
#include "astrodb.h"
#include "deepskyobj.h"
#include "galaxy.h"
#include "globular.h"
#include "opencluster.h"
#include "nebula.h"

using namespace std;

bool DscDataLoader::load(istream& in)
{
    Tokenizer tokenizer(&in);
    Parser parser(&tokenizer);

    bindtextdomain(resourcePath.c_str(), resourcePath.c_str()); // domain name is the same as resource path

    while (tokenizer.nextToken() != Tokenizer::TokenEnd)
    {
        string objType;
        string objName;

        if (tokenizer.getTokenType() != Tokenizer::TokenName)
        {
            DPRINTF(0, "Error parsing deep sky catalog file.\n");
            return false;
        }
        objType = tokenizer.getNameValue();

        AstroCatalog::IndexNumber objCatalogNumber = AstroCatalog::InvalidIndex;
        if (tokenizer.getTokenType() == Tokenizer::TokenNumber)
        {
            objCatalogNumber       = (AstroCatalog::IndexNumber) tokenizer.getNumberValue();
            tokenizer.nextToken();
        }

        if (tokenizer.nextToken() != Tokenizer::TokenString)
        {
            DPRINTF(0, "Error parsing deep sky catalog file: bad name.\n");
            return false;
        }
        objName = tokenizer.getStringValue();

        Value* objParamsValue    = parser.readValue();
        if (objParamsValue == nullptr ||
            objParamsValue->getType() != Value::HashType)
        {
            DPRINTF(0, "Error parsing deep sky catalog entry %s\n", objName.c_str());
            return false;
        }

        Hash* objParams    = objParamsValue->getHash();
        assert(objParams != nullptr);

        DeepSkyObject* obj = nullptr;
        if (compareIgnoringCase(objType, "Galaxy") == 0)
            obj = new Galaxy();
        else if (compareIgnoringCase(objType, "Globular") == 0)
            obj = new Globular();
        else if (compareIgnoringCase(objType, "Nebula") == 0)
            obj = new Nebula();
        else if (compareIgnoringCase(objType, "OpenCluster") == 0)
            obj = new OpenCluster();

        if (obj != nullptr && obj->load(objParams, resourcePath))
        {
            obj->loadCategories(objParams, DataDisposition::Add, resourcePath);
            delete objParamsValue;

            obj->setIndex(objCatalogNumber);

            if (!m_db->addDSO(obj))
            {
                delete obj;
                return false;
            }

            // Iterate through the string for names delimited
            // by ':', and insert them into the DSO database.
            // Note that db->add() will skip empty names.
            string::size_type startPos   = 0;
            while (startPos != string::npos)
            {
                string::size_type next    = objName.find(':', startPos);
                string::size_type length  = string::npos;
                if (next != string::npos)
                {
                    length = next - startPos;
                    ++next;
                }
                string DSOName = objName.substr(startPos, length);
                m_db->addName(objCatalogNumber, DSOName);
                if (DSOName != _(DSOName.c_str()))
                    m_db->addName(objCatalogNumber, _(DSOName.c_str()));
                startPos   = next;
            }
        }
        else
        {
            DPRINTF(1, "Bad Deep Sky Object definition--will continue parsing file.\n");
            delete objParamsValue;
            return false;
        }
    }
    return true;
}
