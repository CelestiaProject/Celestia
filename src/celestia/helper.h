#pragma once

#include <string>

#include <celengine/body.h>

class Selection;
class Renderer;

class Helper
{
 public:
    static bool hasPrimary(const Body* body);
    static Selection getPrimary(const Body* body);

    static std::string getRenderInfo(const Renderer*);

 private:
    static bool hasPrimaryStar(const Body* body);
    static bool hasPrimaryBody(const Body* body, BodyClassification classification);
    static bool hasBarycenter(const Body* body);
};
