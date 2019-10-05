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
#include <celutil/color.h>
#include "astrodb.h"
#include "shadermanager.h"
#include "vertexobject.h"

class Renderer;
class AsterismList;

class Asterism
{
 public:
    Asterism(std::string);
    ~Asterism() = default;

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

    // total number of vertexes in the asterism
    uint16_t vertex_count{ 0 };

    bool active{ true };
    bool useOverrideColor{ false };
    Color color;

    friend class AsterismList;
};

class AsterismList : public std::vector<Asterism*>
{
 public:
    void render(const Color& color, const Renderer& renderer);

 private:
    void cleanup();
    void prepare();

    GLfloat *vtx_buf{ nullptr };
    GLsizei vtx_num{ 0 };
    bool prepared{ false };
    celgl::VertexObject m_vo{GL_ARRAY_BUFFER, 0, GL_STATIC_DRAW};

    ShaderProperties shadprop{ ShaderProperties::UniformColor };
};

AsterismList* ReadAsterismList(std::istream&, const AstroDatabase&);

#endif // _CELENGINE_ASTERISM_H_
