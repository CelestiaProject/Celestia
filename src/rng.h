// rng.h

#ifndef _RNG_H_
#define _RNG_H_

class RandomNumberGenerator
{
public:
    RandomNumberGenerator();
    RandomNumberGenerator(long s);

    void seed(long);

    uint32 randomInt();
    float randomFloat();
    double randomeDouble();

private:
    uint32 state;
};

#endif // _RNG_H_
