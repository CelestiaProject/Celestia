#pragma once

#include <string>
#include <memory>
#include "parser.h"

namespace celengine
{
class BaseProperty;
class Config
{
 public:
    void setProperty(BaseProperty*);
    const Value operator[](const std::string& name);
};

class BaseProperty
{
 public:
    virtual void update() = 0;
    friend class Config;
};

template<typename T>
class Property : BaseProperty
{
 public:
    Property() = default;

    Property(Config *config, std::string name) :
        m_config(config),
        m_name(std::move(name))
    {
        m_config->setProperty(this);
    };

    Property(Config *config, std::string name, T value) :
        m_config(config),
        m_name(std::move(name)),
        m_value(std::move(value))
    {
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
        return m_value;
    };

    T operator()() const
    {
        return get();
    };

    // Used by Config to propagate changes
    void update() override;

 private:
    Config         *m_config { nullptr };
    std::string     m_name;
    T               m_value  {};
};

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

Property<bool> p;
} // namespace;
