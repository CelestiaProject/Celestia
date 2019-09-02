#include "configuration.h"
#include "property.h"
#include <iostream>
#include <algorithm>


namespace celestia
{
namespace engine
{

#ifdef DEBUG
void Config::dump() const
{
    for (const auto &m : m_values)
    {
        std::cout << m.first << ' ';
        auto d = m.second;
        std::cout << d->getType() << ' ';
        switch (d->getType())
        {
        case Value::NullType:
            std::cout << "null";
            break;
        case Value::NumberType:
            std::cout << d->getNumber();
            break;
        case Value::StringType:
            std::cout << d->getString();
            break;
        case Value::BooleanType:
            std::cout << d->getBoolean();
            break;
        default:
            std::cout << "not supported yet";
            break;
        }
        std::cout << '\n';
    }
}
#endif

void Config::addProperty(IProperty *p)
{
    if (std::find(m_props.begin(), m_props.end(), p) == m_props.end())
        m_props.push_back(p);
    p->update();
}

void Config::removeProperty(IProperty *p)
{
    auto pos = std::find(m_props.begin(), m_props.end(), p);
    if (pos != m_props.end())
        m_props.erase(pos);
}

const Value* Config::find(const std::string& name) const
{
    static Value v;

    auto it = m_values.find(name);
    if (it != m_values.end())
        return it->second;

    return &v;
}

void Config::beginUpdate()
{
    m_update = true;
}

void Config::set(const std::string& name, const Value* value)
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
    for (auto *p : m_props)
        p->update();
}

Config::~Config()
{
    for (const auto &p : m_values)
        delete p.second;
}

}
} // namespace;
