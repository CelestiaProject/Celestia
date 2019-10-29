#pragma once

#include <config.h>

//#if __cplusplus >= 201703L
//#include <filesystem>
//namespace fs = std::filesystem;
#if defined(HAVE_EXPERIMENTAL_FILESYSTEM)
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#include "fs.h"
namespace fs = celestia::filesystem;
#endif
