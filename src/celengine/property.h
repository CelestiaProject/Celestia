#pragma once

#include <cassert>
#include <functional>
#include <celutil/watcher.h>
#include <celengine/configuration.h>


namespace celestia
{
namespace engine
{

template<typename T>
class Property : public utility::Watcher<Config>
{
 public:
    using validate_fn_t = std::function<T(const T&)>;

//    Property() = default;

    Property(const std::shared_ptr<Config>& config,
             const std::string &name,
             const T &_default,
             validate_fn_t validate = nullptr) :
        utility::Watcher<Config>(config),
        m_name      { name     },
        m_default   { _default },
        m_validate  { validate }
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
        return m_has_value ? m_value : m_default;
    };

    T operator()() const
    {
        return get();
    };

    // Used by Config to propagate changes
    void notifyChange(int = 0) override;

 private:
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
