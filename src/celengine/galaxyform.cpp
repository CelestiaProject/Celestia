// galaxyform.cpp
//
// Copyright (C) 2001-present, the Celestia Development Team
// Original version by Chris Laurel, Fridger Schrempp, and Toti
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cassert>
#include <memory>

#include <celmath/randutils.h>
#include <celutil/logger.h>

#include "image.h"
#include "render.h"
#include "texture.h"
#include "galaxy.h"
#include "galaxyform.h"

namespace celestia::engine
{
namespace
{
constexpr unsigned int kIrrGalaxyPoints = 3500u;

std::optional<celestia::engine::GalacticForm>
buildGalacticForm(const fs::path& filename)
{
    GalacticForm::Blob b;
    GalacticForm::BlobVector galacticPoints;

    // Load templates in standard .png format
    int width, height, rgb, j = 0, kmin = 9;
    float h = 0.75f;
    std::unique_ptr<Image> img = LoadImageFromFile(filename);
    if (img == nullptr)
    {
        celestia::util::GetLogger()->error("The galaxy template *** {} *** could not be loaded!\n", filename);
        return std::nullopt;
    }
    width  = img->getWidth();
    height = img->getHeight();
    rgb    = img->getComponents();

    auto& rng = celmath::getRNG();
    rng.seed(1312);
    for (int i = 0; i < width * height; i++)
    {
        std::uint8_t value = img->getPixels()[rgb * i];
        if (value > 10)
        {
            auto idxf = static_cast<float>(i);
            auto wf = static_cast<float>(width);
            auto hf = static_cast<float>(height);
            float z = std::floor(idxf / wf);
            float x = (idxf - wf * z - 0.5f * (wf - 1.0f)) / wf;
            z  = (0.5f * (hf - 1.0f) - z) / hf;
            x  += celmath::RealDists<float>::SignedUnit(rng) * 0.008f;
            z  += celmath::RealDists<float>::SignedUnit(rng) * 0.008f;
            float r2 = x * x + z * z;

            float y;
            if (filename != "models/E0.png")
            {
                float y0 = 0.5f * Galaxy::kMaxSpiralThickness * std::sqrt(static_cast<float>(value)/256.0f) * std::exp(- 5.0f * r2);
                float B = (r2 > 0.35f) ? 1.0f: 0.75f; // the darkness of the "dust lane", 0 < B < 1
                float p0 = 1.0f - B * std::exp(-h * h); // the uniform reference probability, envelopping prob*p0.
                float yr, prob;
                do
                {
                    // generate "thickness" y of spirals with emulation of a dust lane
                    // in galctic plane (y=0)
                    yr = celmath::RealDists<float>::SignedUnit(rng) * h;
                    prob = (1.0f - B * std::exp(-yr * yr))/p0;
                } while (celmath::RealDists<float>::Unit(rng) > prob);
                b.brightness = static_cast<std::uint8_t>(value * prob);
                y = y0 * yr / h;
            }
            else
            {
                // generate spherically symmetric distribution from E0.png
                float yy, prob;
                do
                {
                    yy = celmath::RealDists<float>::SignedUnit(rng);
                    float ry2 = 1.0f - yy * yy;
                    prob = ry2 > 0 ? std::sqrt(ry2): 0.0f;
                } while (celmath::RealDists<float>::Unit(rng) > prob);
                y = yy * std::sqrt(0.25f - r2) ;
                b.brightness = value;
                kmin = 12;
            }

            b.position = Eigen::Vector3f(x, y, z);
            b.colorIndex = static_cast<std::uint8_t>(std::min(b.position.norm() * 511.0f, 255.0f));
            galacticPoints.push_back(b);
            j++;
        }
    }

    // sort to start with the galaxy center region (x^2 + y^2 + z^2 ~ 0), such that
    // the biggest (brightest) sprites will be localized there!
    std::sort(galacticPoints.begin(), galacticPoints.end(),
              [](const auto &b1, const auto &b2) { return b1.position.squaredNorm() < b2.position.squaredNorm(); });

    // reshuffle the galaxy points randomly...except the first kmin+1 in the center!
    // the higher that number the stronger the central "glow"
    std::shuffle(galacticPoints.begin() + kmin, galacticPoints.end(), celmath::getRNG());

    std::optional<celestia::engine::GalacticForm> galacticForm(std::in_place);
    galacticForm->blobs = std::move(galacticPoints);
    galacticForm->scale = Eigen::Vector3f::Ones();

    return galacticForm;
}

} // anonymous namespace

GalacticFormManager::GalacticFormManager()
{
    initializeStandardForms();
}

const GalacticForm*
GalacticFormManager::getForm(int form) const
{
    assert(form < static_cast<int>(galacticForms.size()));
    return galacticForms[form].has_value()
        ? &*galacticForms[form]
        : nullptr;
}

int
GalacticFormManager::getCustomForm(const fs::path& path)
{
    if (auto iter = customForms.find(path); iter != customForms.end())
    {
        return iter->second;
    }

    auto result = static_cast<int>(galacticForms.size());
    customForms[path] = result;
    galacticForms.push_back(buildGalacticForm(path));
    return result;
}

int
GalacticFormManager::getCount() const
{
    return static_cast<int>(galacticForms.size());
}

void
GalacticFormManager::initializeStandardForms()
{
    // Irregular Galaxies
    unsigned int galaxySize = kIrrGalaxyPoints, ip = 0;
    GalacticForm::Blob b;
    Eigen::Vector3f p;

    GalacticForm::BlobVector irregularPoints;
    irregularPoints.reserve(galaxySize);

    auto& rng = celmath::getRNG();
    while (ip < galaxySize)
    {
        p = Eigen::Vector3f(celmath::RealDists<float>::SignedUnit(rng),
                            celmath::RealDists<float>::SignedUnit(rng),
                            celmath::RealDists<float>::SignedUnit(rng));
        float r  = p.norm();
        if (r < 1)
        {
            Eigen::Vector3f p1(p.array() + 5.0f);
            float prob = (1.0f - r) * (celmath::fractalsum(p1, 8.0f) + 1.0f) * 0.5f;
            if (celmath::RealDists<float>::Unit(rng) < prob)
            {
                b.position   = p;
                b.brightness = std::uint8_t(64);
                b.colorIndex = static_cast<std::uint8_t>(std::min(r * 511.0f, 255.0f));
                irregularPoints.push_back(b);
                ++ip;
            }
        }
    }

    auto& irregularForm = galacticForms.emplace_back(std::in_place);
    irregularForm->blobs = std::move(irregularPoints);
    irregularForm->scale = Eigen::Vector3f::Constant(0.5f);

    // Spiral Galaxies, 7 classical Hubble types

    galacticForms.push_back(buildGalacticForm("models/S0.png"));
    galacticForms.push_back(buildGalacticForm("models/Sa.png"));
    galacticForms.push_back(buildGalacticForm("models/Sb.png"));
    galacticForms.push_back(buildGalacticForm("models/Sc.png"));
    galacticForms.push_back(buildGalacticForm("models/SBa.png"));
    galacticForms.push_back(buildGalacticForm("models/SBb.png"));
    galacticForms.push_back(buildGalacticForm("models/SBc.png"));

    // Elliptical Galaxies , 8 classical Hubble types, E0..E7,
    //
    // To save space: generate spherical E0 template from S0 disk
    // via rescaling by (1.0f, 3.8f, 1.0f).

    for (unsigned int eform = 0; eform <= 7; ++eform)
    {
        float ell = 1.0f - static_cast<float>(eform) / 8.0f;

        // note the correct x,y-alignment of 'ell' scaling!!
        // build all elliptical templates from rescaling E0
        auto ellipticalForm = buildGalacticForm("models/E0.png");
        if (ellipticalForm.has_value())
        {
            ellipticalForm->scale = Eigen::Vector3f(ell, ell, 1.0f);

            for (auto& blob : ellipticalForm->blobs)
            {
                blob.colorIndex = static_cast<std::uint8_t>(std::ceil(0.76f * static_cast<float>(blob.colorIndex)));
            }
        }

        galacticForms.push_back(std::move(ellipticalForm));
    }
}

GalacticFormManager* GalacticFormManager::get()
{
    static GalacticFormManager* galacticFormManager = new GalacticFormManager();
    return galacticFormManager;
}

} // namespace celestia::engine
