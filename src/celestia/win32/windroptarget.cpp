// windroptarget.cpp
//
// Copyright (C) 2023, Celestia Development Team
//
// Extracted from winmain.cpp:
// Copyright (C) 2001-2007, Chris Laurel <claurel@shatters.net>
//
// A very minimal IDropTarget interface implementation.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "windroptarget.h"

#include <array>
#include <string_view>

#include <celestia/celestiacore.h>

#include "tstring.h"

using namespace std::string_view_literals;

namespace celestia::win32
{

namespace
{

constexpr tstring_view UniformResourceLocatorFormat = TEXT("UniformResourceLocator"sv);

} // end unnamed namespace

CelestiaDropTarget::CelestiaDropTarget(CelestiaCore* _appCore) :
    appCore(_appCore)
{
}

HRESULT
CelestiaDropTarget::QueryInterface(REFIID iid, void** ppvObject)
{
    if (iid == IID_IUnknown || iid == IID_IDropTarget)
    {
        *ppvObject = this;
        AddRef();
        return ResultFromScode(S_OK);
    }
    else
    {
        *ppvObject = NULL;
        return ResultFromScode(E_NOINTERFACE);
    }
}

ULONG
CelestiaDropTarget::AddRef(void)
{
    return ++refCount;
}

ULONG
CelestiaDropTarget::Release(void)
{
    if (--refCount == 0)
    {
        delete this;
        return 0;
    }

    return refCount;
}

STDMETHODIMP
CelestiaDropTarget::DragEnter(IDataObject* pDataObject,
                              DWORD grfKeyState,
                              POINTL pt,
                              DWORD* pdwEffect)
{
    return S_OK;
}

STDMETHODIMP
CelestiaDropTarget::DragOver(DWORD grfKeyState,
                             POINTL pt,
                             DWORD* pdwEffect)
{
    return S_OK;
}

STDMETHODIMP
CelestiaDropTarget::DragLeave(void)
{
    return S_OK;
}


STDMETHODIMP
CelestiaDropTarget::Drop(IDataObject* pDataObject,
                         DWORD grfKeyState,
                         POINTL pt,
                         DWORD* pdwEffect)
{
    IEnumFORMATETC* enumFormat = nullptr;
    HRESULT hr = pDataObject->EnumFormatEtc(DATADIR_GET, &enumFormat);
    if (FAILED(hr) || enumFormat == nullptr)
        return E_FAIL;

    FORMATETC format;
    ULONG nFetched;
    while (enumFormat->Next(1, &format, &nFetched) == S_OK)
    {
        std::array<TCHAR, 512> buf;
        if (GetClipboardFormatName(format.cfFormat, buf.data(), static_cast<int>(buf.size() - 1)) == 0 ||
            tstring_view(buf.data()) != UniformResourceLocatorFormat)
        {
            continue;
        }

        STGMEDIUM medium;
        if (pDataObject->GetData(&format, &medium) == S_OK &&
            medium.tymed == TYMED_HGLOBAL && medium.hGlobal != 0)
        {
            auto s = static_cast<const char*>(GlobalLock(medium.hGlobal));
            appCore->goToUrl(s);
            GlobalUnlock(medium.hGlobal);
            break;
        }
    }

    enumFormat->Release();

    return E_FAIL;
}

} // end namespace celestia::win32
