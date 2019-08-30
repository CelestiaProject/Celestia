#include "property.h"


namespace celestia
{
namespace engine
{

template<>
void Property<double>::update()
{
    m_value = (*m_config)[m_name].getNumber();
}

template<>
void Property<std::string>::update()
{
    m_value = (*m_config)[m_name].getString();
}

template<>
void Property<bool>::update()
{
    m_value = (*m_config)[m_name].getBoolean();
}

}
} // namespace;
