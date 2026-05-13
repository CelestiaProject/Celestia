// nebula.h
//
// Copyright (C) 2003-present, the Celestia Development Team
// Copyright (C) 2003, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>

#include "deepskyobj.h"
#include "meshmanager.h"
#include "renderflags.h"

namespace celestia::util
{
class AssociativeArray;
}

class Nebula : public DeepSkyObject
{
public:
    Nebula() = default;

    const char* getType() const override;
    void setType(const std::string&) override;
    std::string getDescription() const override;

    // pick: the preconditional sphere-ray intersection test is enough for now
    bool load(const celestia::util::AssociativeArray*,
              const std::filesystem::path&,
              celestia::engine::GeometryPaths&,
              std::string_view) override;

    RenderFlags getRenderMask() const override;
    RenderLabels getLabelMask() const override;

    void setGeometry(celestia::engine::GeometryHandle);
    celestia::engine::GeometryHandle getGeometry() const;

    DeepSkyObjectType getObjType() const override;

    enum class Type
    {
        NotDefined         = 0,
        Emission           = 1,  // includes Herbig–Haro objects and misc. emission nebula not listed below
        Reflection         = 2,  // includes misc. reflection nebula not listed below
        Dark               = 3,
        Planetary          = 4,
        SupernovaRemnant   = 5,
        HII_Region         = 6,
        Protoplanetary     = 7
    };

    Type getNebulaType() const;

private:
    celestia::engine::GeometryHandle geometry{ celestia::engine::GeometryHandle::Invalid };
    Type type{ Type::NotDefined };
};
