#pragma once

#include <string>
#include <string_view>

#include <celcompat/filesystem.h>

namespace celestia::util
{
std::string BuildInfoURL(std::string_view infoUrl, const fs::path &resPath);
}
