#pragma once

#include <config.h>

#if __cplusplus >= 201703L
#include <filesystem>
namespace fs = std::filesystem;
#elif defined(HAVE_EXPERIMENTAL_FILESYSTEM)
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#include <celutil/fs.h>
namespace fs = celutil::filesystem;
#endif
