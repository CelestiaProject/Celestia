#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace celestia::util
{
std::string BuildInfoURL(std::string_view infoUrl, const std::filesystem::path &resPath);
}
