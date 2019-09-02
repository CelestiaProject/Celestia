#pragma once

#include <memory>
#include "configuration.h"


namespace celestia
{
namespace engine
{

class IProperty
{
 public:
    virtual void update() = 0;
    friend class Config;
};

template<typename T>
class Property : IProperty
{
 public:
    Property() = default;

    Property(std::shared_ptr<Config> config, std::string name, T def) :
        m_config(std::move(config)),
        m_name(std::move(name)),
        m_default(std::move(def))
    {
        m_config->addProperty(this);
    };

    ~Property()
    {
        m_config->removeProperty(this);
    };

    // Getters and setters
    inline Property& set(T value)
    {
        m_value = value;
    };

    Property& operator() (T value)
    {
        return set(value);
    };

    Property& operator=(T value)
    {
        return set(value);
    };

    T get() const
    {
        return m_has_value ? m_value : m_default;
    };

    T operator()() const
    {
        return get();
    };

    // Used by Config to propagate changes
    void update() override;

 private:
    std::shared_ptr<Config> m_config        { nullptr };
    std::string             m_name;
    T                       m_value         {};
    T                       m_default       {};
    bool                    m_has_value     { false };
};

using NumericProperty = Property<double>;
using StringProperty  = Property<std::string>;
using BoolProperty    = Property<bool>;

}
} // namespace;
