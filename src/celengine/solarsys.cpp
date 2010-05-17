// solarsys.cpp
//
// Copyright (C) 2001-2009, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// Solar system catalog parser.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cassert>
// #include <limits>
#include <cstdio>

#ifndef _WIN32
#ifndef TARGET_OS_MAC
#include <config.h>
#endif /* ! TARGET_OS_MAC */
#endif /* ! _WIN32 */

#include <celutil/debug.h>
#include <celmath/mathlib.h>
#include <celutil/util.h>
#include <cstdio>
#include <limits>
#include "astro.h"
#include "parser.h"
#include "texmanager.h"
#include "meshmanager.h"
#include "universe.h"
#include "multitexture.h"
#include "parseobject.h"
#include "frametree.h"
#include "timeline.h"
#include "timelinephase.h"
#include "atmosphere.h"

using namespace Eigen;
using namespace std;


enum Disposition
{
    AddObject,
    ReplaceObject,
    ModifyObject,
};


enum BodyType
{
    ReferencePoint,
    NormalBody,
    SurfaceObject,
    UnknownBodyType,
};


/*!
  Solar system catalog (.ssc) files contain items of three different types:
  bodies, locations, and alternate surfaces.  Bodies planets, moons, asteroids,
  comets, and spacecraft.  Locations are points on the surfaces of bodies which
  may be labelled but aren't rendered.  Alternate surfaces are additional
  surface definitions for bodies.

  An ssc file contains zero or more definitions of this form:

  \code
  [disposition] [item type] "name" "parent name"
  {
     ...object info fields...
  }
  \endcode

  The disposition of the object determines what happens if an item with the
  same parent and same name already exists.  It may be one of the following:
  - Add - Default if none is specified.  Add the item even if one of the
    same name already exists.
  - Replace - Replace an existing item with the new one
  - Modify - Modify the existing item, changing the fields that appear
    in the new definition.

  All dispositions are equivalent to add if no item of the same name
  already exists.

  The item type is one of Body, Location, or AltSurface, defaulting to
  Body when no type is given.

  The name and parent name are both mandatory.
*/

static void errorMessagePrelude(const Tokenizer& tok)
{
    cerr << _("Error in .ssc file (line ") << tok.getLineNumber() << "): ";
}

static void sscError(const Tokenizer& tok,
                     const string& msg)
{
    errorMessagePrelude(tok);
    cerr << msg << '\n';
}


// Object class properties
static const int CLASSES_UNCLICKABLE           = Body::Invisible |
                                                 Body::Diffuse;

static const int CLASSES_INVISIBLE_AS_POINT    = Body::Invisible      |
                                                 Body::SurfaceFeature |
                                                 Body::Component      |
                                                 Body::Diffuse;

static const int CLASSES_SECONDARY_ILLUMINATOR = Body::Planet      |
                                                 Body::Moon        |
                                                 Body::MinorMoon   |
                                                 Body::DwarfPlanet |
                                                 Body::Asteroid    |
                                                 Body::Comet;

typedef map<std::string, int, CompareIgnoringCasePredicate> ClassificationTable;
static ClassificationTable Classifications;

void InitializeClassifications()
{
    Classifications["planet"]         = Body::Planet;
    Classifications["dwarfplanet"]    = Body::DwarfPlanet;
    Classifications["moon"]           = Body::Moon;
    Classifications["minormoon"]      = Body::MinorMoon;
    Classifications["comet"]          = Body::Comet;
    Classifications["asteroid"]       = Body::Asteroid;
    Classifications["spacecraft"]     = Body::Spacecraft;
    Classifications["invisible"]      = Body::Invisible;
    Classifications["surfacefeature"] = Body::SurfaceFeature;
    Classifications["component"]      = Body::Component;
    Classifications["diffuse"]        = Body::Diffuse;
}

int GetClassificationId(const string& className)
{
    if (Classifications.empty())
        InitializeClassifications();
    ClassificationTable::iterator iter = Classifications.find(className);
    if (iter == Classifications.end())
        return Body::Unknown;
    else
        return iter->second;
}


//! Maximum depth permitted for nested frames.
static unsigned int MaxFrameDepth = 50;

static bool isFrameCircular(const ReferenceFrame& frame, ReferenceFrame::FrameType frameType)
{
    return frame.nestingDepth(MaxFrameDepth, frameType) > MaxFrameDepth;
}



static Location* CreateLocation(Hash* locationData,
                                Body* body)
{
    Location* location = new Location();

    Vector3d longlat(0.0, 0.0, 0.0);
    locationData->getSphericalTuple("LongLat", longlat);

    Vector3f position = body->planetocentricToCartesian(longlat).cast<float>();
    location->setPosition(position);

    double size = 1.0;
    locationData->getLength("Size", size);

    location->setSize((float) size);

    double importance = -1.0;
    locationData->getNumber("Importance", importance);
    location->setImportance((float) importance);

    string featureTypeName;
    if (locationData->getString("Type", featureTypeName))
        location->setFeatureType(Location::parseFeatureType(featureTypeName));

    Color labelColor;
    if (locationData->getColor("LabelColor", labelColor))
    {
        location->setLabelColor(labelColor);
        location->setLabelColorOverridden(true);
    }

    return location;
}


static void FillinSurface(Hash* surfaceData,
                          Surface* surface,
                          const std::string& path)
{
    surfaceData->getColor("Color", surface->color);

    // Haze is deprecated; used only in pre-OpenGL 2.0 render paths
    Color hazeColor = surface->hazeColor;
    float hazeDensity = hazeColor.alpha();
    if (surfaceData->getColor("HazeColor", hazeColor) | surfaceData->getNumber("HazeDensity", hazeDensity))
    {
        surface->hazeColor = Color(hazeColor.red(), hazeColor.green(),
                                   hazeColor.blue(), hazeDensity);
    }

    surfaceData->getColor("SpecularColor", surface->specularColor);
    surfaceData->getNumber("SpecularPower", surface->specularPower);

    surfaceData->getNumber("LunarLambert", surface->lunarLambert);
#ifdef USE_HDR
    surfaceData->getNumber("NightLightRadiance", surface->nightLightRadiance);
#endif

    string baseTexture;
    string bumpTexture;
    string nightTexture;
    string specularTexture;
    string normalTexture;
    string overlayTexture;
    bool applyBaseTexture = surfaceData->getString("Texture", baseTexture);
    bool applyBumpMap = surfaceData->getString("BumpMap", bumpTexture);
    bool applyNightMap = surfaceData->getString("NightTexture", nightTexture);
    bool separateSpecular = surfaceData->getString("SpecularTexture",
                                                   specularTexture);
    bool applyNormalMap = surfaceData->getString("NormalMap", normalTexture);
    bool applyOverlay = surfaceData->getString("OverlayTexture",
                                               overlayTexture);

    unsigned int baseFlags = TextureInfo::WrapTexture | TextureInfo::AllowSplitting;
    unsigned int bumpFlags = TextureInfo::WrapTexture | TextureInfo::AllowSplitting;
    unsigned int nightFlags = TextureInfo::WrapTexture | TextureInfo::AllowSplitting;
    unsigned int specularFlags = TextureInfo::WrapTexture | TextureInfo::AllowSplitting;

    float bumpHeight = 2.5f;
    surfaceData->getNumber("BumpHeight", bumpHeight);

    bool blendTexture = false;
    surfaceData->getBoolean("BlendTexture", blendTexture);

    bool emissive = false;
    surfaceData->getBoolean("Emissive", emissive);

    bool compressTexture = false;
    surfaceData->getBoolean("CompressTexture", compressTexture);
    if (compressTexture)
        baseFlags |= TextureInfo::CompressTexture;

    if (blendTexture)
        surface->appearanceFlags |= Surface::BlendTexture;
    if (emissive)
        surface->appearanceFlags |= Surface::Emissive;
    if (applyBaseTexture)
        surface->appearanceFlags |= Surface::ApplyBaseTexture;
    if (applyBumpMap || applyNormalMap)
        surface->appearanceFlags |= Surface::ApplyBumpMap;
    if (applyNightMap)
        surface->appearanceFlags |= Surface::ApplyNightMap;
    if (separateSpecular)
        surface->appearanceFlags |= Surface::SeparateSpecularMap;
    if (applyOverlay)
        surface->appearanceFlags |= Surface::ApplyOverlay;
    if (surface->specularColor != Color(0.0f, 0.0f, 0.0f))
        surface->appearanceFlags |= Surface::SpecularReflection;

    if (applyBaseTexture)
        surface->baseTexture.setTexture(baseTexture, path, baseFlags);
    if (applyNightMap)
        surface->nightTexture.setTexture(nightTexture, path, nightFlags);
    if (separateSpecular)
        surface->specularTexture.setTexture(specularTexture, path, specularFlags);

    // If both are present, NormalMap overrides BumpMap
    if (applyNormalMap)
        surface->bumpTexture.setTexture(normalTexture, path, bumpFlags);
    else if (applyBumpMap)
        surface->bumpTexture.setTexture(bumpTexture, path, bumpHeight, bumpFlags);

    if (applyOverlay)
        surface->overlayTexture.setTexture(overlayTexture, path, baseFlags);
}


static Selection GetParentObject(PlanetarySystem* system)
{
    Selection parent;
    Body* primary = system->getPrimaryBody();
    if (primary != NULL)
        parent = Selection(primary);
    else
        parent = Selection(system->getStar());

    return parent;
}


TimelinePhase* CreateTimelinePhase(Body* body,
                                   Universe& universe,
                                   Hash* phaseData,
                                   const string& path,
                                   ReferenceFrame* defaultOrbitFrame,
                                   ReferenceFrame* defaultBodyFrame,
                                   bool isFirstPhase,
                                   bool isLastPhase,
                                   double previousPhaseEnd)
{
    double beginning = previousPhaseEnd;
    double ending = numeric_limits<double>::infinity();

    // Beginning is optional for the first phase of a timeline, and not
    // allowed for the other phases, where beginning is always the ending
    // of the previous phase.
    bool hasBeginning = ParseDate(phaseData, "Beginning", beginning);
    if (!isFirstPhase && hasBeginning)
    {
        clog << "Error: Beginning can only be specified for initial phase of timeline.\n";
        return NULL;
    }

    // Ending is required for all phases except for the final one.
    bool hasEnding = ParseDate(phaseData, "Ending", ending);
    if (!isLastPhase && !hasEnding)
    {
        clog << "Error: Ending is required for all timeline phases other than the final one.\n";
        return NULL;
    }

    // Get the orbit reference frame.
    ReferenceFrame* orbitFrame;
    Value* frameValue = phaseData->getValue("OrbitFrame");
    if (frameValue != NULL)
    {
        orbitFrame = CreateReferenceFrame(universe, frameValue, defaultOrbitFrame->getCenter(), body);
        if (orbitFrame == NULL)
        {
            return NULL;
        }
    }
    else
    {
        // No orbit frame specified; use the default frame.
        orbitFrame = defaultOrbitFrame;
    }
    orbitFrame->addRef();

    // Get the body reference frame
    ReferenceFrame* bodyFrame;
    Value* bodyFrameValue = phaseData->getValue("BodyFrame");
    if (bodyFrameValue != NULL)
    {
        bodyFrame = CreateReferenceFrame(universe, bodyFrameValue, defaultBodyFrame->getCenter(), body);
        if (bodyFrame == NULL)
        {
            orbitFrame->release();
            return NULL;
        }
    }
    else
    {
        // No body frame specified; use the default frame.
        bodyFrame = defaultBodyFrame;
    }
    bodyFrame->addRef();

    // Use planet units (AU for semimajor axis) if the center of the orbit 
    // reference frame is a star.
    bool usePlanetUnits = orbitFrame->getCenter().star() != NULL;

    // Get the orbit
    Orbit* orbit = CreateOrbit(orbitFrame->getCenter(), phaseData, path, usePlanetUnits);
    if (!orbit)
    {
        clog << "Error: missing orbit in timeline phase.\n";
        bodyFrame->release();
        orbitFrame->release();
        return NULL;
    }

    // Get the rotation model
    // TIMELINE-TODO: default rotation model is UniformRotation with a period
    // equal to the orbital period. Should we do something else?
    RotationModel* rotationModel = CreateRotationModel(phaseData, path, orbit->getPeriod());
    if (!rotationModel)
    {
        // TODO: Should distinguish between a missing rotation model (where it's
        // appropriate to use a default one) and a bad rotation model (where
        // we should report an error.)
        rotationModel = new ConstantOrientation(Quaterniond::Identity());
    }

    TimelinePhase* phase = TimelinePhase::CreateTimelinePhase(universe,
                                                              body,
                                                              beginning, ending,
                                                              *orbitFrame,
                                                              *orbit,
                                                              *bodyFrame,
                                                              *rotationModel);

    // Frame ownership transfered to phase; release local references
    orbitFrame->release();
    bodyFrame->release();

    return phase;
}


Timeline* CreateTimelineFromArray(Body* body,
                                  Universe& universe,
                                  Array* timelineArray,
                                  const string& path,
                                  ReferenceFrame* defaultOrbitFrame,
                                  ReferenceFrame* defaultBodyFrame)
{
    Timeline* timeline = new Timeline();
    double previousEnding = -numeric_limits<double>::infinity();

    for (Array::const_iterator iter = timelineArray->begin(); iter != timelineArray->end(); iter++)
    {
        Hash* phaseData = (*iter)->getHash();
        if (phaseData == NULL)
        {
            clog << "Error in timeline of '" << body->getName() << "': phase " << iter - timelineArray->begin() + 1 << " is not a property group.\n";
            delete timeline;
            return NULL;
        }

        bool isFirstPhase = iter == timelineArray->begin();
        bool isLastPhase = *iter == timelineArray->back();

        TimelinePhase* phase = CreateTimelinePhase(body, universe, phaseData,
                                                   path,
                                                   defaultOrbitFrame,
                                                   defaultBodyFrame,
                                                   isFirstPhase, isLastPhase, previousEnding);
        if (phase == NULL)
        {
            clog << "Error in timeline of '" << body->getName() << "', phase " << iter - timelineArray->begin() + 1 << endl;
            delete timeline;
            return NULL;
        }

        previousEnding = phase->endTime();

        timeline->appendPhase(phase);
    }

    return timeline;
}


static bool CreateTimeline(Body* body,
                           PlanetarySystem* system,
                           Universe& universe,
                           Hash* planetData,
                           const string& path,
                           Disposition disposition,
                           BodyType bodyType)
{
    FrameTree* parentFrameTree = NULL;
    Selection parentObject = GetParentObject(system);
    bool orbitsPlanet = false;
    if (parentObject.body())
    {
        parentFrameTree = parentObject.body()->getOrCreateFrameTree();
        orbitsPlanet = true;
    }
    else if (parentObject.star())
    {
        SolarSystem* solarSystem = universe.getSolarSystem(parentObject.star());
        if (solarSystem == NULL)
            solarSystem = universe.createSolarSystem(parentObject.star());
        parentFrameTree = solarSystem->getFrameTree();
    }
    else
    {
        // Bad orbit barycenter specified
        return false;
    }

    ReferenceFrame* defaultOrbitFrame = NULL;
    ReferenceFrame* defaultBodyFrame = NULL;
    if (bodyType == SurfaceObject)
    {
        defaultOrbitFrame = new BodyFixedFrame(parentObject, parentObject);
        defaultBodyFrame = CreateTopocentricFrame(parentObject, parentObject, Selection(body));
        defaultOrbitFrame->addRef();
        defaultBodyFrame->addRef();
    }
    else
    {
        defaultOrbitFrame = parentFrameTree->getDefaultReferenceFrame();
        defaultBodyFrame = parentFrameTree->getDefaultReferenceFrame();
    }
    
    // If there's an explicit timeline definition, parse that. Otherwise, we'll do
    // things the old way.
    Value* value = planetData->getValue("Timeline");
    if (value != NULL)
    {
        if (value->getType() != Value::ArrayType)
        {
            clog << "Error: Timeline must be an array\n";
            return false;
        }

        Timeline* timeline = CreateTimelineFromArray(body, universe, value->getArray(), path,
                                                     defaultOrbitFrame, defaultBodyFrame);
        if (timeline == NULL)
        {
            return false;
        }
        else
        {
            body->setTimeline(timeline);
            return true;
        }
    }

    // Information required for the object timeline.
    ReferenceFrame* orbitFrame   = NULL;
    ReferenceFrame* bodyFrame    = NULL;
    Orbit* orbit                 = NULL;
    RotationModel* rotationModel = NULL;
    double beginning             = -numeric_limits<double>::infinity();
    double ending                =  numeric_limits<double>::infinity();

    // If any new timeline values are specified, we need to overrideOldTimeline will
    // be set to true.
    bool overrideOldTimeline = false;

    // The interaction of Modify with timelines is slightly complicated. If the timeline
    // is specified by putting the OrbitFrame, Orbit, BodyFrame, or RotationModel directly
    // in the object definition (i.e. not inside a Timeline structure), it will completely
    // replace the previous timeline if it contained more than one phase. Otherwise, the
    // properties of the single phase will be modified individually, for compatibility with
    // Celestia versions 1.5.0 and earlier.
    if (disposition == ModifyObject)
    {
        const Timeline* timeline = body->getTimeline();
        if (timeline->phaseCount() == 1)
        {
            const TimelinePhase* phase = timeline->getPhase(0);
            orbitFrame    = phase->orbitFrame();
            bodyFrame     = phase->bodyFrame();
            orbit         = phase->orbit();
            rotationModel = phase->rotationModel();
            beginning     = phase->startTime();
            ending        = phase->endTime();
        }
    }

    // Get the object's orbit reference frame.
    bool newOrbitFrame = false;
    Value* frameValue = planetData->getValue("OrbitFrame");
    if (frameValue != NULL)
    {
        ReferenceFrame* frame = CreateReferenceFrame(universe, frameValue, parentObject, body);
        if (frame != NULL)
        {
            orbitFrame = frame;
            newOrbitFrame = true;
            overrideOldTimeline = true;
        }
    }

    // Get the object's body frame.
    bool newBodyFrame = false;
    Value* bodyFrameValue = planetData->getValue("BodyFrame");
    if (bodyFrameValue != NULL)
    {
        ReferenceFrame* frame = CreateReferenceFrame(universe, bodyFrameValue, parentObject, body);
        if (frame != NULL)
        {
            bodyFrame = frame;
            newBodyFrame = true;
            overrideOldTimeline = true;
        }
    }

    // If no orbit or body frame was specified, use the default ones
    if (orbitFrame == NULL)
        orbitFrame = defaultOrbitFrame;
    if (bodyFrame == NULL)
        bodyFrame = defaultBodyFrame;

    // If the center of the is a star, orbital element units are
    // in AU; otherwise, use kilometers.
    if (orbitFrame->getCenter().star() != NULL)
        orbitsPlanet = false;
    else
        orbitsPlanet = true;

    Orbit* newOrbit = CreateOrbit(orbitFrame->getCenter(), planetData, path, !orbitsPlanet);
    if (newOrbit == NULL && orbit == NULL)
    {
        if (body->getTimeline() && disposition == ModifyObject)
        {
            // The object definition is modifying an existing object with a multiple phase
            // timeline, but no orbit definition was given. This can happen for completely
            // sensible reasons, such a Modify definition that just changes visual properties.
            // Or, the definition may try to change other timeline phase properties such as
            // the orbit frame, but without providing an orbit. In both cases, we'll just
            // leave the original timeline alone.
            return true;
        }
        else
        {
            clog << "No valid orbit specified for object '" << body->getName() << "'. Skipping.\n";
            return false;
        }
    }

    // If a new orbit was given, override any old orbit
    if (newOrbit != NULL)
    {
        orbit = newOrbit;
        overrideOldTimeline = true;
    }

    // Get the rotation model for this body
    double syncRotationPeriod = orbit->getPeriod();
    RotationModel* newRotationModel = CreateRotationModel(planetData, path, syncRotationPeriod);

    // If a new rotation model was given, override the old one
    if (newRotationModel != NULL)
    {
        rotationModel = newRotationModel;
        overrideOldTimeline = true;
    }

    // If there was no rotation model specified, nor a previous rotation model to
    // override, create the default one.
    if (rotationModel == NULL)
    {
        // If no rotation model is provided, use a default rotation model--
        // a uniform rotation that's synchronous with the orbit (appropriate
        // for nearly all natural satellites in the solar system.)
        rotationModel = CreateDefaultRotationModel(syncRotationPeriod);
    }
    
    if (ParseDate(planetData, "Beginning", beginning))
        overrideOldTimeline = true;
    if (ParseDate(planetData, "Ending", ending))
        overrideOldTimeline = true;

    // Something went wrong if the disposition isn't modify and no timeline
    // is to be created.
    assert(disposition == ModifyObject || overrideOldTimeline);

    if (overrideOldTimeline)
    {
        if (beginning >= ending)
        {
            clog << "Beginning time must be before Ending time.\n";
            return false;
        }

        // We finally have an orbit, rotation model, frames, and time range. Create
        // the object timeline.
        TimelinePhase* phase = TimelinePhase::CreateTimelinePhase(universe,
                                                                  body,
                                                                  beginning, ending,
                                                                  *orbitFrame,
                                                                  *orbit,
                                                                  *bodyFrame,
                                                                  *rotationModel);

        // We've already checked that beginning < ending; nothing else should go
        // wrong during the creation of a TimelinePhase.
        assert(phase != NULL);
        if (phase == NULL)
        {
            clog << "Internal error creating TimelinePhase.\n";
            return false;
        }

        Timeline* timeline = new Timeline();
        timeline->appendPhase(phase);

        body->setTimeline(timeline);

        // Check for circular references in frames; this can only be done once the timeline
        // has actually been set.
        // TIMELINE-TODO: This check is not comprehensive; it won't find recursion in
        // multiphase timelines.
        if (newOrbitFrame && isFrameCircular(*body->getOrbitFrame(0.0), ReferenceFrame::PositionFrame))
        {
            clog << "Orbit frame for " << body->getName() << " is nested too deep (probably circular)\n";
            return false;
        }

        if (newBodyFrame && isFrameCircular(*body->getBodyFrame(0.0), ReferenceFrame::OrientationFrame))
        {
            clog << "Body frame for " << body->getName() << " is nested too deep (probably circular)\n";
            return false;
        }
    }

    return true;
}


// Create a body (planet, moon, spacecraft, etc.) using the values from a
// property list. The usePlanetsUnits flags specifies whether period and
// semi-major axis are in years and AU rather than days and kilometers.
static Body* CreateBody(const string& name,
                        PlanetarySystem* system,
                        Universe& universe,
                        Body* existingBody,
                        Hash* planetData,
                        const string& path,
                        Disposition disposition,
                        BodyType bodyType)
{
    Body* body = NULL;

    if (disposition == ModifyObject || disposition == ReplaceObject)
    {
        body = existingBody;
    }

    if (body == NULL)
    {
        body = new Body(system, name);
        // If the body doesn't exist, always treat the disposition as 'Add'
        disposition = AddObject;
        
        // Set the default classification for new objects based on the body type.
        // This may be overridden by the Class property.
        if (bodyType == SurfaceObject)
        {
            body->setClassification(Body::SurfaceFeature);
        }
    }

    if (!CreateTimeline(body, system, universe, planetData, path, disposition, bodyType))
    {
        // No valid timeline given; give up.
        if (body != existingBody)
            delete body;
        return NULL;
    }

    // Three values control the shape and size of an ellipsoidal object:
    // semiAxes, radius, and oblateness. It is an error if neither the
    // radius nor semiaxes are set. If both are set, the radius is
    // multipled by each of the specified semiaxis to give the shape of
    // the body ellipsoid. Oblateness is ignored if semiaxes are provided;
    // otherwise, the ellipsoid has semiaxes: ( radius, radius, 1-radius ).
    // These rather complex rules exist to maintain backward compatibility.
    //
    // If the body also has a mesh, it is always scaled in x, y, and z by
    // the maximum semiaxis, never anisotropically.

    double radius = (double) body->getRadius();
    bool radiusSpecified = false;
    if (planetData->getLength("Radius", radius))
    {
        body->setSemiAxes(Vector3f::Constant((float) radius));
        radiusSpecified = true;
    }

    Vector3d semiAxes = Vector3d::Ones();
    if (planetData->getVector("SemiAxes", semiAxes))
    {
        if (radiusSpecified)
        {
            // if the radius has been specified, treat SemiAxes as dimensionless
            // (i.e. ignore units) and multiply the radius by the SemiAxes
            semiAxes *= radius;
        }
        else
        {
            double semiAxesScale = 1.0;
            planetData->getLengthScale("SemiAxes", semiAxesScale);
            semiAxes *= semiAxesScale;
        }
        // Swap y and z to match internal coordinate system
        body->setSemiAxes(Vector3f((float) semiAxes.x(), (float) semiAxes.z(), (float) semiAxes.y()));
    }
    else
    {
        double oblateness = 0.0;
        if (planetData->getNumber("Oblateness", oblateness))
        {
            body->setSemiAxes((float) body->getRadius() * Vector3f(1.0f, 1.0f - (float) oblateness, 1.0f));
        }
    }

    int classification = body->getClassification();
    string classificationName;
    if (planetData->getString("Class", classificationName))
        classification = GetClassificationId(classificationName);

    if (classification == Body::Unknown)
    {
        // Try to guess the type
        if (system->getPrimaryBody() != NULL)
        {
            if(radius > 0.1)
                classification = Body::Moon;
            else
                classification = Body::Spacecraft;
        }
        else
        {
            if (radius < 1000.0)
                classification = Body::Asteroid;
            else
                classification = Body::Planet;
        }
    }
    body->setClassification(classification);

    if (classification == Body::Invisible)
        body->setVisible(false);

    // Set default properties for the object based on its classification
    if (classification & CLASSES_INVISIBLE_AS_POINT)
        body->setVisibleAsPoint(false);
    if ((classification & CLASSES_SECONDARY_ILLUMINATOR) == 0)
        body->setSecondaryIlluminator(false);
    if (classification & CLASSES_UNCLICKABLE)
        body->setClickable(false);

    string infoURL;
    if (planetData->getString("InfoURL", infoURL))
    {
        if (infoURL.find(':') == string::npos)
        {
            // Relative URL, the base directory is the current one,
            // not the main installation directory
            if (path[1] == ':')
                // Absolute Windows path, file:/// is required
                infoURL = "file:///" + path + "/" + infoURL;
            else if (!path.empty())
                infoURL = path + "/" + infoURL;
        }
        body->setInfoURL(infoURL);
    }

    double albedo = 0.5;
    if (planetData->getNumber("Albedo", albedo))
        body->setAlbedo((float) albedo);

    // TODO - add mass units
    double mass = 0.0;
    if (planetData->getNumber("Mass", mass))
        body->setMass((float) mass);

    Quaternionf orientation = Quaternionf::Identity();
    if (planetData->getRotation("Orientation", orientation))
    {
        body->setGeometryOrientation(orientation);
    }

    Surface surface;
    if (disposition == ModifyObject)
    {
        surface = body->getSurface();
    }
    else
    {
        surface.color = Color(1.0f, 1.0f, 1.0f);
        surface.hazeColor = Color(0.0f, 0.0f, 0.0f, 0.0f);
    }
    FillinSurface(planetData, &surface, path);
    body->setSurface(surface);

    {
        string geometry("");
        if (planetData->getString("Mesh", geometry))
        {
            Vector3f geometryCenter(Vector3f::Zero());
            if (planetData->getVector("MeshCenter", geometryCenter))
            {
                // TODO: Adjust bounding radius if model center isn't
                // (0.0f, 0.0f, 0.0f)
            }

            bool isNormalized = true;
            planetData->getBoolean("NormalizeMesh", isNormalized);

            float geometryScale = 1.0f;
            planetData->getLength("MeshScale", geometryScale);

            ResourceHandle geometryHandle = GetGeometryManager()->getHandle(GeometryInfo(geometry, path, geometryCenter, 1.0f, isNormalized));
            body->setGeometry(geometryHandle);
            body->setGeometryScale(geometryScale);
        }
    }

    // Read the atmosphere
    {
        Value* atmosDataValue = planetData->getValue("Atmosphere");
        if (atmosDataValue != NULL)
        {
            if (atmosDataValue->getType() != Value::HashType)
            {
                cout << "ReadSolarSystem: Atmosphere must be an assoc array.\n";
            }
            else
            {
                Hash* atmosData = atmosDataValue->getHash();
                assert(atmosData != NULL);

                Atmosphere* atmosphere = NULL;
                if (disposition == ModifyObject)
                {
                    atmosphere = body->getAtmosphere();
                    if (atmosphere == NULL)
                    {
                        Atmosphere atm;
                        body->setAtmosphere(atm);
                        atmosphere = body->getAtmosphere();
                    }
                }
                else
                {
                    atmosphere = new Atmosphere();
                }
                atmosData->getLength("Height", atmosphere->height);
                atmosData->getColor("Lower", atmosphere->lowerColor);
                atmosData->getColor("Upper", atmosphere->upperColor);
                atmosData->getColor("Sky", atmosphere->skyColor);
                atmosData->getColor("Sunset", atmosphere->sunsetColor);

                atmosData->getNumber("Mie", atmosphere->mieCoeff);
                atmosData->getLength("MieScaleHeight", atmosphere->mieScaleHeight);
                atmosData->getNumber("MieAsymmetry", atmosphere->miePhaseAsymmetry);
                atmosData->getVector("Rayleigh", atmosphere->rayleighCoeff);
                //atmosData->getNumber("RayleighScaleHeight", atmosphere->rayleighScaleHeight);
                atmosData->getVector("Absorption", atmosphere->absorptionCoeff);

                // Get the cloud map settings
                atmosData->getLength("CloudHeight", atmosphere->cloudHeight);
                if (atmosData->getNumber("CloudSpeed", atmosphere->cloudSpeed))
                    atmosphere->cloudSpeed = degToRad(atmosphere->cloudSpeed);

                string cloudTexture;
                if (atmosData->getString("CloudMap", cloudTexture))
                {
                    atmosphere->cloudTexture.setTexture(cloudTexture,
                                                        path,
                                                        TextureInfo::WrapTexture);
                }

                string cloudNormalMap;
                if (atmosData->getString("CloudNormalMap", cloudNormalMap))
                {
                    atmosphere->cloudNormalMap.setTexture(cloudNormalMap,
                                                           path,
                                                           TextureInfo::WrapTexture);
                }

                double cloudShadowDepth = 0.0;
                if (atmosData->getNumber("CloudShadowDepth", cloudShadowDepth))
                {
                    cloudShadowDepth = max(0.0, min(1.0, cloudShadowDepth));  // clamp to [0, 1]
                    atmosphere->cloudShadowDepth = (float) cloudShadowDepth;
                }

                body->setAtmosphere(*atmosphere);
                if (disposition != ModifyObject)
                    delete atmosphere;
            }
        }
    }

    // Read the ring system
    {
        Value* ringsDataValue = planetData->getValue("Rings");
        if (ringsDataValue != NULL)
        {
            if (ringsDataValue->getType() != Value::HashType)
            {
                cout << "ReadSolarSystem: Rings must be an assoc array.\n";
            }
            else
            {
                Hash* ringsData = ringsDataValue->getHash();
                // ASSERT(ringsData != NULL);

                RingSystem rings(0.0f, 0.0f);
                if (body->getRings() != NULL)
                    rings = *body->getRings();

                double inner = 0.0, outer = 0.0;
                if (ringsData->getLength("Inner", inner))
                    rings.innerRadius = (float) inner;
                if (ringsData->getLength("Outer", outer))
                    rings.outerRadius = (float) outer;

                Color color(1.0f, 1.0f, 1.0f);
                if (ringsData->getColor("Color", color))
                    rings.color = color;

                string textureName;
                if (ringsData->getString("Texture", textureName))
                    rings.texture = MultiResTexture(textureName, path);

                body->setRings(rings);
            }
        }
    }
    
    bool clickable = true;
    if (planetData->getBoolean("Clickable", clickable))
    {
        body->setClickable(clickable);
    }

    bool visible = true;
    if (planetData->getBoolean("Visible", visible))
    {
        body->setVisible(visible);
    }

    Color orbitColor;
    if (planetData->getColor("OrbitColor", orbitColor))
    {
        body->setOrbitColorOverridden(true);
        body->setOrbitColor(orbitColor);
    }

    return body;
}


// Create a barycenter object using the values from a hash
static Body* CreateReferencePoint(const string& name,
                                  PlanetarySystem* system,
                                  Universe& universe,
                                  Body* existingBody,
                                  Hash* refPointData,
                                  const string& path,
                                  Disposition disposition)
{
    Body* body = NULL;

    if (disposition == ModifyObject || disposition == ReplaceObject)
    {
        body = existingBody;
    }

    if (body == NULL)
    {
        body = new Body(system, name);
        // If the point doesn't exist, always treat the disposition as 'Add'
        disposition = AddObject;
    }

    body->setSemiAxes(Vector3f::Ones());
    body->setClassification(Body::Invisible);
    body->setVisible(false);
    body->setVisibleAsPoint(false);
    body->setClickable(false);

    if (!CreateTimeline(body, system, universe, refPointData, path, disposition, ReferencePoint))
    {
        // No valid timeline given; give up.
        if (body != existingBody)
            delete body;
        return NULL;
    }

    // Reference points can be marked visible; no geometry is shown, but the label and orbit
    // will be.
    bool visible = false;
    if (refPointData->getBoolean("Visible", visible))
    {
        body->setVisible(visible);
    }

    bool clickable = false;
    if (refPointData->getBoolean("Clickable", clickable))
    {
        body->setClickable(clickable);
    }

    return body;
}


bool LoadSolarSystemObjects(istream& in,
                            Universe& universe,
                            const std::string& directory)
{
    Tokenizer tokenizer(&in);
    Parser parser(&tokenizer);

    while (tokenizer.nextToken() != Tokenizer::TokenEnd)
    {
        // Read the disposition; if none is specified, the default is Add.
        Disposition disposition = AddObject;
        if (tokenizer.getTokenType() == Tokenizer::TokenName)
        {
            if (tokenizer.getNameValue() == "Add")
            {
                disposition = AddObject;
                tokenizer.nextToken();
            }
            else if (tokenizer.getNameValue() == "Replace")
            {
                disposition = ReplaceObject;
                tokenizer.nextToken();
            }
            else if (tokenizer.getNameValue() == "Modify")
            {
                disposition = ModifyObject;
                tokenizer.nextToken();
            }
        }

        // Read the item type; if none is specified the default is Body
        string itemType("Body");
        if (tokenizer.getTokenType() == Tokenizer::TokenName)
        {
            itemType = tokenizer.getNameValue();
            tokenizer.nextToken();
        }

        if (tokenizer.getTokenType() != Tokenizer::TokenString)
        {
            sscError(tokenizer, "object name expected");
            return false;
        }
        
        // The name list is a string with zero more names. Multiple names are
        // delimited by colons.
        string nameList = tokenizer.getStringValue().c_str();

        if (tokenizer.nextToken() != Tokenizer::TokenString)
        {
            sscError(tokenizer, "bad parent object name");
            return false;
        }
        string parentName = tokenizer.getStringValue().c_str();

        Value* objectDataValue = parser.readValue();
        if (objectDataValue == NULL)
        {
            sscError(tokenizer, "bad object definition");
            return false;
        }

        if (objectDataValue->getType() != Value::HashType)
        {
            sscError(tokenizer, "{ expected");
            delete objectDataValue;
            return false;
        }
        Hash* objectData = objectDataValue->getHash();

        Selection parent = universe.findPath(parentName, NULL, 0);
        PlanetarySystem* parentSystem = NULL;
        
        vector<string> names;
        // Iterate through the string for names delimited
        // by ':', and insert them into the name list.
        if (nameList.empty())
        {
            names.push_back("");
        }
        else
        {
            string::size_type startPos   = 0;
            while (startPos != string::npos)
            {
                string::size_type next    = nameList.find(':', startPos);
                string::size_type length  = string::npos;
                if (next != string::npos)
                {
                    length   = next - startPos;
                    ++next;
                }
                names.push_back(nameList.substr(startPos, length));
                startPos   = next;
            }
        }
        string primaryName = names.front();

        BodyType bodyType = UnknownBodyType;
        if (itemType == "Body")
            bodyType = NormalBody;
        else if (itemType == "ReferencePoint")
            bodyType = ReferencePoint;
        else if (itemType == "SurfaceObject")
            bodyType = SurfaceObject;
        
        if (bodyType != UnknownBodyType)
        {
            //bool orbitsPlanet = false;
            if (parent.star() != NULL)
            {
                SolarSystem* solarSystem = universe.getSolarSystem(parent.star());
                if (solarSystem == NULL)
                {
                    // No solar system defined for this star yet, so we need
                    // to create it.
                    solarSystem = universe.createSolarSystem(parent.star());
                }
                parentSystem = solarSystem->getPlanets();
            }
            else if (parent.body() != NULL)
            {
                // Parent is a planet or moon
                parentSystem = parent.body()->getSatellites();
                if (parentSystem == NULL)
                {
                    // If the planet doesn't already have any satellites, we
                    // have to create a new planetary system for it.
                    parentSystem = new PlanetarySystem(parent.body());
                    parent.body()->setSatellites(parentSystem);
                }
                //orbitsPlanet = true;
            }
            else
            {
                errorMessagePrelude(tokenizer);
                cerr << _("parent body '") << parentName << _("' of '") << primaryName << _("' not found.") << endl;
            }

            if (parentSystem != NULL)
            {
                Body* existingBody = parentSystem->find(primaryName);
                if (existingBody)
                {
                    if (disposition == AddObject)
                    {
                        errorMessagePrelude(tokenizer);
                        cerr << _("warning duplicate definition of ") <<
                            parentName << " " <<  primaryName << '\n';
                    }
                    else if (disposition == ReplaceObject)
                    {
                        existingBody->setDefaultProperties();
                    }
                }

                Body* body;
                if (bodyType == ReferencePoint)
                    body = CreateReferencePoint(primaryName, parentSystem, universe, existingBody, objectData, directory, disposition);
                else
                    body = CreateBody(primaryName, parentSystem, universe, existingBody, objectData, directory, disposition, bodyType);
                
                if (body != NULL && disposition == AddObject)
                {
                    vector<string>::const_iterator iter = names.begin();
                    iter++;
                    while (iter != names.end())
                    {
                        body->addAlias(*iter);
                        iter++;
                    }
                }
            }
        }
        else if (itemType == "AltSurface")
        {
            Surface* surface = new Surface();
            surface->color = Color(1.0f, 1.0f, 1.0f);
            surface->hazeColor = Color(0.0f, 0.0f, 0.0f, 0.0f);
            FillinSurface(objectData, surface, directory);
            if (surface != NULL && parent.body() != NULL)
                parent.body()->addAlternateSurface(primaryName, surface);
            else
                sscError(tokenizer, _("bad alternate surface"));
        }
        else if (itemType == "Location")
        {
            if (parent.body() != NULL)
            {
                Location* location = CreateLocation(objectData, parent.body());
                if (location != NULL)
                {
                    location->setName(primaryName);
                    parent.body()->addLocation(location);
                }
                else
                {
                    sscError(tokenizer, _("bad location"));
                }
            }
            else
            {
                errorMessagePrelude(tokenizer);
                cerr << _("parent body '") << parentName << _("' of '") << primaryName << _("' not found.\n");
            }
        }
        delete objectDataValue;
    }

    // TODO: Return some notification if there's an error parsing the file
    return true;
}


SolarSystem::SolarSystem(Star* _star) : 
    star(_star),
    planets(NULL),
    frameTree(NULL)
{
    planets = new PlanetarySystem(star);
    frameTree = new FrameTree(star);
}


Star* SolarSystem::getStar() const
{
    return star;
}

Vector3f SolarSystem::getCenter() const
{
    // TODO: This is a very simple method at the moment, but it will get
    // more complex when planets around multistar systems are supported
    // where the planets may orbit the center of mass of two stars.
    return star->getPosition();
}

PlanetarySystem* SolarSystem::getPlanets() const
{
    return planets;
}

FrameTree* SolarSystem::getFrameTree() const
{
    return frameTree;
}
