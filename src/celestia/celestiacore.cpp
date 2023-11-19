// celestiacore.cpp
//
// Platform-independent UI handling and initialization for Celestia.
// winmain, gtkmain, and qtmain are thin, platform-specific modules
// that sit directly on top of CelestiaCore and feed it mouse and
// keyboard events.  CelestiaCore then turns those events into calls
// to Renderer and Simulation.
//
// Copyright (C) 2001-2009, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "celestiacore.h"
#include "favorites.h"
#include "textprintposition.h"
#include "url.h"
#include "viewmanager.h"
#include <celastro/astro.h>
#include <celastro/date.h>
#include <celcompat/numbers.h>
#include <celengine/asterism.h>
#include <celengine/body.h>
#include <celengine/boundaries.h>
#include <celengine/dsoname.h>
#include <celengine/location.h>
#include <celengine/overlay.h>
#include <celengine/console.h>
#include <celengine/starname.h>
#include <celengine/textlayout.h>
#include <celscript/legacy/execution.h>
#include <celscript/legacy/cmdparser.h>
#include <celengine/multitexture.h>
#ifdef USE_SPICE
#include <celephem/spiceinterface.h>
#endif
#include <celengine/axisarrow.h>
#include <celengine/planetgrid.h>
#include <celengine/visibleregion.h>
#include <celengine/framebuffer.h>
#include <celengine/fisheyeprojectionmode.h>
#include <celengine/perspectiveprojectionmode.h>
#include <celmath/geomutil.h>
#include <celutil/color.h>
#include <celutil/filetype.h>
#include <celutil/formatnum.h>
#include <celutil/fsutils.h>
#include <celutil/logger.h>
#include <celutil/gettext.h>
#include <celutil/utf8.h>
#include <celcompat/filesystem.h>
#include <Eigen/Geometry>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <cctype>
#include <cwctype>
#include <cstring>
#include <cassert>
#include <ctime>
#include <set>
#include <celengine/rectangle.h>
#include <celengine/mapmanager.h>
#include <fmt/ostream.h>
#ifdef USE_MINIAUDIO
#include "miniaudiosession.h"
#endif

#ifdef CELX
#include <celephem/scriptobject.h>
#endif
#include <celttf/truetypefont.h>

using namespace Eigen;
using namespace std;
using namespace celestia;
using namespace celestia::astro::literals;
using namespace celestia::engine;
using namespace celestia::scripts;
using namespace celestia::util;

static const int DragThreshold = 3;

// Perhaps you'll want to put this stuff in configuration file.
static const double CoarseTimeScaleFactor = 10.0;
static const double FineTimeScaleFactor = 2.0;
static const double fMaxKeyAccel = 20.0;
static const float RotationBraking = 10.0f;
static const float RotationDecay = 2.0f;
static const double MaximumTimeRate = 1.0e15;
static const float stdFOV = 45.0_deg;
static float KeyRotationAccel = 120.0_deg;
static float MouseRotationSensitivity = 1.0_deg;

namespace
{

bool is_valid_directory(const fs::path& dir)
{
    if (dir.empty())
        return false;

    std::error_code ec;
    if (!fs::is_directory(dir, ec))
    {
        GetLogger()->error(_("Path {} doesn't exist or isn't a directory\n"), dir);
        return false;
    }

    return true;
}

bool ReadLeapSecondsFile(const fs::path& path, std::vector<astro::LeapSecondRecord> &leapSeconds)
{
    std::ifstream file(path);
    if (!file.good())
    {
        GetLogger()->error("Failed to open leapseconds file {}\n", path);
        return false;
    }


    std::string s;
    for (int line = 1; std::getline(file, s); line++)
    {
        const char *ptr = &s[0];
        while (*ptr != 0 && std::isspace(static_cast<unsigned char>(*ptr))) ptr++;
        if (*ptr == '#' || *ptr == 0)
            continue;

        unsigned timestamp = 0u; // NTP timestamp is 32 bit unsigned value
        int seconds = 0;
        if (sscanf(ptr, "%u %i", &timestamp, &seconds) != 2)
        {
            GetLogger()->error("Failed to parse leapseconds file {}, line {}, column {}\n", path, line, ptr - &s[0]);
            leapSeconds.clear();
            return false;
        }
        double jd = (timestamp - 2208988800) / 86400.0 + 2440587.5;
        leapSeconds.push_back({seconds, jd});
    }

    std::sort(leapSeconds.begin(), leapSeconds.end(), [](const auto& a, const auto& b) { return a.t < b.t; });

    astro::setLeapSeconds(leapSeconds);
    return true;
}
}

// If right dragging to rotate, adjust the rotation rate based on the
// distance from the reference object.  This makes right drag rotation
// useful even when the camera is very near the surface of an object.
// Disable adjustments if the reference is a deep sky object, since they
// have no true surface (and the observer is likely to be inside one.)
float ComputeRotationCoarseness(Simulation& sim)
{
    float coarseness = 1.5f;

    Selection selection = sim.getActiveObserver()->getFrame()->getRefObject();
    if (selection.getType() == SelectionType::Star ||
        selection.getType() == SelectionType::Body)
    {
        double radius = selection.radius();
        double t = sim.getTime();
        UniversalCoord observerPosition = sim.getActiveObserver()->getPosition();
        UniversalCoord selectionPosition = selection.getPosition(t);
        double distance = observerPosition.distanceFromKm(selectionPosition);
        double altitude = distance - radius;
        if (altitude > 0.0 && altitude < radius)
        {
            coarseness *= (float) max(0.01, altitude / radius);
        }
    }

    return coarseness;
}


CelestiaCore::CelestiaCore() :
    /* Get a renderer here so it may be queried for capabilities of the
       underlying engine even before rendering is enabled. It's initRenderer()
       routine will be called much later. */
    renderer(new Renderer()),
    timer(new Timer()),
    m_legacyPlugin(new LegacyScriptPlugin(this)),
#ifdef CELX
    m_luaPlugin(new LuaScriptPlugin(this)),
#endif
    m_scriptMaps(new ScriptMaps()),
    oldFOV(stdFOV),
    console(new Console(*renderer, 200, 120)),
    m_tee(std::cout, std::cerr)
{

    CreateLogger();

    for (int i = 0; i < KeyCount; i++)
    {
        keysPressed[i] = false;
        shiftKeysPressed[i] = false;
    }
    for (int i = 0; i < JoyButtonCount; i++)
        joyButtonsPressed[i] = false;

    clog.rdbuf(console->rdbuf());
    cerr.rdbuf(console->rdbuf());
    console->setWindowHeight(Console::PageRows);
}

CelestiaCore::~CelestiaCore()
{
    if (movieCapture != nullptr)
        recordEnd();

    delete timer;
    delete renderer;

    if (m_logfile.good())
        m_logfile.close();

    DestroyLogger();
}

void CelestiaCore::readFavoritesFile()
{
    // Set up favorites list
    fs::path path;
    if (!config->paths.favoritesFile.empty())
        path = config->paths.favoritesFile;
    else
        path = "favorites.cel";

#ifndef PORTABLE_BUILD
    if (path.is_relative())
        path = WriteableDataPath() / path;
#endif

    ifstream in(path, ios::in);
    if (in.good())
    {
        favorites = ReadFavoritesList(in);
        if (favorites == nullptr)
            GetLogger()->error(_("Error reading favorites file {}.\n"), path);
    }
}

void CelestiaCore::writeFavoritesFile()
{
    fs::path path;
    if (!config->paths.favoritesFile.empty())
        path = config->paths.favoritesFile;
    else
        path = "favorites.cel";

#ifndef PORTABLE_BUILD
    if (path.is_relative())
        path = WriteableDataPath() / path;
#endif

    error_code ec;
    bool isDir = fs::is_directory(path.parent_path(), ec);
    if (ec)
    {
        GetLogger()->error(_("Failed to check directory existance for favorites file {}\n"), path);
        return;
    }
    if (!isDir)
    {
        fs::create_directory(path.parent_path(), ec);
        if (ec)
        {
            GetLogger()->error(_("Failed to create a directory for favorites file {}\n"), path);
            return;
        }
    }

    ofstream out(path, ios::out);
    if (out.good())
        WriteFavoritesList(*favorites, out);
}

void CelestiaCore::activateFavorite(FavoritesEntry& fav)
{
    sim->cancelMotion();
    sim->setTime(fav.jd);
    sim->setObserverPosition(fav.position);
    sim->setObserverOrientation(fav.orientation);
    sim->setSelection(sim->findObjectFromPath(fav.selectionName));
    sim->setFrame(fav.coordSys, sim->getSelection());
}

void CelestiaCore::addFavorite(const string &name, const string &parentFolder, FavoritesList::iterator* iter)
{
    FavoritesList::iterator pos;
    if(!iter)
        pos = favorites->end();
    else
        pos = *iter;
    auto* fav = new FavoritesEntry();
    fav->jd = sim->getTime();
    fav->position = sim->getObserver().getPosition();
    fav->orientation = sim->getObserver().getOrientationf();
    fav->name = name;
    fav->isFolder = false;
    fav->parentFolder = parentFolder;

    Selection sel = sim->getSelection();
    if (sel.deepsky() != nullptr)
        fav->selectionName = sim->getUniverse()->getDSOCatalog()->getDSOName(sel.deepsky());
    else
        fav->selectionName = sel.getName();

    fav->coordSys = sim->getFrame()->getCoordinateSystem();

    favorites->insert(pos, fav);
}

void CelestiaCore::addFavoriteFolder(const string &name, FavoritesList::iterator* iter)
{
    FavoritesList::iterator pos;
    if(!iter)
        pos = favorites->end();
    else
        pos = *iter;
    auto* fav = new FavoritesEntry();
    fav->name = name;
    fav->isFolder = true;

    favorites->insert(pos, fav);
}

FavoritesList* CelestiaCore::getFavorites()
{
    return favorites;
}


const DestinationList* CelestiaCore::getDestinations()
{
    return destinations;
}


// Used in the super-secret edit mode
void showSelectionInfo(const Selection& sel)
{
    Quaternionf orientation;
    if (sel.deepsky() != nullptr)
        orientation = sel.deepsky()->getOrientation();
    else if (sel.body() != nullptr)
        orientation = sel.body()->getGeometryOrientation();

    AngleAxisf aa(orientation);

    GetLogger()->info("{}\nOrientation: [{}, {}, {}], {:.1f}\n", sel.getName(), aa.axis().x(), aa.axis().y(), aa.axis().z(), math::radToDeg(aa.angle()));
}


void CelestiaCore::cancelScript()
{
    if (m_script != nullptr)
    {
        if (is_set(hud->textEnterMode(), Hud::TextEnterMode::PassToScript))
            setTextEnterMode(hud->textEnterMode() & ~Hud::TextEnterMode::PassToScript);
        scriptState = ScriptCompleted;
        m_script = nullptr;
    }
}


void CelestiaCore::runScript(const fs::path& filename, bool i18n)
{
    cancelScript();
    auto maybeLocaleFilename = i18n ? LocaleFilename(filename) : filename;

    if (m_legacyPlugin->isOurFile(maybeLocaleFilename))
    {
        m_script = m_legacyPlugin->loadScript(maybeLocaleFilename);
        if (m_script != nullptr)
            scriptState = sim->getPauseState() ? ScriptPaused : ScriptRunning;
    }
#ifdef CELX
    else if (m_luaPlugin->isOurFile(maybeLocaleFilename))
    {
        m_script = m_luaPlugin->loadScript(maybeLocaleFilename);
        if (m_script != nullptr)
            scriptState = sim->getPauseState() ? ScriptPaused : ScriptRunning;
    }
#endif
    else
    {
        fatalError(_("Invalid filetype"));
    }
}


static bool checkMask(int modifiers, int mask)
{
    return (modifiers & mask) == mask;
}

void CelestiaCore::mouseButtonDown(float x, float y, int button)
{
    mouseMotion = 0.0f;

#ifdef CELX
    if (m_script != nullptr)
    {
        if (m_script->handleMouseButtonEvent(x, y, button, true))
            return;
    }
#endif
   if (m_scriptHook != nullptr && m_scriptHook->call("mousebuttondown", x, y, button))
        return;

    if (viewManager->views().size() < 2)
        return;

    // To select the clicked into view before a drag.
    viewManager->pickView(sim, metrics, x, y);

    if (button == LeftButton) // look if click is near a view border
        viewManager->tryStartResizing(metrics, x, y);
}

void CelestiaCore::mouseButtonUp(float x, float y, int button)
{
    // Four pixel tolerance for picking
    float obsPickTolerance = sim->getActiveObserver()->getFOV() / static_cast<float>(metrics.height) * this->pickTolerance;

    if (viewManager->stopResizing())
        return;

#ifdef CELX
    if (m_script != nullptr)
    {
        if (m_script->handleMouseButtonEvent(x, y, button, false))
            return;
    }
#endif
    if (m_scriptHook != nullptr && m_scriptHook->call("mousebuttonup", x, y, button))
        return;

    // If the mouse hasn't moved much since it was pressed, treat this
    // as a selection or context menu event.  Otherwise, assume that the
    // mouse was dragged and ignore the event.
    if (mouseMotion < DragThreshold)
    {
        if (button == LeftButton)
        {
            viewManager->pickView(sim, metrics, x, y);

            float pickX;
            float pickY;
            float aspectRatio = static_cast<float>(metrics.width) / static_cast<float>(metrics.height);
            viewManager->activeView()->mapWindowToView(x / static_cast<float>(metrics.width),
                                                       y / static_cast<float>(metrics.height),
                                                       pickX, pickY);
            pickX *= aspectRatio;
            if (isViewportEffectUsed)
                viewportEffect->distortXY(pickX, pickY);

            Vector3f pickRay = renderer->getProjectionMode()->getPickRay(pickX, pickY, viewManager->activeView()->getObserver()->getZoom());

            Selection oldSel = sim->getSelection();
            Selection newSel = sim->pickObject(pickRay, renderer->getRenderFlags(), obsPickTolerance);
            addToHistory();
            sim->setSelection(newSel);
            if (!oldSel.empty() && oldSel == newSel)
                sim->centerSelection();
        }
        else if (button == RightButton)
        {
            float pickX, pickY;
            float aspectRatio = static_cast<float>(metrics.width) / static_cast<float>(metrics.height);
            viewManager->activeView()->mapWindowToView(x / static_cast<float>(metrics.width),
                                                       y / static_cast<float>(metrics.height),
                                                       pickX, pickY);
            pickX *= aspectRatio;
            if (isViewportEffectUsed)
                viewportEffect->distortXY(pickX, pickY);

            Vector3f pickRay = renderer->getProjectionMode()->getPickRay(pickX, pickY, viewManager->activeView()->getObserver()->getZoom());

            Selection sel = sim->pickObject(pickRay, renderer->getRenderFlags(), obsPickTolerance);
            if (!sel.empty())
            {
                if (contextMenuHandler != nullptr)
                    contextMenuHandler->requestContextMenu(x, y, sel);
            }
        }
        else if (button == MiddleButton)
        {
            auto observer = viewManager->activeView()->getObserver();
            auto currentZoom = observer->getZoom();
            if (currentZoom != 1.0f)
            {
                observer->setAlternateZoom(currentZoom);
                observer->setZoom(1.0f);
            }
            else
            {
                observer->setZoom(observer->getAlternateZoom());
            }
            setFOVFromZoom();

            // If AutoMag, adapt the faintestMag to the new fov
            if((renderer->getRenderFlags() & Renderer::ShowAutoMag) != 0)
                setFaintestAutoMag();
        }
    }
}

void CelestiaCore::mouseWheel(float motion, int modifiers)
{
    if (config->mouse.reverseWheel) motion = -motion;
    if (motion != 0.0)
    {
        if ((modifiers & ShiftKey) != 0)
        {
            zoomTime = timeInfo.currentTime;
            zoomMotion = 0.25f * motion;
        }
        else
        {
            dollyTime = timeInfo.currentTime;
            dollyMotion = 0.25f * motion;
        }
    }
}

/// Handles cursor shape changes on view borders if the cursorHandler is defined.
/// This must be called on mouse move events on the OpenGL Widget.
/// x and y are the pixel coordinates relative to the widget.
void CelestiaCore::mouseMove(float x, float y)
{
    if (m_scriptHook != nullptr && m_scriptHook->call("mousemove", x, y))
        return;

    if (viewManager->views().size() < 2 || cursorHandler == nullptr)
        return;

    switch (viewManager->checkViewBorder(metrics, x, y))
    {
    case ViewBorderType::SizeHorizontal:
        cursorHandler->setCursorShape(CelestiaCore::SizeHorCursor);
        break;
    case ViewBorderType::SizeVertical:
        cursorHandler->setCursorShape(CelestiaCore::SizeVerCursor);
        break;
    default:
        cursorHandler->setCursorShape(defaultCursorShape);
        break;
    }
}

void CelestiaCore::mouseMove(float dx, float dy, int modifiers)
{
    if (viewManager->resizeViews(metrics, dx, dy))
    {
        setFOVFromZoom();
        return;
    }

    if (m_scriptHook != nullptr && m_scriptHook->call("mousebuttonmove", dx, dy, modifiers))
       return;


    if ((modifiers & (LeftButton | RightButton)) != 0)
    {
        if (editMode && checkMask(modifiers, LeftButton | ShiftKey | ControlKey))
        {
            // Rotate the selected object
            Selection sel = sim->getSelection();
            Quaternionf q = Quaternionf::Identity();
            if (sel.getType() == SelectionType::DeepSky)
                q = sel.deepsky()->getOrientation();
            else if (sel.getType() == SelectionType::Body)
                q = sel.body()->getGeometryOrientation();

            q = math::XRotation(dy / static_cast<float>(metrics.height)) *
                math::YRotation(dx / static_cast<float>(metrics.width)) * q;

            if (sel.getType() == SelectionType::DeepSky)
                sel.deepsky()->setOrientation(q);
            else if (sel.getType() == SelectionType::Body)
                sel.body()->setGeometryOrientation(q);
        }
        else if (editMode && checkMask(modifiers, RightButton | ShiftKey | ControlKey))
        {
            // Rotate the selected object about an axis from its center to the
            // viewer.
            Selection sel = sim->getSelection();
            if (sel.deepsky() != nullptr)
            {
                double t = sim->getTime();
                Vector3d v = sel.getPosition(t).offsetFromKm(sim->getObserver().getPosition());
                Vector3f axis = v.cast<float>().normalized();

                Quaternionf r(AngleAxisf(dx / static_cast<float>(metrics.width), axis));

                Quaternionf q = sel.deepsky()->getOrientation();
                sel.deepsky()->setOrientation(r * q);
            }
        }
        else if (checkMask(modifiers, LeftButton | RightButton) ||
                 checkMask(modifiers, LeftButton | ControlKey))
        {
            // Y-axis controls distance (exponentially), and x-axis motion
            // rotates the camera about the view normal.
            float amount = dy / static_cast<float>(metrics.height);
            sim->changeOrbitDistance(amount * 5);
            if (dx * dx > dy * dy)
            {
                Observer& observer = sim->getObserver();
                Vector3d v(0, 0, dx * -MouseRotationSensitivity);
                v *= 0.5;

                Quaterniond obsOrientation = observer.getOrientation();
                Quaterniond dr = Quaterniond(0.0, v.x(), v.y(), v.z()) * obsOrientation;
                obsOrientation = Quaterniond(dr.coeffs() + obsOrientation.coeffs());
                obsOrientation.normalize();
                observer.setOrientation(obsOrientation);
            }
        }
        else if (checkMask(modifiers, LeftButton | ShiftKey))
        {
            // Mouse zoom control
            float amount = dy / static_cast<float>(metrics.height);
            float minFOV = renderer->getProjectionMode()->getMinimumFOV();
            float maxFOV = renderer->getProjectionMode()->getMaximumFOV();
            float fov = sim->getActiveObserver()->getFOV();

            // In order for the zoom to have the right feel, it should be
            // exponential.
            float newFOV = minFOV + (float) exp(log(fov - minFOV) + amount * 4);
            if (newFOV > maxFOV)
                newFOV = maxFOV;
            if (newFOV > minFOV)
            {
                sim->getActiveObserver()->setFOV(newFOV);
                setZoomFromFOV();
            }

            if ((renderer->getRenderFlags() & Renderer::ShowAutoMag))
            {
                setFaintestAutoMag();
                flash(fmt::sprintf(_("Magnitude limit: %.2f"), sim->getFaintestVisible()));
            }
        }
        else
        {
            // For a small field of view, rotate the camera more finely
            float coarseness = 1.5f;
            if ((modifiers & RightButton) == 0)
            {
                coarseness = math::radToDeg(sim->getActiveObserver()->getFOV()) / 30.0f;
            }
            else
            {
                // If right dragging to rotate, adjust the rotation rate
                // based on the distance from the reference object.
                coarseness = ComputeRotationCoarseness(*sim);
            }

            Quaternionf q = math::XRotation(dy / static_cast<float>(metrics.height) * coarseness) *
                            math::YRotation(dx / static_cast<float>(metrics.width) * coarseness);
            if ((modifiers & RightButton) != 0)
                sim->orbit(q);
            else
                sim->rotate(q.conjugate());
        }

        mouseMotion += abs(dy) + abs(dx);
    }
}


void CelestiaCore::joystickAxis(int axis, float amount)
{
    float deadZone = 0.25f;

    if (abs(amount) < deadZone)
        amount = 0.0f;
    else
        amount = (amount - deadZone) * (1.0f / (1.0f - deadZone));

    amount = math::sign(amount) * math::square(amount);

    if (axis == Joy_XAxis)
        joystickRotation.y() = amount;
    else if (axis == Joy_YAxis)
        joystickRotation.x() = -amount;
}


void CelestiaCore::joystickButton(int button, bool down)
{
    if (button >= 0 && button < JoyButtonCount)
        joyButtonsPressed[button] = down;
}


void CelestiaCore::keyDown(int key, int modifiers)
{
    if (m_scriptHook != nullptr && m_scriptHook->call("keydown", float(key), float(modifiers)))
        return;

    switch (key)
    {
    case Key_F1:
        sim->setTargetSpeed(0);
        break;
    case Key_F2:
        sim->setTargetSpeed(1.0f);
        break;
    case Key_F3:
        sim->setTargetSpeed(1000.0f);
        break;
    case Key_F4:
        sim->setTargetSpeed(1.0_c);
        break;
    case Key_F5:
        sim->setTargetSpeed(10.0_c);
        break;
    case Key_F6:
        sim->setTargetSpeed(1.0_au);
        break;
    case Key_F7:
        sim->setTargetSpeed(1.0_ly);
        break;
    case Key_F11:
        if (movieCapture != nullptr)
        {
            if (isRecording())
                recordPause();
            else
                recordBegin();
        }
        break;
    case Key_F12:
        if (movieCapture != nullptr)
            recordEnd();
        break;
    case Key_NumPad2:
    case Key_NumPad4:
    case Key_NumPad6:
    case Key_NumPad7:
    case Key_NumPad8:
    case Key_NumPad9:
        sim->setTargetSpeed(sim->getTargetSpeed());
        break;

    case Key_Down:
        if (showConsole)
            console->scroll(1);
        break;

    case Key_Up:
        if (showConsole)
            console->scroll(-1);
        break;

    case Key_PageDown:
        if (showConsole)
            console->scroll(Console::PageRows);
        else
            back();
        break;

    case Key_PageUp:
        if (showConsole)
            console->scroll(-Console::PageRows);
        else
            forward();
        break;
    }

    if (KeyAccel < fMaxKeyAccel)
        KeyAccel *= 1.1;

    // Only process alphanumeric keys if we're not in text enter mode
    if (std::islower(key))
        key = std::toupper(key);
    if (!(key >= 'A' && key <= 'Z' && (hud->textEnterMode() != Hud::TextEnterMode::Normal) ))
    {
        if (modifiers & ShiftKey)
            shiftKeysPressed[key] = true;
        else
            keysPressed[key] = true;
    }
}

void CelestiaCore::keyUp(int key, int)
{
    KeyAccel = 1.0;
    if (std::islower(key))
        key = std::toupper(key);
    keysPressed[key] = false;
    shiftKeysPressed[key] = false;
}

static string getKeyName(const char* c, int modifiers)
{
    unsigned int length = strlen(c);

    // Translate control characters
    if (length == 1 && c[0] >= '\001' && c[0] <= '\032')
    {
        return fmt::format("C-{:c}", static_cast<char>('\140' + c[0]));
    }

    if (modifiers & CelestiaCore::ControlKey)
    {
        return fmt::format("C-{}", c);
    }

    return string(c);
}

void CelestiaCore::charEntered(char c, int modifiers)
{
    char C[2];
    C[0] = c;
    C[1] = '\0';
    charEntered(C, modifiers);
}

void CelestiaCore::charEntered(const char *c_p, int modifiers)
{
    Observer* observer = sim->getActiveObserver();

    char c = *c_p;


#ifdef CELX
    if (m_script != nullptr &&
        is_set(hud->textEnterMode(), Hud::TextEnterMode::PassToScript) &&
        c != '\033' &&
        m_script->charEntered(c_p))
    {
        return;
    }

#endif

    if (is_set(hud->textEnterMode(), Hud::TextEnterMode::AutoComplete))
    {
        charEnteredAutoComplete(c_p);
        return;
    }

#ifdef CELX
    if (m_script != nullptr)
    {
        if (c != '\033')
        {
            string keyName = getKeyName(c_p, modifiers);
            if (m_script->handleKeyEvent(keyName.c_str()))
                return;
        }
    }
#endif
    if (m_scriptHook != nullptr && m_scriptHook->call("charentered", getKeyName(c_p, modifiers).c_str()))
        return;

    char C = std::toupper(static_cast<unsigned char>(c));
    switch (C)
    {
    case '\001': // Ctrl+A
        renderer->setRenderFlags(renderer->getRenderFlags() ^ Renderer::ShowAtmospheres);
        notifyWatchers(RenderFlagsChanged);
        break;

    case '\002': // Ctrl+B
        renderer->setRenderFlags(renderer->getRenderFlags() ^ Renderer::ShowBoundaries);
        notifyWatchers(RenderFlagsChanged);
        break;

    case '\n':
    case '\r':
        setTextEnterMode(hud->textEnterMode() | Hud::TextEnterMode::AutoComplete);
        break;

    case '\b':
        sim->setSelection(sim->getSelection().parent());
        break;

    case '\014': // Ctrl+L
        renderer->setRenderFlags(renderer->getRenderFlags() ^ Renderer::ShowNightMaps);
        notifyWatchers(RenderFlagsChanged);
        break;

    case '\013': // Ctrl+K
        renderer->setRenderFlags(renderer->getRenderFlags() ^ Renderer::ShowMarkers);
        if (renderer->getRenderFlags() & Renderer::ShowMarkers)
        {
            flash(_("Markers enabled"));
        }
        else
            flash(_("Markers disabled"));
        notifyWatchers(RenderFlagsChanged);
        break;

    case '\005':  // Ctrl+E
        renderer->setRenderFlags(renderer->getRenderFlags() ^ Renderer::ShowEclipseShadows);
        notifyWatchers(RenderFlagsChanged);
        break;

    case '\007':  // Ctrl+G
        flash(_("Goto surface"));
        addToHistory();
        sim->geosynchronousFollow();
        sim->gotoSurface(5.0);
        break;

    case '\006': // Ctrl+F
        addToHistory();
        altAzimuthMode = !altAzimuthMode;
        if (altAzimuthMode)
        {
            flash(_("Alt-azimuth mode enabled"));
        }
        else
            flash(_("Alt-azimuth mode disabled"));
        break;

    case 127: // Delete
        deleteView();
        break;

    case '\011': // TAB
        viewManager->nextView(sim);
        break;

    case '\020':  // Ctrl+P
        if (!sim->getSelection().empty())
        {
            Selection sel = sim->getSelection();
            if (sim->getUniverse()->isMarked(sel, 1))
            {
                sim->getUniverse()->unmarkObject(sel, 1);
            }
            else
            {
                celestia::MarkerRepresentation markerRep(celestia::MarkerRepresentation::Diamond);
                markerRep.setSize(10.0f);
                markerRep.setColor({0.0f, 1.0f, 0.0f, 0.9f});

                sim->getUniverse()->markObject(sel, markerRep, 1);
            }
        }
        break;

    case '\025': // Ctrl+U
        splitView(View::VerticalSplit);
        break;

    case '\022': // Ctrl+R
        splitView(View::HorizontalSplit);
        break;

    case '\004': // Ctrl+D
        singleView();
        break;

    case '\023':  // Ctrl+S
        renderer->setStarStyle((Renderer::StarStyle) (((int) renderer->getStarStyle() + 1) %
                                                      (int) Renderer::StarStyleCount));
        switch (renderer->getStarStyle())
        {
        case Renderer::FuzzyPointStars:
            flash(_("Star style: fuzzy points"));
            break;
        case Renderer::PointStars:
            flash(_("Star style: points"));
            break;
        case Renderer::ScaledDiscStars:
            flash(_("Star style: scaled discs"));
            break;
        default:
            break;
        }

        notifyWatchers(RenderFlagsChanged);
        break;

    case '\024':  // Ctrl+T
        renderer->setRenderFlags(renderer->getRenderFlags() ^ Renderer::ShowCometTails);
        if (renderer->getRenderFlags() & Renderer::ShowCometTails)
        {
            flash(_("Comet tails enabled"));
        }
        else
            flash(_("Comet tails disabled"));
        notifyWatchers(RenderFlagsChanged);
        break;

    case '\027':  // Ctrl+W
        wireframe = !wireframe;
        renderer->setRenderMode(wireframe ? RenderMode::Line : RenderMode::Fill);
        break;

    case '\030':  // Ctrl+X
        renderer->setRenderFlags(renderer->getRenderFlags() ^ Renderer::ShowSmoothLines);
        if (renderer->getRenderFlags() & Renderer::ShowSmoothLines)
        {
            flash(_("Anti-aliasing enabled"));
        }
        else
        {
            flash(_("Anti-aliasing disabled"));
        }
        notifyWatchers(RenderFlagsChanged);
        break;

    case '\031':  // Ctrl+Y
        renderer->setRenderFlags(renderer->getRenderFlags() ^ Renderer::ShowAutoMag);
        if (renderer->getRenderFlags() & Renderer::ShowAutoMag)
        {
            flash(_("Auto-magnitude enabled"));
            setFaintestAutoMag();
        }
        else
        {
            flash(_("Auto-magnitude disabled"));
        }
        notifyWatchers(RenderFlagsChanged);
        break;


    case '\033': // Escape
        cancelScript();
        addToHistory();
        if (hud->textEnterMode() != Hud::TextEnterMode::Normal)
        {
            setTextEnterMode(Hud::TextEnterMode::Normal);
        }
        else
        {
            if (sim->getObserverMode() == Observer::Travelling)
                sim->setObserverMode(Observer::Free);
            else
                sim->setFrame(ObserverFrame::Universal, Selection());
            if (!sim->getTrackedObject().empty())
                sim->setTrackedObject(Selection());
        }
        flash(_("Cancel"));
        break;

    case ' ':
        if (sim->getPauseState() == true)
        {
#ifdef USE_MINIAUDIO
            resumeAudioIfNeeded();
#endif

            if (scriptState == ScriptPaused)
                scriptState = ScriptRunning;
            sim->setPauseState(false);
        }
        else
        {
#ifdef USE_MINIAUDIO
            pauseAudioIfNeeded();
#endif

            sim->setPauseState(true);

            // If there's a script running then pause it.  This has the
            // potentially confusing side effect of rendering nonfunctional
            // goto, center, and other movement commands.
            if (m_script != nullptr)
            {
                if (scriptState == ScriptRunning)
                    scriptState = ScriptPaused;
            }
            else
            {
                if (scriptState == ScriptPaused)
                    scriptState = ScriptRunning;
            }
        }

        if (sim->getPauseState() == true)
        {
            if (scriptState == ScriptPaused)
                flash(_("Time and script are paused"));
            else
                flash(_("Time is paused"));
        }
        else
        {
            flash(_("Resume"));
        }
        break;

    case '!':
        if (editMode)
        {
            showSelectionInfo(sim->getSelection());
        }
        else
        {
            sim->setTime(astro::UTCtoTDB(astro::Date::systemDate()));
        }
        break;

    case '%':
        switch (renderer->getStarColorTable())
        {
        case ColorTableType::Enhanced:
            renderer->setStarColorTable(ColorTableType::Blackbody_D65);
            flash(_("Star color: Blackbody D65"));
            notifyWatchers(RenderFlagsChanged);
            break;

        case ColorTableType::Blackbody_D65:
            renderer->setStarColorTable(ColorTableType::SunWhite);
            flash(_("Star color: Blackbody (Solar Whitepoint)"));
            notifyWatchers(RenderFlagsChanged);
            break;

        case ColorTableType::SunWhite:
            renderer->setStarColorTable(ColorTableType::VegaWhite);
            flash(_("Star color: Blackbody (Vega Whitepoint)"));
            notifyWatchers(RenderFlagsChanged);
            break;

        case ColorTableType::VegaWhite:
            renderer->setStarColorTable(ColorTableType::Enhanced);
            flash(_("Star color: Classic"));
            notifyWatchers(RenderFlagsChanged);
            break;
        }
        break;

    case '^':
        renderer->setRenderFlags(renderer->getRenderFlags() ^ Renderer::ShowNebulae);
        notifyWatchers(RenderFlagsChanged);
        break;


    case '&':
        renderer->setLabelMode(renderer->getLabelMode() ^ Renderer::LocationLabels);
        notifyWatchers(LabelFlagsChanged);
        break;

    case '*':
        addToHistory();
        sim->reverseObserverOrientation();
        break;

    case '?':
        addToHistory();
        if (!sim->getSelection().empty())
        {
            Vector3d v = sim->getSelection().getPosition(sim->getTime()).offsetFromKm(sim->getObserver().getPosition());
            int hours, mins;
            float secs;
            string buf;
            if (v.norm() >= 86400.0_c)
            {
                // Light travel time in years, if >= 1day
                buf = fmt::sprintf(_("Light travel time:  %.4f yr"),
                                   astro::kilometersToLightYears(v.norm()));
            }
            else
            {
                // If Light travel delay < 1 day, display in [ hr : min : sec ]
                getLightTravelDelay(v.norm(), hours, mins, secs);
                if (hours == 0)
                {
                    buf = fmt::sprintf(_("Light travel time:  %d min  %.1f s"),
                                       mins, secs);
                }
                else
                {
                    buf = fmt::sprintf(_("Light travel time:  %d h  %d min  %.1f s"),
                                       hours, mins, secs);
                }
            }
            flash(buf, 2.0);
        }
        break;

    case '-':
        addToHistory();

        if (sim->getSelection().body() &&
            (sim->getTargetSpeed() < 0.99_c))
        {
            Vector3d v = sim->getSelection().getPosition(sim->getTime()).offsetFromKm(sim->getObserver().getPosition());
            timeInfo.lightTravelFlag = !timeInfo.lightTravelFlag;
            if (timeInfo.lightTravelFlag)
            {
                flash(_("Light travel delay included"), 2.0);
                setLightTravelDelay(v.norm());
            }
            else
            {
                flash(_("Light travel delay switched off"), 2.0);
                setLightTravelDelay(-v.norm());
            }
        }
        else
        {
            flash(_("Light travel delay ignored"));
        }
        break;

    case ',':
        addToHistory();
        if (observer->getFOV() > renderer->getProjectionMode()->getMinimumFOV())
        {
            observer->setFOV(observer->getFOV() / 1.05f);
            setZoomFromFOV();
            if((renderer->getRenderFlags() & Renderer::ShowAutoMag))
            {
                setFaintestAutoMag();
                setlocale(LC_NUMERIC, "");
                string buf = fmt::sprintf(_("Magnitude limit: %.2f"), sim->getFaintestVisible());
                setlocale(LC_NUMERIC, "C");
                flash(buf);
            }
        }
        break;

    case '.':
        addToHistory();
        if (observer->getFOV() < renderer->getProjectionMode()->getMaximumFOV())
        {
            observer->setFOV(observer->getFOV() * 1.05f);
            setZoomFromFOV();
            if((renderer->getRenderFlags() & Renderer::ShowAutoMag) != 0)
            {
                setFaintestAutoMag();
                setlocale(LC_NUMERIC, "");
                string buf = fmt::sprintf(_("Magnitude limit: %.2f"), sim->getFaintestVisible());
                setlocale(LC_NUMERIC, "C");
                flash(buf);
            }
        }
        break;

    case '+':
        addToHistory();
        if (observer->getDisplayedSurface() != "")
        {
            observer->setDisplayedSurface("");
            flash(_("Using normal surface textures."));
        }
        else
        {
            observer->setDisplayedSurface("limit of knowledge");
            flash(_("Using limit of knowledge surface textures."));
        }
        break;

    case '/':
        renderer->setRenderFlags(renderer->getRenderFlags() ^ Renderer::ShowDiagrams);
        notifyWatchers(RenderFlagsChanged);
        break;

    case '0':
        addToHistory();
        sim->selectPlanet(-1);
        break;

    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        addToHistory();
        if (!(modifiers & ControlKey))
            sim->selectPlanet(c - '1');
        break;

    case ';':
        renderer->setRenderFlags(renderer->getRenderFlags() ^ Renderer::ShowCelestialSphere);
        notifyWatchers(RenderFlagsChanged);
        break;

    case '=':
        renderer->setLabelMode(renderer->getLabelMode() ^ Renderer::ConstellationLabels);
        notifyWatchers(LabelFlagsChanged);
        break;

    case 'B':
        renderer->setLabelMode(renderer->getLabelMode() ^ Renderer::StarLabels);
        notifyWatchers(LabelFlagsChanged);
        break;

    case 'C':
        addToHistory();
        if (c == 'c')
            sim->centerSelection();
        else
            sim->centerSelectionCO();
        break;

    case 'E':
        if (c == 'e')
            renderer->setLabelMode(renderer->getLabelMode() ^ Renderer::GalaxyLabels);
        else
            renderer->setLabelMode(renderer->getLabelMode() ^ Renderer::GlobularLabels);
        notifyWatchers(LabelFlagsChanged);
        break;

    case 'F':
        addToHistory();
        flash(_("Follow"));
        sim->follow();
        break;

    case 'G':
        addToHistory();
        if (sim->getFrame()->getCoordinateSystem() == ObserverFrame::Universal)
            sim->follow();
        sim->gotoSelection(5.0, Vector3f::UnitY(), ObserverFrame::ObserverLocal);
        break;

    case 'H':
        addToHistory();
        sim->setSelection(sim->getUniverse()->getStarCatalog()->find(0));
        break;

    case 'I':
        renderer->setRenderFlags(renderer->getRenderFlags() ^ Renderer::ShowCloudMaps);
        notifyWatchers(RenderFlagsChanged);
        break;

    case 'J':
        addToHistory();
        sim->setTimeScale(-sim->getTimeScale());
        if (sim->getTimeScale() >= 0)
            flash(_("Time: Forward"));
        else
            flash(_("Time: Backward"));
        break;

    case 'K':
        addToHistory();
        if (abs(sim->getTimeScale()) > TimeInfo::MinimumTimeRate)
        {
            if (c == 'k')
                sim->setTimeScale(sim->getTimeScale() / CoarseTimeScaleFactor);
            else
                sim->setTimeScale(sim->getTimeScale() / FineTimeScaleFactor);
            setlocale(LC_NUMERIC, "");
            string buf = fmt::sprintf(_("Time rate: %.6g"),  sim->getTimeScale()); // XXX %'.12g
            setlocale(LC_NUMERIC, "C");
            flash(buf);
        }
        break;

    case 'L':
        addToHistory();
        if (abs(sim->getTimeScale()) < MaximumTimeRate)
        {
            if (c == 'l')
                sim->setTimeScale(sim->getTimeScale() * CoarseTimeScaleFactor);
            else
                sim->setTimeScale(sim->getTimeScale() * FineTimeScaleFactor);
            setlocale(LC_NUMERIC, "");
            string buf = fmt::sprintf(_("Time rate: %.6g"),  sim->getTimeScale()); // XXX %'.12g
            setlocale(LC_NUMERIC, "C");
            flash(buf);
        }
        break;

    case 'M':
        if (c == 'm')
            renderer->setLabelMode(renderer->getLabelMode() ^ Renderer::MoonLabels);
        else
            renderer->setLabelMode(renderer->getLabelMode() ^ Renderer::MinorMoonLabels);
        notifyWatchers(LabelFlagsChanged);
        break;

    case 'N':
        renderer->setLabelMode(renderer->getLabelMode() ^ Renderer::SpacecraftLabels);
        notifyWatchers(LabelFlagsChanged);
        break;

    case 'O':
        renderer->setRenderFlags(renderer->getRenderFlags() ^ Renderer::ShowOrbits);
        notifyWatchers(RenderFlagsChanged);
        break;

    case 'P':
        if (c == 'p')
            renderer->setLabelMode(renderer->getLabelMode() ^ Renderer::PlanetLabels);
        else
            renderer->setLabelMode(renderer->getLabelMode() ^ Renderer::DwarfPlanetLabels);
        notifyWatchers(LabelFlagsChanged);
        break;

    case 'R':
        if (c == 'r') // Skip rangechecking as setResolution does it already
            renderer->setResolution(renderer->getResolution() - 1);
        else
            renderer->setResolution(renderer->getResolution() + 1);
        switch (renderer->getResolution())
        {
        case 0:
            flash(_("Low res textures"));
            break;
        case 1:
            flash(_("Medium res textures"));
            break;
        case 2:
            flash(_("High res textures"));
            break;
        }
        notifyWatchers(RenderFlagsChanged); //how to synchronize with menu?
        break;

    case 'Q':
        sim->setTargetSpeed(-sim->getTargetSpeed());
        break;

    case 'S':
        sim->setTargetSpeed(0);
        break;

    case 'T':
        addToHistory();
        if (sim->getTrackedObject().empty())
            sim->setTrackedObject(sim->getSelection());
        else
            sim->setTrackedObject(Selection());
        break;

    case 'U':
        if (c == 'u')
            renderer->setRenderFlags(renderer->getRenderFlags() ^ Renderer::ShowGalaxies);
        else
            renderer->setRenderFlags(renderer->getRenderFlags() ^ Renderer::ShowGlobulars);
        notifyWatchers(RenderFlagsChanged);
        break;

    case 'V':
        setHudDetail((getHudDetail() + 1) % 3);
        break;

    case 'W':
        if (c == 'w')
            renderer->setLabelMode(renderer->getLabelMode() ^ Renderer::AsteroidLabels);
        else
            renderer->setLabelMode(renderer->getLabelMode() ^ Renderer::CometLabels);
        notifyWatchers(LabelFlagsChanged);
        break;

    case 'X':
        sim->setTargetSpeed(sim->getTargetSpeed());
        break;

    case 'Y':
        flash(_("Sync Orbit"));
        addToHistory();
        sim->geosynchronousFollow();
        break;

    case ':':
        flash(_("Lock"));
        addToHistory();
        sim->phaseLock();
        break;

    case '"':
        flash(_("Chase"));
        addToHistory();
        sim->chase();
        break;

    case '[':
        if ((renderer->getRenderFlags() & Renderer::ShowAutoMag) == 0)
        {
            if (sim->getFaintestVisible() > 1.0f)
            {
                setFaintest(sim->getFaintestVisible() - 0.2f);
                notifyWatchers(FaintestChanged);
                setlocale(LC_NUMERIC, "");
                string buf = fmt::sprintf(_("Magnitude limit:  %.2f"),
                                          sim->getFaintestVisible());
                setlocale(LC_NUMERIC, "C");
                flash(buf);
            }
        }
        else if (renderer->getFaintestAM45deg() > 6.0f)
        {
            renderer->setFaintestAM45deg(renderer->getFaintestAM45deg() - 0.1f);
            setFaintestAutoMag();
            setlocale(LC_NUMERIC, "");
            string buf = fmt::sprintf(_("Auto magnitude limit at 45 degrees:  %.2f"),
                                      renderer->getFaintestAM45deg());
            setlocale(LC_NUMERIC, "C");
            flash(buf);
        }
        break;

    case '\\':
        addToHistory();
        sim->setTimeScale(1.0f);
        break;

    case ']':
        if((renderer->getRenderFlags() & Renderer::ShowAutoMag) == 0)
        {
            if (sim->getFaintestVisible() < 15.0f)
            {
                setFaintest(sim->getFaintestVisible() + 0.2f);
                notifyWatchers(FaintestChanged);
                setlocale(LC_NUMERIC, "");
                string buf = fmt::sprintf(_("Magnitude limit:  %.2f"),
                                          sim->getFaintestVisible());
                setlocale(LC_NUMERIC, "C");
                flash(buf);
            }
        }
        else if (renderer->getFaintestAM45deg() < 12.0f)
        {
            renderer->setFaintestAM45deg(renderer->getFaintestAM45deg() + 0.1f);
            setFaintestAutoMag();
            setlocale(LC_NUMERIC, "");
            string buf = fmt::sprintf(_("Auto magnitude limit at 45 degrees:  %.2f"),
                                      renderer->getFaintestAM45deg());
            setlocale(LC_NUMERIC, "C");
            flash(buf);
        }
        break;

    case '`':
        hud->hudSettings().showFPSCounter = !hud->hudSettings().showFPSCounter;
        break;

    case '{':
        {
            if (renderer->getAmbientLightLevel() > 0.05f)
                renderer->setAmbientLightLevel(renderer->getAmbientLightLevel() - 0.05f);
            else
                renderer->setAmbientLightLevel(0.0f);
            notifyWatchers(AmbientLightChanged);
            setlocale(LC_NUMERIC, "");
            string buf = fmt::sprintf(_("Ambient light level:  %.2f"),
                                      renderer->getAmbientLightLevel());
            setlocale(LC_NUMERIC, "C");
            flash(buf);
        }
        break;

    case '}':
        {
            if (renderer->getAmbientLightLevel() < 0.95f)
                renderer->setAmbientLightLevel(renderer->getAmbientLightLevel() + 0.05f);
            else
                renderer->setAmbientLightLevel(1.0f);
            notifyWatchers(AmbientLightChanged);
            setlocale(LC_NUMERIC, "");
            string buf = fmt::sprintf(_("Ambient light level:  %.2f"),
                                      renderer->getAmbientLightLevel());
            setlocale(LC_NUMERIC, "C");
            flash(buf);
        }
        break;

    case '(':
        {
            Galaxy::decreaseLightGain();
            setlocale(LC_NUMERIC, "");
            string buf = fmt::sprintf("%s:  %3.0f %%", _("Light gain"), Galaxy::getLightGain() * 100.0f);
            setlocale(LC_NUMERIC, "C");
            flash(buf);
            notifyWatchers(GalaxyLightGainChanged);
        }
        break;

    case ')':
        {
            Galaxy::increaseLightGain();
            setlocale(LC_NUMERIC, "");
            string buf = fmt::sprintf("%s:  %3.0f %%", _("Light gain"), Galaxy::getLightGain() * 100.0f);
            setlocale(LC_NUMERIC, "C");
            flash(buf);
            notifyWatchers(GalaxyLightGainChanged);
        }
        break;

    case '~':
        showConsole = !showConsole;
        break;

    case '@':
        // TODO: 'Edit mode' should be eliminated; it can be done better
        // with a Lua script.
        editMode = !editMode;
        break;
    }
}


void CelestiaCore::charEnteredAutoComplete(const char* c_p)
{
    switch (hud->textInput().charEntered(sim, c_p, (renderer->getLabelMode() & Renderer::LocationLabels) != 0))
    {
    case CharEnteredResult::Finished:
        updateSelectionFromInput();
        [[fallthrough]];

    case CharEnteredResult::Cancelled:
        setTextEnterMode(hud->textEnterMode() & ~Hud::TextEnterMode::AutoComplete);
        [[fallthrough]];

    default: // CharEnteredResult::Normal
        return;
    }
}


void CelestiaCore::updateSelectionFromInput()
{
    auto typedText = hud->textInput().getTypedText();
    if (typedText.empty())
        return;

    Selection sel = sim->findObjectFromPath(typedText, true);
    if (sel.empty())
    {
        auto completion = hud->textInput().getCompletion();
        if (!completion.empty())
            sel = sim->findObjectFromPath(completion.front(), true);
    }

    if (!sel.empty())
    {
        addToHistory();
        sim->setSelection(sel);
    }
}


void CelestiaCore::getLightTravelDelay(double distanceKm, int& hours, int& mins, float& secs)
{
    // light travel time in hours
    double lt = distanceKm / (3600.0_c);
    hours = (int) lt;
    double mm = (lt - hours) * 60;
    mins = (int) mm;
    secs = (float) ((mm  - mins) * 60);
}


void CelestiaCore::setLightTravelDelay(double distanceKm)
{
    // light travel time in days
    double lt = distanceKm / (86400.0_c);
    sim->setTime(sim->getTime() - lt);
}


bool CelestiaCore::getAltAzimuthMode() const
{
    return altAzimuthMode;
}

void CelestiaCore::setAltAzimuthMode(bool enable)
{
    altAzimuthMode = enable;
}

void CelestiaCore::start()
{
    time_t curtime = time(nullptr);
    start(astro::UTCtoTDB((double) curtime / 86400.0 + (double) astro::Date(1970, 1, 1)));
}

void CelestiaCore::start(double t)
{
    if (!config->paths.initScriptFile.empty())
    {
        // using the KdeAlerter in runScript would create an infinite loop,
        // break it here by resetting config->initScriptFile:
        fs::path filename(config->paths.initScriptFile);
        // Don't use {} as it will throw error C2593 on MSVC
        config->paths.initScriptFile = fs::path();
        runScript(filename);
    }

    // Set the simulation starting time to the current system time
    sim->setTime(t);
    sim->update(0.0);

    sysTime = timer->getTime();

    if (!startURL.empty())
        goToUrl(startURL);
}

void CelestiaCore::setStartURL(const string &url)
{
    if (!url.substr(0, 4).compare("cel:"))
    {
        startURL = url;
        // Don't use {} as it will throw error C2593 on MSVC
        config->paths.initScriptFile = fs::path();
    }
    else
    {
        config->paths.initScriptFile = url;
    }
}

void CelestiaCore::tick()
{
    tick(timer->getTime() - sysTime);
}

void CelestiaCore::tick(double dt)
{
    sysTime += dt;

    // The time step is normally driven by the system clock; however, when
    // recording a movie, we fix the time step the frame rate of the movie.
    if (movieCapture != nullptr && recording)
        dt = 1.0 / static_cast<double>(movieCapture->getFrameRate());

    // Pause script execution
    if (scriptState == ScriptPaused)
        dt = 0.0;

    timeInfo.currentTime += dt;

    // Mouse wheel zoom
    if (zoomMotion != 0.0f)
    {
        double span = 0.1;
#if 0
        double fraction;

        if (currentTime - zoomTime >= span)
            fraction = (zoomTime + span) - (currentTime - dt);
        else
            fraction = dt / span;
#endif

        // sim->changeOrbitDistance(zoomMotion * (float) fraction);
        if (timeInfo.currentTime - zoomTime >= span)
            zoomMotion = 0.0f;
    }

    // Mouse wheel dolly
    if (dollyMotion != 0.0)
    {
        double span = 0.1;
        double fraction;

        if (timeInfo.currentTime - dollyTime >= span)
            fraction = (dollyTime + span) - (timeInfo.currentTime - dt);
        else
            fraction = dt / span;

        sim->changeOrbitDistance((float) (dollyMotion * fraction));
        if (timeInfo.currentTime - dollyTime >= span)
            dollyMotion = 0.0f;
    }

    // Keyboard dolly
    if (keysPressed[Key_Home])
        sim->changeOrbitDistance((float) (-dt * 2));
    if (keysPressed[Key_End])
        sim->changeOrbitDistance((float) (dt * 2));

    // Keyboard rotate
    Vector3d av = sim->getObserver().getAngularVelocity();

    av = av * exp(-dt * RotationDecay);

    float fov = sim->getActiveObserver()->getFOV() / stdFOV;
    Selection refObject = sim->getFrame()->getRefObject();

    // Handle arrow keys; disable them when the log console is displayed,
    // because then they're used to scroll up and down.
    if (!showConsole)
    {
        if (!altAzimuthMode)
        {
            if (keysPressed[Key_Left])
                av += Vector3d::UnitZ() * (dt * -KeyRotationAccel);
            if (keysPressed[Key_Right])
                av += Vector3d::UnitZ() * (dt * KeyRotationAccel);
            if (keysPressed[Key_Down])
                av += Vector3d::UnitX() * (dt * fov * -KeyRotationAccel);
            if (keysPressed[Key_Up])
                av += Vector3d::UnitX() * (dt * fov * KeyRotationAccel);
        }
        else
        {
            if (!refObject.empty())
            {
                Quaterniond orientation = sim->getObserver().getOrientation();
                Vector3d up = sim->getObserver().getPosition().offsetFromKm(refObject.getPosition(sim->getTime()));
                up.normalize();

                Vector3d v = up * (KeyRotationAccel * dt);
                v = orientation * v;

                if (keysPressed[Key_Left])
                    av -= v;
                if (keysPressed[Key_Right])
                    av += v;
                if (keysPressed[Key_Down])
                    av += Vector3d::UnitX() * (dt * fov * -KeyRotationAccel);
                if (keysPressed[Key_Up])
                    av += Vector3d::UnitX() * (dt * fov * KeyRotationAccel);
            }
        }
    }

    if (keysPressed[Key_NumPad4])
        av += Vector3d(0.0, dt * fov * -KeyRotationAccel, 0.0);
    if (keysPressed[Key_NumPad6])
        av += Vector3d(0.0, dt * fov * KeyRotationAccel, 0.0);
    if (keysPressed[Key_NumPad2])
        av += Vector3d(dt * fov * -KeyRotationAccel, 0.0, 0.0);
    if (keysPressed[Key_NumPad8])
        av += Vector3d(dt * fov * KeyRotationAccel, 0.0, 0.0);
    if (keysPressed[Key_NumPad7] || joyButtonsPressed[JoyButton7])
        av += Vector3d(0.0, 0.0, dt * -KeyRotationAccel);
    if (keysPressed[Key_NumPad9] || joyButtonsPressed[JoyButton8])
        av += Vector3d(0.0, 0.0, dt * KeyRotationAccel);

    //Use Boolean to indicate if sim->setTargetSpeed() is called
    bool bSetTargetSpeed = false;
    if (joystickRotation != Vector3f::Zero())
    {
        bSetTargetSpeed = true;

        av += (dt * KeyRotationAccel) * joystickRotation.cast<double>();
        sim->setTargetSpeed(sim->getTargetSpeed());
    }

    if (keysPressed[Key_NumPad5])
        av = av * exp(-dt * RotationBraking);

    sim->getObserver().setAngularVelocity(av);

    if (keysPressed[static_cast<int>('A')] || joyButtonsPressed[JoyButton2])
    {
        bSetTargetSpeed = true;

        if (float currentSpeed = sim->getTargetSpeed(); currentSpeed != 0.0f)
            sim->setTargetSpeed(currentSpeed * std::exp(static_cast<float>(dt) * 3.0f * accelerationCoefficient));
        else
            sim->setTargetSpeed(0.1f);
    }
    if (keysPressed[static_cast<int>('Z')] || joyButtonsPressed[JoyButton1])
    {
        if (float currentSpeed = sim->getTargetSpeed(); currentSpeed != 0.0f)
        {
            bSetTargetSpeed = true;
            if (std::abs(currentSpeed) < 0.1f)
                sim->setTargetSpeed(0.0f);
            else
                sim->setTargetSpeed(currentSpeed / std::exp(static_cast<float>(dt) * 3.0f * decelerationCoefficient));
        }
    }
    if (!bSetTargetSpeed && av.norm() > 0.0f)
    {
        // Force observer velocity vector to align with observer direction if an observer
        // angular velocity still exists.
        sim->setTargetSpeed(sim->getTargetSpeed());
    }

    if (!refObject.empty())
    {
        Quaternionf q = Quaternionf::Identity();
        float coarseness = ComputeRotationCoarseness(*sim);

        if (shiftKeysPressed[Key_Left])
            q = q * math::YRotation((float) (dt * -KeyRotationAccel * coarseness));
        if (shiftKeysPressed[Key_Right])
            q = q * math::YRotation((float) (dt *  KeyRotationAccel * coarseness));
        if (shiftKeysPressed[Key_Up])
            q = q * math::XRotation((float) (dt * -KeyRotationAccel * coarseness));
        if (shiftKeysPressed[Key_Down])
            q = q * math::XRotation((float) (dt *  KeyRotationAccel * coarseness));
        sim->orbit(q);
    }

    // If there's a script running, tick it
    if (m_script != nullptr)
    {
        m_script->handleTickEvent(dt);
        if (scriptState == ScriptRunning)
        {
            bool finished = m_script->tick(dt);
            if (finished)
                cancelScript();
        }
    }
    if (m_scriptHook != nullptr)
        m_scriptHook->call("tick", dt);

    sim->update(dt);
}


void CelestiaCore::draw()
{
    if (!viewUpdateRequired())
        return;

    // Render each view
    for (const auto view : viewManager->views())
        draw(view);

    // Reset to render to the main window
    if (viewManager->views().size() > 1)
        renderer->setRenderRegion(0, 0, metrics.width, metrics.height, false);

    bool toggleAA = renderer->isMSAAEnabled();
    if (toggleAA && (renderer->getRenderFlags() & Renderer::ShowCloudMaps))
        renderer->disableMSAA();

    renderOverlay();
    if (showConsole)
    {
        console->setFont(hud->font());
        console->setColor(1.0f, 1.0f, 1.0f, 1.0f);
        console->begin();
        console->moveBy(static_cast<float>(metrics.insetLeft), static_cast<float>(metrics.screenDpi) / 25.4f * 53.0f);
        console->render(Console::PageRows);
        console->end();
    }

    if (toggleAA)
        renderer->enableMSAA();

    if (movieCapture != nullptr && recording)
        movieCapture->captureFrame();

    // Frame rate counter
    nFrames++;
    if (nFrames == 100 || sysTime - fpsCounterStartTime > 10.0)
    {
        timeInfo.fps = static_cast<float>(nFrames / (sysTime - fpsCounterStartTime));
        nFrames = 0;
        fpsCounterStartTime = sysTime;
    }
}


void CelestiaCore::resize(GLsizei w, GLsizei h)
{
    if (h == 0)
        h = 1;

    if (renderer != nullptr)
    {
        renderer->setViewport(0, 0, w, h);
        renderer->resize(w, h);
    }
    hud->setWindowSize(w, h);
    console->setScale(w, h);
    metrics.width = w;
    metrics.height = h;

    setFOVFromZoom();
    if (m_scriptHook != nullptr && m_scriptHook->call("resize", float(w), float(h)))
        return;
}

void CelestiaCore::draw(View* view)
{
    if (view->type != View::ViewWindow) return;

    bool viewportEffectUsed = false;

    FramebufferObject *fbo = nullptr;
    if (viewportEffect != nullptr)
    {
        // create/update FBO for viewport effect
        view->updateFBO(metrics.width, metrics.height);
        fbo = view->getFBO();
    }
    bool process = fbo != nullptr && viewportEffect->preprocess(renderer, fbo);

    auto x = static_cast<int>(view->x * static_cast<float>(metrics.width));
    auto y = static_cast<int>(view->y * static_cast<float>(metrics.height));
    auto viewWidth = static_cast<int>(view->width * static_cast<float>(metrics.width));
    auto viewHeight = static_cast<int>(view->height * static_cast<float>(metrics.height));
    // If we need to process, we draw to the FBO which starts at point zero
    renderer->setRenderRegion(process ? 0 : x, process ? 0 : y, viewWidth, viewHeight, !view->isRootView());

    if (view->isRootView())
        sim->render(*renderer);
    else
        sim->render(*renderer, *view->observer);

    // Viewport need to be reset to start from (x,y) instead of point zero
    if (process && (x != 0 || y != 0))
        renderer->setRenderRegion(x, y, viewWidth, viewHeight);

    if (process && viewportEffect->prerender(renderer, fbo))
    {
        if (viewportEffect->render(renderer, fbo, viewWidth, viewHeight))
            viewportEffectUsed = true;
        else
            GetLogger()->error("Unable to render viewport effect.\n");
    }
    isViewportEffectUsed = viewportEffectUsed;
}

void CelestiaCore::setSafeAreaInsets(int left, int top, int right, int bottom)
{
    metrics.insetLeft = left;
    metrics.insetTop = top;
    metrics.insetRight = right;
    metrics.insetBottom = bottom;
}

std::tuple<int, int, int, int> CelestiaCore::getSafeAreaInsets() const
{
    return std::make_tuple(metrics.insetLeft, metrics.insetTop, metrics.insetRight, metrics.insetBottom);
}

std::tuple<int, int> CelestiaCore::getWindowDimension() const
{
    return std::make_tuple(metrics.width, metrics.height);
}

void CelestiaCore::setAccelerationCoefficient(float coefficient)
{
    accelerationCoefficient = coefficient;
}

void CelestiaCore::setDecelerationCoefficient(float coefficient)
{
    decelerationCoefficient = coefficient;
}

// Return true if anything changed that requires re-rendering. Otherwise, we
// can skip rendering, keep the GPU idle, and save power.
bool CelestiaCore::viewUpdateRequired() const
{
#if 1
    // Enable after 1.5.0
    return true;
#else
    bool isPaused = sim->getPauseState() || sim->getTimeScale() == 0.0;

    // See if the camera in any of the views is moving
    bool observersMoving = false;
    for (const auto v : views)
    {
        if (v->observer->getAngularVelocity().length() > 1.0e-10 ||
            v->observer->getVelocity().length() > 1.0e-12)
        {
            observersMoving = true;
            break;
        }
    }

    if (viewChanged ||
        !isPaused ||
        observersMoving ||
        dollyMotion != 0.0 ||
        zoomMotion != 0.0 ||
        scriptState == ScriptRunning ||
        renderer->settingsHaveChanged())
    {
        return true;
    }
    else
    {
        return false;
    }
#endif
}


void CelestiaCore::splitView(View::Type type, View* av, float splitPos)
{
    switch (viewManager->splitView(sim, type, av, splitPos))
    {
    case ViewSplitResult::Ignored:
        return;
    case ViewSplitResult::NotSplittable:
        flash(_("View too small to be split"));
        return;
    default: // ViewSplitResult::SplitOk
        setFOVFromZoom();
        flash(_("Added view"));
        break;
    }
}


void CelestiaCore::setFOVFromZoom()
{
    auto projectionMode = renderer->getProjectionMode();
    for (const auto v : viewManager->views())
    {
        if (v->type != View::ViewWindow)
            continue;

        projectionMode->setSize(v->width * static_cast<float>(metrics.width), v->height * static_cast<float>(metrics.height));
        v->observer->setFOV(projectionMode->getFOV(v->observer->getZoom()));
    }
}

void CelestiaCore::setZoomFromFOV()
{
    auto projectionMode = renderer->getProjectionMode();
    for (auto v : viewManager->views())
    {
        if (v-> type != View::ViewWindow)
            continue;

        projectionMode->setSize(v->width * static_cast<float>(metrics.width), v->height * static_cast<float>(metrics.height));
        v->observer->setZoom(projectionMode->getZoom(v->observer->getFOV()));
    }
}

void CelestiaCore::singleView(const View* av)
{
    viewManager->singleView(sim, av);
    setFOVFromZoom();
}

void CelestiaCore::setActiveView(const View* v)
{
    viewManager->setActiveView(sim, v);
}

void CelestiaCore::deleteView(View* v)
{
    if (viewManager->deleteView(sim, v))
        setFOVFromZoom();
}

bool CelestiaCore::getFramesVisible() const
{
    return viewManager->showViewFrames();
}

void CelestiaCore::setFramesVisible(bool visible)
{
    viewManager->showViewFrames(visible);
}

bool CelestiaCore::getActiveFrameVisible() const
{
    return viewManager->showActiveViewFrame();
}

void CelestiaCore::setActiveFrameVisible(bool visible)
{
    viewManager->showActiveViewFrame(visible);
}


Renderer* CelestiaCore::getRenderer() const
{
    return renderer;
}

Simulation* CelestiaCore::getSimulation() const
{
    return sim;
}

void CelestiaCore::showText(std::string_view s,
                            int horig, int vorig,
                            int hoff, int voff,
                            double duration)
{
    auto [emWidth, height] = hud->titleMetrics();
    hud->showText(TextPrintPosition::relative(horig, vorig, hoff, voff, emWidth, height),
                  s, duration, timeInfo.currentTime);
}

void CelestiaCore::showTextAtPixel(std::string_view s, int x, int y, double duration)
{
    hud->showText(TextPrintPosition::absolute(x, y), s, duration, timeInfo.currentTime);
}


int CelestiaCore::getTextWidth(std::string_view s) const
{
    return hud->getTextWidth(s);
}


void CelestiaCore::setScriptImage(std::unique_ptr<OverlayImage>&& _image)
{
    hud->setImage(std::move(_image), timeInfo.currentTime);
}


void CelestiaCore::renderOverlay()
{
    if (m_scriptHook != nullptr)
        m_scriptHook->call("renderoverlay");

    hud->renderOverlay(metrics, sim, *viewManager, movieCapture, timeInfo, m_script != nullptr, editMode);
}


class SolarSystemLoader
{
    Universe* universe;
    ProgressNotifier* notifier;
    const vector<fs::path>& skip;

 public:
    SolarSystemLoader(Universe* u,
                      ProgressNotifier* pn,
                      const vector<fs::path>& skip) :
        universe(u),
        notifier(pn),
        skip(skip)
    {
    }

    void process(const fs::path& filepath)
    {
        if (DetermineFileType(filepath) != ContentType::CelestiaCatalog)
            return;

        if (find(begin(skip), end(skip), filepath) != end(skip))
        {
            GetLogger()->info(_("Skipping solar system catalog: {}\n"), filepath);
            return;
        }
        GetLogger()->info(_("Loading solar system catalog: {}\n"), filepath);
        if (notifier != nullptr)
            notifier->update(filepath.filename().string());

        ifstream solarSysFile(filepath, ios::in);
        if (solarSysFile.good())
        {
            LoadSolarSystemObjects(solarSysFile,
                                   *universe,
                                   filepath.parent_path());
        }
    }
};

template <class OBJDB> class CatalogLoader
{
    OBJDB*      objDB;
    string      typeDesc;
    ContentType contentType;
    ProgressNotifier* notifier;
    const vector<fs::path>& skip;

 public:
    CatalogLoader(OBJDB* db,
                  const std::string& typeDesc,
                  const ContentType& contentType,
                  ProgressNotifier* pn,
                  const vector<fs::path>& skip) :
        objDB      (db),
        typeDesc   (typeDesc),
        contentType(contentType),
        notifier   (pn),
        skip       (skip)
    {
    }

    void process(const fs::path& filepath)
    {
        if (DetermineFileType(filepath) != contentType)
            return;

        if (find(begin(skip), end(skip), filepath) != end(skip))
        {
            GetLogger()->info(_("Skipping {} catalog: {}\n"), typeDesc, filepath);
            return;
        }
        GetLogger()->info(_("Loading {} catalog: {}\n"), typeDesc, filepath);
        if (notifier != nullptr)
            notifier->update(filepath.filename().string());

        ifstream catalogFile(filepath, ios::in);
        if (catalogFile.good())
        {
            if (!objDB->load(catalogFile, filepath.parent_path()))
                GetLogger()->error(_("Error reading {} catalog file: {}\n"), typeDesc, filepath);
        }
    }
};

using StarLoader = CatalogLoader<StarDatabaseBuilder>;
using DeepSkyLoader = CatalogLoader<DSODatabase>;


bool CelestiaCore::initSimulation(const fs::path& configFileName,
                                  const vector<fs::path>& extrasDirs,
                                  ProgressNotifier* progressNotifier)
{
    config = std::make_unique<CelestiaConfig>();
    bool hasConfig = false;
    if (!configFileName.empty())
    {
        hasConfig = ReadCelestiaConfig(configFileName, *config);
    }
    else
    {
        hasConfig = ReadCelestiaConfig("celestia.cfg", *config);

        fs::path localConfigFile = PathExp("~/.celestia.cfg");
        if (!localConfigFile.empty())
            hasConfig |= ReadCelestiaConfig(localConfigFile, *config);

        localConfigFile = PathExp("~/.celestia-1.7.cfg");
        if (!localConfigFile.empty())
            hasConfig |= ReadCelestiaConfig(localConfigFile, *config);
    }

    if (!hasConfig)
    {
        fatalError(_("Error reading configuration file."), false);
        return false;
    }

    // Set the console log size; ignore any request to use less than 100 lines
    if (config->consoleLogRows > 100)
        console->setRowCount(config->consoleLogRows);

    if (!config->paths.leapSecondsFile.empty())
        ReadLeapSecondsFile(config->paths.leapSecondsFile, leapSeconds);

#ifdef USE_SPICE
    if (!celestia::ephem::InitializeSpice())
    {
        fatalError(_("Initialization of SPICE library failed."), false);
        return false;
    }
#endif

    // Insert additional extras directories into the configuration. These
    // additional directories typically come from the command line. It may
    // be useful to permit other command line overrides of config file fields.
    // Only insert the additional extras directories that aren't also
    // listed in the configuration file. The additional directories are added
    // after the ones from the config file and the order in which they were
    // specified is preserved. This process in O(N*M), but the number of
    // additional extras directories should be small.
    for (const auto& dir : extrasDirs)
    {
        if (find(config->paths.extrasDirs.begin(), config->paths.extrasDirs.end(), dir) ==
            config->paths.extrasDirs.end())
        {
            config->paths.extrasDirs.push_back(dir);
        }
    }

    hud = std::make_unique<Hud>();

#ifdef CELX
    initLuaHook(progressNotifier);
#endif

    KeyRotationAccel = math::degToRad(config->mouse.rotateAcceleration);
    MouseRotationSensitivity = math::degToRad(config->mouse.rotationSensitivity);

    readFavoritesFile();

    // If we couldn't read the favorites list from a file, allocate
    // an empty list.
    if (favorites == nullptr)
        favorites = new FavoritesList();

    universe = new Universe();


    /***** Load star catalogs *****/

    if (!readStars(*config, progressNotifier))
    {
        fatalError(_("Cannot read star database."), false);
        return false;
    }


    /***** Load the deep sky catalogs *****/

    auto dsoDB = std::make_unique<DSODatabase>();
    dsoDB->setNameDatabase(std::make_unique<DSONameDatabase>());

    // Load first the vector of dsoCatalogFiles in the data directory (deepsky.dsc, globulars.dsc,...):

    for (const auto& file : config->paths.dsoCatalogFiles)
    {
        if (progressNotifier)
            progressNotifier->update(file.string());

        ifstream dsoFile(file, ios::in);
        if (!dsoFile.good())
        {
            GetLogger()->error(_("Error opening deepsky catalog file {}.\n"), file);
        }
        if (!dsoDB->load(dsoFile, ""))
        {
            GetLogger()->error(_("Cannot read Deep Sky Objects database {}.\n"), file);
        }
    }

    // Next, read all the deep sky files in the extras directories
    {
        std::vector<fs::path> entries;
        DeepSkyLoader loader(dsoDB.get(), "deep sky object",
                             ContentType::CelestiaDeepSkyCatalog,
                             progressNotifier,
                             config->paths.skipExtras);
        for (const auto& dir : config->paths.extrasDirs)
        {
            if (!is_valid_directory(dir))
                continue;

            entries.clear();
            std::error_code ec;
            auto iter = fs::recursive_directory_iterator(dir, ec);
            for (; iter != end(iter); iter.increment(ec))
            {
                if (ec)
                    continue;
                if (!fs::is_directory(iter->path(), ec))
                    entries.push_back(iter->path());
            }
            std::sort(begin(entries), end(entries));
            for (const auto& fn : entries)
                loader.process(fn);
        }
    }
    dsoDB->finish();
    universe->setDSOCatalog(std::move(dsoDB));


    /***** Load the solar system catalogs *****/
    // First read the solar system files listed individually in the
    // config file.
    universe->setSolarSystemCatalog(std::make_unique<SolarSystemCatalog>());
    for (const auto& file : config->paths.solarSystemFiles)
    {
        if (progressNotifier)
            progressNotifier->update(file.string());

        std::ifstream solarSysFile(file, std::ios::in);
        if (!solarSysFile.good())
        {
            GetLogger()->error(_("Error opening solar system catalog {}.\n"), file);
        }
        else
        {
            LoadSolarSystemObjects(solarSysFile, *universe);
        }
    }

    // Next, read all the solar system files in the extras directories
    {
        vector<fs::path> entries;
        SolarSystemLoader loader(universe, progressNotifier, config->paths.skipExtras);
        for (const auto& dir : config->paths.extrasDirs)
        {
            if (!is_valid_directory(dir))
                continue;

            entries.clear();
            std::error_code ec;
            auto iter = fs::recursive_directory_iterator(dir, ec);
            for (; iter != end(iter); iter.increment(ec))
            {
                if (ec)
                    continue;
                if (!fs::is_directory(iter->path(), ec))
                    entries.push_back(iter->path());
            }
            sort(begin(entries), end(entries));
            for(const auto& fn : entries)
                loader.process(fn);
        }
    }

    // Load asterisms:
    if (!config->paths.asterismsFile.empty())
        loadAsterismsFile(config->paths.asterismsFile);

    if (!config->paths.boundariesFile.empty())
    {
        std::ifstream boundariesFile(config->paths.boundariesFile, ios::in);
        if (!boundariesFile.good())
        {
            GetLogger()->error(_("Error opening constellation boundaries file {}.\n"),
                               config->paths.boundariesFile);
        }
        else
        {
            universe->setBoundaries(ReadBoundaries(boundariesFile));
        }
    }

    // Load destinations list
    if (!config->paths.destinationsFile.empty())
    {
        fs::path localeDestinationsFile = LocaleFilename(config->paths.destinationsFile);
        ifstream destfile(localeDestinationsFile, ios::in);
        if (destfile.good())
        {
            destinations = ReadDestinationList(destfile);
        }
    }

    std::shared_ptr<ProjectionMode> projectionMode = nullptr;
    if (compareIgnoringCase(config->projectionMode, "fisheye") == 0)
    {
        projectionMode = std::make_shared<FisheyeProjectionMode>(static_cast<float>(metrics.width),
                                                                 static_cast<float>(metrics.height),
                                                                 metrics.screenDpi);
    }
    else
    {
        if (!config->projectionMode.empty() && compareIgnoringCase(config->projectionMode, "perspective") != 0)
        {
            GetLogger()->warn("Unknown projection mode {}\n", config->projectionMode);
        }
        projectionMode = std::make_shared<PerspectiveProjectionMode>(static_cast<float>(metrics.width),
                                                                     static_cast<float>(metrics.height),
                                                                     distanceToScreen,
                                                                     metrics.screenDpi);
    }
    renderer->setProjectionMode(projectionMode);

    if (!config->viewportEffect.empty() && config->viewportEffect != "none")
    {
        if (config->viewportEffect == "passthrough")
            viewportEffect = unique_ptr<ViewportEffect>(new PassthroughViewportEffect);
        else if (config->viewportEffect == "warpmesh")
        {
            if (config->paths.warpMeshFile.empty())
            {
                GetLogger()->warn("No warp mesh file specified for this effect\n");
            }
            else
            {
                WarpMeshManager *manager = GetWarpMeshManager();
                WarpMesh *mesh = manager->find(manager->getHandle(WarpMeshInfo(config->paths.warpMeshFile)));
                if (mesh != nullptr)
                    viewportEffect = unique_ptr<ViewportEffect>(new WarpMeshViewportEffect(mesh));
                else
                    GetLogger()->error("Failed to read warp mesh file {}\n", config->paths.warpMeshFile);
            }
        }
        else
        {
            GetLogger()->warn("Unknown viewport effect {}\n", config->viewportEffect);
        }
    }

    if (!config->measurementSystem.empty())
    {
        if (compareIgnoringCase(config->measurementSystem, "imperial") == 0)
            hud->hudSettings().measurementSystem = MeasurementSystem::Imperial;
        else if (compareIgnoringCase(config->measurementSystem, "metric") == 0)
            hud->hudSettings().measurementSystem = MeasurementSystem::Metric;
        else
            GetLogger()->warn("Unknown measurement system {}\n", config->measurementSystem);
    }

    if (!config->temperatureScale.empty())
    {
        if (compareIgnoringCase(config->temperatureScale, "kelvin") == 0)
            hud->hudSettings().temperatureScale = TemperatureScale::Kelvin;
        else if (compareIgnoringCase(config->temperatureScale, "celsius") == 0)
            hud->hudSettings().temperatureScale = TemperatureScale::Celsius;
        else if (compareIgnoringCase(config->temperatureScale, "fahrenheit") == 0)
            hud->hudSettings().temperatureScale = TemperatureScale::Fahrenheit;
        else
            GetLogger()->warn("Unknown temperature scale {}\n", config->temperatureScale);
    }

    if (!config->scriptSystemAccessPolicy.empty())
    {
        if (compareIgnoringCase(config->scriptSystemAccessPolicy, "ask") == 0)
            scriptSystemAccessPolicy = ScriptSystemAccessPolicy::Ask;
        else if (compareIgnoringCase(config->scriptSystemAccessPolicy, "allow") == 0)
            scriptSystemAccessPolicy = ScriptSystemAccessPolicy::Allow;
        else if (compareIgnoringCase(config->scriptSystemAccessPolicy, "deny") == 0)
            scriptSystemAccessPolicy = ScriptSystemAccessPolicy::Deny;
        else
            GetLogger()->warn("Unknown script system access policy {}\n", config->scriptSystemAccessPolicy);
    }

    if (!config->layoutDirection.empty())
    {
        if (compareIgnoringCase(config->layoutDirection, "ltr") == 0)
            metrics.layoutDirection = LayoutDirection::LeftToRight;
        else if (compareIgnoringCase(config->layoutDirection, "rtl") == 0)
            metrics.layoutDirection = LayoutDirection::RightToLeft;
        else
            GetLogger()->warn("Unknown layout direction {}\n", config->layoutDirection);
    }

    sim = new Simulation(universe);
    if ((renderer->getRenderFlags() & Renderer::ShowAutoMag) == 0)
    {
        sim->setFaintestVisible(config->renderDetails.faintestVisible);
    }

    viewManager = std::make_unique<ViewManager>(new View(View::ViewWindow, sim->getActiveObserver(), 0.0f, 0.0f, 1.0f, 1.0f));

    if (!compareIgnoringCase(getConfig()->mouse.cursor, "inverting crosshair"))
    {
        defaultCursorShape = CelestiaCore::InvertedCrossCursor;
    }

    if (!compareIgnoringCase(getConfig()->mouse.cursor, "arrow"))
    {
        defaultCursorShape = CelestiaCore::ArrowCursor;
    }

    if (cursorHandler != nullptr)
    {
        cursorHandler->setCursorShape(defaultCursorShape);
    }

    return true;
}

static std::shared_ptr<TextureFont>
LoadFontHelper(const Renderer *renderer, const fs::path &p)
{
    if (p.is_absolute())
        return LoadTextureFont(renderer, p);

    int index = 0;
    int size = TextureFont::kDefaultSize;
    fs::path path = LocaleFilename(ParseFontName(fs::path("fonts") / p, index, size));

    return LoadTextureFont(renderer, path, index, size);
}

bool CelestiaCore::initRenderer([[maybe_unused]] bool useMesaPackInvert)
{
    renderer->setRenderFlags(Renderer::ShowStars |
                             Renderer::ShowPlanets |
                             Renderer::ShowAtmospheres |
                             Renderer::ShowAutoMag);

    Renderer::DetailOptions detailOptions;
    detailOptions.orbitPathSamplePoints = config->renderDetails.orbitPathSamplePoints;
    detailOptions.shadowTextureSize = config->renderDetails.shadowTextureSize;
    detailOptions.eclipseTextureSize = config->renderDetails.eclipseTextureSize;
    detailOptions.orbitWindowEnd = config->renderDetails.orbitWindowEnd;
    detailOptions.orbitPeriodsShown = config->renderDetails.orbitPeriodsShown;
    detailOptions.linearFadeFraction = config->renderDetails.linearFadeFraction;
#ifndef GL_ES
    detailOptions.useMesaPackInvert = useMesaPackInvert;
#endif

    // Prepare the scene for rendering.
    if (!renderer->init(metrics.width, metrics.height, detailOptions))
    {
        fatalError(_("Failed to initialize renderer"), false);
        return false;
    }

    if ((renderer->getRenderFlags() & Renderer::ShowAutoMag) != 0)
    {
        renderer->setFaintestAM45deg(renderer->getFaintestAM45deg());
        setFaintestAutoMag();
    }

    auto mainFont = config->fonts.mainFont.empty()
                ? LoadFontHelper(renderer, "DejaVuSans.ttf,12")
                : LoadFontHelper(renderer, config->fonts.mainFont);
    if (mainFont != nullptr)
        hud->font(mainFont);
    else
        std::cout << _("Error loading font; text will not be visible.\n");

    if (auto titleFont = config->fonts.titleFont.empty() ? nullptr : LoadFontHelper(renderer, config->fonts.titleFont);
        titleFont != nullptr)
    {
        hud->titleFont(titleFont);
    }
    else if (mainFont != nullptr)
    {
        hud->titleFont(mainFont);
    }

    // Set up the overlay
    {
    auto overlay = std::make_unique<Overlay>(*renderer);
    overlay->setTextAlignment(metrics.layoutDirection == LayoutDirection::RightToLeft
                                  ? TextLayout::HorizontalAlignment::Right
                                  : TextLayout::HorizontalAlignment::Left);
    overlay->setWindowSize(metrics.width, metrics.height);
    hud->setOverlay(std::move(overlay));
    }

    if (config->fonts.labelFont.empty())
    {
        renderer->setFont(Renderer::FontNormal, hud->font());
    }
    else
    {
        auto labelFont = LoadFontHelper(renderer, config->fonts.labelFont);
        renderer->setFont(Renderer::FontNormal, labelFont == nullptr ? hud->font() : labelFont);
    }

    renderer->setFont(Renderer::FontLarge, hud->titleFont());
    renderer->setRTL(metrics.layoutDirection == LayoutDirection::RightToLeft);
    return true;
}


static void loadCrossIndex(StarDatabaseBuilder& starDBBuilder,
                           StarCatalog catalog,
                           const fs::path& filename)
{
    if (!filename.empty())
    {
        ifstream xrefFile(filename, ios::in | ios::binary);
        if (xrefFile.good())
        {
            if (!starDBBuilder.loadCrossIndex(catalog, xrefFile))
                GetLogger()->error(_("Error reading cross index {}\n"), filename);
            else
                GetLogger()->info(_("Loaded cross index {}\n"), filename);
        }
    }
}


bool CelestiaCore::readStars(const CelestiaConfig& cfg,
                             ProgressNotifier* progressNotifier)
{
    StarDetails::SetStarTextures(cfg.starTextures);

    std::unique_ptr<StarNameDatabase> starNameDB = nullptr;
    ifstream starNamesFile(cfg.paths.starNamesFile, ios::in);
    if (starNamesFile.good())
    {
        starNameDB = StarNameDatabase::readNames(starNamesFile);
        if (starNameDB == nullptr)
            GetLogger()->error(_("Error reading star names file\n"));
    }
    else
    {
        GetLogger()->error(_("Error opening {}\n"), cfg.paths.starNamesFile);
    }

    // First load the binary star database file.  The majority of stars
    // will be defined here.
    StarDatabaseBuilder starDBBuilder;
    if (!cfg.paths.starDatabaseFile.empty())
    {
        if (progressNotifier)
            progressNotifier->update(cfg.paths.starDatabaseFile.string());

        ifstream starFile(cfg.paths.starDatabaseFile, ios::in | ios::binary);
        if (!starFile.good())
        {
            GetLogger()->error(_("Error opening {}\n"), cfg.paths.starDatabaseFile);
            return false;
        }

        if (!starDBBuilder.loadBinary(starFile))
        {
            GetLogger()->error(_("Error reading stars file\n"));
            return false;
        }
    }

    if (starNameDB == nullptr)
        starNameDB = std::make_unique<StarNameDatabase>();
    starDBBuilder.setNameDatabase(std::move(starNameDB));

    loadCrossIndex(starDBBuilder, StarCatalog::HenryDraper, cfg.paths.HDCrossIndexFile);
    loadCrossIndex(starDBBuilder, StarCatalog::SAO,         cfg.paths.SAOCrossIndexFile);
    loadCrossIndex(starDBBuilder, StarCatalog::Gliese,      cfg.paths.GlieseCrossIndexFile);

    // Next, read any ASCII star catalog files specified in the StarCatalogs
    // list.
    for (const auto& file : config->paths.starCatalogFiles)
    {
        if (file.empty())
            continue;

        ifstream starFile(file, ios::in);
        if (starFile.good())
            starDBBuilder.load(starFile);
        else
            GetLogger()->error(_("Error opening star catalog {}\n"), file);
    }

    // Now, read supplemental star files from the extras directories
    {
        vector<fs::path> entries;
        StarLoader loader(&starDBBuilder,
                          "star",
                          ContentType::CelestiaStarCatalog,
                          progressNotifier,
                          config->paths.skipExtras);
        for (const auto& dir : config->paths.extrasDirs)
        {
            if (!is_valid_directory(dir))
                continue;

            entries.clear();
            std::error_code ec;
            auto iter = fs::recursive_directory_iterator(dir, ec);
            for (; iter != end(iter); iter.increment(ec))
            {
                if (ec)
                    continue;
                if (!fs::is_directory(iter->path(), ec))
                    entries.push_back(iter->path());
            }
            std::sort(begin(entries), end(entries));
            for (const auto& fn : entries)
                loader.process(fn);
        }
    }

    universe->setStarCatalog(starDBBuilder.finish());
    return true;
}


/// Set the faintest visible star magnitude; adjust the renderer's
/// brightness parameters appropriately.
void CelestiaCore::setFaintest(float magnitude)
{
    sim->setFaintestVisible(magnitude);
}

/// Set faintest visible star magnitude and saturation magnitude
/// for a given field of view;
/// adjust the renderer's brightness parameters appropriately.
void CelestiaCore::setFaintestAutoMag()
{
    float faintestMag;
    renderer->autoMag(faintestMag, sim->getActiveObserver()->getZoom());
    sim->setFaintestVisible(faintestMag);
}

void CelestiaCore::fatalError(const string& msg, bool visual)
{
    if (alerter == nullptr)
    {
        if (visual)
            flash(msg);
        else
            GetLogger()->error(msg.c_str());
    }
    else
    {
        alerter->fatalError(msg);
    }
}

void CelestiaCore::setAlerter(Alerter* a)
{
    alerter = a;
}

CelestiaCore::Alerter* CelestiaCore::getAlerter() const
{
    return alerter;
}

/// Sets the cursor handler object.
/// This must be set before calling initSimulation
/// or the default cursor will not be used.
void CelestiaCore::setCursorHandler(CursorHandler* handler)
{
    cursorHandler = handler;
}

CelestiaCore::CursorHandler* CelestiaCore::getCursorHandler() const
{
    return cursorHandler;
}

void CelestiaCore::setContextMenuHandler(ContextMenuHandler* handler)
{
    contextMenuHandler = handler;
}

CelestiaCore::ContextMenuHandler* CelestiaCore::getContextMenuHandler() const
{
    return contextMenuHandler;
}

bool CelestiaCore::setRendererFont(const fs::path& fontPath, int collectionIndex, int fontSize, Renderer::FontStyle fontStyle)
{
    if (auto f = LoadTextureFont(renderer, fontPath, collectionIndex, fontSize); f != nullptr)
    {
        renderer->setFont(fontStyle, f);
        return true;
    }
    return false;
}

int CelestiaCore::getTimeZoneBias() const
{
    return timeInfo.timeZoneBias;
}

bool CelestiaCore::getLightDelayActive() const
{
    return timeInfo.lightTravelFlag;
}

void CelestiaCore::setLightDelayActive(bool lightDelayActive)
{
    timeInfo.lightTravelFlag = lightDelayActive;
}

void CelestiaCore::setTextEnterMode(Hud::TextEnterMode mode)
{
    if (mode != hud->textEnterMode())
    {
        hud->textEnterMode(mode);
        notifyWatchers(TextEnterModeChanged);
    }
}

Hud::TextEnterMode CelestiaCore::getTextEnterMode() const
{
    return hud->textEnterMode();
}

void CelestiaCore::setScreenDpi(int dpi)
{
    metrics.screenDpi = dpi;
    renderer->setScreenDpi(dpi);
    setFOVFromZoom();
}

int CelestiaCore::getScreenDpi() const
{
    return metrics.screenDpi;
}

void CelestiaCore::setDistanceToScreen(int dts)
{
    distanceToScreen = dts;
    renderer->getProjectionMode()->setDistanceToScreen(dts);
    setFOVFromZoom();
}

int CelestiaCore::getDistanceToScreen() const
{
    return distanceToScreen;
}

void CelestiaCore::setTimeZoneBias(int bias)
{
    timeInfo.timeZoneBias = bias;
    notifyWatchers(TimeZoneChanged);
}


string CelestiaCore::getTimeZoneName() const
{
    return timeZoneName;
}


void CelestiaCore::setTimeZoneName(const string& zone)
{
    timeZoneName = zone;
}


int CelestiaCore::getHudDetail()
{
    return hud->detail();
}

void CelestiaCore::setHudDetail(int newHudDetail)
{
    hud->detail(newHudDetail);
    notifyWatchers(VerbosityLevelChanged);
}


const Color& CelestiaCore::getTextColor() const
{
    return hud->hudSettings().textColor;
}

void CelestiaCore::setTextColor(const Color& newTextColor)
{
    hud->hudSettings().textColor = newTextColor;
}


astro::Date::Format CelestiaCore::getDateFormat() const
{
    return hud->dateFormat();
}

void CelestiaCore::setDateFormat(astro::Date::Format format)
{
    hud->dateFormat(format);
}

HudElements CelestiaCore::getOverlayElements() const
{
    return hud->hudSettings().overlayElements;
}

void CelestiaCore::setOverlayElements(HudElements _overlayElements)
{
    hud->hudSettings().overlayElements = _overlayElements;
}

void CelestiaCore::initMovieCapture(MovieCapture* mc)
{
    if (movieCapture == nullptr)
        movieCapture = mc;
}

void CelestiaCore::recordBegin()
{
    if (movieCapture != nullptr)
    {
        recording = true;
        movieCapture->recordingStatus(true);
    }
}

void CelestiaCore::recordPause()
{
    recording = false;
    if (movieCapture != nullptr) movieCapture->recordingStatus(false);
}

void CelestiaCore::recordEnd()
{
    if (movieCapture != nullptr)
    {
        recordPause();
        movieCapture->end();
        delete movieCapture;
        movieCapture = nullptr;
    }
}

bool CelestiaCore::isCaptureActive()
{
    return movieCapture != nullptr;
}

bool CelestiaCore::isRecording()
{
    return recording;
}

void CelestiaCore::flash(const string& s, double duration)
{
    if (hud->detail() > 0)
        showText(s, -1, -1, 0, 5, duration);
}


const CelestiaConfig* CelestiaCore::getConfig() const
{
    return config.get();
}


void CelestiaCore::addWatcher(CelestiaWatcher* watcher)
{
    assert(watcher != nullptr);
    watchers.push_back(watcher);
}

void CelestiaCore::removeWatcher(CelestiaWatcher* watcher)
{
    vector<CelestiaWatcher*>::iterator iter =
        find(watchers.begin(), watchers.end(), watcher);
    if (iter != watchers.end())
        watchers.erase(iter);
}

void CelestiaCore::notifyWatchers(int property)
{
    for (const auto watcher : watchers)
    {
        watcher->notifyChange(this, property);
    }
}


bool CelestiaCore::goToUrl(const string& urlStr)
{
    Url url(this);
    if (!url.parse(urlStr))
    {
        fatalError(_("Invalid URL"));
        return false;
    }
    url.goTo();
    notifyWatchers(RenderFlagsChanged | LabelFlagsChanged);
    return true;
}


void CelestiaCore::addToHistory()
{
    if (!history.empty() && historyCurrent < history.size() - 1)
    {
        // truncating history to current position
        while (historyCurrent != history.size() - 1)
            history.pop_back();
    }
    history.emplace_back(this);
    historyCurrent = history.size() - 1;
    notifyWatchers(HistoryChanged);
}


void CelestiaCore::back()
{
    if (historyCurrent == 0)
        return;

    if (historyCurrent == history.size() - 1)
    {
        addToHistory();
        historyCurrent = history.size()-1;
    }
    historyCurrent--;
    history[historyCurrent].goTo();
    notifyWatchers(HistoryChanged|RenderFlagsChanged|LabelFlagsChanged);
}


void CelestiaCore::forward()
{
    if (history.size() == 0) return;
    if (historyCurrent == history.size()-1) return;
    historyCurrent++;
    history[historyCurrent].goTo();
    notifyWatchers(HistoryChanged|RenderFlagsChanged|LabelFlagsChanged);
}


const vector<Url>& CelestiaCore::getHistory() const
{
    return history;
}

vector<Url>::size_type CelestiaCore::getHistoryCurrent() const
{
    return historyCurrent;
}

void CelestiaCore::setHistoryCurrent(vector<Url>::size_type curr)
{
    if (curr >= history.size()) return;
    if (historyCurrent == history.size()) {
        addToHistory();
    }
    historyCurrent = curr;
    history[curr].goTo();
    notifyWatchers(HistoryChanged|RenderFlagsChanged|LabelFlagsChanged);
}


/*! Toggle the specified reference mark for a selection.
 *  a selection. The default value for the selection argument is
 *  the current simulation selection. This method does nothing
 *  if the selection isn't a solar system body.
 */
void CelestiaCore::toggleReferenceMark(const string& refMark, Selection sel)
{
    Body* body = nullptr;

    if (sel.empty())
        body = getSimulation()->getSelection().body();
    else
        body = sel.body();

    // Reference marks can only be set for solar system bodies.
    if (body == nullptr)
        return;

    if (body->findReferenceMark(refMark))
    {
        body->removeReferenceMark(refMark);
    }
    else
    {
        if (refMark == "body axes")
        {
            body->addReferenceMark(std::make_unique<BodyAxisArrows>(*body));
        }
        else if (refMark == "frame axes")
        {
            body->addReferenceMark(std::make_unique<FrameAxisArrows>(*body));
        }
        else if (refMark == "sun direction")
        {
            body->addReferenceMark(std::make_unique<SunDirectionArrow>(*body));
        }
        else if (refMark == "velocity vector")
        {
            body->addReferenceMark(std::make_unique<VelocityVectorArrow>(*body));
        }
        else if (refMark == "spin vector")
        {
            body->addReferenceMark(std::make_unique<SpinVectorArrow>(*body));
        }
        else if (refMark == "frame center direction")
        {
            double now = getSimulation()->getTime();
            auto arrow = std::make_unique<BodyToBodyDirectionArrow>(*body, body->getOrbitFrame(now)->getCenter());
            arrow->setTag(refMark);
            body->addReferenceMark(std::move(arrow));
        }
        else if (refMark == "planetographic grid")
        {
            body->addReferenceMark(std::make_unique<PlanetographicGrid>(*body));
        }
        else if (refMark == "terminator")
        {
            double now = getSimulation()->getTime();
            Star* sun = nullptr;
            Body* b = body;
            while (b != nullptr)
            {
                Selection center = b->getOrbitFrame(now)->getCenter();
                if (center.star() != nullptr)
                    sun = center.star();
                b = center.body();
            }

            if (sun != nullptr)
            {
                auto visibleRegion = std::make_unique<VisibleRegion>(*body, Selection(sun));
                visibleRegion->setTag("terminator");
                body->addReferenceMark(std::move(visibleRegion));
            }
        }
    }
}


/*! Return whether the specified reference mark is enabled for a
 *  a selection. The default value for the selection argument is
 *  the current simulation selection.
 */
bool CelestiaCore::referenceMarkEnabled(const string& refMark, Selection sel) const
{
    Body* body = nullptr;

    if (sel.empty())
        body = getSimulation()->getSelection().body();
    else
        body = sel.body();

    // Reference marks can only be set for solar system bodies.
    if (body == nullptr)
        return false;

    return body->findReferenceMark(refMark) != nullptr;
}


#ifdef CELX
bool CelestiaCore::initLuaHook(ProgressNotifier* progressNotifier)
{
    return CreateLuaEnvironment(this, config.get(), progressNotifier);
}
#endif

std::string_view CelestiaCore::getTypedText() const
{
    return hud->textInput().getTypedText();
}

void CelestiaCore::setTypedText(const char *c_p)
{
    hud->textInput().appendText(sim, c_p, (renderer->getLabelMode() & Renderer::LocationLabels) != 0);
}

vector<Observer*> CelestiaCore::getObservers() const
{
    vector<Observer*> observerList;
    for (const auto view : viewManager->views())
        if (view->type == View::ViewWindow)
            observerList.push_back(view->observer);
    return observerList;
}

View* CelestiaCore::getViewByObserver(const Observer *obs) const
{
    auto end = viewManager->views().end();
    auto it = std::find_if(viewManager->views().begin(), end,
                           [obs](const View* view) { return view->observer == obs; });
    return it == end ? nullptr : *it;
}

void CelestiaCore::getCaptureInfo(std::array<int, 4>& viewport, celestia::engine::PixelFormat& format) const
{
    renderer->getViewport(viewport);
    format = renderer->getPreferredCaptureFormat();
}

bool CelestiaCore::captureImage(std::uint8_t* buffer,
                                const std::array<int, 4>& viewport,
                                celestia::engine::PixelFormat format) const
{
    if (renderer->captureFrame(viewport[0], viewport[1],
                               viewport[2], viewport[3],
                               format, buffer))
    {
        return true;
    }

    GetLogger()->error(_("Unable to capture a frame!\n"));
    return false;
}

bool CelestiaCore::saveScreenShot(const fs::path& filename, ContentType type) const
{
    if (type == ContentType::Unknown)
        type = DetermineFileType(filename);

    if (!Image::canSave(type))
    {
        GetLogger()->error(_("Unsupported image type: {}!\n"), filename);
        return false;
    }

    std::array<int, 4> viewport;
    PixelFormat format;
    getCaptureInfo(viewport, format);
    Image image(format, viewport[2], viewport[3]);
    if (!captureImage(image.getPixels(), viewport, format))
        return false;

    return image.save(filename, type);
}

#ifdef USE_MINIAUDIO
std::shared_ptr<celestia::AudioSession> CelestiaCore::getAudioSession(int channel) const
{
    auto it = audioSessions.find(channel);
    return it == audioSessions.end() ? nullptr : it->second;
}

bool CelestiaCore::isPlayingAudio(int channel) const
{
    auto audioSession = getAudioSession(channel);
    return audioSession && audioSession->isPlaying();
}

bool CelestiaCore::playAudio(int channel, const fs::path &path, double startTime, float volume, float pan, bool loop, bool nopause)
{
    stopAudio(channel);
    auto audioSession = make_shared<MiniAudioSession>(path, volume, pan, loop, nopause);
    audioSessions[channel] = audioSession;
    return audioSession->play(startTime);
}

bool CelestiaCore::resumeAudio(int channel)
{
    auto audioSession = getAudioSession(channel);
    return audioSession ? audioSession->play() : false;
}

void CelestiaCore::pauseAudio(int channel)
{
    auto audioSession = getAudioSession(channel);
    if (audioSession)
        audioSession->stop();
}

void CelestiaCore::stopAudio(int channel)
{
    auto audioSession = getAudioSession(channel);
    if (audioSession)
    {
        audioSession->stop();
        audioSessions.erase(channel);
    }
}

bool CelestiaCore::seekAudio(int channel, double seconds)
{
    auto audioSession = getAudioSession(channel);
    return audioSession ? audioSession->seek(seconds) : false;
}

void CelestiaCore::setAudioVolume(int channel, float volume)
{
    auto audioSession = getAudioSession(channel);
    if (audioSession)
        audioSession->setVolume(volume);
}

void CelestiaCore::setAudioPan(int channel, float pan)
{
    auto audioSession = getAudioSession(channel);
    if (audioSession)
        audioSession->setPan(pan);
}

void CelestiaCore::setAudioLoop(int channel, bool loop)
{
    auto audioSession = getAudioSession(channel);
    if (audioSession)
        audioSession->setLoop(loop);
}

void CelestiaCore::setAudioNoPause(int channel, bool nopause)
{
    auto audioSession = getAudioSession(channel);
    if (audioSession)
        audioSession->setNoPause(nopause);
}

void CelestiaCore::pauseAudioIfNeeded()
{
    for (auto const &[_, value] : audioSessions)
        if (!value->nopause())
            value->stop();
}

void CelestiaCore::resumeAudioIfNeeded()
{
    for (auto const &[_, value] : audioSessions)
        if (!value->nopause())
            value->play();
}
#endif

void CelestiaCore::setMeasurementSystem(MeasurementSystem newMeasurement)
{
    if (hud->hudSettings().measurementSystem != newMeasurement)
    {
        hud->hudSettings().measurementSystem = newMeasurement;
        notifyWatchers(MeasurementSystemChanged);
    }
}

MeasurementSystem CelestiaCore::getMeasurementSystem() const
{
    return hud->hudSettings().measurementSystem;
}

void CelestiaCore::setTemperatureScale(TemperatureScale newScale)
{
    if (hud->hudSettings().temperatureScale != newScale)
    {
        hud->hudSettings().temperatureScale = newScale;
        notifyWatchers(TemperatureScaleChanged);
    }
}

TemperatureScale CelestiaCore::getTemperatureScale() const
{
    return hud->hudSettings().temperatureScale;
}

void CelestiaCore::setScriptSystemAccessPolicy(ScriptSystemAccessPolicy newPolicy)
{
    scriptSystemAccessPolicy = newPolicy;
}

CelestiaCore::ScriptSystemAccessPolicy CelestiaCore::getScriptSystemAccessPolicy() const
{
    return scriptSystemAccessPolicy;
}

celestia::LayoutDirection CelestiaCore::getLayoutDirection() const
{
    return metrics.layoutDirection;
}

void CelestiaCore::setLayoutDirection(celestia::LayoutDirection value)
{
    metrics.layoutDirection = value;
    hud->setTextAlignment(metrics.layoutDirection);
    renderer->setRTL(metrics.layoutDirection == LayoutDirection::RightToLeft);
}

void CelestiaCore::setLogFile(const fs::path &fn)
{
    m_logfile = std::ofstream(fn);
    if (m_logfile.good())
    {
        m_tee = teestream(m_logfile, *console);
        clog.rdbuf(m_tee.rdbuf());
        cerr.rdbuf(m_tee.rdbuf());
    }
    else
    {
        GetLogger()->error("Unable to open log file {}\n", fn);
    }
}

void CelestiaCore::loadAsterismsFile(const fs::path &path)
{
    if (ifstream asterismsFile(path, ios::in); !asterismsFile.good())
    {
        GetLogger()->error(_("Error opening asterisms file {}.\n"), path);
    }
    else
    {
        std::unique_ptr<AsterismList> asterisms = ReadAsterismList(asterismsFile, *universe->getStarCatalog());
        universe->setAsterisms(std::move(asterisms));
    }
}
