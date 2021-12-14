// asterism.h
//
// Copyright (C) 2001-2008, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELENGINE_ASTERISM_H_
#define _CELENGINE_ASTERISM_H_

#include <string>
#include <string_view>
#include <vector>
#include <iostream>
#include <celutil/color.h>

class StarDatabase;

class Asterism
{
 public:
    Asterism(std::string_view);
    ~Asterism() = default;
    Asterism() = delete;
    Asterism(const Asterism&) = delete;
    Asterism(Asterism&&) = delete;
    Asterism& operator=(const Asterism&) = delete;
    Asterism& operator=(Asterism&&) = delete;

    typedef std::vector<Eigen::Vector3f> Chain;

    std::string getName(bool i18n = false) const;
    int getChainCount() const;
    const Chain& getChain(int) const;

    bool getActive() const;
    void setActive(bool _active);

    Color getOverrideColor() const;
    void setOverrideColor(const Color &c);
    void unsetOverrideColor();
    bool isColorOverridden() const;

    void addChain(Chain&);

 private:
    std::string name;
    std::string i18nName;
    std::vector<Chain*> chains;
    Color color;

    bool active             { true };
    bool useOverrideColor   { false };
};

typedef std::vector<Asterism*> AsterismList;

AsterismList* ReadAsterismList(std::istream&, const StarDatabase&);

#endif // _CELENGINE_ASTERISM_H_
