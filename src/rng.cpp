// rng.cpp

#include <stdlib.h>
#include "celestia.h"
#include "rng.h"


static const uint32 gen = 1423130227u;


RandomNumberGenerator::RandomNumberGenerator() : state(0)
{
}


RandomNumberGenerator::RandomNumberGenerator(uint32) : state(s)
{
}


void RandomNumberGenerator::seed(long s)
{
    state = s;
}


RandomNumberGenerator::randomInt()
{
    state = gen * state + gen * 2;
    return state;
}


double RandomNumberGenerator::randomDouble()
{
    return (double) randomInt() / (double) ~((uint32) 1);
}


float RandomNumberGenerator::randomFloat()
{
    // Use randomDouble() in order to avoid round off errors that may result
    // from casting large integer values to floats.
    return (float) randomDouble();
}
