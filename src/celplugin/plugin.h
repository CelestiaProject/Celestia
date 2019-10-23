#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <celcompat/filesystem.h>
#ifdef _WIN32
#include <windows.h>
#endif

#define CELESTIA_PLUGIN_ENTRY_NAME get_celestia_plugin_info
#define CELESTIA_PLUGIN_ENTRY_NAME_STR "get_celestia_plugin_info"

#ifdef _WIN32
#define CELESTIA_PLUGIN_EXPORTABLE extern "C" __declspec(dllexport)
#else
#define CELESTIA_PLUGIN_EXPORTABLE extern "C"
#endif

#define CELESTIA_PLUGIN_ENTRYPOINT CELESTIA_PLUGIN_EXPORTABLE PluginInfo* CELESTIA_PLUGIN_ENTRY_NAME

class CelestiaCore;
class CelestiaConfig;
class ProgressNotifier;
class IScript;
class Hash;
class Renderer;
class CachingOrbit;
class RotationModel;

namespace celestia
{
namespace plugin
{

class PluginManager;

enum PluginType : uint32_t
{
    Nothing = 0,
    Script  = 0x0001,
    Audio   = 0x0002,
    Video   = 0x0004,
    Render  = 0x0010,
};

#pragma pack(push,1)
struct PluginInfo
{
    uint16_t    APIVersion;
    uint16_t    Reserved1;
    PluginType  Type;
    uint32_t    Reserved2;
    void       *IDDPtr; // IDD == Implementation Defined Data
};
#pragma pack(pop)

class Plugin
{
 public:
    Plugin() = default;
    ~Plugin();
    Plugin(const Plugin&) = delete;
    Plugin(Plugin&&);
    Plugin& operator=(const Plugin&) = delete;
    Plugin& operator=(Plugin&&) = default;

    PluginInfo* getPluginInfo() const;
    void* loadSym(const char*) const;
    PluginType getType() const { return m_type; }

    static Plugin* load(const fs::path&);
    bool isSupportedVersion() const;

    // pointers to plugin functions
    /// scripting support
    typedef bool(CreateScriptEnvironmentFunc)(CelestiaCore*, const CelestiaConfig*, ProgressNotifier*);
    typedef IScript*(CreateScriptFunc)(CelestiaCore*);
    typedef RotationModel*(CreateScritedRotationFunc)(const std::string&, const std::string&, Hash*);
    typedef CachingOrbit*(CreateScritedOrbitFunc)(const std::string&, const std::string&, Hash*);

    /// renderer support
    typedef Renderer*(CreateRendererFunc)();

    bool createScriptEnvironment(CelestiaCore *appCore, const CelestiaConfig *config, ProgressNotifier *progressNotifier) const;
    IScript* createScript(CelestiaCore *appCore) const;
    RotationModel* createScritedRotation(const std::string& moduleName, const std::string& funcName, Hash* parameters) const;
    CachingOrbit* createScritedOrbit(const std::string& moduleName, const std::string& funcName, Hash* parameters) const;
    Renderer* createRenderer() const;

 private:

#ifdef _WIN32
    HMODULE m_handle { nullptr };
#else
    void   *m_handle { nullptr };
#endif

    PluginType m_type { Nothing };

    typedef PluginInfo*(RegisterFunc)();
    RegisterFunc *m_regfn;

    union
    {
        struct
        {
            CreateScriptEnvironmentFunc *createScriptEnvironment;
            CreateScriptFunc            *createScript;
            CreateScritedRotationFunc   *createScritedRotation;
            CreateScritedOrbitFunc      *createScritedOrbit;
        };
        struct
        {
            CreateRendererFunc         *createRenderer;
        };
    } m_func;

    friend class PluginManager;
}; // Plugin

}
}
