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

#include <array>
#include <fstream>
#include <string>
#include <functional>
#include <string_view>
#include <tuple>
#include <celutil/filetype.h>
#include <celutil/timer.h>
#include <celutil/watcher.h>
// #include <celutil/watchable.h>
#include <celengine/solarsys.h>
#include <celengine/overlay.h>
#include <celengine/pixelformat.h>
#include <celengine/texture.h>
#include <celengine/universe.h>
#include <celengine/render.h>
#include <celengine/simulation.h>
#include <celengine/overlayimage.h>
#include <celengine/viewporteffect.h>
#include <celutil/tee.h>
#include "configfile.h"
#include "favorites.h"
#include "destination.h"
#include "moviecapture.h"
#include "view.h"
#ifdef CELX
#include <celscript/lua/celx.h>
#include <celscript/lua/luascript.h>
#endif
#include <celscript/common/script.h>
#include <celscript/legacy/legacyscript.h>
#include <celscript/common/scriptmaps.h>

class Url;
// class CelestiaWatcher;
class CelestiaCore;
// class astro::Date;
class Console;

namespace celestia
{
class TextPrintPosition;
#ifdef USE_MINIAUDIO
class AudioSession;
#endif
}

typedef Watcher<CelestiaCore> CelestiaWatcher;

class ProgressNotifier
{
public:
    ProgressNotifier() = default;
    virtual ~ProgressNotifier() = default;

    virtual void update(const std::string&) = 0;
};

class CelestiaCore // : public Watchable<CelestiaCore>
{
 public:
    enum
    {
        LeftButton   = 0x01,
        MiddleButton = 0x02,
        RightButton  = 0x04,
        ShiftKey     = 0x08,
        ControlKey   = 0x10,
#ifdef __APPLE__
        AltKey       = 0x20,
#endif
    };

    enum CursorShape
    {
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

    enum
    {
        Joy_XAxis           = 0,
        Joy_YAxis           = 1,
        Joy_ZAxis           = 2,
    };

    enum
    {
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

    enum
    {
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
        LabelFlagsChanged           = 0x0001,
        RenderFlagsChanged          = 0x0002,
        VerbosityLevelChanged       = 0x0004,
        TimeZoneChanged             = 0x0008,
        AmbientLightChanged         = 0x0010,
        FaintestChanged             = 0x0020,
        HistoryChanged              = 0x0040,
        TextEnterModeChanged        = 0x0080,
        GalaxyLightGainChanged      = 0x0100,
        MeasurementSystemChanged    = 0x0200,
        TemperatureScaleChanged     = 0x0400,
        TintSaturationChanged       = 0x0800,
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

    enum MeasurementSystem
    {
        Metric      = 0,
        Imperial    = 1,
    };

    enum TemperatureScale
    {
        Kelvin      = 0,
        Celsius     = 1,
        Fahrenheit  = 2,
    };

    enum class ScriptSystemAccessPolicy
    {
        Ask         = 0,
        Allow       = 1,
        Deny        = 2,
    };

    enum class LayoutDirection
    {
        LeftToRight   = 0,
        RightToLeft   = 1,
    };

 public:
    CelestiaCore();
    ~CelestiaCore();

    bool initSimulation(const fs::path& configFileName = fs::path(),
                        const std::vector<fs::path>& extrasDirs = {},
                        ProgressNotifier* progressNotifier = nullptr);
    bool initRenderer();
    void start(double t);
    void start();
    void getLightTravelDelay(double distanceKm, int&, int&, float&);
    void setLightTravelDelay(double distanceKm);

    // URLs and history navigation
    void setStartURL(const std::string& url);
    bool goToUrl(const std::string& urlStr);
    void addToHistory();
    void back();
    void forward();
    const std::vector<Url>& getHistory() const;
    std::vector<Url>::size_type getHistoryCurrent() const;
    void setHistoryCurrent(std::vector<Url>::size_type curr);

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
    void draw(View*);
    void tick();

    Simulation* getSimulation() const;
    Renderer* getRenderer() const;
    void showText(std::string_view s,
                  int horig = 0, int vorig = 0,
                  int hoff = 0, int voff = 0,
                  double duration = 1.0e9);
    void showTextAtPixel(std::string_view s,
                         int x = 0, int y = 0,
                         double duration = 1.0e9);
    int getTextWidth(const std::string &s) const;

    void readFavoritesFile();
    void writeFavoritesFile();
    void activateFavorite(FavoritesEntry&);
    void addFavorite(const std::string&, const std::string&, FavoritesList::iterator* iter=nullptr);
    void addFavoriteFolder(const std::string&, FavoritesList::iterator* iter=nullptr);
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

    void runScript(const fs::path& filename, bool i18n = true);
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

    void addWatcher(CelestiaWatcher*);
    void removeWatcher(CelestiaWatcher*);

    void setFaintest(float);
    void setFaintestAutoMag();

    std::vector<Observer*> getObservers() const;
    View* getViewByObserver(const Observer*) const;
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
    void setSafeAreaInsets(int left, int top, int right, int bottom);
    std::tuple<int, int, int, int> getSafeAreaInsets() const;  // left, top, right, bottom
    std::tuple<int, int> getWindowDimension() const;
    int getSafeAreaWidth() const;
    int getSafeAreaHeight() const;
    // Offset is applied towards the safearea
    int getSafeAreaStart(int offset = 0) const;
    int getSafeAreaEnd(int offset = 0) const;
    int getSafeAreaTop(int offset = 0) const;
    int getSafeAreaBottom(int offset = 0) const;
    float getPickTolerance() const;
    void setPickTolerance(float);

    float getMaximumFOV() const;
    void setFOVFromZoom();
    void setZoomFromFOV();

    void flash(const std::string&, double duration = 1.0);

    CelestiaConfig* getConfig() const;

    void notifyWatchers(int);

    void setLogFile(const fs::path&);

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

    class ContextMenuHandler
    {
    public:
        virtual ~ContextMenuHandler() = default;
        virtual void requestContextMenu(float, float, Selection) = 0;
    };

    void setContextMenuHandler(ContextMenuHandler*);
    ContextMenuHandler* getContextMenuHandler() const;

    void setCustomDateFormatter(std::function<std::string(double)> df) { customDateFormatter = df; };

    bool setFont(const fs::path& fontPath, int collectionIndex, int fontSize);
    bool setTitleFont(const fs::path& fontPath, int collectionIndex, int fontSize);
    bool setRendererFont(const fs::path& fontPath, int collectionIndex, int fontSize, Renderer::FontStyle fontStyle);
    void clearFonts();

    void toggleReferenceMark(const std::string& refMark, Selection sel = Selection());
    bool referenceMarkEnabled(const std::string& refMark, Selection sel = Selection()) const;

    void fatalError(const std::string&, bool visual = true);

    void setScriptImage(std::unique_ptr<OverlayImage>&&);

    const std::string& getTypedText() const { return typedText; }
    void setTypedText(const char *);

    void setScriptHook(std::unique_ptr<celestia::scripts::IScriptHook> &&hook) { m_scriptHook = std::move(hook); }
    const std::shared_ptr<celestia::scripts::ScriptMaps>& scriptMaps() const { return m_scriptMaps; }

    void getCaptureInfo(std::array<int, 4>& viewport, celestia::PixelFormat& format) const;
    bool captureImage(std::uint8_t* buffer, const std::array<int, 4>& viewport, celestia::PixelFormat format) const;
    bool saveScreenShot(const fs::path&, ContentType = ContentType::Unknown) const;

#ifdef USE_MINIAUDIO
    bool isPlayingAudio(int channel) const;
    bool playAudio(int channel, const fs::path& path, double startTime, float volume, float pan, bool loop, bool nopause);
    bool resumeAudio(int channel);
    void pauseAudio(int channel);
    void stopAudio(int channel);
    bool seekAudio(int channel, double time);
    void setAudioVolume(int channel, float volume);
    void setAudioPan(int channel, float pan);
    void setAudioLoop(int channel, bool loop);
    void setAudioNoPause(int channel, bool nopause);

    void pauseAudioIfNeeded();
    void resumeAudioIfNeeded();
#endif

    void setMeasurementSystem(MeasurementSystem);
    MeasurementSystem getMeasurementSystem() const;
    void setTemperatureScale(TemperatureScale);
    TemperatureScale getTemperatureScale() const;

    ScriptSystemAccessPolicy getScriptSystemAccessPolicy() const;
    void setScriptSystemAccessPolicy(ScriptSystemAccessPolicy);

    LayoutDirection getLayoutDirection() const;
    void setLayoutDirection(LayoutDirection);

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

    std::shared_ptr<TextureFont> font{ nullptr };
    std::shared_ptr<TextureFont> titleFont{ nullptr };

    std::string messageText;
    std::unique_ptr<celestia::TextPrintPosition> messageTextPosition;
    double messageStart{ 0.0 };
    double messageDuration{ 0.0 };
    Color textColor{ 1.0f, 1.0f, 1.0f };

    const Color frameColor{ 0.5f, 0.5f, 0.5f, 1.0f };
    const Color activeFrameColor{ 0.5f, 0.5f, 1.0f, 1.0f };
    const Color consoleColor{ 0.7f, 0.7f, 1.0f, 0.2f };

    std::unique_ptr<OverlayImage> image;

    std::string typedText;
    std::vector<std::string> typedTextCompletion;
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

    std::unique_ptr<celestia::scripts::IScript>             m_script;
    std::unique_ptr<celestia::scripts::IScriptHook>         m_scriptHook;
    std::unique_ptr<celestia::scripts::LegacyScriptPlugin>  m_legacyPlugin;
#ifdef CELX
    std::unique_ptr<celestia::scripts::LuaScriptPlugin>     m_luaPlugin;
#endif
    std::shared_ptr<celestia::scripts::ScriptMaps>          m_scriptMaps;

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

#ifdef USE_MINIAUDIO
    std::map<int, std::shared_ptr<celestia::AudioSession>> audioSessions;

    std::shared_ptr<celestia::AudioSession> getAudioSession(int channel) const;
#endif

    Alerter* alerter{ nullptr };
    std::vector<CelestiaWatcher*> watchers;
    CursorHandler* cursorHandler{ nullptr };
    CursorShape defaultCursorShape{ CelestiaCore::CrossCursor };
    ContextMenuHandler* contextMenuHandler{ nullptr };
    std::function<std::string(double)> customDateFormatter{ nullptr };

    std::vector<Url> history;
    std::vector<Url>::size_type historyCurrent{ 0 };
    std::string startURL;

    std::list<View*> views;
    std::list<View*>::iterator activeView{ views.begin() };
    bool showActiveViewFrame{ false };
    bool showViewFrames{ true };
    View *resizeSplit{ nullptr };

    int screenDpi{ 96 };
    int distanceToScreen{ 400 };

    float pickTolerance { 4.0f };

    std::unique_ptr<ViewportEffect> viewportEffect { nullptr };
    bool isViewportEffectUsed { false };

    struct EdgeInsets
    {
        int left;
        int top;
        int right;
        int bottom;
    };

    EdgeInsets safeAreaInsets { 0, 0, 0, 0 };

    MeasurementSystem measurement { Metric };
    TemperatureScale temperatureScale { Kelvin };

    ScriptSystemAccessPolicy scriptSystemAccessPolicy { ScriptSystemAccessPolicy::Ask };

    LayoutDirection layoutDirection { LayoutDirection::LeftToRight };

    Selection lastSelection;
    std::string selectionNames;

    std::unique_ptr<Console> console;
    std::ofstream m_logfile;
    teestream m_tee;

    std::vector<astro::LeapSecondRecord> leapSeconds;

#ifdef CELX
    friend View* getViewByObserver(CelestiaCore*, Observer*);
    friend void getObservers(CelestiaCore*, std::vector<Observer*>&);
    friend std::shared_ptr<TextureFont> getFont(CelestiaCore*);
    friend std::shared_ptr<TextureFont> getTitleFont(CelestiaCore*);
#endif
};

#endif // _CELESTIACORE_H_
