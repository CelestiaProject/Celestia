#ifndef _HELPER_H_
#define _HELPER_H_

class Body;
class Selection;

class Helper
{
 public:
    static bool hasPrimary(const Body* body);
    static Selection getPrimary(const Body* body);

 private:
    static bool hasPrimaryStar(const Body* body);
    static bool hasPrimaryBody(const Body* body, int classification);
    static bool hasBarycenter(const Body* body);
};

#endif