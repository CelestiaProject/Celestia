#pragma once

#include <string>
#include <memory>
#include <vector>
#include <map>
#include "value.h"
#include <celutil/util.h>


namespace celestia
{
namespace engine
{
class IConfigUpdater;
class IConfigWriter;
class IProperty;

class Config
{
 public:
    Config() = default;
    Config(const Config&) = delete;
    Config(Config&&) = delete;
    ~Config();

    Config& operator=(const Config&) = delete;
    Config& operator=(Config&&) = delete;

    void addProperty(IProperty*);
    void removeProperty(IProperty*);
    const Value* find(const std::string& name) const;

#ifdef DEBUG
    void dump() const;
#else
    void dump() const {};
#endif

 private:
    struct Cmp
    {
        bool operator()(const std::string &a, const std::string &b) const
        {
            return compareIgnoringCase(a, b) < 0;
        }
    };

    void onUpdate();
    void beginUpdate();
    void set(const std::string& name, const Value *value);
    void endUpdate();

    std::vector<IProperty*> m_props;
    std::map<std::string, const Value*, Cmp> m_values;

    bool m_update { false };

    friend class IConfigUpdater;
    friend class IConfigWriter;
    friend class IProperty;
};


// Proxy class to invoke private Config's methods from derived classes
class IConfigUpdater
{   
    std::shared_ptr<Config> m_cfg;
 public:
    IConfigUpdater(const std::shared_ptr<Config> &cfg) : m_cfg(cfg) {}
    inline void beginUpdate() { m_cfg->beginUpdate(); }
    inline void set(const std::string &name, Value *value) { m_cfg->set(name, value); }
    inline void endUpdate() { m_cfg->endUpdate(); }
};


}
} // namespace;
