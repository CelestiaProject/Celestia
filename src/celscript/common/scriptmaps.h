#pragma once

#include <map>
#include <string>
#include <celutil/color.h>

namespace celestia::scripts
{

// String to flag mappings
typedef std::map<std::string, uint32_t> FlagMap;
typedef std::map<std::string, uint64_t> FlagMap64;
typedef std::map<std::string, Color*>   ColorMap;

class ScriptMaps
{
 public:
    ScriptMaps();
    ~ScriptMaps() = default;
    ScriptMaps(const ScriptMaps&) = delete;
    ScriptMaps(ScriptMaps&&) = delete;
    ScriptMaps& operator=(const ScriptMaps&) = delete;
    ScriptMaps& operator=(ScriptMaps&&) = delete;

    FlagMap64 RenderFlagMap;
    FlagMap   LabelFlagMap;
    FlagMap64 LocationFlagMap;
    FlagMap   BodyTypeMap;
    FlagMap   OverlayElementMap;
    FlagMap   OrbitVisibilityMap;
    ColorMap  LineColorMap;
    ColorMap  LabelColorMap;
};

} // end namespace celestia::scripts
