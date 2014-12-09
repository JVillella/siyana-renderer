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

#ifndef TRIANGLEMESH_H_INCLUDED
#define TRIANGLEMESH_H_INCLUDED

#define __OPENCL_HOST__
    #include "Utilities.h"
#undef __OPENCL_HOST__

#include <iostream>
#include <vector>
#include <math.h>
#include "rply.h" //for parsing .ply files

using namespace std;

inline static
float3 SolidTriangleNormal(const float3& v0, const float3& v1, const float3& v2) {
    return normalize(cross(v1 - v0, v2 - v0)); //Return unit normal
}

//Only for building. NOT for OpenCL
class Mesh { //intermediate triangle mesh
public:
    //Data
    vector<Triangle> triangles;
    vector<float3> vertices;
    vector<float3> normals;
    vector<float2> uvs;
    Material mat;
    //Functions
    //NOTE: Only for debugging!
    void AddTriangle(const float3& v0, const float3& v1, const float3& v2) {
        Triangle tri;
        tri.vi.x = vertices.size();
        tri.vi.y = vertices.size() + 1;
        tri.vi.z = vertices.size() + 2;
        triangles.push_back(tri);

        vertices.push_back(v0);
        vertices.push_back(v1);
        vertices.push_back(v2);

        float3 normal = SolidTriangleNormal(v0, v1, v2);
        normals.push_back(normal);
        normals.push_back(normal);
        normals.push_back(normal);

        uvs.push_back(float2(0,0));
        uvs.push_back(float2(0,0));
        uvs.push_back(float2(0,0));
    }
};

void SetMesh(Mesh* mesh, const char* filename);

#endif // TRIANGLEMESH_H_INCLUDED
