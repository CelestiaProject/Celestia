#ifndef _HELPER_H_
#define _HELPER_H_

class Body;
class Selection;

class Helper
{
    public:
        static bool hasPrimary(Body* body);
        static Selection getPrimary(Body *body);

    private:
        static bool hasPrimaryStar(Body *body);
        static bool hasPrimaryBody(Body *body, int classification);
        static bool hasBarycenter(Body *body);
};

#endif