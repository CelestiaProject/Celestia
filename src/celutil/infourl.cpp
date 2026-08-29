#include "infourl.h"

#include <system_error>

#include <fmt/format.h>

#include <celutil/fsutils.h>

#ifdef _WIN32
#include <algorithm>
#include "winutil.h"
#endif

using namespace std::string_view_literals;

namespace celestia::util
{

#ifdef _WIN32
namespace
{
constexpr std::string_view filePrefix = "file:///"sv;
constexpr std::wstring_view extPrefix = L"\\\\?\\"sv;
} // end unnamed namespace
#endif

std::string
BuildInfoURL(std::string_view infoUrl, const std::filesystem::path &resPath)
{
    if (infoUrl.starts_with("http://"sv) || infoUrl.starts_with("https://"sv))
        return std::string{ infoUrl };

    std::error_code ec;
    std::filesystem::path canonical = std::filesystem::canonical(resPath / U8Path(infoUrl), ec);
    if (ec)
        return {};

#ifdef _WIN32
    std::wstring_view canonicalView = canonical.native();
    // Remove extended filepath prefix if any
    if (canonicalView.starts_with(extPrefix))
        canonicalView = canonicalView.substr(extPrefix.size());

    std::string fileUrl{ filePrefix };
    WideToUTF8(canonicalView, fileUrl);
    // Make sure the UTF-8 conversion worked
    if (fileUrl.size() == filePrefix.size())
        return {};

    std::ranges::replace(fileUrl, '\\', '/');
    return fileUrl;
#else
    return fmt::format("file://{}", canonical.native());
#endif
}

} // end namespace celestia::util
