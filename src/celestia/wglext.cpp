// wglext.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// Set up pointers for WGL functions.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "celengine/gl.h"
//#define WGL_WGLEXT_PROTOTYPES 1
#include "wglext.h"
#include <windows.h>
#include <cstdio>
#include <vector>

using namespace std;

static vector<string> supportedExtensions;


extern "C"
{
PFNWGLGETEXTENSIONSSTRINGARBPROC    wglGetExtensionsStringARB    = NULL;

// WGL_ARB_pixel_format functions
PFNWGLGETPIXELFORMATATTRIBIVARBPROC wglGetPixelFormatAttribivARB = NULL;
PFNWGLGETPIXELFORMATATTRIBFVARBPROC wglGetPixelFormatAttribfvARB = NULL;
PFNWGLCHOOSEPIXELFORMATARBPROC      wglChoosePixelFormatARB      = NULL;

// WGL_ARB_pbuffer functions
PFNWGLCREATEPBUFFERARBPROC          wglCreatePbufferARB          = NULL;
PFNWGLGETPBUFFERDCARBPROC           wglGetPbufferDCARB           = NULL;
PFNWGLRELEASEPBUFFERDCARBPROC       wglReleasePbufferDCARB       = NULL;
PFNWGLDESTROYPBUFFERARBPROC         wglDestroyPbufferARB         = NULL;
PFNWGLQUERYPBUFFERARBPROC           wglQueryPbufferARB           = NULL;
};


// Select a floating point pixel format
int FindFormatFloat(HDC hDC)
{
    int ifmtList[] = {
        WGL_DRAW_TO_PBUFFER_ARB,       TRUE,
        WGL_SUPPORT_OPENGL_ARB,        TRUE,
        //WGL_DOUBLE_BUFFER_ARB,         TRUE,
        WGL_PIXEL_TYPE_ARB,            WGL_TYPE_RGBA_FLOAT_ATI,
        WGL_DEPTH_BITS_ARB,            24,
        WGL_COLOR_BITS_ARB,             0,
        WGL_RED_BITS_ARB,               16,
        WGL_GREEN_BITS_ARB,             16,
        WGL_BLUE_BITS_ARB,              16,
        WGL_ALPHA_BITS_ARB,             0,
        WGL_ACCUM_BITS_ARB,             0,
        WGL_STENCIL_BITS_ARB,           8,
        WGL_BIND_TO_TEXTURE_RGB_ARB,    TRUE,
        0
    };

    int             pixelFormatIndex = -1;
    int             pixFormats[256];
    unsigned int    numFormats;

    wglChoosePixelFormatARB(hDC, ifmtList, NULL, 256, pixFormats, &numFormats);

    if (numFormats != 0)
    {
        for (unsigned int i = 0; i < numFormats; i++)
        {
            int results[7];
            int query[] = { WGL_COLOR_BITS_ARB,
                            WGL_DEPTH_BITS_ARB,
                            WGL_STENCIL_BITS_ARB,
                            WGL_RED_BITS_ARB,
                            WGL_GREEN_BITS_ARB,
                            WGL_BLUE_BITS_ARB,
                            WGL_ALPHA_BITS_ARB };

            wglGetPixelFormatAttribivARB(hDC, pixFormats[i], 0,
                                         7, query, results);

            // Select an fp16 format.  No existing hardware fully supports
            // fp32 render targets (no alpha blending, etc.), and fp16 is
            // probably good enough anyway.
            if (results[0] == 64 && results[3] == 16)
                return pixFormats[i];
        }
    }

    return -1;
}


// Called when dummy window is opened. Its only job is to get hold
// of wgl extensions.
static void WGLCallback(HWND wnd)
{
    if (wnd == NULL)
        return;
    
    HDC	hdc = GetDC(wnd);

    if (hdc == NULL)
        return;

    PIXELFORMATDESCRIPTOR pfd = { 
        sizeof(PIXELFORMATDESCRIPTOR),  //  size of this pfd 
        1,                              // version number 
        PFD_DRAW_TO_WINDOW |            // support window 
        PFD_SUPPORT_OPENGL |            // support OpenGL 
        PFD_DOUBLEBUFFER,		// double buffered 
        PFD_TYPE_RGBA,                  // RGBA type 
        24,                             // 24-bit color depth 
        0, 0, 0, 0, 0, 0,		// color bits ignored 
        0,                              // no alpha buffer 
        0,                              // shift bit ignored 
        0,                              // no accumulation buffer 
        0, 0, 0, 0,                     // accum bits ignored 
        24,                             // 24-bit z-buffer     
        0,                              // no stencil buffer 
        0,                              // no auxiliary buffer 
        PFD_MAIN_PLANE,                 // main layer 
        0,                              // reserved 
        0, 0, 0                         // layer masks ignored 
    }; 

    unsigned int	pixelFormat = ChoosePixelFormat(hdc, &pfd);

    DescribePixelFormat(hdc, pixelFormat, sizeof(PIXELFORMATDESCRIPTOR), &pfd);
    if (!SetPixelFormat(hdc, pixelFormat, &pfd))
        return;

    HGLRC context = wglCreateContext(hdc);
    if (context == NULL)
        return;

    wglMakeCurrent(hdc, context);

    wglGetExtensionsStringARB = (PFNWGLGETEXTENSIONSSTRINGARBPROC) wglGetProcAddress("wglGetExtensionsStringARB");
    if (wglGetExtensionsStringARB == NULL)
        return;

    const char	*ext = wglGetExtensionsStringARB(hdc);
    char        buff[100];

    if (ext == NULL)
        return;

    while (*ext != '\0')
    {
        // TODO: This is prone to buffer overflows if the driver reports a
        // bad extension string.
        sscanf(ext, "%s", buff);

        // Check for required extension and fill in pointers
        if (strcmp(buff, "WGL_ARB_pixel_format") == 0)
        {
            wglGetPixelFormatAttribivARB = (PFNWGLGETPIXELFORMATATTRIBIVARBPROC) wglGetProcAddress("wglGetPixelFormatAttribivARB");
            wglGetPixelFormatAttribfvARB = (PFNWGLGETPIXELFORMATATTRIBFVARBPROC) wglGetProcAddress("wglGetPixelFormatAttribfvARB");
            wglChoosePixelFormatARB      = (PFNWGLCHOOSEPIXELFORMATARBPROC) wglGetProcAddress("wglChoosePixelFormatARB");
            
            if (wglChoosePixelFormatARB      != NULL &&
                wglGetPixelFormatAttribivARB != NULL &&
                wglGetPixelFormatAttribfvARB != NULL)
            {
                supportedExtensions.push_back(buff);
            }
        }
        else if (strcmp(buff, "WGL_ARB_pbuffer") == 0)
        {
            wglCreatePbufferARB    = (PFNWGLCREATEPBUFFERARBPROC) wglGetProcAddress("wglCreatePbufferARB");
            wglGetPbufferDCARB     = (PFNWGLGETPBUFFERDCARBPROC) wglGetProcAddress("wglGetPbufferDCARB");
            wglReleasePbufferDCARB = (PFNWGLRELEASEPBUFFERDCARBPROC) wglGetProcAddress("wglReleasePbufferDCARB");
            wglDestroyPbufferARB   = (PFNWGLDESTROYPBUFFERARBPROC) wglGetProcAddress("wglDestroyPbufferARB");
            wglQueryPbufferARB     = (PFNWGLQUERYPBUFFERARBPROC) wglGetProcAddress("wglQueryPbufferARB");
            if (wglCreatePbufferARB         != NULL &&
                wglGetPbufferDCARB          != NULL &&
                wglReleasePbufferDCARB      != NULL &&
                wglDestroyPbufferARB        != NULL &&
                wglQueryPbufferARB          != NULL)
            {
                supportedExtensions.push_back(buff);
            }
        }
        else if (strcmp(buff, "WGL_ARB_multisample") == 0)
        {
            //wglSampleCoverageARB = (PFNWGLSAMPLECOVERAGEARBPROC) wglGetProcAddres("wglSampleCoverageARB");
            supportedExtensions.push_back(buff);
        }
        else if (strcmp(buff, "WGL_ATI_pixel_format_float") == 0)
        {
            supportedExtensions.push_back(buff);
        }
        else if (strcmp(buff, "WGL_ARB_render_texture") == 0)
        {
        }

        ext += strlen(buff) + 1;					
    }
    
    // Close down this context
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(context);
    ReleaseDC(wnd, hdc);
}


// This window exists only to call WGLCallback when its created
static LRESULT CALLBACK WGLWindProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) 
    { 
    case WM_CREATE:
        {
            CREATESTRUCT *cs = (CREATESTRUCT *) lParam;
            WGLCallback(hwnd);
            return -1; 
	}
 
    default: 
        return DefWindowProc(hwnd, uMsg, wParam, lParam); 
    } 

    return 0; 
}


// Create a dummy window to initialise WGL extensions.
void InitWGLExtensions(HINSTANCE appInstance)
{
    WNDCLASS wglClass = {
        0, WGLWindProc, 0, 0, appInstance, NULL, NULL, NULL, NULL, "InitWGL"
    };
    ATOM classId  = RegisterClass(&wglClass);	
    HWND hwnd = CreateWindow("InitWGL", "",
                             0, 0, 0, 0, 0,
                             (HWND) NULL,
                             (HMENU) NULL,
                             appInstance,
                             NULL);
}


bool WGLExtensionSupported(const string& extName)
{
    for (unsigned int i = 0; i < supportedExtensions.size(); i++)
    {
        if (supportedExtensions[i] == extName)
            return true;
    }

    return false;
}
