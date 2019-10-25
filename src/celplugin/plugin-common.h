// plugin-common.h
//
// Copyright (C) 2019, Celestia Development Team
//
// Common definitions for application and module sides
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <cstdint>

#define CELESTIA_PLUGIN_ENTRY_NAME get_celestia_plugin_info
#define CELESTIA_PLUGIN_ENTRY_NAME_STR "get_celestia_plugin_info"

#ifdef _WIN32
#define CELESTIA_PLUGIN_EXPORTABLE extern "C" __declspec(dllexport)
#else
#define CELESTIA_PLUGIN_EXPORTABLE extern "C" __attribute__ ((visibility ("default")))
#endif

#define CELESTIA_PLUGIN_ENTRYPOINT CELESTIA_PLUGIN_EXPORTABLE celestia::plugin::PluginInfo* CELESTIA_PLUGIN_ENTRY_NAME

namespace celestia
{
namespace plugin
{

constexpr const uint16_t CurrentAPIVersion = 0x0107;

enum PluginType : uint32_t
{
    TestPlugin  = 0,
    Scripting   = 0x0001,
    Rendering   = 0x0002,
    AudioInput  = 0x0010,
    AudioOutput = 0x0020,
    VideoInput  = 0x0040,
    VideoOutput = 0x0080,
};

#pragma pack(push,1)
struct PluginInfo
{
    PluginInfo() = delete;
    ~PluginInfo() = default;
    PluginInfo(const PluginInfo&) = default;
    PluginInfo(PluginInfo&&) = default;
    PluginInfo& operator=(const PluginInfo&) = default;
    PluginInfo& operator=(PluginInfo&&) = default;

    PluginInfo(uint16_t _APIVersion, PluginType _Type, const char *_ID) :
        APIVersion(_APIVersion),
        Type(_Type),
        ID(_ID)
    {}
    PluginInfo(PluginType _Type, const char *_ID) :
        PluginInfo(CurrentAPIVersion, _Type, _ID)
    {}

    uint16_t    APIVersion;
    uint16_t    Reserved1;
    PluginType  Type;
    uint32_t    Reserved2;
    const char *ID;
};
#pragma pack(pop)

}
}
