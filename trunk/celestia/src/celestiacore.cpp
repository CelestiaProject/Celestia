// celestiacore.cpp
// 
// Platform-independent UI handling and initialization for Celestia.
// winmain, gtkmain, and glutmain are thin, platform-specific modules
// that sit directly on top of CelestiaCore and feed it mouse and
// keyboard events.  CelestiaCore then turns those events into calls
// to Renderer and Simulation.
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <iostream>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <cstdlib>
#include <cctype>
#include <cstring>
#include "gl.h"
#include "celestia.h"
#include "vecmath.h"
#include "quaternion.h"
#include "util.h"
#include "timer.h"
#include "stardb.h"
#include "solarsys.h"
#include "asterism.h"
#include "mathlib.h"
#include "astro.h"
#include "overlay.h"
#include "favorites.h"
#include "celestiacore.h"
#include "execution.h"
#include "cmdparser.h"

using namespace std;

static const int DragThreshold = 3;

//Perhaps you'll want to put this stuff in configuration file.
static const float fIncrementFactor = 10.0f;
static const double fMinSlewRate = 3.0;
static const double fMaxKeyAccel = 20.0;
static const float fAltitudeThreshold = 4.0f;

static void warning(string s)
{
    cout << s;
}

// Extremely basic implementation of an ExecutionEnvironment for
// running scripts.
class CoreExecutionEnvironment : public ExecutionEnvironment
{
private:
    CelestiaCore& core;

public:
    CoreExecutionEnvironment(CelestiaCore& _core) : core(_core)
    {
    }

    Simulation* getSimulation() const
    {
        return core.getSimulation();
    }

    Renderer* getRenderer() const
    {
        return core.getRenderer();
    }

    void showText(string s)
    {
        core.showText(s);
    }
};


CelestiaCore::CelestiaCore() :
    config(NULL),
    starDB(NULL),
    solarSystemCatalog(NULL),
    galaxies(NULL),
    asterisms(NULL),
    favorites(NULL),
    destinations(NULL),
    sim(NULL),
    renderer(NULL),
    overlay(NULL),
    width(1),
    height(1),
    font(NULL),
    titleFont(NULL),
    messageText(""),
    typedText(""),
    textEnterMode(false),
    hudDetail(1),
    wireframe(false),
    editMode(false),
    currentScript(NULL),
    initScript(NULL),
    demoScript(NULL),
    runningScript(NULL),
    execEnv(NULL),
    timeZoneBias(0),
    showFPSCounter(false),
    nFrames(0),
    fps(0.0),
    fpsCounterStartTime(0.0),
    mouseMotion(0.0f),
    mouseWheelMotion(0.0),
    mouseWheelTime(0.0),
    currentTime(0.0),
    timeScale(1.0),
    paused(false),
    contextMenuCallback(NULL),
    logoTexture(NULL),
    alerter(NULL),
    KeyAccel(1.0)
{
    execEnv = new CoreExecutionEnvironment(*this);
    for (int i = 0; i < KeyCount; i++)
        keysPressed[i] = false;
}

CelestiaCore::~CelestiaCore()
{
    delete execEnv;
}


void CelestiaCore::writeFavoritesFile()
{
    if (config->favoritesFile != "")
    {
        ofstream out(config->favoritesFile.c_str(), ios::out);
        if (out.good())
        WriteFavoritesList(*favorites, out);
    }
}

void CelestiaCore::activateFavorite(FavoritesEntry& fav)
{
    sim->cancelMotion();
    sim->setTime(fav.jd);
    sim->getObserver().setPosition(fav.position);
    sim->getObserver().setOrientation(fav.orientation);
}

void CelestiaCore::addFavorite(string name)
{
    FavoritesEntry* fav = new FavoritesEntry();
    fav->jd = sim->getTime();
    fav->position = sim->getObserver().getPosition();
    fav->orientation = sim->getObserver().getOrientation();
    fav->name = name;
    favorites->insert(favorites->end(), fav);
    writeFavoritesFile();
}

const FavoritesList* CelestiaCore::getFavorites()
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
    if (sel.galaxy != NULL)
    {
        cout << sel.galaxy->getName() << '\n';
        Vec3f axis;
        float angle;
        sel.galaxy->getOrientation().getAxisAngle(axis, angle);
        cout << "Orientation: " << '[' << axis.x << ',' << axis.y << ',' << axis.z << "], " << radToDeg(angle) << '\n';
    }
}


void CelestiaCore::cancelScript()
{
    if (runningScript != NULL)
    {
        delete runningScript;
        runningScript = NULL;
    }
    messageText = "";
}


bool checkMask(int modifiers, int mask)
{
    return (modifiers & mask) == mask;
}

void CelestiaCore::mouseButtonDown(float x, float y, int button)
{
    mouseMotion = 0.0f;
}

void CelestiaCore::mouseButtonUp(float x, float y, int button)
{
    // If the mouse hasn't moved much since it was pressed, treat this
    // as a selection or context menu event.  Otherwise, assume that the
    // mouse was dragged and ignore the event.
    if (mouseMotion < DragThreshold)
    {
        if (button == LeftButton)
        {
            Vec3f pickRay = renderer->getPickRay((int) x, (int) y);
            Selection oldSel = sim->getSelection();
            Selection newSel = sim->pickObject(pickRay);
            sim->setSelection(newSel);
            if (!oldSel.empty() && oldSel == newSel)
                sim->centerSelection();
        }
        else if (button == RightButton)
        {
            Vec3f pickRay = renderer->getPickRay((int) x, (int) y);
            Selection sel = sim->pickObject(pickRay);
            if (!sel.empty())
            {
                if (contextMenuCallback != NULL)
                    contextMenuCallback(x, y, sel);
            }
        }
    }
}

void CelestiaCore::mouseWheel(float motion)
{
    if (motion != 0.0)
    {
        mouseWheelTime = currentTime;
        mouseWheelMotion = 0.25f * motion;
    }
}

void CelestiaCore::mouseMove(float dx, float dy, int modifiers)
{
    if ((modifiers & (LeftButton | RightButton)) != 0)
    {
        if (editMode && checkMask(modifiers, LeftButton | ShiftKey | ControlKey))
        {
            // Rotate the selected object
            Selection sel = sim->getSelection();
            Quatf q(1);
            if (sel.galaxy != NULL)
                q = sel.galaxy->getOrientation();

            q.yrotate(dx / width);
            q.xrotate(dy / height);

            if (sel.galaxy != NULL)
                sel.galaxy->setOrientation(q);
        }
        else if (checkMask(modifiers, LeftButton | RightButton) ||
                 checkMask(modifiers, LeftButton | ControlKey))
        {
            float amount = dy / height;
            sim->changeOrbitDistance(amount * 5);
        }
        else if (checkMask(modifiers, LeftButton | ShiftKey))
        {
            // Mouse zoom control
            float amount = dy / height;
            float minFOV = 0.01f;
            float maxFOV = 120.0f;
            float fov = renderer->getFieldOfView();

            if (fov < minFOV)
                fov = minFOV;

            // In order for the zoom to have the right feel, it should be
            // exponential.
            float newFOV = minFOV + (float) exp(log(fov - minFOV) + amount * 4);
            if (newFOV < minFOV)
                newFOV = minFOV;
            else if (newFOV > maxFOV)
                newFOV = maxFOV;
            renderer->setFieldOfView(newFOV);
        }
        else
        {
            Quatf q(1);
            // For a small field of view, rotate the camera more finely
            float coarseness = renderer->getFieldOfView() / 30.0f;
            q.yrotate(dx / width * coarseness);
            q.xrotate(dy / height * coarseness);
            if ((modifiers & RightButton) != 0)
                sim->orbit(q);
            else
                sim->rotate(~q);
        }

        mouseMotion += abs(dy) + abs(dx);
    }
}


void CelestiaCore::keyDown(int key)
{
    switch (key)
    {
    case Key_F1:
        sim->setTargetSpeed(0);
        break;
    case Key_F2:
        sim->setTargetSpeed(astro::kilometersToLightYears(1.0));
        break;
    case Key_F3:
        sim->setTargetSpeed(astro::kilometersToLightYears(1000.0));
        break;
    case Key_F4:
        sim->setTargetSpeed(astro::kilometersToLightYears(astro::speedOfLight));
        break;
    case Key_F5:
        sim->setTargetSpeed(astro::kilometersToLightYears(1000000.0));
        break;
    case Key_F6:
        sim->setTargetSpeed(astro::AUtoLightYears(1));
        break;
    case Key_F7:
        sim->setTargetSpeed(1);
        break;
    case Key_NumPad2:
    case Key_NumPad4:
    case Key_NumPad6:
    case Key_NumPad7:
    case Key_NumPad8:
    case Key_NumPad9:
        sim->setTargetSpeed(sim->getTargetSpeed());
        break;
    }

    if(KeyAccel < fMaxKeyAccel)
        KeyAccel *= 1.1;

    // Only process alphanumeric keys if we're not in text enter mode
    if (!(key >= 'A' && key <= 'Z' && textEnterMode))
        keysPressed[key] = true;
}

void CelestiaCore::keyUp(int key)
{
    KeyAccel = 1.0;
    keysPressed[key] = false;
}

void CelestiaCore::charEntered(char c)
{
    if (textEnterMode)
    {
        if (c == ' ' || isalpha(c) || isdigit(c) || ispunct(c))
        {
            typedText += c;
        }
        else if (c == '\b')
        {
            if (typedText.size() > 0)
                typedText = string(typedText, 0, typedText.size() - 1);
        }
        else if (c == '\n' || c == '\r')
        {
            if (typedText != "")
            {
                Selection sel = sim->findObjectFromPath(typedText);
                if (!sel.empty())
                    sim->setSelection(sel);
                typedText = "";
            }
            textEnterMode = false;
        }
        return;
    }

    c = toupper(c);
    switch (c)
    {
    case 'S':
        sim->setTargetSpeed(0);
        break;

    case 'Q':
        sim->setTargetSpeed(-sim->getTargetSpeed());
        break;

    case 'X':
        sim->setTargetSpeed(sim->getTargetSpeed());
        break;

    case 'G':
        sim->gotoSelection(5.0, Vec3f(0, 1, 0), astro::ObserverLocal);
        break;

    case 'C':
        sim->centerSelection();
        break;

    case 'F':
        sim->follow();
        break;

    case 'Y':
        sim->geosynchronousFollow();
        break;

    case 'T':
        sim->track();
        break;

    case 'H':
        sim->selectStar(0);
        break;

    case 'V':
        hudDetail = (hudDetail + 1) % 3;
        break;

    case ',':
        if (renderer->getFieldOfView() > 1.0f)
            renderer->setFieldOfView(renderer->getFieldOfView() / 1.1f);
        break;

    case '.':
        if (renderer->getFieldOfView() < 120.0f)
            renderer->setFieldOfView(renderer->getFieldOfView() * 1.1f);
        break;

    case 'K':
        sim->setTimeScale(sim->getTimeScale() / fIncrementFactor);
        break;

    case 'L':
        sim->setTimeScale(sim->getTimeScale() * fIncrementFactor);
        break;

    case 'J':
        sim->setTimeScale(-sim->getTimeScale());
        break;

    case '\\':
        sim->setTimeScale(1.0);
        break;

    case 'B':
        renderer->setLabelMode(renderer->getLabelMode() ^ Renderer::StarLabels);
        break;
        
    case 'N':
        renderer->setLabelMode(renderer->getLabelMode() ^ Renderer::MajorPlanetLabels);
        break;

    case 'M':
        renderer->setLabelMode(renderer->getLabelMode() ^ Renderer::MinorPlanetLabels);
        break;

    case 'O':
        renderer->setRenderFlags(renderer->getRenderFlags() ^ Renderer::ShowOrbits);
        break;

    case '\020':  // Ctrl+P
        if (renderer->fragmentShaderSupported())
            renderer->setFragmentShaderEnabled(!renderer->getFragmentShaderEnabled());
        break;

    case '\026':  // Ctrl+V
        if (renderer->vertexShaderSupported())
            renderer->setVertexShaderEnabled(!renderer->getVertexShaderEnabled());
        break;

    case 'I':
        renderer->setRenderFlags(renderer->getRenderFlags() ^ Renderer::ShowCloudMaps);
        break;

    case 'U':
        renderer->setRenderFlags(renderer->getRenderFlags() ^ Renderer::ShowGalaxies);
        break;

    case '/':
        renderer->setRenderFlags(renderer->getRenderFlags() ^ Renderer::ShowDiagrams);
        break;

    case ';':
        renderer->setRenderFlags(renderer->getRenderFlags() ^ Renderer::ShowCelestialSphere);
        break;

    case '=':
        renderer->setLabelMode(renderer->getLabelMode() ^ Renderer::ConstellationLabels);
        break;

    case '~':
        editMode = !editMode;
        break;

    case '!':
        if (editMode)
            showSelectionInfo(sim->getSelection());
        break;

    case '`':
        showFPSCounter = !showFPSCounter;
        break;

    case 'D':
        if (runningScript == NULL && demoScript != NULL)
            runningScript = new Execution(*demoScript, *execEnv);
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
        sim->selectPlanet(c - '1');
        break;

    case '0':
        sim->selectPlanet(-1);
        break;

    case 'W':
        wireframe = !wireframe;
        renderer->setRenderMode(wireframe ? GL_LINE : GL_FILL);
        break;

    case '[':
        if (sim->getFaintestVisible() > 1.0f)
            setFaintest(sim->getFaintestVisible() - 0.5f);
        break;

    case ']':
        if (sim->getFaintestVisible() < 12.0f)
            setFaintest(sim->getFaintestVisible() + 0.5f);
        break;

    case '{':
        if (renderer->getAmbientLightLevel() > 0.05f)
            renderer->setAmbientLightLevel(renderer->getAmbientLightLevel() - 0.05f);
        else
            renderer->setAmbientLightLevel(0.0f);
        break;

    case '}':
        if (renderer->getAmbientLightLevel() < 0.95f)
            renderer->setAmbientLightLevel(renderer->getAmbientLightLevel() + 0.05f);
        else
            renderer->setAmbientLightLevel(1.0f);
        break;

    case '*':
        sim->getObserver().setOrientation(Quatf(1));
        break;

    case '\n':
    case '\r':
        textEnterMode = true;
        break;

    case '\033':
        cancelScript();
        if (textEnterMode == true)
            textEnterMode = false;
        else
            sim->setFrame(astro::Universal, Selection());
        break;

    case ' ':
        if (paused)
        {
            sim->setTimeScale(timeScale);
            // CheckMenuItem(menuBar, ID_TIME_FREEZE, MF_UNCHECKED);
        }
        else
        {
            timeScale = sim->getTimeScale();
            sim->setTimeScale(0.0);
            // CheckMenuItem(menuBar, ID_TIME_FREEZE, MF_CHECKED);
        }
        paused = !paused;
        break;
    }
}


void CelestiaCore::start(double t)
{
    if (initScript != NULL)
        runningScript = new Execution(*initScript, *execEnv);

    // Set the simulation starting time to the current system time
    sim->setTime(t);
    sim->update(0.0);
}


void CelestiaCore::tick(double dt)
{
    double lastTime = currentTime;
    currentTime += dt;

    // Frame rate counter
    nFrames++;
    if (nFrames == 100)
    {
        fps = (double) nFrames / (currentTime - fpsCounterStartTime);
        nFrames = 0;
        fpsCounterStartTime = currentTime;
    }

    // Mouse wheel zoom
    if (mouseWheelMotion != 0.0f)
    {
        double mouseWheelSpan = 0.1;
        double fraction;
                
        if (currentTime - mouseWheelTime >= mouseWheelSpan)
            fraction = (mouseWheelTime + mouseWheelSpan) - lastTime;
        else
            fraction = dt / mouseWheelSpan;

        sim->changeOrbitDistance(mouseWheelMotion * (float) fraction);
        if (currentTime - mouseWheelTime >= mouseWheelSpan)
            mouseWheelMotion = 0.0f;
    }

    // Keyboard zoom
    if (keysPressed[Key_Home])
        sim->changeOrbitDistance(-dt * 2);
    if (keysPressed[Key_End])
        sim->changeOrbitDistance(dt * 2);

    // Keyboard rotate
    Quatf q(1);
    if (keysPressed[Key_Left])
        q.zrotate((float) dt * 2);
    if (keysPressed[Key_Right])
        q.zrotate((float) dt * -2);
    if (keysPressed[Key_Down])
        q.xrotate((float) dt * 2);
    if (keysPressed[Key_Up])
        q.xrotate((float) dt * -2);

    if(keysPressed[Key_NumPad4])
      q.yrotate((float)degToRad(dt * -fMinSlewRate * KeyAccel));
    if(keysPressed[Key_NumPad6])
      q.yrotate((float)degToRad(dt * fMinSlewRate * KeyAccel));
    if(keysPressed[Key_NumPad2])
      q.xrotate((float)degToRad(dt * -fMinSlewRate * KeyAccel));
    if(keysPressed[Key_NumPad8])
      q.xrotate((float)degToRad(dt * fMinSlewRate * KeyAccel));
    if(keysPressed[Key_NumPad7])
      q.zrotate((float)degToRad(dt * -fMinSlewRate * KeyAccel));
    if(keysPressed[Key_NumPad9])
      q.zrotate((float)degToRad(dt * fMinSlewRate * KeyAccel));
    sim->rotate(q);

    if (keysPressed['A'])
    {
        if (sim->getTargetSpeed() == 0.0f)
            sim->setTargetSpeed(astro::kilometersToLightYears(0.1f));
        else
            sim->setTargetSpeed(sim->getTargetSpeed() * (float) exp(dt * 3));
    }
    if (keysPressed['Z'])
    {
        sim->setTargetSpeed(sim->getTargetSpeed() / (float) exp(dt * 3));
    }

    // If there's a script running, tick it
    if (runningScript != NULL)
    {
        bool finished = runningScript->tick(dt);
        if (finished)
            cancelScript();
    }

    sim->update(dt);
}


void CelestiaCore::draw()
{
    sim->render(*renderer);
    renderOverlay();
#if 0
    GLenum err = glGetError();
    if (err != GL_NO_ERROR)
    {
        cout << "GL error: " << gluErrorString(err) << '\n';
    }
#endif
}


void CelestiaCore::resize(GLsizei w, GLsizei h)
{
    if (h == 0)
	h = 1;

    glViewport(0, 0, w, h);
    if (renderer != NULL)
        renderer->resize(w, h);
    if (overlay != NULL)
        overlay->setWindowSize(w, h);
    width = w;
    height = h;
}


void CelestiaCore::setContextMenuCallback(ContextMenuFunc callback)
{
    contextMenuCallback = callback;
}


Renderer* CelestiaCore::getRenderer() const
{
    return renderer;
}

Simulation* CelestiaCore::getSimulation() const
{
    return sim;
}

void CelestiaCore::showText(string s)
{
    messageText = s;
}

static void displayDistance(Overlay& overlay, double distance)
{
    if (distance >= astro::AUtoLightYears(1000.0f))
        overlay.printf("%.3f ly", distance);
    else if (distance >= astro::kilometersToLightYears(10000000.0))
        overlay.printf("%.3f au", astro::lightYearsToAU(distance));
    else if (distance > astro::kilometersToLightYears(1.0f))
        overlay.printf("%.3f km", astro::lightYearsToKilometers(distance));
    else
        overlay.printf("%.3f m", astro::lightYearsToKilometers(distance) * 1000.0f);
}


static void displayStarNames(Overlay& overlay,
                             Star& star,
                             StarDatabase& starDB)
{
    StarNameDatabase::NumberIndex::const_iterator iter =
        starDB.getStarNames(star.getCatalogNumber());

    bool first = true;
    while (iter != starDB.finalName() &&
           iter->first == star.getCatalogNumber())
    {
        if (!first)
            overlay << " / ";
        else
            first = false;
        overlay << iter->second;
        iter++;
    }

    uint32 hd = star.getCatalogNumber(Star::HDCatalog);
    uint32 hip = star.getCatalogNumber(Star::HIPCatalog);
    if (hd != Star::InvalidCatalogNumber && hd != 0)
    {
        if (!first)
            overlay << " / ";
        else
            first = false;
        overlay << "HD " << hd;
    }
    if (hip != Star::InvalidCatalogNumber && hip != 0)
    {
        if (!first)
            overlay << " / ";
        else
            first = false;
        overlay << "HIP " << hip;
    }
}


static void displayStarInfo(Overlay& overlay,
                            int detail,
                            Star& star,
                            SolarSystemCatalog& solarSystems,
                            double distance)
{
    overlay << "Distance: ";
    displayDistance(overlay, distance);
    overlay << '\n';
    
    overlay.printf("Abs (app) mag = %.2f (%.2f)\n",
                   star.getAbsoluteMagnitude(),
                   astro::absToAppMag(star.getAbsoluteMagnitude(), (float) distance));
    overlay << "Class: " << star.getStellarClass() << '\n';
    if (detail > 1)
    {
        overlay << "Surface Temp: " << star.getTemperature() << " K\n";
        overlay.printf("Radius: %.2f Rsun\n", star.getRadius() / 696000.0f);

        SolarSystemCatalog::iterator iter = solarSystems.find(star.getCatalogNumber());
        if (iter != solarSystems.end())
            overlay << "Planetary companions present\n";
    }
}

static void displayPlanetInfo(Overlay& overlay,
                              int detail,
                              Body& body,
                              double t,
                              double distance)
{
    // If within fAltitudeThreshold radii of planet, show altitude instead of distance
    double kmDistance = astro::lightYearsToKilometers(distance);
    if((kmDistance < (fAltitudeThreshold * body.getRadius())) &&
       (kmDistance > body.getRadius()))
    {
        kmDistance -= body.getRadius();
        overlay << "Altitude: ";
        distance = astro::kilometersToLightYears(kmDistance);
        displayDistance(overlay, distance);
        overlay << '\n';
    }
    else
    {
        overlay << "Distance: ";
        displayDistance(overlay, distance);
        overlay << '\n';
    }

    overlay << "Radius: ";
    distance = astro::kilometersToLightYears(body.getRadius());
    displayDistance(overlay, distance);
    overlay << '\n';

    if (detail > 1)
    {
        overlay << "Day length: " << body.getRotationPeriod() * 24.0 << " hours\n";
        
        PlanetarySystem* system = body.getSystem();
        if (system != NULL)
        {
            const Star* sun = system->getStar();
            if (sun != NULL)
            {
                double distFromSun = body.getHeliocentricPosition(t).distanceFromOrigin();
                float planetTemp = sun->getTemperature() * 
                    (float) (pow(1 - body.getAlbedo(), 0.25) *
                             sqrt(sun->getRadius() / (2 * distFromSun)));
                overlay << setprecision(0);
                overlay << "Temperature: " << planetTemp << " K\n";
                overlay << setprecision(3);
            }
        }
    }
}

static void displayGalaxyInfo(Overlay& overlay,
                              int detail,
                              Galaxy& galaxy,
                              double distance)
{
    overlay << "Distance: ";
    displayDistance(overlay, distance);
    overlay << '\n';
    overlay << "Type: " << galaxy.getType() << '\n';
    overlay << "Radius: " << galaxy.getRadius() << " ly\n";
}

void CelestiaCore::renderOverlay()
{
    if (font == NULL)
        return;

    overlay->setFont(font);

    int fontHeight = font->getHeight();
    int emWidth = font->getWidth("M");

    overlay->begin();

    // Time and date
    if (hudDetail > 0)
    {
        glPushMatrix();
        glColor4f(0.7f, 0.7f, 1.0f, 1.0f);
        glTranslatef(width - 11 * emWidth, height - fontHeight, 0);
        overlay->beginText();
        *overlay << astro::Date(sim->getTime() + 
                                astro::secondsToJulianDate(timeZoneBias)) << '\n';
        if (paused)
        {
            glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
            *overlay << "Paused";
        }
        else
        {
            double timeScale = sim->getTimeScale();
            if (abs(timeScale - 1) < 1e-6)
                *overlay << "Real time";
            else if (abs(timeScale) == 0.0f)
                *overlay << "Time stopped";
            else if (abs(timeScale) > 1.0)
                *overlay << timeScale << "x faster";
            else
                *overlay << 1.0 / timeScale << "x slower";
        }
        overlay->endText();
        glPopMatrix();
    }

    // Speed
    if (hudDetail > 0)
    {
        glPushMatrix();
        glTranslatef(0, fontHeight * 2 + 5, 0);
        glColor4f(0.7f, 0.7f, 1.0f, 1.0f);

        overlay->beginText();
        *overlay << '\n';
        if (showFPSCounter)
            *overlay << "FPS: " << fps;
        overlay->setf(ios::fixed);
        *overlay << "\nSpeed: " << setprecision(3);

        double speed = sim->getObserver().getVelocity().length();
        if (speed < astro::kilometersToLightYears(1.0f))
            *overlay << astro::lightYearsToKilometers(speed) * 1000.0f << " m/s";
        else if (speed < astro::kilometersToLightYears(10000.0f))
            *overlay << astro::lightYearsToKilometers(speed) << " km/s";
        else if (speed < astro::kilometersToLightYears((float) astro::speedOfLight * 100.0f))
            *overlay << astro::lightYearsToKilometers(speed) / astro::speedOfLight << 'c';
        else if (speed < astro::AUtoLightYears(1000.0f))
            *overlay << astro::lightYearsToAU(speed) << " AU/s";
        else
            *overlay << speed << " ly/s";
        *overlay << setprecision(3);

        overlay->endText();
        glPopMatrix();
    }

    // Field of view and camera mode
    if (hudDetail > 0)
    {
        float fov = renderer->getFieldOfView();

        Simulation::ObserverMode mode = sim->getObserverMode();
        char* modeName = "";
        if (mode == Simulation::Travelling)
        {
            modeName = "Travelling";
        }
        else
        {
            switch (sim->getFrame().coordSys)
            {
            case astro::Ecliptical:
                modeName = "Following";
                break;
            case astro::Geographic:
                modeName = "Sync Orbiting";
                break;
            }
        }

        glPushMatrix();
        glTranslatef(width - emWidth * 11, fontHeight + 5, 0);
        overlay->beginText();
        glColor4f(0.6f, 0.6f, 1.0f, 1);
        *overlay << modeName << '\n';
        glColor4f(0.7f, 0.7f, 1.0f, 1.0f);
        overlay->printf("FOV: %6.2f\n", fov);
        overlay->endText();
        glPopMatrix();
    }

    // Selection info
    Selection sel = sim->getSelection();
    if (!sel.empty() && hudDetail > 0)
    {
        glPushMatrix();
        glColor4f(0.7f, 0.7f, 1.0f, 1.0f);
        glTranslatef(0, height - titleFont->getHeight(), 0);

        overlay->beginText();
        Vec3d v = sim->getSelectionPosition(sel, sim->getTime()) - 
            sim->getObserver().getPosition();
        if (sel.star != NULL)
        {
            overlay->setFont(titleFont);
            displayStarNames(*overlay,
                             *sel.star,
                             *(sim->getStarDatabase()));
            overlay->setFont(font);
            *overlay << '\n';
            displayStarInfo(*overlay,
                            hudDetail,
                            *sel.star,
                            *(sim->getSolarSystemCatalog()),
                            v.length());
        }
        else if (sel.body != NULL)
        {
            overlay->setFont(titleFont);
            *overlay << sel.body->getName();
            overlay->setFont(font);
            *overlay << '\n';
            displayPlanetInfo(*overlay,
                              hudDetail,
                              *sel.body,
                              sim->getTime(),
                              v.length());
        }
        else if (sel.galaxy != NULL)
        {
            overlay->setFont(titleFont);
            *overlay << sel.galaxy->getName();
            overlay->setFont(font);
            *overlay << '\n';
            displayGalaxyInfo(*overlay, hudDetail, *sel.galaxy, v.length());
        }
        overlay->endText();

        glPopMatrix();
    }

    // Text input
    if (textEnterMode)
    {
        overlay->setFont(titleFont);
        glPushMatrix();
        glColor4f(0.7f, 0.7f, 1.0f, 0.2f);
        overlay->rect(0, 0, width, 70);
        glTranslatef(0, fontHeight * 3 + 5, 0);
        glColor4f(0.6f, 0.6f, 1.0f, 1);
        *overlay << "Target name: " << typedText;
        glPopMatrix();
        overlay->setFont(font);
    }

    // Text messages
    if (messageText != "")
    {
        glPushMatrix();
        glColor4f(1, 1, 1, 1);
        glTranslatef(0, fontHeight * 5 + 5, 0);
        overlay->beginText();
        *overlay << messageText;
        overlay->endText();
        glPopMatrix();
    }

    if (editMode)
    {
        glPushMatrix();
        glTranslatef((width - font->getWidth("Edit Mode")) / 2, height - fontHeight, 0);
        glColor4f(1, 0, 1, 1);
        *overlay << "Edit Mode";
        glPopMatrix();
    }

    // Show logo at start
    if (logoTexture != NULL)
    {
        glEnable(GL_TEXTURE_2D);
        if (currentTime < 5.0)
        {
            int xSize = (int) (logoTexture->getWidth() * 0.8f);
            int ySize = (int) (logoTexture->getHeight() * 0.8f);
            int left = (width - xSize) / 2;
            int bottom = height / 2;

            float topAlpha, botAlpha;
            if (currentTime < 4.0)
            {
                botAlpha = (float) clamp(currentTime / 1.0);
                topAlpha = (float) clamp(currentTime / 4.0);
            }
            else
            {
                botAlpha = topAlpha = (float) (5.0 - currentTime);
            }

            glBindTexture(GL_TEXTURE_2D, logoTexture->getName());
            glBegin(GL_QUADS);
            glColor4f(0.8f, 0.8f, 1.0f, botAlpha);
            glTexCoord2f(0, 1);
            glVertex2f(left, bottom);
            glTexCoord2f(1, 1);
            glVertex2f(left + xSize, bottom);
            glColor4f(0.6f, 0.6f, 1.0f, topAlpha);
            glTexCoord2f(1, 0);
            glVertex2f(left + xSize, bottom + ySize);
            glTexCoord2f(0, 0);
            glVertex2f(left, bottom + ySize);
            glEnd();
        }
        else
        {
            delete logoTexture;
            logoTexture = NULL;
        }
    }

    overlay->end();
}


bool CelestiaCore::initSimulation()
{
    // Say we're not ready to render yet.
    // bReady = false;
#ifdef REQUIRE_LICENSE_FILE
    // Check for the presence of the license file--don't run unless it's there.
    {
        ifstream license("License.txt");
        if (!license.good())
        {
            fatalError("License file 'License.txt' is missing!");
            return false;
        }
    }
#endif

    config = ReadCelestiaConfig("celestia.cfg");
    if (config == NULL)
    {
        fatalError("Error reading configuration file.");;
        return false;
    }

    // Set up favorites list
    if (config->favoritesFile != "")
    {
        ifstream in(config->favoritesFile.c_str(), ios::in);

        if (in.good())
        {
            favorites = ReadFavoritesList(in);
            if (favorites == NULL)
            {
                warning("Error reading favorites file.");
            }
        }
    }

    // If we couldn't read the favorites list from a file, allocate
    // an empty list.
    if (favorites == NULL)
        favorites = new FavoritesList();

    if (!readStars(*config))
    {
        fatalError("Cannot read star database.");
        return false;
    }

    solarSystemCatalog = new SolarSystemCatalog();
    {
        for (vector<string>::const_iterator iter = config->solarSystemFiles.begin();
             iter != config->solarSystemFiles.end();
             iter++)
        {
            ifstream solarSysFile(iter->c_str(), ios::in);
            if (!solarSysFile.good())
            {
                warning("Error opening solar system catalog.\n");
            }
            else
            {
                ReadSolarSystems(solarSysFile, *starDB, *solarSystemCatalog);
            }
        }
    }

    if (config->galaxyCatalog != "")
    {
        ifstream galaxiesFile(config->galaxyCatalog.c_str(), ios::in);
        if (!galaxiesFile.good())
        {
            warning("Error opening galaxies file.");
        }
        else
        {
            galaxies = ReadGalaxyList(galaxiesFile);
        }
    }

    if (config->asterismsFile != "")
    {
        ifstream asterismsFile(config->asterismsFile.c_str(), ios::in);
        if (!asterismsFile.good())
        {
            warning("Error opening asterisms file.");
        }
        else
        {
            asterisms = ReadAsterismList(asterismsFile, *starDB);
        }
    }

    // Load initialization script
    if (config->initScriptFile != "")
    {
        ifstream scriptfile(config->initScriptFile.c_str());
        CommandParser parser(scriptfile);
        initScript = parser.parse();
        if (initScript == NULL)
        {
            const vector<string>* errors = parser.getErrors();
            for_each(errors->begin(), errors->end(), printlineFunc<string>(cout));
        }
    }

    // Load demo script
    if (config->demoScriptFile != "")
    {
        ifstream scriptfile(config->demoScriptFile.c_str());
        CommandParser parser(scriptfile);
        demoScript = parser.parse();
        if (demoScript == NULL)
        {
            const vector<string>* errors = parser.getErrors();
            for_each(errors->begin(), errors->end(), printlineFunc<string>(cout));
        }
    }

    // Load destinations list
    if (config->destinationsFile != "")
    {
        ifstream destfile(config->destinationsFile.c_str());
        if (destfile.good())
        {
            destinations = ReadDestinationList(destfile);
        }
    }

    sim = new Simulation();
    sim->setStarDatabase(starDB, solarSystemCatalog, galaxies);
    sim->setFaintestVisible(config->faintestVisible);

    return true;
}


bool CelestiaCore::initRenderer()
{
    renderer = new Renderer();

    // Prepare the scene for rendering.
    if (!renderer->init((int) width, (int) height))
    {
        fatalError("Failed to initialize renderer");
        return false;
    }

#ifdef _WIN32
    if (renderer->fragmentShaderSupported())
        renderer->setFragmentShaderEnabled(true);
    if (renderer->vertexShaderSupported())
        renderer->setVertexShaderEnabled(true);
#endif

    // Set up the star labels
    for (vector<string>::const_iterator iter = config->labelledStars.begin();
         iter != config->labelledStars.end();
         iter++)
    {
        Star* star = starDB->find(*iter);
        if (star != NULL)
            renderer->addLabelledStar(star);
    }

    renderer->setBrightnessBias(0.0f);
    renderer->setBrightnessScale(1.0f / (config->faintestVisible + 1.0f));

    renderer->showAsterisms(asterisms);

    if (config->mainFont == "")
        font = LoadTextureFont("fonts/default.txf");
    else
        font = LoadTextureFont(string("fonts") + "/" + config->mainFont);
    if (font == NULL)
    {
        cout << "Error loading font; text will not be visible.";
    }

    if (config->titleFont != "")
        titleFont = LoadTextureFont(string("fonts") + "/" + config->titleFont);
    if (titleFont == NULL)
        titleFont = font;

    // Set up the overlay
    overlay = new Overlay();
    overlay->setWindowSize(width, height);

    if (config->labelFont == "")
    {
        renderer->setFont(font);
    }
    else
    {
        TextureFont* labelFont = LoadTextureFont(string("fonts") + "/" + config->labelFont);
        if (labelFont == NULL)
            renderer->setFont(font);
        else
            renderer->setFont(labelFont);
    }

    if (config->logoTextureFile != "")
    {
        logoTexture = LoadTextureFromFile(string("textures") + "/" + config->logoTextureFile);
        logoTexture->bindName();
    }
    
    return true;
}


bool CelestiaCore::readStars(const CelestiaConfig& cfg)
{
    ifstream starFile(cfg.starDatabaseFile.c_str(), ios::in | ios::binary);
    if (!starFile.good())
    {
	cerr << "Error opening " << cfg.starDatabaseFile << '\n';
        return false;
    }

    ifstream starNamesFile(cfg.starNamesFile.c_str(), ios::in);
    if (!starNamesFile.good())
    {
	cerr << "Error opening " << cfg.starNamesFile << '\n';
        return false;
    }

    starDB = StarDatabase::read(starFile);
    if (starDB == NULL)
    {
	cerr << "Error reading stars file\n";
        return false;
    }

    StarNameDatabase* starNameDB = StarNameDatabase::readNames(starNamesFile);
    if (starNameDB == NULL)
    {
        cerr << "Error reading star names file\n";
        return false;
    }

    starDB->setNameDatabase(starNameDB);

    return true;
}


// Set the faintest visible star magnitude; adjust the renderer's
// brightness parameters appropriately.
void CelestiaCore::setFaintest(float magnitude)
{
    renderer->setBrightnessBias(0.0f);
    renderer->setBrightnessScale(1.0f / (magnitude + 1.0f));
    sim->setFaintestVisible(magnitude);
}


void CelestiaCore::fatalError(const string& msg)
{
    if (alerter == NULL)
        cout << msg;
    else
        alerter->fatalError(msg);
}


void CelestiaCore::setAlerter(Alerter* a)
{
    alerter = a;
}


int CelestiaCore::getTimeZoneBias() const
{
    return timeZoneBias;
}


void CelestiaCore::setTimeZoneBias(int bias)
{
    timeZoneBias = bias;
}

