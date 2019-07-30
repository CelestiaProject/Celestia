#include "config.h"
#include "property.h"


namespace celengine
{

void Config::setProperty(BaseProperty *p)
{
    auto pos = std::find(m_props.begin(), m_props.end(), p);
    if (pos != m_props.end())
        m_props.push_back(p);
}

void Config::removeProperty(BaseProperty *p)
{
    auto pos = std::find(m_props.begin(), m_props.end(), p);
    if (pos != m_props.end())
        m_props.erase(pos);
}

const Value Config::operator[](const std::string& name)
{
    auto pos = m_values.find(name);
    if (pos != m_values.end())
        return pos->second;

    return Value();
}

void Config::beginUpdate()
{
    m_update = true;
}

void Config::set(const std::string& name, const Value& value)
{
    m_values[name] = value;
}

void Config::endUpdate()
{
    m_update = false;
    onUpdate();
}

void Config::onUpdate()
{
    for (auto* p : m_props)
        p->update();
}

} // namespace;
