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
#include <config.h>
#include <celutil/debug.h>
#include <celmath/mathlib.h>
#include <celutil/util.h>
#include <limits>
#include <fmt/printf.h>
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
using namespace celmath;

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
    fmt::fprintf(cerr,_("Error in .ssc file (line %d): "), tok.getLineNumber());
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

    Vector3d longlat(Vector3d::Zero());
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
                          const fs::path& path)
{
    surfaceData->getColor("Color", surface->color);
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
    if (primary != nullptr)
        parent = Selection(primary);
    else
        parent = Selection(system->getStar());

    return parent;
}


TimelinePhase::SharedConstPtr CreateTimelinePhase(Body* body,
                                                  Universe& universe,
                                                  Hash* phaseData,
                                                  const fs::path& path,
                                                  const ReferenceFrame::SharedConstPtr& defaultOrbitFrame,
                                                  const ReferenceFrame::SharedConstPtr& defaultBodyFrame,
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
        return nullptr;
    }

    // Ending is required for all phases except for the final one.
    bool hasEnding = ParseDate(phaseData, "Ending", ending);
    if (!isLastPhase && !hasEnding)
    {
        clog << "Error: Ending is required for all timeline phases other than the final one.\n";
        return nullptr;
    }

    // Get the orbit reference frame.
    ReferenceFrame::SharedConstPtr orbitFrame;
    Value* frameValue = phaseData->getValue("OrbitFrame");
    if (frameValue != nullptr)
    {
        orbitFrame = CreateReferenceFrame(universe, frameValue, defaultOrbitFrame->getCenter(), body);
        if (orbitFrame == nullptr)
        {
            return nullptr;
        }
    }
    else
    {
        // No orbit frame specified; use the default frame.
        orbitFrame = defaultOrbitFrame;
    }

    // Get the body reference frame
    ReferenceFrame::SharedConstPtr bodyFrame;
    Value* bodyFrameValue = phaseData->getValue("BodyFrame");
    if (bodyFrameValue != nullptr)
    {
        bodyFrame = CreateReferenceFrame(universe, bodyFrameValue, defaultBodyFrame->getCenter(), body);
        if (bodyFrame == nullptr)
        {
            return nullptr;
        }
    }
    else
    {
        // No body frame specified; use the default frame.
        bodyFrame = defaultBodyFrame;
    }

    // Use planet units (AU for semimajor axis) if the center of the orbit
    // reference frame is a star.
    bool usePlanetUnits = orbitFrame->getCenter().star() != nullptr;

    // Get the orbit
    Orbit* orbit = CreateOrbit(orbitFrame->getCenter(), phaseData, path, usePlanetUnits);
    if (!orbit)
    {
        clog << "Error: missing orbit in timeline phase.\n";
        return nullptr;
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

    auto phase = TimelinePhase::CreateTimelinePhase(universe,
                                                    body,
                                                    beginning, ending,
                                                    orbitFrame,
                                                    *orbit,
                                                    bodyFrame,
                                                    *rotationModel);

    // Frame ownership transfered to phase; release local references

    return phase;
}


Timeline* CreateTimelineFromArray(Body* body,
                                  Universe& universe,
                                  ValueArray* timelineArray,
                                  const fs::path& path,
                                  const ReferenceFrame::SharedConstPtr& defaultOrbitFrame,
                                  const ReferenceFrame::SharedConstPtr& defaultBodyFrame)
{
    auto* timeline = new Timeline();
    double previousEnding = -numeric_limits<double>::infinity();

    for (ValueArray::const_iterator iter = timelineArray->begin(); iter != timelineArray->end(); iter++)
    {
        Hash* phaseData = (*iter)->getHash();
        if (phaseData == nullptr)
        {
            clog << "Error in timeline of '" << body->getName().str() << "': phase " << iter - timelineArray->begin() + 1 << " is not a property group.\n";
            delete timeline;
            return nullptr;
        }

        bool isFirstPhase = iter == timelineArray->begin();
        bool isLastPhase = *iter == timelineArray->back();

        auto phase = CreateTimelinePhase(body, universe, phaseData,
                                         path,
                                         defaultOrbitFrame,
                                         defaultBodyFrame,
                                         isFirstPhase, isLastPhase, previousEnding);
        if (phase == nullptr)
        {
            clog << "Error in timeline of '" << body->getName().str() << "', phase " << iter - timelineArray->begin() + 1 << endl;
            delete timeline;
            return nullptr;
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
                           const fs::path& path,
                           DataDisposition disposition,
                           BodyType bodyType)
{
    FrameTree* parentFrameTree = nullptr;
    Selection parentObject = GetParentObject(system);
    bool orbitsPlanet = false;
    if (parentObject.body())
    {
        parentFrameTree = parentObject.body()->getOrCreateFrameTree();
        //orbitsPlanet = true;
    }
    else if (parentObject.star())
    {
        SolarSystem* solarSystem = universe.getSolarSystem(parentObject.star());
        if (solarSystem == nullptr)
            solarSystem = universe.createSolarSystem(parentObject.star());
        parentFrameTree = solarSystem->getFrameTree();
    }
    else
    {
        // Bad orbit barycenter specified
        return false;
    }

    ReferenceFrame::SharedConstPtr defaultOrbitFrame;
    ReferenceFrame::SharedConstPtr defaultBodyFrame;
    if (bodyType == SurfaceObject)
    {
        defaultOrbitFrame = make_shared<BodyFixedFrame>(parentObject, parentObject);
        defaultBodyFrame = CreateTopocentricFrame(parentObject, parentObject, Selection(body));
    }
    else
    {
        defaultOrbitFrame = parentFrameTree->getDefaultReferenceFrame();
        defaultBodyFrame = parentFrameTree->getDefaultReferenceFrame();
    }

    // If there's an explicit timeline definition, parse that. Otherwise, we'll do
    // things the old way.
    Value* value = planetData->getValue("Timeline");
    if (value != nullptr)
    {
        if (value->getType() != Value::ArrayType)
        {
            clog << "Error: Timeline must be an array\n";
            return false;
        }

        Timeline* timeline = CreateTimelineFromArray(body, universe, value->getArray(), path,
                                                     defaultOrbitFrame, defaultBodyFrame);

        if (!timeline)
            return false;

        body->setTimeline(timeline);
        return true;
    }

    // Information required for the object timeline.
    ReferenceFrame::SharedConstPtr orbitFrame;
    ReferenceFrame::SharedConstPtr bodyFrame;
    Orbit* orbit                 = nullptr;
    RotationModel* rotationModel = nullptr;
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
    if (disposition == DataDisposition::Modify)
    {
        const Timeline* timeline = body->getTimeline();
        if (timeline->phaseCount() == 1)
        {
            auto phase = timeline->getPhase(0).get();
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
    if (frameValue != nullptr)
    {
        auto frame = CreateReferenceFrame(universe, frameValue, parentObject, body);
        if (frame != nullptr)
        {
            orbitFrame = frame;
            newOrbitFrame = true;
            overrideOldTimeline = true;
        }
    }

    // Get the object's body frame.
    bool newBodyFrame = false;
    Value* bodyFrameValue = planetData->getValue("BodyFrame");
    if (bodyFrameValue != nullptr)
    {
        auto frame = CreateReferenceFrame(universe, bodyFrameValue, parentObject, body);
        if (frame != nullptr)
        {
            bodyFrame = frame;
            newBodyFrame = true;
            overrideOldTimeline = true;
        }
    }

    // If no orbit or body frame was specified, use the default ones
    if (orbitFrame == nullptr)
        orbitFrame = defaultOrbitFrame;
    if (bodyFrame == nullptr)
        bodyFrame = defaultBodyFrame;

    // If the center of the is a star, orbital element units are
    // in AU; otherwise, use kilometers.
    orbitsPlanet = orbitFrame->getCenter().star() == nullptr;

    Orbit* newOrbit = CreateOrbit(orbitFrame->getCenter(), planetData, path, !orbitsPlanet);
    if (newOrbit == nullptr && orbit == nullptr)
    {
        if (body->getTimeline() && disposition == DataDisposition::Modify)
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
            clog << "No valid orbit specified for object '" << body->getName().str() << "'. Skipping.\n";
            return false;
        }
    }

    // If a new orbit was given, override any old orbit
    if (newOrbit != nullptr)
    {
        orbit = newOrbit;
        overrideOldTimeline = true;
    }

    // Get the rotation model for this body
    double syncRotationPeriod = orbit->getPeriod();
    RotationModel* newRotationModel = CreateRotationModel(planetData, path, syncRotationPeriod);

    // If a new rotation model was given, override the old one
    if (newRotationModel != nullptr)
    {
        rotationModel = newRotationModel;
        overrideOldTimeline = true;
    }

    // If there was no rotation model specified, nor a previous rotation model to
    // override, create the default one.
    if (rotationModel == nullptr)
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
    assert(disposition == DataDisposition::Modify || overrideOldTimeline);

    if (overrideOldTimeline)
    {
        if (beginning >= ending)
        {
            clog << "Beginning time must be before Ending time.\n";
            delete rotationModel;
            return false;
        }

        // We finally have an orbit, rotation model, frames, and time range. Create
        // the object timeline.
        auto phase = TimelinePhase::CreateTimelinePhase(universe,
                                                        body,
                                                        beginning, ending,
                                                        orbitFrame,
                                                        *orbit,
                                                        bodyFrame,
                                                        *rotationModel);

        // We've already checked that beginning < ending; nothing else should go
        // wrong during the creation of a TimelinePhase.
        assert(phase != nullptr);
        if (phase == nullptr)
        {
            clog << "Internal error creating TimelinePhase.\n";
            return false;
        }

        auto* timeline = new Timeline();
        timeline->appendPhase(phase);

        body->setTimeline(timeline);

        // Check for circular references in frames; this can only be done once the timeline
        // has actually been set.
        // TIMELINE-TODO: This check is not comprehensive; it won't find recursion in
        // multiphase timelines.
        if (newOrbitFrame && isFrameCircular(*body->getOrbitFrame(0.0), ReferenceFrame::PositionFrame))
        {
            clog << "Orbit frame for " << body->getName().str() << " is nested too deep (probably circular)\n";
            return false;
        }

        if (newBodyFrame && isFrameCircular(*body->getBodyFrame(0.0), ReferenceFrame::OrientationFrame))
        {
            clog << "Body frame for " << body->getName().str() << " is nested too deep (probably circular)\n";
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
                        const fs::path& path,
                        DataDisposition disposition,
                        BodyType bodyType)
{
    Body* body = nullptr;

    if (disposition == DataDisposition::Modify || disposition == DataDisposition::Replace)
    {
        body = existingBody;
    }

    if (body == nullptr)
    {
        body = new Body(system, name);
        // If the body doesn't exist, always treat the disposition as 'Add'
        disposition = DataDisposition::Add;

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
        return nullptr;
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

    auto radius = (double) body->getRadius();
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
        if (system->getPrimaryBody() != nullptr)
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

    string infoURL; // FIXME: should be own class
    if (planetData->getString("InfoURL", infoURL))
    {
        if (infoURL.find(':') == string::npos)
        {
            // Relative URL, the base directory is the current one,
            // not the main installation directory
            const string p = path.string();
            if (p[1] == ':')
                // Absolute Windows path, file:/// is required
                infoURL = "file:///" + p + "/" + infoURL;
            else if (!p.empty())
                infoURL = p + "/" + infoURL;
        }
        body->setInfoURL(infoURL);
    }

    double t;
    if (planetData->getNumber("Albedo", t))
    {
        fmt::fprintf(cerr, "Deprecated parameter Albedo used in %s definition.\nUse GeomAlbedo instead.", name);
        body->setGeomAlbedo((float) t);
    }

    if (planetData->getNumber("GeomAlbedo", t))
        body->setGeomAlbedo((float) t);

    if (planetData->getNumber("BondAlbedo", t))
    {
        if (t >= 0.0 && t <= 1.0)
            body->setBondAlbedo((float) t);
        else
            fmt::fprintf(cerr, "Incorrect BondAlbedo value: %lf", t);
    }

    if (planetData->getNumber("Temperature", t))
        body->setTemperature((float) t);

    if (planetData->getNumber("TempDiscrepancy", t))
        body->setTempDiscrepancy((float) t);


    // TODO - add mass units
    if (planetData->getNumber("Mass", t))
        body->setMass((float) t);

    if (planetData->getNumber("Density", t))
       body->setDensity((float) t);

    Quaternionf orientation = Quaternionf::Identity();
    if (planetData->getRotation("Orientation", orientation))
    {
        body->setGeometryOrientation(orientation);
    }

    Surface surface;
    if (disposition == DataDisposition::Modify)
    {
        surface = body->getSurface();
    }
    else
    {
        surface.color = Color(1.0f, 1.0f, 1.0f);
    }
    FillinSurface(planetData, &surface, path);
    body->setSurface(surface);

    {
        string geometry;
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
        if (atmosDataValue != nullptr)
        {
            if (atmosDataValue->getType() != Value::HashType)
            {
                cout << "ReadSolarSystem: Atmosphere must be an assoc array.\n";
            }
            else
            {
                Hash* atmosData = atmosDataValue->getHash();
                assert(atmosData != nullptr);

                Atmosphere* atmosphere = nullptr;
                if (disposition == DataDisposition::Modify)
                {
                    atmosphere = body->getAtmosphere();
                    if (atmosphere == nullptr)
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
                if (disposition != DataDisposition::Modify)
                    delete atmosphere;
            }
        }
    }

    // Read the ring system
    {
        Value* ringsDataValue = planetData->getValue("Rings");
        if (ringsDataValue != nullptr)
        {
            if (ringsDataValue->getType() != Value::HashType)
            {
                cout << "ReadSolarSystem: Rings must be an assoc array.\n";
            }
            else
            {
                Hash* ringsData = ringsDataValue->getHash();
                // ASSERT(ringsData != nullptr);

                RingSystem rings(0.0f, 0.0f);
                if (body->getRings() != nullptr)
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

    // Read comet tail color
    Color cometTailColor;
    if(planetData->getColor("TailColor", cometTailColor))
    {
        body->setCometTailColor(cometTailColor);
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
                                  const fs::path& path,
                                  DataDisposition disposition)
{
    Body* body = nullptr;

    if (disposition == DataDisposition::Modify || disposition == DataDisposition::Replace)
    {
        body = existingBody;
    }

    if (body == nullptr)
    {
        body = new Body(system, name);
        // If the point doesn't exist, always treat the disposition as 'Add'
        disposition = DataDisposition::Add;
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
        return nullptr;
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
                            const fs::path& directory)
{
    Tokenizer tokenizer(&in);
    Parser parser(&tokenizer);

    const char* d = directory.string().c_str();
    bindtextdomain(d, d); // domain name is the same as resource path

    while (tokenizer.nextToken() != Tokenizer::TokenEnd)
    {
        // Read the disposition; if none is specified, the default is Add.
        DataDisposition disposition = DataDisposition::Add;
        if (tokenizer.getTokenType() == Tokenizer::TokenName)
        {
            if (tokenizer.getNameValue() == "Add")
            {
                disposition = DataDisposition::Add;
                tokenizer.nextToken();
            }
            else if (tokenizer.getNameValue() == "Replace")
            {
                disposition = DataDisposition::Replace;
                tokenizer.nextToken();
            }
            else if (tokenizer.getNameValue() == "Modify")
            {
                disposition = DataDisposition::Modify;
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
        if (objectDataValue == nullptr)
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

        Selection parent = universe.findPath(parentName, nullptr, 0);
        PlanetarySystem* parentSystem = nullptr;

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
            if (parent.star() != nullptr)
            {
                SolarSystem* solarSystem = universe.getSolarSystem(parent.star());
                if (solarSystem == nullptr)
                {
                    // No solar system defined for this star yet, so we need
                    // to create it.
                    solarSystem = universe.createSolarSystem(parent.star());
                }
                parentSystem = solarSystem->getPlanets();
            }
            else if (parent.body() != nullptr)
            {
                // Parent is a planet or moon
                parentSystem = parent.body()->getSatellites();
                if (parentSystem == nullptr)
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
                fmt::fprintf(cerr, _("parent body '%s' of '%s' not found.\n"), parentName, primaryName);
            }

            if (parentSystem != nullptr)
            {
                Body* existingBody = parentSystem->find(primaryName);
                if (existingBody)
                {
                    if (disposition == DataDisposition::Add)
                    {
                        errorMessagePrelude(tokenizer);
                        fmt::fprintf(cerr, _("warning duplicate definition of %s %s\n"), parentName, primaryName);
                    }
                    else if (disposition == DataDisposition::Replace)
                    {
                        existingBody->setDefaultProperties();
                    }
                }

                Body* body;
                if (bodyType == ReferencePoint)
                    body = CreateReferencePoint(primaryName, parentSystem, universe, existingBody, objectData, directory, disposition);
                else
                    body = CreateBody(primaryName, parentSystem, universe, existingBody, objectData, directory, disposition, bodyType);

                if (body != nullptr)
                {
                    body->loadCategories(objectData, disposition, directory.string());
                    if (disposition == DataDisposition::Add)
                        for (const auto& name : names)
                            body->addAlias(name);
                }
            }
        }
        else if (itemType == "AltSurface")
        {
            Surface* surface = new Surface();
            surface->color = Color(1.0f, 1.0f, 1.0f);
            FillinSurface(objectData, surface, directory);
            if (parent.body() != nullptr)
                parent.body()->addAlternateSurface(primaryName, surface);
            else
                sscError(tokenizer, _("bad alternate surface"));
        }
        else if (itemType == "Location")
        {
            if (parent.body() != nullptr)
            {
                Location* location = CreateLocation(objectData, parent.body());
                location->loadCategories(objectData, disposition, directory.string());
                if (location != nullptr)
                {
                    location->addName(primaryName);
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
                fmt::fprintf(cerr, _("parent body '%s' of location '%s' not found.\n"), parentName, primaryName);
            }
        }
        delete objectDataValue;
    }

    // TODO: Return some notification if there's an error parsing the file
    return true;
}


SolarSystem::SolarSystem(Star* _star) :
    star(_star),
    planets(nullptr),
    frameTree(nullptr)
{
    planets = new PlanetarySystem(star);
    frameTree = new FrameTree(star);
}

SolarSystem::~SolarSystem()
{
    delete planets;
    delete frameTree;
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
    return star->getPosition().cast<float>();
}

PlanetarySystem* SolarSystem::getPlanets() const
{
    return planets;
}

FrameTree* SolarSystem::getFrameTree() const
{
    return frameTree;
}
