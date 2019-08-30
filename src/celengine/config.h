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
class IProperty;
class IConfigWriter;

class Config
{
 public:
//    using SharedPtr = std::shared_ptr<Config>;
    typedef std::shared_ptr<Config> SharedPtr;

    void addProperty(IProperty*);
    void removeProperty(IProperty*);
    const Value operator[](const std::string& name);

 private:
    struct Cmp
    {
        bool operator()(const std::string &a, const std::string &b)
        {
            return compareIgnoringCase(a, b) == 0;
        }
    };

    void onUpdate();
    void beginUpdate();
    void set(const std::string& name, const Value& value);
    void endUpdate();

    std::vector<IProperty*>             m_props;
    std::map<std::string, Value, Cmp>   m_values;

    bool m_update { false };

    friend class IConfigWriter;
    friend class IProperty;
};

}
} // namespace;
