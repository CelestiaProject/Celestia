#pragma once

#include <config.h>

#ifdef HAVE_FILESYSTEM
#include <filesystem>
namespace fs = std::filesystem;
#elif defined(HAVE_EXPERIMENTAL_FILESYSTEM)
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#include "fs.h"
namespace fs = celestia::filesystem;
#endif
