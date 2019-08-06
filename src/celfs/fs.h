#pragma once

#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#endif
#include <system_error>
#include <string>
#include <iostream>
#include <memory>
#ifdef _WIN32
#include <celutil/winutil.h>
#endif


namespace celfs
{
class filesystem_error : std::system_error
{
 public:
    filesystem_error(std::error_code ec, const char* msg) :
        std::system_error(ec, msg)
    {
    }
}; // filesystem_error


class path
{
 public:
#ifdef _WIN32
    using string_type = std::wstring;
#else
    using string_type = std::string;
#endif
    using value_type = string_type::value_type;

#ifdef _WIN32
    static constexpr value_type preferred_separator = L'\\';
#else
    static constexpr value_type preferred_separator = '/';
#endif

    enum format
    {
        native_format,
        generic_format,
        auto_format
    };

    path() = default;
    path(const path&) = default;
    path(path&&) = default;
    path(string_type&& p, format fmt = auto_format) :
        m_path(std::move(p)),
        m_fmt(fmt)
    {
    };
    template<typename T> path(const T& p, format fmt = auto_format ) :
        m_path(p),
        m_fmt(fmt)
    {
    };
    ~path() = default;
    path& operator=(const path& p) = default;
    path& operator=(path&&) = default;

    template<typename T> path& append(const T& p)
    {
        if (empty())
            m_path = p;
        else
            m_path.append(1, preferred_separator).append(p.c_str());
        return *this;
    }

    template<typename T> path& operator/=(const T& p)
    {
        return append(p);
    }

    template<typename T> path& concat(const T& p)
    {
        m_path.append(p.c_str());
        return *this;
    }

    template<typename T> path& operator+=(const T& p)
    {
        return concat(p);
    }

    bool operator==(const path& other) const noexcept
    {
        return m_path == other.m_path && m_fmt == other.m_fmt;
    }

    bool operator!=(const path& other) const noexcept
    {
        return !(*this == other);
    }

    bool operator<(const path& other) const noexcept
    {
        return m_path < other.m_path;
    }

    bool operator>(const path& other) const noexcept
    {
        return m_path > other.m_path;
    }

    bool operator<=(const path& other) const noexcept
    {
        return m_path <= other.m_path;
    }

    bool operator>=(const path& other) const noexcept
    {
        return m_path >= other.m_path;
    }

    std::string string() const noexcept
    {
        return u8string();
    }

    std::wstring wstring() const noexcept
    {
#ifdef _WIN32
        return m_path;
#else
        return L""; // UB
#endif
    }

    std::string u8string() const noexcept
    {
#ifdef _WIN32
        return WideToUTF8(m_path);
#else
        return m_path;
#endif
    }

    const value_type* c_str() const noexcept
    {
        return m_path.c_str();
    }

    const string_type& native() const noexcept
    {
       return m_path;
    }

    operator string_type() const noexcept
    {
        return m_path;
    }

    bool empty() const noexcept
    {
        return m_path.empty();
    }

    path filename() const;
    path stem() const;
    path extension() const;
    path parent_path() const;

    bool is_relative() const
    {
        return !is_absolute();
    }
    bool is_absolute() const;

 private:
    string_type m_path;
    format      m_fmt       { auto_format };
}; // path

path operator/(const path& lhs, const path& rhs);

std::ostream& operator<<(std::ostream& os, const path& p);

path u8path(const std::string& source);


class directory_iterator;
class recursive_directory_iterator;
class directory_entry
{
 public:
    directory_entry() = default;
    directory_entry(const directory_entry&) = default;
    directory_entry(directory_entry&&) = default;
    explicit directory_entry(const path& p) :
        m_path(p)
    {
    };
    directory_entry& operator=(const directory_entry&) = default;
    directory_entry& operator=(directory_entry&&) = default;

    const class path& path() const noexcept
    {
        return m_path;
    }
    operator const class path& () const noexcept
    {
        return m_path;
    };

    bool operator==(const directory_entry& other) const noexcept
    {
        return m_path == other.m_path;
    }
    bool operator!=(const directory_entry& other) const noexcept
    {
        return !(*this == other);
    }

 private:
    friend class directory_iterator;
    friend class recursive_directory_iterator;

    class path m_path;
}; // directory_entry


class directory_iterator
{
 public:
    using value_type        = directory_entry;
    using difference_type   = std::ptrdiff_t;
    using pointer           = const directory_entry*;
    using reference         = const directory_entry&;
    using iterator_category = std::input_iterator_tag;

    directory_iterator() = default;
    explicit directory_iterator(const path& p);
    directory_iterator(const path& p, std::error_code& ec);

    directory_iterator(const directory_iterator&) = default;
    directory_iterator(directory_iterator&&) = default;
    ~directory_iterator() = default;

    directory_iterator& operator=(const directory_iterator&) = default;
    directory_iterator& operator=(directory_iterator&&) = default;

    directory_iterator& operator++();

    const directory_entry& operator*() const noexcept
    {
        return m_entry;
    }

    const directory_entry* operator->() const noexcept
    {
        return &m_entry;
    }

    bool operator==(const directory_iterator& other) const noexcept;
    bool operator!=(const directory_iterator& other) const noexcept
    {
        return !(*this == other);
    }

 private:
    struct SearchImpl;
    void reset();

    path                        m_path      {};
    directory_entry             m_entry     {};
    std::error_code             m_ec        {};
    std::shared_ptr<SearchImpl> m_search    { nullptr };
}; // directory_iterator

inline directory_iterator begin(directory_iterator iter) noexcept
{
    return iter;
}

inline directory_iterator end(directory_iterator) noexcept
{
    return directory_iterator();
}


class recursive_directory_iterator
{
 public:
    using value_type        = directory_entry;
    using difference_type   = std::ptrdiff_t;
    using pointer           = const directory_entry*;
    using reference         = const directory_entry&;
    using iterator_category = std::input_iterator_tag;

    recursive_directory_iterator() = default;
    recursive_directory_iterator(const recursive_directory_iterator&) = default;
    recursive_directory_iterator(recursive_directory_iterator&&) noexcept = default;
    explicit recursive_directory_iterator(const path& p);
//    recursive_directory_iterator(const path&, directory_options);
//    recursive_directory_iterator(const path&, directory_options, std::error_code&);
    recursive_directory_iterator(const path& p, std::error_code& ec);
    ~recursive_directory_iterator() = default;

    recursive_directory_iterator& operator=(const recursive_directory_iterator&) = default;
    recursive_directory_iterator& operator=(recursive_directory_iterator&&) = default;

    bool recursion_pending() const
    {
        return m_pending;
    };

    void disable_recursion_pending()
    {
        m_pending = false;
    };

    void pop();
    void pop(std::error_code& ec);

    recursive_directory_iterator& operator++();

    const directory_entry& operator*() const noexcept
    {
        return *m_iter;
    }

    const directory_entry* operator->() const noexcept
    {
        return &*m_iter;
    }

    bool operator==(const recursive_directory_iterator& other) const noexcept;
    bool operator!=(const recursive_directory_iterator& other) const noexcept
    {
        return !(*this == other);
    }

 private:
    void reset();

    std::error_code m_ec                {};
    int             m_depth             { 0 };
    bool            m_pending           { true };

    struct DirStack;
    std::shared_ptr<DirStack>   m_dirs  { nullptr };
    directory_iterator          m_iter  {};
}; // recursive_directory_iterator

inline recursive_directory_iterator begin(recursive_directory_iterator iter) noexcept
{
    return iter;
}

inline recursive_directory_iterator end(recursive_directory_iterator) noexcept
{
    return recursive_directory_iterator();
}


uintmax_t file_size(const path& p, std::error_code& ec) noexcept;
uintmax_t file_size(const path& p);

bool exists(const path& p);
bool exists(const path& p, std::error_code& ec) noexcept;

bool is_directory(const path& p);
bool is_directory(const path& p, std::error_code& ec) noexcept;
};
