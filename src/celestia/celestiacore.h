// celestiacore.h
//
// Copyright (C) 2001-2009, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELESTIACORE_H_
#define _CELESTIACORE_H_

#include <celutil/timer.h>
#include <celutil/watcher.h>
// #include <celutil/watchable.h>
#include <celengine/solarsys.h>
#include <celengine/overlay.h>
#include "command.h"
#include "execution.h"
#include <celengine/texture.h>
#include <celengine/universe.h>
#include <celengine/render.h>
#include <celengine/simulation.h>
#include <GL/glew.h>
#include "configfile.h"
#include "favorites.h"
#include "destination.h"
#include "moviecapture.h"
#ifdef CELX
#include "celx.h"
#endif
class Url;

// class CelestiaWatcher;
class CelestiaCore;

// class astro::Date;

typedef Watcher<CelestiaCore> CelestiaWatcher;

class ProgressNotifier
{
public:
    ProgressNotifier() {};
    virtual ~ProgressNotifier() {};

    virtual void update(const std::string&) = 0;
};

class View
{
 public:
    enum Type {
        ViewWindow      = 1,
        HorizontalSplit = 2,
        VerticalSplit   = 3
    };

    View(Type, Observer*, float, float, float, float);

    void mapWindowToView(float, float, float&, float&) const;

 public:
    Type type;

    Observer* observer;
    View *parent;
    View *child1;
    View *child2;
    float x;
    float y;
    float width;
    float height;
    uint64_t renderFlags;
    int labelMode;
    float zoom;
    float alternateZoom;

    void walkTreeResize(View*, int);
    bool walkTreeResizeDelta(View*, float, bool);
};


class CelestiaCore // : public Watchable<CelestiaCore>
{
 public:
    enum {
        LeftButton   = 0x01,
        MiddleButton = 0x02,
        RightButton  = 0x04,
        ShiftKey     = 0x08,
        ControlKey   = 0x10,
    };

    enum CursorShape {
        ArrowCursor         = 0,
        UpArrowCursor       = 1,
        CrossCursor         = 2,
        InvertedCrossCursor = 3,
        WaitCursor          = 4,
        BusyCursor          = 5,
        IbeamCursor         = 6,
        SizeVerCursor       = 7,
        SizeHorCursor       = 8,
        SizeBDiagCursor     = 9,
        SizeFDiagCursor     = 10,
        SizeAllCursor       = 11,
        SplitVCursor        = 12,
        SplitHCursor        = 13,
        PointingHandCursor  = 14,
        ForbiddenCursor     = 15,
        WhatsThisCursor     = 16,
    };

    enum {
        Joy_XAxis           = 0,
        Joy_YAxis           = 1,
        Joy_ZAxis           = 2,
    };

    enum {
        JoyButton1          = 0,
        JoyButton2          = 1,
        JoyButton3          = 2,
        JoyButton4          = 3,
        JoyButton5          = 4,
        JoyButton6          = 5,
        JoyButton7          = 6,
        JoyButton8          = 7,
        JoyButtonCount      = 8,
    };

    enum {
        Key_Left            =  1,
        Key_Right           =  2,
        Key_Up              =  3,
        Key_Down            =  4,
        Key_Home            =  5,
        Key_End             =  6,
        Key_PageUp          =  7,
        Key_PageDown        =  8,
        Key_Insert          =  9,
        Key_Delete          = 10,
        Key_F1              = 11,
        Key_F2              = 12,
        Key_F3              = 13,
        Key_F4              = 14,
        Key_F5              = 15,
        Key_F6              = 16,
        Key_F7              = 17,
        Key_F8              = 18,
        Key_F9              = 19,
        Key_F10             = 20,
        Key_F11             = 21,
        Key_F12             = 22,
        Key_NumPadDecimal   = 23,
        Key_NumPad0         = 24,
        Key_NumPad1         = 25,
        Key_NumPad2         = 26,
        Key_NumPad3         = 27,
        Key_NumPad4         = 28,
        Key_NumPad5         = 29,
        Key_NumPad6         = 30,
        Key_NumPad7         = 31,
        Key_NumPad8         = 32,
        Key_NumPad9         = 33,
        Key_BackTab         = 127,
        KeyCount            = 128,
    };

    enum
    {
        LabelFlagsChanged       = 1,
        RenderFlagsChanged      = 2,
        VerbosityLevelChanged   = 4,
        TimeZoneChanged         = 8,
        AmbientLightChanged     = 16,
        FaintestChanged         = 32,
        HistoryChanged          = 64,
        TextEnterModeChanged    = 128,
        GalaxyLightGainChanged  = 256,
    };

    enum
    {
        KbNormal         = 0,
        KbAutoComplete   = 1,
        KbPassToScript   = 2,
    };

    enum
    {
        ShowNoElement = 0x001,
        ShowTime      = 0x002,
        ShowVelocity  = 0x004,
        ShowSelection = 0x008,
        ShowFrame     = 0x010,
    };

    typedef void (*ContextMenuFunc)(float, float, Selection);

 private:
    class OverlayImage
    {
     public:
        OverlayImage(fs::path, Overlay*);
        ~OverlayImage() { delete texture; }
        OverlayImage()               =default;
        OverlayImage(OverlayImage&)  =delete;
        OverlayImage(OverlayImage&&) =delete;

        void render(float, int, int);
        inline bool isNewImage(const fs::path& f) { return filename != f; }

        void setStartTime(float t) { start = t; }
        void setDuration(float t) { duration = t; }
        void setOffset(float x, float y) { offsetX = x; offsetY = y; }
        void setAlpha(float t) { alpha = t; }
        void fitScreen(bool t) { fitscreen = t; }

     private:
        float start{ 0.0f };
        float duration{ 0.0f };
        float offsetX{ 0.0f };
        float offsetY{ 0.0f };
        float alpha{ 0.0f };
        bool  fitscreen{ false };
        fs::path filename;
        Texture* texture{ nullptr };
        Overlay* overlay;
    };

 public:
    CelestiaCore();
    ~CelestiaCore();

    bool initSimulation(const fs::path& configFileName = fs::path(),
                        const std::vector<fs::path>& extrasDirs = {},
                        ProgressNotifier* progressNotifier = nullptr);
    bool initRenderer();
    void start(double t);
    void getLightTravelDelay(double distanceKm, int&, int&, float&);
    void setLightTravelDelay(double distanceKm);

    // URLs and history navigation
    void setStartURL(std::string url);
    void goToUrl(const std::string& urlStr);
    void addToHistory();
    void back();
    void forward();
    const std::vector<Url*>& getHistory() const;
    std::vector<Url*>::size_type getHistoryCurrent() const;
    void setHistoryCurrent(std::vector<Url*>::size_type curr);

    // event processing methods
    void charEntered(const char*, int modifiers = 0);
    void charEntered(char, int modifiers = 0);
    void keyDown(int key, int modifiers = 0);
    void keyUp(int key, int modifiers = 0);
    void mouseWheel(float, int);
    void mouseButtonDown(float, float, int);
    void mouseButtonUp(float, float, int);
    void mouseMove(float, float, int);
    void mouseMove(float, float);
    void pickView(float, float);
    void joystickAxis(int axis, float amount);
    void joystickButton(int button, bool down);
    void resize(GLsizei w, GLsizei h);
    void draw();
    void tick();

    Simulation* getSimulation() const;
    Renderer* getRenderer() const;
    void showText(std::string s,
                  int horig = 0, int vorig = 0,
                  int hoff = 0, int voff = 0,
                  double duration = 1.0e9);
    int getTextWidth(string s) const;

    void readFavoritesFile();
    void writeFavoritesFile();
    void activateFavorite(FavoritesEntry&);
    void addFavorite(std::string, std::string, FavoritesList::iterator* iter=nullptr);
    void addFavoriteFolder(std::string, FavoritesList::iterator* iter=nullptr);
    FavoritesList* getFavorites();

    bool viewUpdateRequired() const;
    void setViewChanged();

    const DestinationList* getDestinations();

    int getTimeZoneBias() const;
    void setTimeZoneBias(int);
    std::string getTimeZoneName() const;
    void setTimeZoneName(const std::string&);
    void setTextEnterMode(int);
    int getTextEnterMode() const;

    void initMovieCapture(MovieCapture*);
    void recordBegin();
    void recordPause();
    void recordEnd();
    bool isCaptureActive();
    bool isRecording();

    void runScript(CommandSequence*);
    void runScript(const fs::path& filename);
    void cancelScript();
    void resumeScript();

    int getHudDetail();
    void setHudDetail(int);
    Color getTextColor();
    void setTextColor(Color);
    astro::Date::Format getDateFormat() const;
    void setDateFormat(astro::Date::Format format);
    int getOverlayElements() const;
    void setOverlayElements(int);

    void setContextMenuCallback(ContextMenuFunc);

    void addWatcher(CelestiaWatcher*);
    void removeWatcher(CelestiaWatcher*);

    void setFaintest(float);
    void setFaintestAutoMag();

    void splitView(View::Type type, View* av = nullptr, float splitPos = 0.5f);
    void singleView(View* av = nullptr);
    void deleteView(View* v = nullptr);
    void setActiveView(View* v = nullptr);
    bool getFramesVisible() const;
    void setFramesVisible(bool);
    bool getActiveFrameVisible() const;
    void setActiveFrameVisible(bool);
    bool getLightDelayActive() const;
    void setLightDelayActive(bool);
    bool getAltAzimuthMode() const;
    void setAltAzimuthMode(bool);
    int getScreenDpi() const;
    void setScreenDpi(int);
    int getDistanceToScreen() const;
    void setDistanceToScreen(int);

    void setFOVFromZoom();
    void setZoomFromFOV();

    void flash(const std::string&, double duration = 1.0);

    CelestiaConfig* getConfig() const;

    void notifyWatchers(int);

    class Alerter
    {
    public:
        virtual ~Alerter() {};
        virtual void fatalError(const std::string&) = 0;
    };

    void setAlerter(Alerter*);
    Alerter* getAlerter() const;

    class CursorHandler
    {
    public:
        virtual ~CursorHandler() {};
        virtual void setCursorShape(CursorShape) = 0;
        virtual CursorShape getCursorShape() const = 0;
    };

    void setCursorHandler(CursorHandler*);
    CursorHandler* getCursorHandler() const;

    void toggleReferenceMark(const std::string& refMark, Selection sel = Selection());
    bool referenceMarkEnabled(const std::string& refMark, Selection sel = Selection()) const;

    void fatalError(const std::string&, bool visual = true);

    void setScriptImage(float, float, float, float, const fs::path&, bool);

    const std::string& getTypedText() const { return typedText; }
    void setTypedText(const char *);

 protected:
    bool readStars(const CelestiaConfig&, ProgressNotifier*);
    void renderOverlay();
#ifdef CELX
    bool initLuaHook(ProgressNotifier*);
#endif // CELX

 private:
    CelestiaConfig* config{ nullptr };

    Universe* universe{ nullptr };

    FavoritesList* favorites{ nullptr };
    DestinationList* destinations{ nullptr };

    Simulation* sim{ nullptr };
    Renderer* renderer{ nullptr };
    Overlay* overlay{ nullptr };
    int width{ 1 };
    int height{ 1 };

    TextureFont* font{ nullptr };
    TextureFont* titleFont{ nullptr };

    std::string messageText;
    int messageHOrigin{ 0 };
    int messageVOrigin{ 0 };
    int messageHOffset{ 0 };
    int messageVOffset{ 0 };
    double messageStart{ 0.0 };
    double messageDuration{ 0.0 };
    Color textColor{ 1.0f, 1.0f, 1.0f };

    const Color frameColor{ 0.5f, 0.5f, 0.5f, 1.0f };
    const Color activeFrameColor{ 0.5f, 0.5f, 1.0f, 1.0f };
    const Color consoleColor{ 0.7f, 0.7f, 1.0f, 0.2f };

    OverlayImage *image{ nullptr };

    std::string typedText;
    std::vector<Name> typedTextCompletion;
    int typedTextCompletionIdx{ -1 };
    int textEnterMode{ KbNormal };
    int hudDetail{ 2 }; // def 1
    astro::Date::Format dateFormat{ astro::Date::Locale };
    int dateStrWidth{ 0 };
    int overlayElements{ ShowTime | ShowVelocity | ShowSelection | ShowFrame };
    bool wireframe{ false };
    bool editMode{ false };
    bool altAzimuthMode{ false };
    bool showConsole{ false };
    bool lightTravelFlag{ false };
    double flashFrameStart{ 0.0 };

    Timer* timer{ nullptr };

    Execution* runningScript{ nullptr };
    ExecutionEnvironment* execEnv{ nullptr };

#ifdef CELX
    LuaState* celxScript{ nullptr };
    LuaState* luaHook{ nullptr };     // Lua hook context
    LuaState* luaSandbox{ nullptr };  // Safe Lua context for ssc scripts
#endif // CELX

    enum ScriptState
    {
        ScriptCompleted,
        ScriptRunning,
        ScriptPaused,
    };
    ScriptState scriptState{ ScriptCompleted };

    int timeZoneBias{ 0 };         // Diff in secs between local time and GMT
    std::string timeZoneName;      // Name of the current time zone

    // Frame rate counter variables
    bool showFPSCounter{ false };
    int nFrames{ 0 };
    double fps{ 0.0 };
    double fpsCounterStartTime{ 0.0 };

    float oldFOV;
    float mouseMotion{ 0.0f };
    double dollyMotion{ 0.0 };
    double dollyTime{ 0.0 };
    double zoomMotion{ 0.0 };
    double zoomTime{ 0.0 };

    double sysTime{ 0.0 };
    double currentTime{ 0.0 };

    bool viewChanged{ true };

    Eigen::Vector3f joystickRotation{ Eigen::Vector3f::Zero() };
    bool joyButtonsPressed[JoyButtonCount];
    bool keysPressed[KeyCount];
    bool shiftKeysPressed[KeyCount];
    double KeyAccel{ 1.0 };

    MovieCapture* movieCapture{ nullptr };
    bool recording{ false };

    ContextMenuFunc contextMenuCallback{ nullptr };

    Texture* logoTexture{ nullptr };

    Alerter* alerter{ nullptr };
    std::vector<CelestiaWatcher*> watchers;
    CursorHandler* cursorHandler{ nullptr };
    CursorShape defaultCursorShape{ CelestiaCore::CrossCursor };

    std::vector<Url*> history;
    std::vector<Url*>::size_type historyCurrent{ 0 };
    std::string startURL;

    std::list<View*> views;
    std::list<View*>::iterator activeView{ views.begin() };
    bool showActiveViewFrame{ false };
    bool showViewFrames{ true };
    View *resizeSplit{ nullptr };

    int screenDpi{ 96 };
    int distanceToScreen{ 400 };

    Selection lastSelection;
    string selectionNames;

#ifdef CELX
    friend View* getViewByObserver(CelestiaCore*, Observer*);
    friend void getObservers(CelestiaCore*, std::vector<Observer*>&);
    friend TextureFont* getFont(CelestiaCore*);
    friend TextureFont* getTitleFont(CelestiaCore*);
#endif
};

#endif // _CELESTIACORE_H_
