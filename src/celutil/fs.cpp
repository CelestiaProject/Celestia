#include "fs.h"
#include <cstring>
#include <vector>
#include <memory>
#ifdef _WIN32
#include "winutil.h"
#else
#include <sys/stat.h>
#define UTF8ToCurrentCP(s) (s)
#define CurrentCPToUTF8(s) (s)
#endif


namespace celutil
{
namespace filesystem
{
path operator/(const path& lhs, const path& rhs)
{
    return path(lhs) /= rhs;
}

std::ostream& operator<<(std::ostream& os, const path& p)
{
    os << '"' << p.string() << '"';
    return os;
}

path path::filename() const
{
    auto pos = m_path.rfind(preferred_separator);
    if (pos == std::string::npos)
        pos = 0;
    else
        pos++;

    auto fn = m_path.substr(pos);
    if (fn == "." || fn == "..")
        fn.clear();

    return path(fn);
}


path path::parent_path() const
{
    auto pos = m_path.rfind(preferred_separator);
    if (pos == 0)
        return path(m_path.substr(0, 1));

    if (pos == std::string::npos)
        return path();

    return path(m_path.substr(0, pos));
}


struct directory_iterator::SearchImpl
{
    path    m_path      {};
#ifdef _WIN32
    HANDLE  m_handle    { INVALID_HANDLE_VALUE };
#else
    DIR    *m_dir       { nullptr };
#endif
    explicit SearchImpl(const path& p) :
        m_path(p)
    {
    };
    ~SearchImpl();

    bool advance(directory_entry&);
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

static bool is_special_dir(const char* d)
{
    return strcmp(d, ".") == 0 || strcmp(d, "..") == 0;
}

bool directory_iterator::SearchImpl::advance(directory_entry& entry)
{
#ifdef _WIN32
    WIN32_FIND_DATAA findData;

    if (m_handle == INVALID_HANDLE_VALUE)
    {
        m_handle = FindFirstFileA(UTF8ToCurrentCP(m_path / "*").c_str(), &findData);
        if (m_handle == INVALID_HANDLE_VALUE)
            return false;
    }
    else
    {
        if (!FindNextFileA(m_handle, &findData))
            return false;
    }

    while (is_special_dir(findData.cFileName))
    {
        if (!FindNextFileA(m_handle, &findData))
            return false;
    }

    entry = directory_entry(m_path / CurrentCPToUTF8(findData.cFileName));
    return true;
#else
    if (m_dir == nullptr)
    {
        m_dir = opendir(m_path.c_str());
        if (m_dir == nullptr)
            return false;
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
    directory_iterator::directory_iterator(p, m_ec)
{
}

directory_iterator::directory_iterator(const path& p, std::error_code& ec) :
    m_path(p),
    m_ec(ec),
    m_search(std::make_shared<SearchImpl>(p))
{
    if (!m_search->advance(m_entry))
        reset();
}

void directory_iterator::reset()
{
    m_search = nullptr;
    m_path   = std::move(path());
    m_entry  = std::move(directory_entry());
}

bool directory_iterator::operator==(const directory_iterator& other) const noexcept
{
    return m_search == other.m_search && m_entry == other.m_entry;
}

directory_iterator& directory_iterator::operator++()
{
    if (!m_search->advance(m_entry))
        reset();
    return *this;
}


struct recursive_directory_iterator::DirStack
{
    std::vector<directory_iterator> m_dirIters;
};

recursive_directory_iterator::recursive_directory_iterator(const path& p)
{
    if (m_dirs == nullptr)
        m_dirs = std::make_shared<DirStack>();

    m_iter = directory_iterator(p);
}

recursive_directory_iterator& recursive_directory_iterator::operator++()
{
    auto& p = m_iter->path();
    if (m_pending && is_directory(p))
    {
        m_dirs->m_dirIters.emplace_back(m_iter);
        m_iter = directory_iterator(p);
    }
    else
    {
        ++m_iter;
    }

    while (m_iter == end(m_iter))
    {
        if (m_dirs->m_dirIters.empty())
        {
            reset();
            return *this;
        }
        m_iter = std::move(m_dirs->m_dirIters.back());
        ++m_iter;
        m_dirs->m_dirIters.pop_back();
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
    m_dirs->m_dirIters.pop_back();
}

void recursive_directory_iterator::reset()
{
    m_depth   = 0;
    m_pending = false;
    m_dirs    = nullptr;
    m_iter    = directory_iterator();
}

uintmax_t file_size(const path& p, std::error_code& ec) noexcept
{
#ifdef _WIN32
    WIN32_FILE_ATTRIBUTE_DATA attr;
    LARGE_INTEGER fileSize = { 0 };
    if (GetFileAttributesEx(UTF8ToCurrentCP(p).c_str(), GetFileExInfoStandard, &attr))
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
    if (s == static_cast<uintmax_t>(-1))
        throw filesystem_error(ec, "file_size error");
    return s;
}

bool is_directory(const path& p)
{
#ifdef _WIN32
    DWORD attr = GetFileAttributesA(const_cast<char*>(UTF8ToCurrentCP(p).c_str()));
    if (attr == static_cast<DWORD>(-1))
        return false;
    return (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
#else
    struct stat buf;
    return (stat(p.c_str(), &buf) == 0) && S_ISDIR(buf.st_mode);
#endif
}

}
}
