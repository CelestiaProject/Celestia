#include "dsodbbuilder.h"

#include <cmath>
#include <cstdint>

#include <celastro/astro.h>
#include <celcompat/numbers.h>
#include <celutil/logger.h>
#include <celutil/stringutils.h>
#include <celutil/tokenizer.h>
#include "category.h"
#include "deepskyobj.h"
#include "dsodb.h"
#include "dsooctree.h"
#include "galaxy.h"
#include "globular.h"
#include "name.h"
#include "nebula.h"
#include "octreebuilder.h"
#include "opencluster.h"
#include "parser.h"
#include "value.h"

using celestia::util::GetLogger;

namespace astro = celestia::astro;
namespace engine = celestia::engine;
namespace numbers = celestia::numbers;

namespace
{

constexpr float DSO_OCTREE_MAGNITUDE = 8.0f;

} // end unnamed namespace

DSODatabaseBuilder::~DSODatabaseBuilder() = default;

bool
DSODatabaseBuilder::load(std::istream& in, const fs::path& resourcePath)
{
    Tokenizer tokenizer(&in);
    Parser    parser(&tokenizer);

#ifdef ENABLE_NLS
    std::string s = resourcePath.string();
    const char *d = s.c_str();
    bindtextdomain(d, d); // domain name is the same as resource path
#endif

    while (tokenizer.nextToken() != Tokenizer::TokenEnd)
    {
        std::string objType;
        if (auto tokenValue = tokenizer.getNameValue(); tokenValue.has_value())
        {
            objType = *tokenValue;
        }
        else
        {
            GetLogger()->error("Error parsing deep sky catalog file.\n");
            return false;
        }

        auto objCatalogNumber = static_cast<AstroCatalog::IndexNumber>(m_dsos.size());

        tokenizer.nextToken();
        std::string objName;
        if (auto tokenValue = tokenizer.getStringValue(); tokenValue.has_value())
        {
            objName = *tokenValue;
        }
        else
        {
            GetLogger()->error("Error parsing deep sky catalog file: bad name.\n");
            return false;
        }

        const Value objParamsValue = parser.readValue();
        const Hash* objParams = objParamsValue.getHash();
        if (objParams == nullptr)
        {
            GetLogger()->error("Error parsing deep sky catalog entry {}\n", objName.c_str());
            return false;
        }

        std::unique_ptr<DeepSkyObject> obj;
        if (compareIgnoringCase(objType, "Galaxy") == 0)
            obj = std::make_unique<Galaxy>();
        else if (compareIgnoringCase(objType, "Globular") == 0)
            obj = std::make_unique<Globular>();
        else if (compareIgnoringCase(objType, "Nebula") == 0)
            obj = std::make_unique<Nebula>();
        else if (compareIgnoringCase(objType, "OpenCluster") == 0)
            obj = std::make_unique<OpenCluster>();

        if (!obj || !obj->load(objParams, resourcePath))
        {
            GetLogger()->warn("Bad Deep Sky Object definition--will continue parsing file.\n");
            continue;
        }

        UserCategory::loadCategories(obj.get(), *objParams, DataDisposition::Add, resourcePath.string());

        obj->setIndex(objCatalogNumber);
        m_dsos.push_back(std::move(obj));

        obj->setIndex(objCatalogNumber);

        if (objName.empty())
            continue;

        // Iterate through the string for names delimited
        // by ':', and insert them into the DSO database.
        // Note that db->add() will skip empty names.
        std::string::size_type startPos = 0;
        while (startPos != std::string::npos)
        {
            std::string::size_type next    = objName.find(':', startPos);
            std::string::size_type length  = std::string::npos;
            if (next != std::string::npos)
            {
                length = next - startPos;
                ++next;
            }

            m_names.emplace_back(objCatalogNumber, objName.substr(startPos, length));
            startPos   = next;
        }
    }

    return true;
}

std::unique_ptr<DSODatabase>
DSODatabaseBuilder::build()
{
    float absMag = astro::appToAbsMag(DSO_OCTREE_MAGNITUDE, DSO_OCTREE_ROOT_SIZE * numbers::sqrt3_v<float>);

    float avgAbsMag = calcAvgAbsMag();

    engine::DSOOctreeBuilder octreeBuilder(std::move(m_dsos), DSO_OCTREE_ROOT_SIZE, absMag);
    const auto indices = octreeBuilder.indices();
    auto namesDB = std::make_unique<NameDatabase>();
    for (auto& entry : m_names)
        namesDB->add(indices[entry.first], entry.second);

    for (auto& obj : octreeBuilder.objects())
        obj->setIndex(indices[obj->getIndex()]);

    return std::unique_ptr<DSODatabase>(new DSODatabase(octreeBuilder.build(),
                                                        std::move(namesDB),
                                                        avgAbsMag));
}

float
DSODatabaseBuilder::calcAvgAbsMag()
{
    // Kahan-Babuška summation (Neumaier 1974)
    double absMag = 0.0;
    double comp = 0.0;

    std::uint32_t n = 0;
    for (const auto& dso : m_dsos)
    {
        float dsoMag = dso->getAbsoluteMagnitude();

        // take only DSO's with realistic AbsMag entry
        // (> DSO_DEFAULT_ABS_MAGNITUDE) into account
        if (dsoMag < DSO_DEFAULT_ABS_MAGNITUDE)
            continue;

        ++n;
        double temp = absMag + static_cast<double>(dsoMag);
        if (std::abs(absMag) >= std::abs(dsoMag))
            comp += (absMag - temp) + dsoMag;
        else
            comp += (dsoMag - temp) + absMag;
        absMag = temp;
    }

    return static_cast<float>((absMag + comp) / n);
}
