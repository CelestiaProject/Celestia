// perlin.h

#pragma once

#include <Eigen/Core>


extern float noise(float vec[], int len);

extern float noise1(float arg);
extern float noise2(const float vec[]);
extern float noise3(const float vec[]);

extern float turbulence(const float v[], float freq);
extern float turbulence(const Eigen::Vector2f& p, float freq);
extern float turbulence(const Eigen::Vector3f& p, float freq);
extern float fractalsum(const float v[], float freq);
extern float fractalsum(const Eigen::Vector2f& p, float freq);
extern float fractalsum(const Eigen::Vector3f& p, float freq);
