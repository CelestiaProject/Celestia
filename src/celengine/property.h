#pragma once

#include <cassert>
#include <functional>
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
    using validate_fn_t = std::function<T(const T&)>;

    Property() = default;

    Property(std::shared_ptr<Config> config, std::string name, T _default, validate_fn_t validate = nullptr) :
        m_config    (std::move(config)),
        m_name      (std::move(name)),
        m_default   (std::move(_default)),
        m_validate  (std::move(validate))
    {
        assert(m_config != nullptr);
        m_config->addProperty(this);
    };

    ~Property()
    {
        if (m_config != nullptr)
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
    validate_fn_t           m_validate      { nullptr };
};

using NumericProperty = Property<double>;
using StringProperty  = Property<std::string>;
using BooleanProperty = Property<bool>;

}
} // namespace;
