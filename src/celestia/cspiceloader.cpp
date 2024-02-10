// cspiceloader.cpp
//
// Copyright (C) 2024, Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "cspiceloader.h"

#include <celephem/spiceinterface.h>
#include <celutil/logger.h>

using celestia::util::GetLogger;

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace celestia
{

namespace
{

template<typename H, typename T>
bool
SetInterfacePtr(H handle, T& destination, const char* name)
{
#ifdef _WIN32
    destination = reinterpret_cast<T>(GetProcAddress(handle, name));
#else
    destination = reinterpret_cast<T>(dlsym(handle, name));
#endif
    if (destination == nullptr)
    {
        GetLogger()->error("Could not find function {} in cspice library\n", name);
        return false;
    }

    return true;
}

} // end unnamed namespace

SpiceLibraryWrapper::~SpiceLibraryWrapper()
{
    if (handle == nullptr)
        return;

    ephem::SetSpiceInterface(nullptr);

#ifdef _WIN32
    FreeLibrary(static_cast<HMODULE>(handle));
#else
    dlclose(handle);
#endif
}

std::unique_ptr<SpiceLibraryWrapper>
LoadSpiceLibrary()
{
#if defined(_WIN32)
    HMODULE handle = LoadLibraryExW(L"plugins\\cspice.dll", NULL, LOAD_LIBRARY_SEARCH_APPLICATION_DIR);
#elif defined(__APPLE__)
    void* handle = dlopen("libcspice.dylib", RTLD_LAZY | RTLD_LOCAL);
#else
    void* handle = dlopen("libcspice.so", RTLD_LAZY | RTLD_LOCAL);
#endif

    if (!handle)
    {
        GetLogger()->warn("Could not find SPICE toolkit, SPICE will not be enabled\n");
        return nullptr;
    }

    std::unique_ptr<SpiceLibraryWrapper> library(new SpiceLibraryWrapper(handle));

    auto spice = std::make_unique<ephem::SpiceInterface>();
    if (SetInterfacePtr(handle, spice->bodn2c_c, "bodn2c_c") &&
        SetInterfacePtr(handle, spice->card_c,   "card_c"  ) &&
        SetInterfacePtr(handle, spice->erract_c, "erract_c") &&
        SetInterfacePtr(handle, spice->failed_c, "failed_c") &&
        SetInterfacePtr(handle, spice->furnsh_c, "furnsh_c") &&
        SetInterfacePtr(handle, spice->getmsg_c, "getmsg_c") &&
        SetInterfacePtr(handle, spice->kdata_c,  "kdata_c" ) &&
        SetInterfacePtr(handle, spice->ktotal_c, "ktotal_c") &&
        SetInterfacePtr(handle, spice->pxform_c, "pxform_c") &&
        SetInterfacePtr(handle, spice->reset_c,  "reset_c" ) &&
        SetInterfacePtr(handle, spice->scard_c,  "scard_c" ) &&
        SetInterfacePtr(handle, spice->spkcov_c, "spkcov_c") &&
        SetInterfacePtr(handle, spice->spkgeo_c, "spkgeo_c") &&
        SetInterfacePtr(handle, spice->spkgps_c, "spkgps_c") &&
        SetInterfacePtr(handle, spice->tkvrsn_c, "tkvrsn_c") &&
        SetInterfacePtr(handle, spice->wnfetd_c, "wnfetd_c") &&
        SetInterfacePtr(handle, spice->wnincd_c, "wnincd_c"))
    {
        GetLogger()->info("Loaded SPICE toolkit version {}\n", spice->tkvrsn_c("TOOLKIT"));

        // Set the error behavior to the RETURN action, so that
        // Celestia do its own handling of SPICE errors.
        spice->erract_c("SET", 0, const_cast<SpiceChar*>("RETURN"));
        ephem::SetSpiceInterface(spice.release());
    }
    else
    {
        library.reset();
    }

    return library;
}

} // end namespace celestia
