// gui.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "gl.h"
#include "gui.h"

using namespace std;
using namespace gui;


/**** GraphicsContext ****/

GraphicsContext::GraphicsContext() :
    font(NULL)
{
}

TexFont* GraphicsContext::getFont() const
{
    return font;
}

void GraphicsContext::setFont(TexFont* _font)
{
    font = _font;
}


/**** Component ****/

Component::Component() :
    position(0, 0),
    size(0, 0),
    parent(NULL)
{
}

Point2f Component::getPosition() const
{
    return position;
}

Vec2f Component::getSize() const
{
    return size;
}

void Component::reshape(Point2f _position, Vec2f _size)
{
    position = _position;
    size = _size;
}

Component* Component::getParent() const
{
    return parent;
}

void Component::setParent(Component* c)
{
    parent = c;
}


/**** Container ****/

Container::Container() : Component()
{
}

int Container::getComponentCount() const
{
    return components.size();
}

Component* Container::getComponent(int n) const
{
    if (n >= 0 && n < components.size())
        return components[n];
    else
        return NULL;
}

void Container::addComponent(Component* c)
{
    // ASSERT(c->getParent() == NULL)
    components.insert(components.end(), c);
    c->setParent(this);
}

void Container::render(GraphicsContext& gc)
{
    glPushMatrix();
    glTranslatef(getPosition().x, getPosition().y, 0);
    vector<Component*>::iterator iter = components.begin();
    while (iter != components.end())
    {
        (*iter)->render(gc);
        iter++;
    }
    glPopMatrix();
}


/**** Button ****/

Button::Button(string _label) : label(_label)
{
}

string Button::getLabel() const
{
    return label;
}

void Button::setLabel(string _label)
{
    label = _label;
}

void Button::render(GraphicsContext& gc)
{
    glBegin(GL_LINE_LOOP);
    Point2f pos = getPosition();
    Vec2f sz = getSize();
    glVertex3f(pos.x,        pos.y,        0);
    glVertex3f(pos.x + sz.x, pos.y,        0);
    glVertex3f(pos.x + sz.x, pos.y + sz.y, 0);
    glVertex3f(pos.x,        pos.y + sz.y, 0);
    glEnd();

    int maxAscent = 0, maxDescent = 0, stringWidth = 0;
    txfGetStringMetrics(gc.getFont(), label,
                        stringWidth, maxAscent, maxDescent);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, gc.getFont()->texobj);
    glPushMatrix();
    glTranslatef(pos.x + (sz.x - stringWidth) / 2,
                 pos.y + (sz.y - (maxDescent)) / 2,
                 0);
    txfRenderString(gc.getFont(), label);
    glPopMatrix();
    glDisable(GL_TEXTURE_2D);
}

