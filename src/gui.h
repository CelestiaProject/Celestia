// gui.h
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _GUI_H_
#define _GUI_H_

#include <vector>
#include <string>
#include "vecmath.h"
#include "texfont.h"


namespace gui
{
    class GraphicsContext
    {
    public:
        GraphicsContext();
        TexFont* getFont() const;
        void setFont(TexFont*);

    private:
        TexFont* font;
    };


    class Component
    {
    public:
        Component();

        Point2f getPosition() const;
        Vec2f getSize() const;
        void reshape(Point2f, Vec2f);
        Component* getParent() const;
        virtual void render(GraphicsContext&) = 0;

        // protected:
        void setParent(Component*);

    private:
        Point2f position;
        Vec2f size;
        Component* parent;
    };


    class Container : public Component
    {
    public:
        Container();

        int getComponentCount() const;
        Component* getComponent(int) const;
        void addComponent(Component*);
        void render(GraphicsContext&);
        
    private:
        std::vector<Component*> components;
    };


    class Button : public Component
    {
    public:
        Button(std::string);

        std::string getLabel() const;
        void setLabel(std::string);
        void render(GraphicsContext&);

    private:
        std::string label;
    };

};

#endif // _GUI_H_
