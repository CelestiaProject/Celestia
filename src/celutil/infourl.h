#pragma once

#include <string>
#include <celcompat/filesystem.h>

inline std::string
BuildInfoURL(const std::string &infoUrl, const fs::path &resPath)
{
    std::string modifiedURL;
    if (infoUrl.find(':') == std::string::npos && !resPath.empty())
    {
        // Relative URL, the base directory is the current one,
        // not the main installation directory
        std::string p = resPath.string();
        const char *format = p.size() > 1 && p[1] == ':'
            // Absolute Windows path, file:/// is required
            ? "file:///{}/{}"
            : "{}/{}";
        modifiedURL = fmt::format(format, p, infoUrl);
    }
    return modifiedURL.empty() ? infoUrl : modifiedURL;
}
