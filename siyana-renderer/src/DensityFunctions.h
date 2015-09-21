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
