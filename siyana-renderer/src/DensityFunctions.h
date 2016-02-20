#ifndef DENSITYFUNCTIONS_H_INCLUDED
#define DENSITYFUNCTIONS_H_INCLUDED

#define __OPENCL_HOST__
    #include "Utilities.h"
#undef __OPENCL_HOST__

//**uses the libnoise library for generating complex coherent noise**//

float PerlinNoise(const float3& p);
float RidgedTerrain(const float3& p);
float Plane(const float3& p);
float Sphere(const float3& o, float radius, const float3& p);
float Terrain(const float3& p);

#endif // DENSITYFUNCTIONS_H_INCLUDED
