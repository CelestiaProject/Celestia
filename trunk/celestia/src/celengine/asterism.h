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
#include <vector>
#include <iostream>
#include <celengine/stardb.h>

class Asterism
{
 public:
    Asterism(std::string);
    ~Asterism();

    typedef std::vector<Eigen::Vector3f> Chain;

    std::string getName(bool i18n = false) const;
    int getChainCount() const;
    const Chain& getChain(int) const;

	bool getActive() const;
	void setActive(bool _active);

    Color getOverrideColor() const;
	void setOverrideColor(Color c);
	void unsetOverrideColor();
	bool isColorOverridden() const;

    void addChain(Chain&);

 private:
    std::string name;
    std::string i18nName;
    std::vector<Chain*> chains;

	bool active;
	bool useOverrideColor;
    Color color;
};

typedef std::vector<Asterism*> AsterismList;

AsterismList* ReadAsterismList(std::istream&, const StarDatabase&);

#endif // _CELENGINE_ASTERISM_H_



