#pragma once

#include <cstddef>
#include <map>
#include <string_view>
#include <celutil/color.h>

namespace celestia::scripts
{

constexpr inline std::size_t FlagMapNameLength = 20;

// String to flag mappings
typedef std::map<std::string_view, uint32_t> FlagMap;
typedef std::map<std::string_view, uint64_t> FlagMap64;
typedef std::map<std::string_view, Color*>   ColorMap;

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
