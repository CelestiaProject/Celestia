// sdlmain.cpp
//
// Copyright (C) 2020-present, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cctype>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string_view>
#include <system_error>
#include <fmt/format.h>
#include <celcompat/filesystem.h>
#include <celengine/glsupport.h>
#include <celestia/celestiacore.h>
#include <celestia/configfile.h>
#include <celestia/url.h>
#include <celutil/gettext.h>
#include <celutil/localeutil.h>
#include <celutil/tzutil.h>
#include <SDL.h>
#ifdef GL_ES
#include <SDL_opengles2.h>
#else
#include <SDL_opengl.h>
#endif
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

namespace celestia::sdl
{

namespace
{

class SDL_Alerter : public CelestiaCore::Alerter
{
    SDL_Window* window { nullptr };

 public:
    void fatalError(const std::string& msg)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                                 "Fatal Error",
                                 msg.c_str(),
                                 window);
    }
};

class SDL_Application
{
 public:
    SDL_Application() = delete;
    SDL_Application(std::string_view name, int w, int h) :
        m_appName       { name },
        m_windowWidth   { w },
        m_windowHeight  { h }
    {
    }
    ~SDL_Application();

    enum class EventHandleResult
    {
        Handled,
        Ignored,
        NoNewEvent,
        Quit,
    };

    enum class RunLoopState
    {
        Normal,
        Quit,
    };

    static std::shared_ptr<SDL_Application> init(std::string_view, int, int);

    bool createOpenGLWindow();

    bool initCelestiaCore();
    void run();
    EventHandleResult handleEvent();
    RunLoopState update();
    std::string_view getError() const;
    float getScalingFactor() const;

 private:
    void display();

    // handlers
    void handleTextInputEvent(const SDL_TextInputEvent &event);
    void handleKeyPressEvent(const SDL_KeyboardEvent &event);
    void handleKeyReleaseEvent(const SDL_KeyboardEvent &event);
    void handleMousePressEvent(const SDL_MouseButtonEvent &event);
    void handleMouseReleaseEvent(const SDL_MouseButtonEvent &event);
    void handleMouseWheelEvent(const SDL_MouseWheelEvent &event);
    void handleMouseMotionEvent(const SDL_MouseMotionEvent &event);
    void handleWindowEvent(const SDL_WindowEvent &event);

    // aux functions
    void toggleFullscreen();
    void copyURL();
    void pasteURL();
    void configure() const;

    // state variables
    std::string m_appName;
    int m_windowWidth;
    int m_windowHeight;

    int m_lastX                 { 0 };
    int m_lastY                 { 0 };
    bool m_cursorVisible        { true };
    bool m_fullscreen           { false };

    CelestiaCore *m_appCore     { nullptr };
    SDL_Window   *m_mainWindow  { nullptr };
    SDL_GLContext m_glContext   { nullptr };
};

std::shared_ptr<SDL_Application>
SDL_Application::init(std::string_view name, int w, int h)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
        return nullptr;

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
#ifdef GL_ES
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif
    return std::make_shared<SDL_Application>(name, w, h);
}

SDL_Application::~SDL_Application()
{
    delete m_appCore;

    if (m_glContext != nullptr)
        SDL_GL_DeleteContext(m_glContext);
    if (m_mainWindow != nullptr)
        SDL_DestroyWindow(m_mainWindow);
    SDL_Quit();
}

bool
SDL_Application::createOpenGLWindow()
{
    m_mainWindow = SDL_CreateWindow(m_appName.c_str(),
                                    SDL_WINDOWPOS_CENTERED,
                                    SDL_WINDOWPOS_CENTERED,
                                    m_windowWidth,
                                    m_windowHeight,
                                    SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (m_mainWindow == nullptr)
        return false;

    m_glContext = SDL_GL_CreateContext(m_mainWindow);
    if (m_glContext == nullptr)
        return false;

    // First try to enable adaptive sync and then vsync
    if (SDL_GL_SetSwapInterval(-1) == -1)
        SDL_GL_SetSwapInterval(1);
    return true;
}

std::string_view
SDL_Application::getError() const
{
    return SDL_GetError();
}

float
SDL_Application::getScalingFactor() const
{
    return static_cast<float>(m_appCore->getScreenDpi()) / 96.0f;
}

void
SDL_Application::display()
{
    m_appCore->draw();
    SDL_GL_SwapWindow(m_mainWindow);
}

bool
SDL_Application::initCelestiaCore()
{
    m_appCore = new CelestiaCore();
    m_appCore->setAlerter(new SDL_Alerter());
    if (!m_appCore->initSimulation())
        return false;
    if (float screenDpi = 96.0f; SDL_GetDisplayDPI(0, &screenDpi, nullptr, nullptr) == 0)
        m_appCore->setScreenDpi(static_cast<int>(screenDpi));
    return true;
}

void
SDL_Application::configure() const
{
    auto *renderer     = m_appCore->getRenderer();
    const auto *config = m_appCore->getConfig();

    renderer->setRenderFlags(RenderFlags::DefaultRenderFlags);
    renderer->setShadowMapSize(config->renderDetails.ShadowMapSize);
    renderer->setSolarSystemMaxDistance(config->renderDetails.SolarSystemMaxDistance);
}

SDL_Application::EventHandleResult
SDL_Application::handleEvent()
{
    SDL_Event event;
    if (SDL_PollEvent(&event) == 0)
        return EventHandleResult::NoNewEvent;

    switch (event.type)
    {
    case SDL_QUIT:
        return EventHandleResult::Quit;
    case SDL_TEXTINPUT:
        handleTextInputEvent(event.text);
        break;
    case SDL_KEYDOWN:
        handleKeyPressEvent(event.key);
        break;
    case SDL_KEYUP:
        handleKeyReleaseEvent(event.key);
        break;
    case SDL_MOUSEBUTTONDOWN:
        handleMousePressEvent(event.button);
        break;
    case SDL_MOUSEBUTTONUP:
        handleMouseReleaseEvent(event.button);
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
        return EventHandleResult::Ignored;
    }
    return EventHandleResult::Handled;
}

#ifdef __EMSCRIPTEN__
static void mainRunLoopHandler(void *arg)
{
    auto app = reinterpret_cast<SDL_Application *>(arg);
    if (app->update() == SDL_Application::RunLoopState::Quit)
        emscripten_cancel_main_loop();
}
#endif

void
SDL_Application::run()
{
    m_appCore->initRenderer();
    configure();
    m_appCore->start();

    std::string tzName;
    int dstBias;
    if (GetTZInfo(tzName, dstBias))
    {
        m_appCore->setTimeZoneName(tzName);
        m_appCore->setTimeZoneBias(dstBias);
    }
    SDL_GL_GetDrawableSize(m_mainWindow, &m_windowWidth, &m_windowHeight);
    m_appCore->resize(m_windowWidth, m_windowHeight);

    SDL_StartTextInput();

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop_arg(mainRunLoopHandler, this, 0, 1);
#else
    while (update() != RunLoopState::Quit);
#endif
}

SDL_Application::RunLoopState
SDL_Application::update()
{
    bool stop = false;
    while (!stop)
    {
        auto result = handleEvent();
        switch (result)
        {
        case EventHandleResult::Quit:
            return RunLoopState::Quit;
        case EventHandleResult::NoNewEvent:
            stop = true;
        default:
            break;
        }
    }
    m_appCore->tick();
    display();
    return RunLoopState::Normal;
}

int
toCelestiaKey(SDL_Keycode key)
{
    switch (key)
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
        if (key >= 32 && key <= 127)
            return key;
        return -1;
    }
}

void
SDL_Application::handleKeyPressEvent(const SDL_KeyboardEvent &event)
{
    switch (event.keysym.sym)
    {
    case SDLK_TAB:
    case SDLK_BACKSPACE:
    case SDLK_DELETE:
    case SDLK_ESCAPE:
        m_appCore->charEntered(event.keysym.sym, 0);
    case SDLK_RETURN:
        return;
    }

    int key = toCelestiaKey(event.keysym.sym);
    if (key == -1)
        return;

    int mod = 0;
    if ((event.keysym.mod & KMOD_CTRL) != 0)
    {
        mod |= CelestiaCore::ControlKey;

        if (int k = std::tolower(key); k >= 'a' && k <= 'z')
        {
            switch (k)
            {
            case 'c':
                copyURL();
                break;
            case 'v':
                pasteURL();
                break;
            default:
                key = k + 1 - 'a';
                m_appCore->charEntered(key, mod);
            }
            return;
        }
    }
    if ((event.keysym.mod & KMOD_SHIFT) != 0)
        mod |= CelestiaCore::ShiftKey;

    m_appCore->keyDown(key, mod);
}

void
SDL_Application::handleKeyReleaseEvent(const SDL_KeyboardEvent &event)
{
    if (event.keysym.sym == SDLK_RETURN)
    {
        if ((event.keysym.mod & KMOD_ALT) != 0)
            toggleFullscreen();
        else
            m_appCore->charEntered(SDLK_RETURN, 0);
    }
    else
    {
        int key = toCelestiaKey(event.keysym.sym);
        if (key == -1)
            return;
        int mod = 0;
        if ((event.keysym.mod & KMOD_CTRL) != 0)
        {
            mod |= CelestiaCore::ControlKey;
            if (key >= '0' && key <= '9')
                m_appCore->charEntered(key, mod);
        }
        if ((event.keysym.mod & KMOD_SHIFT) != 0)
            mod |= CelestiaCore::ShiftKey;

        m_appCore->keyUp(key, 0);
    }
}

void
SDL_Application::handleTextInputEvent(const SDL_TextInputEvent &event)
{
    m_appCore->charEntered(event.text, 0);
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

void
SDL_Application::handleMousePressEvent(const SDL_MouseButtonEvent &event)
{
    int button = toCelestiaButton(event.button);
    if (button == -1)
        return;

    m_lastX = event.x;
    m_lastY = event.y;
    float scaling = getScalingFactor();
    m_appCore->mouseButtonDown(static_cast<float>(event.x) * scaling, static_cast<float>(event.y) * scaling, button);
}

void
SDL_Application::handleMouseReleaseEvent(const SDL_MouseButtonEvent &event)
{
    int button = toCelestiaButton(event.button);;
    if (button == -1)
        return;

    if (button & (CelestiaCore::LeftButton | CelestiaCore::RightButton))
    {
        if (!m_cursorVisible)
        {
            SDL_ShowCursor(SDL_ENABLE);
            m_cursorVisible = true;
#ifndef __EMSCRIPTEN__
            // Mouse warping is not supported in browser
            SDL_WarpMouseInWindow(m_mainWindow, m_lastX, m_lastY);
#endif
        }
    }
    m_lastX = event.x;
    m_lastY = event.y;
    float scaling = getScalingFactor();
    m_appCore->mouseButtonUp(static_cast<float>(event.x) * scaling, static_cast<float>(event.y) * scaling, button);
}

void
SDL_Application::handleMouseWheelEvent(const SDL_MouseWheelEvent &event)
{
    float scaling = getScalingFactor();
    if (event.y > 0) // scroll up
        m_appCore->mouseWheel(-scaling, 0);
    else if (event.y < 0) // scroll down
        m_appCore->mouseWheel(scaling, 0);
}

void
SDL_Application::handleMouseMotionEvent(const SDL_MouseMotionEvent &event)
{
    if (event.state & (SDL_BUTTON_LMASK | SDL_BUTTON_RMASK))
    {
        int buttons = 0;
        if (event.state & SDL_BUTTON_LMASK)
            buttons |= CelestiaCore::LeftButton;
        if (event.state & SDL_BUTTON_RMASK)
            buttons |= CelestiaCore::RightButton;

        int x = event.x - m_lastX;
        int y = event.y - m_lastY;
        if (m_cursorVisible)
        {
            SDL_ShowCursor(SDL_DISABLE);
            m_cursorVisible = false;
            m_lastX = event.x;
            m_lastY = event.y;
        }

        float scaling = getScalingFactor();
        m_appCore->mouseMove(static_cast<float>(x) * scaling, static_cast<float>(y) * scaling, buttons);

#ifdef __EMSCRIPTEN__
        // Mouse warping is not supported in browser
        m_lastX = event.x;
        m_lastY = event.y;
#else
        SDL_WarpMouseInWindow(m_mainWindow, m_lastX, m_lastY);
#endif
    }
}

void
SDL_Application::handleWindowEvent(const SDL_WindowEvent &event)
{
    switch (event.event)
    {
    case SDL_WINDOWEVENT_RESIZED:
        SDL_GL_GetDrawableSize(m_mainWindow, &m_windowWidth, &m_windowHeight);
        m_appCore->resize(m_windowWidth, m_windowHeight);
        break;
    default:
        break;
    }
}

void
SDL_Application::toggleFullscreen()
{
    int ret = 0;
    if (m_fullscreen)
    {
        ret = SDL_SetWindowFullscreen(m_mainWindow, 0);
        if (ret == 0)
        {
            m_fullscreen = false;
            SDL_SetWindowSize(m_mainWindow, m_windowWidth, m_windowHeight);
            m_appCore->resize(m_windowWidth, m_windowHeight);
        }
    }
    else
    {
        SDL_DisplayMode dm;
        if (SDL_GetDesktopDisplayMode(0, &dm) == 0)
        {
            SDL_SetWindowSize(m_mainWindow, dm.w, dm.h);
            m_appCore->resize(dm.w, dm.h);
        }

        // First try to activate real fullscreen mode
        ret = SDL_SetWindowFullscreen(m_mainWindow, SDL_WINDOW_FULLSCREEN);
        // Then try to emulate fulscreen resizing to the desktop size
        if (ret < 0)
            ret = SDL_SetWindowFullscreen(m_mainWindow, SDL_WINDOW_FULLSCREEN_DESKTOP);
        if (ret == 0)
            m_fullscreen = true;
    }
}

void
SDL_Application::copyURL()
{
    CelestiaState appState(m_appCore);
    appState.captureState();

    if (SDL_SetClipboardText(Url(appState).getAsString().c_str()) == 0)
        m_appCore->flash(_("Copied URL"));
}

void
SDL_Application::pasteURL()
{
    if (SDL_HasClipboardText() != SDL_TRUE)
        return;

    // on error SDL_GetClipboardText returns a new empty string
    char *str = SDL_GetClipboardText(); // don't add const due to SDL_free
    if (*str != '\0' && m_appCore->goToUrl(str))
        m_appCore->flash(_("Pasting URL"));

    SDL_free(str);
}

void
FatalErrorImpl(fmt::string_view format, fmt::format_args args)
{
    auto message = fmt::vformat(format, args);
    int ret = SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                                       "Fatal Error",
                                       message.c_str(),
                                       nullptr);
    if (ret < 0)
        fmt::print(stderr, "{}\n", message);
}

template <typename... Args>
void
FatalError(const char *format, const Args&... args)
{
    FatalErrorImpl(fmt::string_view(format), fmt::make_format_args(args...));
}

void
DumpGLInfo()
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

int
sdlmain(int /* argc */, char ** /* argv */)
{
    CelestiaCore::initLocale();

#ifdef ENABLE_NLS
    bindtextdomain("celestia", LOCALEDIR);
    bind_textdomain_codeset("celestia", "UTF-8");
    bindtextdomain("celestia-data", LOCALEDIR);
    bind_textdomain_codeset("celestia-data", "UTF-8");
    textdomain("celestia");
#endif

    const char *dataDir = getenv("CELESTIA_DATA_DIR");
    if (dataDir == nullptr)
        dataDir = CONFIG_DATA_DIR;

    std::error_code ec;
    fs::current_path(dataDir, ec);
    if (ec)
    {
        FatalError("Cannot chdir to {}, probably due to improper installation", dataDir);
        return 1;
    }

    auto app = SDL_Application::init("Celestia", 640, 480);
    if (app == nullptr)
    {
        FatalError("Could not initialize SDL! Error: {}", SDL_GetError());
        return 2;
    }

    if (!app->initCelestiaCore())
    {
        FatalError("Could not initialize Celestia!");
        return 3;
    }
    if (!app->createOpenGLWindow())
    {
        FatalError("Could not create a OpenGL window! Error: {}", app->getError());
        return 4;
    }

    gl::init();
#ifndef GL_ES
    if (!gl::checkVersion(gl::GL_2_1))
    {
        FatalError("Celestia requires OpenGL 2.1!");
        return 5;
    }
#endif

    DumpGLInfo();

    app->run();

    return 0;
}

} // end unnamed namespace

} // end namespace celestia::sdl

int
main(int argc, char **argv)
{
    return celestia::sdl::sdlmain(argc, argv);
}
