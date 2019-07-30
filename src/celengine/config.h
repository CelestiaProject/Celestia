#pragma once

#include <string>
#include <memory>
#include <vector>
#include <map>
#include "parser.h"


namespace celengine
{
class BaseProperty;

class Config
{
 public:
    void setProperty(BaseProperty*);
    void removeProperty(BaseProperty*);
    const Value operator[](const std::string& name);

    void beginUpdate();
    void set(const std::string& name, const Value& value);
    void endUpdate();

 private:
    void onUpdate();

    std::vector<BaseProperty*>      m_props;
    std::map<std::string, Value>    m_values;

    bool m_update { false };
};

} // namespace;
