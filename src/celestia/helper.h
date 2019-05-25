#ifndef _HELPER_H_
#define _HELPER_H_

#include <string>

class Body;
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
    static bool hasPrimaryBody(const Body* body, int classification);
    static bool hasBarycenter(const Body* body);
};

#endif
