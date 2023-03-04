#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <string_view>

class Color;

namespace celestia::scripts
{

constexpr inline std::size_t FlagMapNameLength = 20;

// String to flag mappings
using FlagMap   = std::map<std::string_view, std::uint32_t>;
using FlagMap64 = std::map<std::string_view, std::uint64_t>;
using ColorMap  = std::map<std::string_view, Color*>;

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
