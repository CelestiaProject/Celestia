// appwindow.cpp
//
// Copyright (C) 2020-present, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "appwindow.h"

#include <cctype>
#include <string>
#include <utility>

#include <celengine/glsupport.h>

#include <SDL_clipboard.h>
#include <SDL_keyboard.h>
#include <SDL_keycode.h>
#include <SDL_messagebox.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include <celestia/celestiacore.h>
#include <celutil/gettext.h>
#include <celutil/tzutil.h>
#include "clipboard.h"
#include "gui.h"
#include "settings.h"

namespace celestia::sdl
{

namespace
{

void
setTimezone(CelestiaCore& appCore)
{
    std::string tzName;
    int dstBias;
    if (GetTZInfo(tzName, dstBias))
    {
        appCore.setTimeZoneName(tzName);
        appCore.setTimeZoneBias(dstBias);
    }
}

int
toCelestiaKey(SDL_Keycode key)
{
    switch (key) //NOSONAR
    {
    case SDLK_DOWN:     return CelestiaCore::Key_Down;
    case SDLK_UP:       return CelestiaCore::Key_Up;
    case SDLK_LEFT:     return CelestiaCore::Key_Left;
    case SDLK_RIGHT:    return CelestiaCore::Key_Right;
    case SDLK_END:      return CelestiaCore::Key_End;
    case SDLK_HOME:     return CelestiaCore::Key_Home;
    case SDLK_PAGEDOWN: return CelestiaCore::Key_PageDown;
    case SDLK_PAGEUP:   return CelestiaCore::Key_PageUp;
    case SDLK_RETURN:
    case SDLK_ESCAPE:
    case SDLK_BACKSPACE:
    case SDLK_DELETE:
    case SDLK_TAB:
    case SDLK_SPACE:
         return key;

    case SDLK_F1:       return CelestiaCore::Key_F1;
    case SDLK_F2:       return CelestiaCore::Key_F2;
    case SDLK_F3:       return CelestiaCore::Key_F3;
    case SDLK_F4:       return CelestiaCore::Key_F4;
    case SDLK_F5:       return CelestiaCore::Key_F5;
    case SDLK_F6:       return CelestiaCore::Key_F6;
    case SDLK_F7:       return CelestiaCore::Key_F7;
    case SDLK_F8:       return CelestiaCore::Key_F8;
    case SDLK_F9:       return CelestiaCore::Key_F9;
    case SDLK_F10:      return CelestiaCore::Key_F10;
    case SDLK_F11:      return CelestiaCore::Key_F11;
    case SDLK_F12:      return CelestiaCore::Key_F12;

    case SDLK_KP_0:     return CelestiaCore::Key_NumPad0;
    case SDLK_KP_1:     return CelestiaCore::Key_NumPad1;
    case SDLK_KP_2:     return CelestiaCore::Key_NumPad2;
    case SDLK_KP_3:     return CelestiaCore::Key_NumPad3;
    case SDLK_KP_4:     return CelestiaCore::Key_NumPad4;
    case SDLK_KP_5:     return CelestiaCore::Key_NumPad5;
    case SDLK_KP_6:     return CelestiaCore::Key_NumPad6;
    case SDLK_KP_7:     return CelestiaCore::Key_NumPad7;
    case SDLK_KP_8:     return CelestiaCore::Key_NumPad8;
    case SDLK_KP_9:     return CelestiaCore::Key_NumPad9;
    case SDLK_KP_DECIMAL:
        return CelestiaCore::Key_NumPadDecimal;

    default:
        return (key >= 32 && key <= 127) ? key : -1;
    }
}

int
toCelestiaButton(int button)
{
    switch (button)
    {
    case SDL_BUTTON_LEFT:
        return CelestiaCore::LeftButton;
    case SDL_BUTTON_MIDDLE:
        return CelestiaCore::MiddleButton;
    case SDL_BUTTON_RIGHT:
        return CelestiaCore::RightButton;
    default:
        return -1;
    }
}

float
getScalingFactor(const CelestiaCore& appCore)
{
    return static_cast<float>(appCore.getScreenDpi()) / 96.0f;
}

#ifdef __EMSCRIPTEN__
void
mainRunLoopHandler(void* arg)
{
    auto app = static_cast<AppWindow*>(arg);
    if (!app->update())
        emscripten_cancel_main_loop();
}
#endif

} // end unnamed namespace

class AppWindow::Alerter : public CelestiaCore::Alerter
{
public:
    explicit Alerter(SDL_Window* win) : window(win) {}

    void fatalError(const std::string& msg) override;

private:
    SDL_Window* window;
};

void
AppWindow::Alerter::fatalError(const std::string& msg)
{
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Fatal Error", msg.c_str(), window);
}

AppWindow::AppWindow(Private,
                     const std::shared_ptr<Environment>& environment,
                     UniqueWindow&& window,
                     UniqueGLContext&& context,
                     bool isFullscreen) :
    m_environment(environment),
    m_window(std::move(window)),
    m_context(std::move(context)),
    m_isFullscreen(isFullscreen)
{
}

AppWindow::~AppWindow() = default;

void
AppWindow::dumpGLInfo() const
{
    const char* s;
    s = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    if (s != nullptr)
        fmt::print("GL Version: {}\n", s);

    s = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
    if (s != nullptr)
        fmt::print("GL Vendor: {}\n", s);

    s = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    if (s != nullptr)
        fmt::print("GL Renderer: {}\n", s);

    s = reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION));
    if (s != nullptr)
        fmt::print("GLSL Version: {}\n", s);
}

bool
AppWindow::run(const Settings& settings)
{
    m_appCore = std::make_unique<CelestiaCore>();
    m_alerter = std::make_unique<Alerter>(m_window.get());
    m_appCore->setAlerter(m_alerter.get());
    if (!m_appCore->initSimulation())
        return false;

    if (float screenDpi = 96.0f; SDL_GetDisplayDPI(0, &screenDpi, nullptr, nullptr) == 0)
        m_appCore->setScreenDpi(static_cast<int>(screenDpi));

    m_appCore->initRenderer(settings.textureResolution);

    auto renderer = m_appCore->getRenderer();
    const auto* config = m_appCore->getConfig();

    renderer->setShadowMapSize(config->renderDetails.ShadowMapSize);
    renderer->setSolarSystemMaxDistance(config->renderDetails.SolarSystemMaxDistance);

    settings.apply(m_appCore.get());

    m_appCore->start();
    setTimezone(*m_appCore);
    SDL_GL_GetDrawableSize(m_window.get(), &m_width, &m_height);
    m_appCore->resize(m_width, m_height);

    SDL_StartTextInput();

    m_gui = Gui::create(m_window.get(), m_context.get(), m_appCore.get(), *m_environment);
    if (m_gui == nullptr)
        return false;

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop_arg(&mainRunLoopHandler, this, 0, 1);
#else
    while (update())
    {
        // loop until done
    }
#endif

    auto saveSettings = Settings::fromApplication(*this, m_appCore.get());
    saveSettings.save(m_environment->getSettingsPath());

    SDL_StopTextInput();

    return true;
}

bool
AppWindow::update()
{
    for (;;)
    {
        SDL_Event event;
        if (SDL_PollEvent(&event) == 0)
            break;

        m_gui->processEvent(event);
        switch (event.type)
        {
        case SDL_QUIT:
            return false;
        case SDL_TEXTINPUT:
            handleTextInputEvent(event.text);
            break;
        case SDL_KEYDOWN:
            handleKeyDownEvent(event.key);
            break;
        case SDL_KEYUP:
            handleKeyUpEvent(event.key);
            break;
        case SDL_MOUSEBUTTONDOWN:
            handleMouseButtonDownEvent(event.button);
            break;
        case SDL_MOUSEBUTTONUP:
            handleMouseButtonUpEvent(event.button);
            break;
        case SDL_MOUSEWHEEL:
            handleMouseWheelEvent(event.wheel);
            break;
        case SDL_MOUSEMOTION:
            handleMouseMotionEvent(event.motion);
            break;
        case SDL_WINDOWEVENT:
            handleWindowEvent(event.window);
            break;
        default:
            break;
        }
    }

    m_appCore->tick();
    m_appCore->draw();
    m_gui->render();
    SDL_GL_SwapWindow(m_window.get());

    return !m_gui->isQuitRequested();
}

void
AppWindow::handleTextInputEvent(const SDL_TextInputEvent& event)
{
    if (m_gui->wantCaptureKeyboard())
        return;

    m_appCore->charEntered(event.text, 0);
}

void
AppWindow::handleKeyDownEvent(const SDL_KeyboardEvent& event)
{
    if (m_gui->wantCaptureKeyboard())
        return;

    switch (event.keysym.sym)
    {
    case SDLK_TAB:
    case SDLK_BACKSPACE:
    case SDLK_DELETE:
    case SDLK_ESCAPE:
        m_appCore->charEntered(static_cast<char>(event.keysym.sym), 0);
        [[fallthrough]];
    case SDLK_RETURN:
        return;
    default:
        break;
    }

    int key = toCelestiaKey(event.keysym.sym);
    if (key == -1)
        return;

    int mod = 0;
    if ((event.keysym.mod & KMOD_CTRL) != 0)
    {
        mod |= CelestiaCore::ControlKey;

        if (int k = std::tolower(static_cast<unsigned char>(key)); k >= 'a' && k <= 'z')
        {
            switch (k)
            {
            case 'c':
                doCopy(*m_appCore);
                break;
            case 'v':
                doPaste(*m_appCore);
                break;
            default:
                key = k + 1 - 'a';
                m_appCore->charEntered(static_cast<char>(key), mod);
            }
            return;
        }
    }

    if ((event.keysym.mod & KMOD_SHIFT) != 0)
        mod |= CelestiaCore::ShiftKey;

    m_appCore->keyDown(key, mod);
}

void
AppWindow::handleKeyUpEvent(const SDL_KeyboardEvent& event)
{
    if (m_gui->wantCaptureKeyboard())
        return;

    if (event.keysym.sym == SDLK_RETURN)
    {
        if ((event.keysym.mod & KMOD_ALT) != 0)
            toggleFullscreen();
        else
            m_appCore->charEntered(SDLK_RETURN, 0);
        return;
    }

    int key = toCelestiaKey(event.keysym.sym);
    if (key == -1)
        return;

    int mod = 0;
    if ((event.keysym.mod & KMOD_CTRL) != 0)
    {
        mod |= CelestiaCore::ControlKey;
        if (key >= '0' && key <= '9')
        {
            m_appCore->charEntered(static_cast<char>(key), mod);
            return;
        }
    }

    if ((event.keysym.mod & KMOD_SHIFT) != 0)
        mod |= CelestiaCore::ShiftKey;

    m_appCore->keyUp(key, mod);
}

void
AppWindow::handleMouseButtonDownEvent(const SDL_MouseButtonEvent& event)
{
    if (m_gui->wantCaptureMouse())
        return;

    int button = toCelestiaButton(event.button);
    if (button == -1)
        return;

    m_lastX = event.x;
    m_lastY = event.y;
    float scaling = getScalingFactor(*m_appCore);
    m_appCore->mouseButtonDown(static_cast<float>(event.x) * scaling,
                               static_cast<float>(event.y) * scaling,
                               button);
}

void
AppWindow::handleMouseButtonUpEvent(const SDL_MouseButtonEvent& event)
{
    if (m_gui->wantCaptureMouse())
        return;

    int button = toCelestiaButton(event.button);
    if (button == -1)
        return;

    if ((button & (CelestiaCore::LeftButton | CelestiaCore::RightButton)) != 0
        && SDL_ShowCursor(SDL_QUERY) == SDL_DISABLE)
    {
        SDL_ShowCursor(SDL_ENABLE);
#ifndef __EMSCRIPTEN__
        // Mouse warping is not supported in browser
        SDL_WarpMouseInWindow(m_window.get(), m_lastX, m_lastY);
#endif
    }

    m_lastX = event.x;
    m_lastY = event.y;
    float scaling = getScalingFactor(*m_appCore);
    m_appCore->mouseButtonUp(static_cast<float>(event.x) * scaling,
                             static_cast<float>(event.y) * scaling,
                             button);
}

void
AppWindow::handleMouseWheelEvent(const SDL_MouseWheelEvent& event)
{
    if (m_gui->wantCaptureMouse())
        return;

    float scaling = getScalingFactor(*m_appCore);
    if (event.y > 0) // scroll up
        m_appCore->mouseWheel(-scaling, 0);
    else if (event.y < 0) // scroll down
        m_appCore->mouseWheel(scaling, 0);
}

void
AppWindow::handleMouseMotionEvent(const SDL_MouseMotionEvent& event)
{
    if (m_gui->wantCaptureMouse())
        return;

    int buttons = 0;
    if ((event.state & SDL_BUTTON_LMASK) != 0)
        buttons |= CelestiaCore::LeftButton;
    if ((event.state & SDL_BUTTON_RMASK) != 0)
        buttons |= CelestiaCore::RightButton;

    if (buttons == 0)
        return;

    Sint32 dx = event.x - m_lastX;
    Sint32 dy = event.y - m_lastY;
    if (SDL_ShowCursor(SDL_QUERY) == SDL_ENABLE)
    {
        SDL_ShowCursor(SDL_DISABLE);
        m_lastX = event.x;
        m_lastY = event.y;
    }

    float scaling = getScalingFactor(*m_appCore);
    m_appCore->mouseMove(static_cast<float>(dx) * scaling,
                         static_cast<float>(dy) * scaling,
                         buttons);

#ifdef __EMSCRIPTEN__
    // Mouse warping is not supported in browser
    m_lastX = event.x;
    m_lastY = event.y;
#else
    SDL_WarpMouseInWindow(m_window.get(), m_lastX, m_lastY);
#endif
}

void
AppWindow::handleWindowEvent(const SDL_WindowEvent& event)
{
    if (event.event == SDL_WINDOWEVENT_RESIZED)
    {
        SDL_GL_GetDrawableSize(m_window.get(), &m_width, &m_height);
        m_appCore->resize(m_width, m_height);
    }
}

void
AppWindow::toggleFullscreen()
{
    if (m_isFullscreen)
    {
        if (SDL_SetWindowFullscreen(m_window.get(), 0) == 0)
            m_isFullscreen = false;
        else
            return;
    }
    else if (SDL_SetWindowFullscreen(m_window.get(), SDL_WINDOW_FULLSCREEN_DESKTOP) == 0)
        m_isFullscreen = true;
    else
        return;

    int width;
    int height;
    SDL_GetWindowSize(m_window.get(), &width, &height);
    m_appCore->resize(width, height);
}

void
AppWindow::getSize(int& width, int& height) const
{
    SDL_GetWindowSize(m_window.get(), &width, &height);
}

void
AppWindow::getPosition(int& x, int& y) const
{
    SDL_GetWindowPosition(m_window.get(), &x, &y);
}

} // end namespace celestia::sdl
