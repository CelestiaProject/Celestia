#include "fs.h"
#include <vector>
#include <memory>
#ifdef _WIN32
#include <celutil/winutil.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif
#if defined(_MSC_VER) && !defined(__clang__)
// M$VC++ build without C++ exceptions are not supported yet
#ifndef __cpp_exceptions
#define __cpp_exceptions 1
#endif
#endif

#if ! __cpp_exceptions
#include <cstdlib>
#endif

#ifdef _WIN32
#define W(s) L##s
#else
#define W(s) s
#endif

namespace celestia
{
namespace filesystem
{

// we should skip "." and ".."
template<typename CharT> static bool is_special_dir(const CharT s[])
{
    return s[0] == '.' && (s[1] == '\0' || (s[1] == '.' && s[2]  == '\0'));
}

path operator/(const path& lhs, const path& rhs)
{
    return path(lhs) /= rhs;
}

std::ostream& operator<<(std::ostream& os, const path& p)
{
    os << '"' << p.string() << '"';
    return os;
}

path u8path(const std::string& source)
{
#ifdef _WIN32
    return UTF8ToWide(source);
#else
    return source; // FIXME
#endif
}

#ifdef _WIN32
void path::fixup_separators()
{
    if (m_fmt == native_format)
        return;

    string_type::size_type pos = 0;
    for(;;)
    {
        pos = m_path.find(L'/', pos);
        if (pos == path::string_type::npos)
            break;
        m_path[pos] = preferred_separator;
        pos++;
    }
}
#endif

path path::filename() const
{
    auto pos = m_path.rfind(preferred_separator);
    if (pos == string_type::npos)
        pos = 0;
    else
        pos++;

    auto fn = m_path.substr(pos);
    if (is_special_dir(fn.c_str()))
        return path();

    path p(fn);
    return p;
}

path path::stem() const
{
    auto fn = filename().native();
    auto pos = fn.rfind(W('.'));

    if (pos == 0 || pos == string_type::npos)
        return fn;

    return fn.substr(0, pos);
}

path path::extension() const
{
    auto fn = filename().native();
    auto pos = fn.rfind(W('.'));

    if (pos == 0 || pos == string_type::npos)
        return path();

    return fn.substr(pos);
}

path path::parent_path() const
{
    auto pos = m_path.rfind(preferred_separator);
    if (pos == 0)
        return path(m_path.substr(0, 1));

    if (pos == string_type::npos)
        return path();

    return path(m_path.substr(0, pos));
}

path& path::replace_extension(const path& replacement)
{
    auto pos = m_path.rfind(W('.'));

    if (pos != (m_path.length() - 1))
    {
        if (pos != 0 && pos != string_type::npos)
            m_path.resize(pos);
    }
    else
    {
        // special case for "/dir/" or "/dir/."
        path basename = filename();
        if (!(basename.empty() || basename == W(".")))
            m_path.resize(pos);
    }

    if (replacement.empty())
        return *this;

    if (replacement.m_path[0] != W('.'))
        m_path += W('.');

    m_path += replacement.m_path;

    return *this;
}


bool path::is_absolute() const
{
#ifdef _WIN32
    return m_path[0] == preferred_separator || m_path[1] == L':';
#else
    return m_path[0] == preferred_separator;
#endif
}

struct directory_iterator::SearchImpl
{
    path                m_path      {};
    directory_options   m_options   { directory_options::none };
#ifdef _WIN32
    HANDLE  m_handle                { INVALID_HANDLE_VALUE };
#else
    DIR    *m_dir                   { nullptr };
#endif
    explicit SearchImpl(const path& p, directory_options options) :
        m_options(options),
        m_path(p)
    {
    };
    ~SearchImpl();

    bool advance(directory_entry&, std::error_code*);
};

directory_iterator::SearchImpl::~SearchImpl()
{
#ifdef _WIN32
    if (m_handle != INVALID_HANDLE_VALUE)
    {
        FindClose(m_handle);
        m_handle = INVALID_HANDLE_VALUE;
    }
#else
    if (m_dir != nullptr)
    {
        closedir(m_dir);
        m_dir = nullptr;
    }
#endif
}

bool directory_iterator::SearchImpl::advance(directory_entry& entry, std::error_code* ec)
{
    if (ec != nullptr)
        ec->clear();

#ifdef _WIN32
    WIN32_FIND_DATAW findData;

    if (m_handle == INVALID_HANDLE_VALUE)
    {
        m_handle = FindFirstFileW((m_path / L"*").c_str(), &findData);
        if (m_handle == INVALID_HANDLE_VALUE)
        {
            std::error_code m_ec = std::error_code(GetLastError(), std::system_category());
            m_ec = std::error_code(GetLastError(), std::system_category());
            if (m_ec.value() == ERROR_ACCESS_DENIED && bool(m_options & directory_options::skip_permission_denied))
                m_ec.clear();
            if (m_ec)
            {
                if (ec != nullptr)
                    *ec = m_ec;
                else
#if __cpp_exceptions
                    throw filesystem_error(m_ec, "celfs::directory_iterator::SearchImpl::advance error");
#else
                    std::abort();
#endif
            }
            return false;
        }
    }
    else
    {
        if (!FindNextFileW(m_handle, &findData))
            return false;
    }

    while (is_special_dir(findData.cFileName))
    {
        if (!FindNextFileW(m_handle, &findData))
            return false;
    }

    entry = directory_entry(std::move(m_path / findData.cFileName));
    return true;
#else
    if (m_dir == nullptr)
    {
        m_dir = opendir(m_path.c_str());
        if (m_dir == nullptr)
        {
            std::error_code m_ec = std::error_code(errno, std::system_category());
            if (m_ec.value() == EACCES && bool(m_options & directory_options::skip_permission_denied))
                m_ec.clear();
            if (m_ec)
            {
                if (ec != nullptr)
                    *ec = m_ec;
                else
#if __cpp_exceptions
                    throw filesystem_error(m_ec, "celfs::directory_iterator::SearchImpl::advance error");
#else
                    std::abort();
#endif
            }
            return false;
        }
    }

    struct dirent* ent = nullptr;
    do
    {
        ent = readdir(m_dir);
        if (ent == nullptr)
            return false;
    }
    while (is_special_dir(ent->d_name));

    entry = directory_entry(std::move(m_path / ent->d_name));
    return true;
#endif
}

directory_iterator::directory_iterator(const path& p) :
    directory_iterator::directory_iterator(p, directory_options::none, nullptr)
{
}

directory_iterator::directory_iterator(const path& p, directory_options options) :
    directory_iterator::directory_iterator(p, options, nullptr)
{
}

directory_iterator::directory_iterator(const path& p, std::error_code& ec) :
    directory_iterator::directory_iterator(p, directory_options::none, &ec)
{
}

directory_iterator::directory_iterator(const path& p, directory_options options, std::error_code& ec) :
    directory_iterator::directory_iterator(p, directory_options::none, &ec)
{
}

directory_iterator::directory_iterator(const path& p, directory_options options, std::error_code* ec, bool advance) :
    m_path(p),
    m_search(new SearchImpl(p, options))
{
    if (advance)
        __increment(ec);
    else
        m_entry = directory_entry(p);
}

void directory_iterator::reset()
{
    *this = directory_iterator();
}

bool directory_iterator::operator==(const directory_iterator& other) const noexcept
{
    return m_search == other.m_search && m_entry == other.m_entry;
}

directory_iterator& directory_iterator::operator++()
{
    return __increment();
}

directory_iterator& directory_iterator::increment(std::error_code &ec)
{
    return __increment(&ec);
}

directory_iterator& directory_iterator::__increment(std::error_code *ec)
{
    if (m_search != nullptr && !m_search->advance(m_entry, ec))
        reset();
    return *this;
}


struct recursive_directory_iterator::DirStack
{
    std::vector<directory_iterator> m_dirIters;
};


recursive_directory_iterator::recursive_directory_iterator(const path& p) : recursive_directory_iterator(p, directory_options::none, nullptr)
{
}

recursive_directory_iterator::recursive_directory_iterator(const path& p, directory_options options) : recursive_directory_iterator(p, options, nullptr)
{
}

recursive_directory_iterator::recursive_directory_iterator(const path& p, std::error_code& ec) : recursive_directory_iterator(p, directory_options::none, &ec)
{
}

recursive_directory_iterator::recursive_directory_iterator(const path& p, directory_options options, std::error_code& ec) : recursive_directory_iterator(p, options, &ec)
{
}

recursive_directory_iterator::recursive_directory_iterator(const path& p, directory_options options, std::error_code* ec)
{
    m_options = options;

    if (m_dirs == nullptr)
        m_dirs = std::shared_ptr<DirStack>(new DirStack);

    m_iter = directory_iterator(p, options, ec);

    if (m_iter == end(m_iter))
        reset();
}

bool recursive_directory_iterator::operator==(const recursive_directory_iterator& other) const noexcept
{
    return m_depth   == other.m_depth   &&
           m_pending == other.m_pending &&
           m_dirs    == other.m_dirs    &&
           m_iter    == other.m_iter;
}


recursive_directory_iterator& recursive_directory_iterator::operator++()
{
    return __increment();
}

recursive_directory_iterator& recursive_directory_iterator::increment(std::error_code &ec)
{
    return __increment(&ec);
}

recursive_directory_iterator& recursive_directory_iterator::__increment(std::error_code *ec)
{
    if (m_dirs == nullptr)
        return *this;

    m_iter.__increment(ec);

    while (m_iter == end(m_iter))
    {
        if (m_dirs->m_dirIters.empty())
        {
            reset();
            return *this;
        }
        m_iter = std::move(m_dirs->m_dirIters.back());
        m_iter.__increment(ec);
        m_dirs->m_dirIters.pop_back();
    }

    auto path = m_iter->path();
    if (m_pending && (path.empty() || is_directory(path)))
    {
        m_dirs->m_dirIters.emplace_back(m_iter);
        m_iter = directory_iterator(path, m_options, ec, false);
    }

    return *this;
}

void recursive_directory_iterator::pop()
{
    std::error_code ec;
    pop(ec);
}

void recursive_directory_iterator::pop(std::error_code& ec)
{
    if (m_dirs->m_dirIters.empty())
        ec = std::error_code(errno, std::system_category()); // filesystem_error
    else
        m_dirs->m_dirIters.pop_back();
}

void recursive_directory_iterator::reset()
{
    *this = recursive_directory_iterator();
}


uintmax_t file_size(const path& p, std::error_code& ec) noexcept
{
#ifdef _WIN32
    WIN32_FILE_ATTRIBUTE_DATA attr;
    LARGE_INTEGER fileSize = { 0 };
    if (GetFileAttributesExW(p.c_str(), GetFileExInfoStandard, &attr))
    {
        fileSize.LowPart  = attr.nFileSizeLow;
        fileSize.HighPart = attr.nFileSizeHigh;
        return static_cast<uintmax_t>(fileSize.QuadPart);
    }
    else
    {
        ec = std::error_code(errno, std::system_category());
        return static_cast<uintmax_t>(-1);
    }
#else
    struct stat stat_buf;
    int rc = stat(p.c_str(), &stat_buf);
    if (rc == -1)
    {
        ec = std::error_code(errno, std::system_category());
        return static_cast<uintmax_t>(-1);
    }

    return static_cast<uintmax_t>(stat_buf.st_size);
#endif
}

uintmax_t file_size(const path& p)
{
    std::error_code ec;
    uintmax_t s = file_size(p, ec);
    if (ec)
#if __cpp_exceptions
        throw filesystem_error(ec, "celfs::file_size error");
#else
        std::abort();
#endif
    return s;
}


bool exists(const path& p, std::error_code& ec) noexcept
{
    ec.clear();
#ifdef _WIN32
    DWORD attr = GetFileAttributesW(&p.native()[0]);
    if (attr != INVALID_FILE_ATTRIBUTES)
        return true;

    switch (GetLastError())
    {
    // behave like boost::filesystem
    case ERROR_FILE_NOT_FOUND:
    case ERROR_PATH_NOT_FOUND:
    case ERROR_INVALID_NAME:
    case ERROR_INVALID_DRIVE:
    case ERROR_NOT_READY:
    case ERROR_INVALID_PARAMETER:
    case ERROR_BAD_PATHNAME:
    case ERROR_BAD_NETPATH:
        return false;
    default:
        break;
    }
#else
    struct stat buf;
    if (stat(p.c_str(), &buf) == 0)
        return true;

    if (errno == ENOENT)
        return false;
#endif
    ec = std::error_code(errno, std::system_category());
    return false;
}

bool exists(const path& p)
{
    std::error_code ec;
    bool r = exists(p, ec);
    if (ec)
#if __cpp_exceptions
        throw filesystem_error(ec, "celfs::exists error");
#else
        std::abort();
#endif
    return r;
}


bool is_directory(const path& p, std::error_code& ec) noexcept
{
    ec.clear();
#ifdef _WIN32
    DWORD attr = GetFileAttributesW(&p.native()[0]);
    if (attr == INVALID_FILE_ATTRIBUTES)
    {
        ec = std::error_code(errno, std::system_category());
        return false;
    }

    return (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
#else
    struct stat buf;
    if (stat(p.c_str(), &buf) != 0)
    {
        ec = std::error_code(errno, std::system_category());
        return false;
    }
    return S_ISDIR(buf.st_mode);
#endif
}

bool is_directory(const path& p)
{
    std::error_code ec;
    bool r = is_directory(p, ec);
    if (ec)
#if __cpp_exceptions
        throw filesystem_error(ec, "celfs::is_directory error");
#else
        std::abort();
#endif
    return r;
}

bool create_directory(const path& p, std::error_code& ec) noexcept
{
    bool r;
#ifdef _WIN32
    r = _wmkdir(p.c_str()) == 0;
#else
    r = mkdir(p.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0;
#endif
    if (!r)
        ec = std::error_code(errno, std::system_category());
    return r;
}

bool create_directory(const path& p)
{
    std::error_code ec;
    bool r = create_directory(p, ec);
    if (ec)
#if __cpp_exceptions
        throw filesystem_error(ec, "celfs::create_directory error");
#else
        std::abort();
#endif
    return r;
}

path current_path()
{
    std::error_code ec;
    path p = current_path(ec);

    if (ec)
#if __cpp_exceptions
        throw filesystem_error(ec, "celfs::current_path error");
#else
        std::abort();
#endif
    return p;
}
path current_path(std::error_code& ec)
{
#ifdef _WIN32
    std::wstring buffer(MAX_PATH + 1, 0);
    DWORD r = GetModuleFileNameW(nullptr, &buffer[0], MAX_PATH);
    if (r == 0)
    {
        ec = std::error_code(errno, std::system_category());
        return path();
    }
    auto pos = buffer.find_last_of(L"\\/");
    return buffer.substr(0, pos);
#else
    std::string buffer(256, 0);
    char *r = getcwd(&buffer[0], buffer.size());
    if (r == nullptr)
    {
        ec = std::error_code(errno, std::system_category());
        return path();
    }
    return buffer;
#endif
}

void current_path(const path& p)
{
    std::error_code ec;
    current_path(p, ec);
    if (ec)
#if __cpp_exceptions
        throw filesystem_error(ec, "celfs::current_path error");
#else
        std::abort();
#endif
}

void current_path(const path& p, std::error_code& ec) noexcept
{
#ifdef _WIN32
    BOOL ret = SetCurrentDirectoryW(p.c_str());
    if (!ret)
        ec = std::error_code(errno, std::system_category());
#else
    int ret = chdir(p.c_str());
    if (ret != 0)
        ec = std::error_code(errno, std::system_category());
#endif
}
}
}
