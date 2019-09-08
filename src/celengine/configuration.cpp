#include "configuration.h"
#include "property.h"
#include <iostream>


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
    notifyWatchers();
}

Config::~Config()
{
    for (const auto &p : m_values)
        delete p.second;
}

}
} // namespace;
