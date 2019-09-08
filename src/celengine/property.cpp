#include "property.h"


namespace celestia
{
namespace engine
{

#define PROPERTY_UPDATE(ValueType) \
    auto v = watched()->find(m_name); \
    m_has_value = v->getType() == Value:: ValueType ## Type; \
    if (m_has_value) \
    { \
        if (m_validate != nullptr) \
            m_value = m_validate(v->get ## ValueType()); \
        else \
            m_value = v->get ## ValueType(); \
    }

template<>
void Property<double>::notifyChange(int /* unused */)
{
    PROPERTY_UPDATE(Number);
}

template<>
void Property<std::string>::notifyChange(int /* unused */)
{
    PROPERTY_UPDATE(String);
}

template<>
void Property<bool>::notifyChange(int /* unused */)
{
    PROPERTY_UPDATE(Boolean);
}

#undef PROPERTY_UPDATE

}
} // namespace;
