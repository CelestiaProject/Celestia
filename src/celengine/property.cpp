#include "property.h"


namespace celestia
{
namespace engine
{

#define PROPERTY_UPDATE(ValueType) \
    auto v = m_config->find(m_name); \
    m_has_value = v->getType() == Value:: ValueType ## Type; \
    if (m_has_value) \
        m_value = v->get ## ValueType (); \

template<>
void Property<double>::update()
{
    PROPERTY_UPDATE(Number);
}

template<>
void Property<std::string>::update()
{
    PROPERTY_UPDATE(String);
}

template<>
void Property<bool>::update()
{
    PROPERTY_UPDATE(Boolean);
}

#undef PROPERTY_UPDATE

}
} // namespace;
