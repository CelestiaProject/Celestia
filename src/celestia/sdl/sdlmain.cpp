//#define GL_ES
#include <cctype>
#include <cstring>
#include <iostream>
#include <memory>
#include <fmt/printf.h>
#include <celengine/glsupport.h>
#include <celutil/gettext.h>
#include <celutil/tzutil.h>
#include <SDL.h>
#ifdef GL_ES
#include <SDL_opengles2.h>
#else
#include <SDL_opengl.h>
#endif
#include <unistd.h>
#include <celestia/celestiacore.h>

namespace celestia
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
    SDL_Application(const std::string name, int w, int h) :
        m_appName       { name },
        m_windowWidth   { w },
        m_windowHeight  { h }
    {
    }
    ~SDL_Application();

    static std::shared_ptr<SDL_Application> init(const std::string, int, int);

    bool createOpenGLWindow();

    bool initCelestiaCore();
    void run();
    const char* getError() const;

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
SDL_Application::init(const std::string name, int w, int h)
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
    return std::shared_ptr<SDL_Application>(new SDL_Application(std::move(name), w, h));
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
                                    SDL_WINDOW_OPENGL|SDL_WINDOW_SHOWN|SDL_WINDOW_RESIZABLE);
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

const char*
SDL_Application::getError() const
{
    return SDL_GetError();
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
    bool ret = m_appCore->initSimulation();
    return ret;
}

void
SDL_Application::run()
{
    m_appCore->initRenderer();
    m_appCore->getRenderer()->setRenderFlags(Renderer::DefaultRenderFlags);
    m_appCore->start();

    std::string tzName;
    int dstBias;
    if (GetTZInfo(tzName, dstBias))
    {
        m_appCore->setTimeZoneName(tzName);
        m_appCore->setTimeZoneBias(dstBias);
    }
    m_appCore->resize(m_windowWidth, m_windowHeight);

    SDL_StartTextInput();

    bool quit = false;
    while (!quit)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event) != 0)
        {
            switch (event.type)
            {
            case SDL_QUIT:
                quit = true;
                break;
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
                break;
            }
        }
        m_appCore->tick();
        display();
    }
}

static int
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
        int k = tolower(key);
        if (k >= 'a' && k <= 'z')
        {
            key = k + 1 - 'a';
            m_appCore->charEntered(key, mod);
            return;
        }
        mod |= CelestiaCore::ControlKey;
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

static int
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
    m_appCore->mouseButtonDown(event.x, event.y, button);
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
            SDL_WarpMouseInWindow(m_mainWindow, m_lastX, m_lastY);
        }
    }
    m_lastX = event.x;
    m_lastY = event.y;
    m_appCore->mouseButtonUp(event.x, event.y, button);
}

void
SDL_Application::handleMouseWheelEvent(const SDL_MouseWheelEvent &event)
{
    if (event.y > 0) // scroll up
        m_appCore->mouseWheel(-1.0f, 0);
    else if (event.y < 0) // scroll down
        m_appCore->mouseWheel(1.0f, 0);
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
        m_appCore->mouseMove(x, y, buttons);
        SDL_WarpMouseInWindow(m_mainWindow, m_lastX, m_lastY);
    }
}

void
SDL_Application::handleWindowEvent(const SDL_WindowEvent &event)
{
    switch (event.event)
    {
    case SDL_WINDOWEVENT_RESIZED:
        m_windowWidth  = event.data1;
        m_windowHeight = event.data2;
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
        if (ret == 0)
            ret = SDL_SetWindowFullscreen(m_mainWindow, SDL_WINDOW_FULLSCREEN_DESKTOP);
        if (ret == 0)
            m_fullscreen = true;
    }
}
}

void
FatalError(const std::string &message)
{
    int ret = SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                                       "Fatal Error",
                                       message.c_str(),
                                       nullptr);
    if (ret < 0)
        std::cerr << message << std::endl;
}

void DumpGLInfo()
{
    const char* s;
    s = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    if (s != nullptr)
        std::cout << s << '\n';

    s = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
    if (s != nullptr)
        std::cout << s << '\n';

    s = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    if (s != nullptr)
        std::cout << s << '\n';

    s = reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION));
    if (s != nullptr)
        std::cout << s << '\n';
}


using namespace celestia;

int main(int argc, char **argv)
{
    setlocale(LC_ALL, "");
    setlocale(LC_NUMERIC, "C");
    bindtextdomain(PACKAGE, LOCALEDIR);
    bind_textdomain_codeset(PACKAGE, "UTF-8");
    textdomain(PACKAGE);

    if (chdir(CONFIG_DATA_DIR) == -1)
    {
        FatalError(fmt::sprintf("Cannot chdir to '%s', probably due to improper installation",
                   CONFIG_DATA_DIR));
        return 1;
    }

    auto app = SDL_Application::init("Celestia", 640, 480);
    if (app == nullptr)
    {
        FatalError(fmt::sprintf("Could not initialize SDL! Error: %s",
                   app->getError()));
        return 2;
    }

    if (!app->initCelestiaCore())
    {
        FatalError("Could not initialize Celestia!");
        return 3;
    }
    if (!app->createOpenGLWindow())
    {
        FatalError(fmt::sprintf("Could not create a OpenGL window! Error: %s",
                   app->getError()));
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
