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
#include <string>
#include <string_view>

#include <celcompat/filesystem.h>
#include <celutil/reshandle.h>
#include "deepskyobj.h"
#include "renderflags.h"

class Nebula : public DeepSkyObject
{
public:
    Nebula() = default;

    const char* getType() const override;
    void setType(const std::string&) override;
    std::string getDescription() const override;

    // pick: the preconditional sphere-ray intersection test is enough for now
    bool load(const AssociativeArray*, const fs::path&, std::string_view) override;

    RenderFlags getRenderMask() const override;
    RenderLabels getLabelMask() const override;

    void setGeometry(ResourceHandle);
    ResourceHandle getGeometry() const;

    DeepSkyObjectType getObjType() const override;

    enum NebulaType
    {
        Emissive           = 0,
        Reflective         = 1,
        Dark               = 2,
        Planetary          = 3,
        Galactic           = 4,
        SupernovaRemnant   = 5,
        Bright_HII_Region  = 6,
        NotDefined         = 7
    };

private:
    ResourceHandle geometry{ InvalidResource };
};
