#include "infourl.h"

#include <system_error>

#include <fmt/format.h>

#ifdef _WIN32
#include <algorithm>
#include "winutil.h"
#endif

using namespace std::string_view_literals;

namespace celestia::util
{

namespace
{
constexpr std::string_view httpPrefix = "http://"sv; //NOSONAR
constexpr std::string_view httpsPrefix = "https://"sv;
#ifdef _WIN32
constexpr std::string_view filePrefix = "file:///"sv;
constexpr std::wstring_view extPrefix = L"\\\\?\\"sv;
#endif
} // end unnamed namespace

std::string
BuildInfoURL(std::string_view infoUrl, const fs::path &resPath)
{
    if ((infoUrl.size() >= httpPrefix.size() && infoUrl.substr(0, httpPrefix.size()) == httpPrefix) ||
        (infoUrl.size() >= httpsPrefix.size() && infoUrl.substr(0, httpsPrefix.size()) == httpsPrefix))
    {
        return std::string{ infoUrl };
    }

    std::error_code ec;
    fs::path canonical = fs::canonical(resPath / fs::u8path(infoUrl), ec);
    if (ec)
        return {};

#ifdef _WIN32
    std::wstring_view canonicalView = canonical.native();
    // Remove extended filepath prefix if any
    if (canonicalView.size() >= extPrefix.size() && canonicalView.substr(0, extPrefix.size()) == extPrefix)
        canonicalView = canonicalView.substr(extPrefix.size());

    std::string fileUrl{ filePrefix };
    WideToUTF8(canonicalView, fileUrl);
    // Make sure the UTF-8 conversion worked
    if (fileUrl.size() == filePrefix.size())
        return {};

    std::replace(fileUrl.begin(), fileUrl.end(), '\\', '/');
    return fileUrl;
#else
    return fmt::format("file://{}", canonical.native());
#endif
}

} // end namespace celestia::util
