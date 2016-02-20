#ifndef TRIANGLEMESH_H_INCLUDED
#define TRIANGLEMESH_H_INCLUDED

#define __OPENCL_HOST__
    #include "Utilities.h"
#undef __OPENCL_HOST__

#include <iostream>
#include <vector>
#include <math.h>
#include <RPly/rply.h>

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
