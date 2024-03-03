#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <string_view>

class Color;
enum class BodyClassification : std::uint32_t;

namespace celestia::scripts
{

constexpr inline std::size_t FlagMapNameLength = 20;

// String to flag mappings
template<typename T>
using ScriptMap = std::map<std::string_view, T>;

struct ScriptMaps
{
    ScriptMaps();
    ~ScriptMaps() = default;
    ScriptMaps(const ScriptMaps&) = delete;
    ScriptMaps(ScriptMaps&&) = delete;
    ScriptMaps& operator=(const ScriptMaps&) = delete;
    ScriptMaps& operator=(ScriptMaps&&) = delete;

    ScriptMap<std::uint64_t>      RenderFlagMap;
    ScriptMap<std::uint32_t>      LabelFlagMap;
    ScriptMap<std::uint64_t>      LocationFlagMap;
    ScriptMap<BodyClassification> BodyTypeMap;
    ScriptMap<std::uint32_t>      OverlayElementMap;
    ScriptMap<std::uint32_t>      OrbitVisibilityMap;
    ScriptMap<Color*>             LineColorMap;
    ScriptMap<Color*>             LabelColorMap;
};

} // end namespace celestia::scripts
