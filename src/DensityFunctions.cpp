/************************************************************************
* Copyright (C) 2013  Julian Villella                                   *
*                                                                       *
* This file is part of Siyana Renderer                                  *
*                                                                       *
* This program is free software: you can redistribute it and/or modify  *
* it under the terms of the GNU General Public License as published by  *
* the Free Software Foundation, either version 3 of the License, or     *
* (at your option) any later version.                                   *
*                                                                       *
* This program is distributed in the hope that it will be useful,       *
* but WITHOUT ANY WARRANTY; without even the implied warranty of        *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
* GNU General Public License for more details.                          *
*                                                                       *
* You should have received a copy of the GNU General Public License     *
* along with this program.  If not, see <http://www.gnu.org/licenses/>. *
*                                                                       *
* Website: http://jvillella.com                                         *
*************************************************************************/

#include "DensityFunctions.h"
#include <iostream>
#include <noise/noise.h>

using namespace std;
using namespace noise;

//**uses the libnoise library for generating complex coherent noise**//

namespace {
    module::Perlin perlin_noise;
    module::RidgedMulti ridged_noise;
};

float Terrain(const float3& p) {
    //+1 so added final noise (after clamp) doesnt cause terrain to go underneath MC grid
    float base = -p.y + EPSILON;
    float hill_density = base;
    float3 input = p * 0.01f;

    ridged_noise.SetOctaveCount(4);
    ridged_noise.SetFrequency(0.75f);
    hill_density += (float)ridged_noise.GetValue(input.x, input.y, input.z)*15.f;
    //max is to force the plane to be at the bottom...so no empty base
    hill_density = max(hill_density, base);

    float ground_dens = base;
    perlin_noise.SetOctaveCount(6);
    perlin_noise.SetFrequency(1.f);
    ground_dens += perlin_noise.GetValue(input.x, input.y, input.z)*20.f + 20.f;
    ground_dens = max(ground_dens, base);

    return hill_density + ground_dens;
}
float RidgedTerrain(const float3& p) {
    return ridged_noise.GetValue(p.x, p.y, p.z);
}
float PerlinNoise(const float3& p) {
    return perlin_noise.GetValue(p.x, p.y, p.z);
}

float Plane(const float3& p) {
    return -p.y + EPSILON;
}

float Sphere(const float3& o, float radius, const float3& p) {
    return dot(p - o, p - o) - radius * radius;
}
