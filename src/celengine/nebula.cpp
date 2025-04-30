// nebula.cpp
//
// Copyright (C) 2003, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <algorithm>
#include <celmath/mathlib.h>
#include <celmath/vecgl.h>
#include <celutil/associativearray.h>
#include <celutil/fsutils.h>
#include <celutil/gettext.h>
#include <celutil/logger.h>
#include <celutil/stringutils.h>
#include <fmt/format.h>
#include "meshmanager.h"
#include "nebula.h"
#include "rendcontext.h"
#include "render.h"

namespace engine = celestia::engine;
namespace util = celestia::util;

using namespace std::string_view_literals;
using util::GetLogger;

namespace
{
struct NebulaTypeName
{
    const char* name;
    Nebula::Type type;
};

constexpr std::array NebulaTypeNames =
{
    NebulaTypeName{ " ", Nebula::Type::NotDefined },
    NebulaTypeName{ "Emission", Nebula::Type::Emission },
    NebulaTypeName{ "Reflection",  Nebula::Type::Reflection },
    NebulaTypeName{ "Dark",  Nebula::Type::Dark },
    NebulaTypeName{ "Planetary",  Nebula::Type::Planetary },
    NebulaTypeName{ "SupernovaRemnant",  Nebula::Type::SupernovaRemnant },
    NebulaTypeName{ "HII_Region", Nebula::Type::HII_Region },
    NebulaTypeName{ "Protoplanetary", Nebula::Type::Protoplanetary },
};

}

const char*
Nebula::getType() const
{
    return NebulaTypeNames[static_cast<std::size_t>(type)].name;
}

void
Nebula::setType(const std::string& typeStr)
{
    type = Nebula::Type::NotDefined;
    auto iter = std::find_if(std::begin(NebulaTypeNames), std::end(NebulaTypeNames),
                             [&](const NebulaTypeName& n) { return compareIgnoringCase(n.name, typeStr) == 0; });
    if (iter != std::end(NebulaTypeNames))
        type = iter->type;
}

std::string
Nebula::getDescription() const
{
    return fmt::format(_("Nebula: {}"), getType());
}

ResourceHandle
Nebula::getGeometry() const
{
    return geometry;
}

void
Nebula::setGeometry(ResourceHandle _geometry)
{
    geometry = _geometry;
}

DeepSkyObjectType
Nebula::getObjType() const
{
    return DeepSkyObjectType::Nebula;
}

bool
Nebula::load(const util::AssociativeArray* params, const fs::path& resPath, std::string_view name)
{
    if (const std::string* typeName = params->getString("Type"); typeName != nullptr)
        setType(*typeName);
    
    if (const std::string* t = params->getString("Mesh"); t != nullptr)
    {
        auto geometryFileName = util::U8FileName(*t);
        if (!geometryFileName.has_value())
        {
            GetLogger()->error("Invalid filename in Mesh\n");
            return false;
        }

        ResourceHandle geometryHandle =
            engine::GetGeometryManager()->getHandle(engine::GeometryInfo(*geometryFileName, resPath));
        setGeometry(geometryHandle);
    }

    return DeepSkyObject::load(params, resPath, name);
}

RenderFlags
Nebula::getRenderMask() const
{
    return RenderFlags::ShowNebulae;
}

RenderLabels
Nebula::getLabelMask() const
{
    return RenderLabels::NebulaLabels;
}

Nebula::Type
Nebula::getNebulaType() const
{
    return type;
}
