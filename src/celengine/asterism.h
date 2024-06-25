// asterism.h
//
// Copyright (C) 2001-2008, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

#include <Eigen/Core>

#include <celutil/color.h>

class StarDatabase;

class Asterism
{
public:
    using Chain = std::vector<Eigen::Vector3f>;

    Asterism(std::string&&, std::vector<Chain>&&);
    ~Asterism() = default;

    Asterism(const Asterism&) = delete;
    Asterism(Asterism&&) noexcept = default;
    Asterism& operator=(const Asterism&) = delete;
    Asterism& operator=(Asterism&&) noexcept = default;

    std::string_view getName(bool i18n = false) const;
    int getChainCount() const;
    const Chain& getChain(int) const;

    bool getActive() const;
    void setActive(bool _active);

    Color getOverrideColor() const;
    void setOverrideColor(const Color &c);
    void unsetOverrideColor();
    bool isColorOverridden() const;

    const Eigen::Vector3f& averagePosition() const;

private:
    std::string m_name;
#ifdef ENABLE_NLS
    std::string m_i18nName;
#endif
    std::vector<Chain> m_chains;
    Eigen::Vector3f m_averagePosition{ Eigen::Vector3f::Zero() };
    Color m_color;

    bool m_active           { true };
    bool m_useOverrideColor { false };
};

using AsterismList = std::vector<Asterism>;

std::unique_ptr<AsterismList> ReadAsterismList(std::istream&, const StarDatabase&);
