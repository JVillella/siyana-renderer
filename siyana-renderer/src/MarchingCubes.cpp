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

#include "MarchingCubes.h"
#include "MCLookupTables.h"
#include <string.h>
#include <iostream>
#include "DensityFunctions.h"
using namespace std;

//boost
#include <boost/thread/thread.hpp>
#include <boost/asio.hpp>

using namespace boost;
using namespace asio;

#define M_1_UV_SCALE 0.2f //bigger num = larger texture appearance

extern Camera cam; //for centering terrain around eye

struct Voxel {
	float3 pos[8];
	float density[8];
};

//------------------------------------------Look Ups
static int3 RELATIVE_CORNER_POS[8] = {
    int3(0, 0, 0), //0
    int3(1, 0, 0), //1
    int3(1, 0, 1), //2
    int3(0, 0, 1), //3
    int3(0, 1, 0), //4
    int3(1, 1, 0), //5
    int3(1, 1, 1), //6
    int3(0, 1, 1)  //7
};

//0 means x-edges, 1 means y-edges, 2 means z-edges
//case 0: return 0; case 1: return 2; case 2: return 0;
//case 3: return 2; case 4: return 0; case 5: return 2;
//case 6: return 0; case 7: return 2; case 8: return 1;
//case 9: return 1; case 10:return 1; case 11:return 1;
static int EDGE_TYPE[12] = {0, 2, 0, 2, 0, 2, 0, 2, 1, 1, 1, 1};

static int3 EDGE_OFFSETS[12] = {
    int3(0, 0, 0), //0 //x-axis
    int3(1, 0, 0), //1 //y-axis
    int3(0, 0, 1), //2 //x-axis
    int3(0, 0, 0), //3 //y-axis
    int3(0, 1, 0), //4 //x-axis
    int3(1, 1, 0), //5 //y-axis
    int3(0, 1, 1), //6 //x-axis
    int3(0, 1, 0), //7 //y-axis
    int3(0, 0, 0), //8 //z-axis
    int3(1, 0, 0), //9 //z-axis
    int3(1, 0, 1), //10//z-axis
    int3(0, 0, 1)  //11//z-axis
};

//------------------------------------------Utilities
static inline
float DensityFunction(const float3& pos) {
    //return Plane(pos);
    return Terrain(pos);
}

//Linear interpolation
static inline
float3 Lerp(const float3& vert0, const float3& vert1, float val0, float val1) {
    return vert0 + (vert1 - vert0) * ((-val0) / (val1 - val0));
    //OLD//return vert0 + (-val0) * (vert1 - vert0) / (val1 - val0);
}

//TODO [23:12] <LordCrc> say i is the index "behind", ie x-1 and j is the index in front, ie x+1, but i and j have been clamped using min/max
//[23:12] <LordCrc> then you should divide by (j-i)

//Our density value function - creates the field we are polygonizing
static inline
float3 GetVertexNormal(const int3& vc, int edge_idx, const float*** densities, const int3& num_voxels) {
    //determine grid endpoints where the normals need to be calculated at
    //gp0 = grid position 0
    int3 gp0 = vc, gp1 = vc;
    switch(edge_idx) {
        case 0: gp0 += RELATIVE_CORNER_POS[0]; gp1 += RELATIVE_CORNER_POS[1]; break;
        case 1: gp0 += RELATIVE_CORNER_POS[1]; gp1 += RELATIVE_CORNER_POS[2]; break;
        case 2: gp0 += RELATIVE_CORNER_POS[2]; gp1 += RELATIVE_CORNER_POS[3]; break;
        case 3: gp0 += RELATIVE_CORNER_POS[3]; gp1 += RELATIVE_CORNER_POS[0]; break;
        case 4: gp0 += RELATIVE_CORNER_POS[4]; gp1 += RELATIVE_CORNER_POS[5]; break;
        case 5: gp0 += RELATIVE_CORNER_POS[5]; gp1 += RELATIVE_CORNER_POS[6]; break;
        case 6: gp0 += RELATIVE_CORNER_POS[6]; gp1 += RELATIVE_CORNER_POS[7]; break;
        case 7: gp0 += RELATIVE_CORNER_POS[7]; gp1 += RELATIVE_CORNER_POS[4]; break;
        case 8: gp0 += RELATIVE_CORNER_POS[0]; gp1 += RELATIVE_CORNER_POS[4]; break;
        case 9: gp0 += RELATIVE_CORNER_POS[1]; gp1 += RELATIVE_CORNER_POS[5]; break;
        case 10:gp0 += RELATIVE_CORNER_POS[2]; gp1 += RELATIVE_CORNER_POS[6]; break;
        case 11:gp0 += RELATIVE_CORNER_POS[3]; gp1 += RELATIVE_CORNER_POS[7]; break;
    }
    //determine the normals at the two gridpoints
    //gn0 = grid normal 0; gn1 = grid normal 1; (df/dx, df/dy, df/dz)
    int min_gp0x = max(gp0.x - 1, 0); int max_gp0x = min(gp0.x + 1, num_voxels.x);
    int min_gp0y = max(gp0.y - 1, 0); int max_gp0y = min(gp0.y + 1, num_voxels.y);
    int min_gp0z = max(gp0.z - 1, 0); int max_gp0z = min(gp0.z + 1, num_voxels.z);
    float3 gn0(densities[gp0.z][gp0.y][max_gp0x] - densities[gp0.z][gp0.y][min_gp0x],
               densities[gp0.z][max_gp0y][gp0.x] - densities[gp0.z][min_gp0y][gp0.x],
               densities[max_gp0z][gp0.y][gp0.x] - densities[min_gp0z][gp0.y][gp0.x]);
    gn0 = normalize(gn0);
    if(gn0.y < 0.f) gn0 *= -1.f; //flip if needed

    int min_gp1x = max(gp1.x - 1, 0); int max_gp1x = min(gp1.x + 1, num_voxels.x);
    int min_gp1y = max(gp1.y - 1, 0); int max_gp1y = min(gp1.y + 1, num_voxels.y);
    int min_gp1z = max(gp1.z - 1, 0); int max_gp1z = min(gp1.z + 1, num_voxels.z);
    float3 gn1(densities[gp0.z][gp0.y][max_gp1x] - densities[gp0.z][gp0.y][min_gp1x],
               densities[gp0.z][max_gp1y][gp0.x] - densities[gp0.z][min_gp1y][gp0.x],
               densities[max_gp1z][gp0.y][gp0.x] - densities[min_gp1z][gp0.y][gp0.x]);
    gn1 = normalize(gn1);
    if(gn1.y < 0.f) gn1 *= -1.f; //flip if needed

    return normalize(Lerp(gn0, gn1, densities[gp0.z][gp0.y][gp0.x], densities[gp1.z][gp1.y][gp1.x]));
}

//------------------------------------------Marching Cubes Algorithm
//TriangulateVoxelData(...) determines where to placed triangles (maximum 5)
//depending on sign and value of the scalar field at passed voxel's corners.
//vc = absolute voxel position (integers)
static void TriangulateVoxelData(const Voxel& v, const int3& vc, const float*** densities,
    int*** xedge_cache, int*** yedge_cache, int*** zedge_cache, vector<Triangle>* tris,
    vector<float3>* verts, vector<float3>* normals, vector<float2>* uvs, const int3& num_voxels) {
    //***determine case number (a.k.a voxel index)***
    //determine corners that are inside the terrain
    int voxel_index = 0;
    if(v.density[0] > 0.f) voxel_index |= 1;
    if(v.density[1] > 0.f) voxel_index |= 2;
    if(v.density[2] > 0.f) voxel_index |= 4;
    if(v.density[3] > 0.f) voxel_index |= 8;
    if(v.density[4] > 0.f) voxel_index |= 16;
    if(v.density[5] > 0.f) voxel_index |= 32;
    if(v.density[6] > 0.f) voxel_index |= 64;
    if(v.density[7] > 0.f) voxel_index |= 128;

    if(EDGE_TABLE[voxel_index] == 0) {
        return; //voxel completely in or out of surface
    }
    if(EDGE_TABLE[voxel_index] == 255) {
        return; //voxel completely in or out of surface
    }

    //determine vertices
    //there are 12 vertices because they are found on the edge of a voxel and there are 12 edges
    float3 lerp_verts[12];

    //bit operators...works on each bit of binary number
    if(EDGE_TABLE[voxel_index] & 1)
        lerp_verts[0] = Lerp(v.pos[0], v.pos[1], v.density[0], v.density[1]);
    if(EDGE_TABLE[voxel_index] & 2)
        lerp_verts[1] = Lerp(v.pos[1], v.pos[2], v.density[1], v.density[2]);
    if(EDGE_TABLE[voxel_index] & 4)
        lerp_verts[2] = Lerp(v.pos[2], v.pos[3], v.density[2], v.density[3]);
    if(EDGE_TABLE[voxel_index] & 8)
        lerp_verts[3] = Lerp(v.pos[3], v.pos[0], v.density[3], v.density[0]);
    if(EDGE_TABLE[voxel_index] & 16)
        lerp_verts[4] = Lerp(v.pos[4], v.pos[5], v.density[4], v.density[5]);
    if(EDGE_TABLE[voxel_index] & 32)
        lerp_verts[5] = Lerp(v.pos[5], v.pos[6], v.density[5], v.density[6]);
    if(EDGE_TABLE[voxel_index] & 64)
        lerp_verts[6] = Lerp(v.pos[6], v.pos[7], v.density[6], v.density[7]);
    if(EDGE_TABLE[voxel_index] & 128)
        lerp_verts[7] = Lerp(v.pos[7], v.pos[4], v.density[7], v.density[4]);
    if(EDGE_TABLE[voxel_index] & 256)
        lerp_verts[8] = Lerp(v.pos[0], v.pos[4], v.density[0], v.density[4]);
    if(EDGE_TABLE[voxel_index] & 512)
        lerp_verts[9] = Lerp(v.pos[1], v.pos[5], v.density[1], v.density[5]);
    if(EDGE_TABLE[voxel_index] & 1024)
        lerp_verts[10]= Lerp(v.pos[2], v.pos[6], v.density[2], v.density[6]);
    if(EDGE_TABLE[voxel_index] & 2048)
        lerp_verts[11]= Lerp(v.pos[3], v.pos[7], v.density[3], v.density[7]);

    //***construct triangles***
    for(int i = 0; TRIANGLE_TABLE[voxel_index][i] != -1; i += 3) {
        //tris
        Triangle tri;
        float3 first_vert;
        for (int j = 0; j < 3; j++) { //for each vert
            int edge_idx = TRIANGLE_TABLE[voxel_index][i + j];
            int edge_type = EDGE_TYPE[edge_idx]; //x, y, or z axis we are on
            int3 e = vc + EDGE_OFFSETS[edge_idx];

            if(edge_type == 0) {
                if(xedge_cache[e.z][e.y][e.x] == -1) {
                    xedge_cache[e.z][e.y][e.x] = verts->size(); //update cache
                    verts->push_back(lerp_verts[edge_idx]);
                    normals->push_back(GetVertexNormal(vc, edge_idx, densities, num_voxels));
                    uvs->push_back(float2(0, 0)); //filled later. allocating for now
                }
                tri.vi[j] = xedge_cache[e.z][e.y][e.x];
            } else if(edge_type == 1) {
                if(yedge_cache[e.z][e.y][e.x] == -1) {
                    yedge_cache[e.z][e.y][e.x] = verts->size(); //update cache
                    verts->push_back(lerp_verts[edge_idx]);
                    normals->push_back(GetVertexNormal(vc, edge_idx, densities, num_voxels));
                    uvs->push_back(float2(0, 0)); //filled later

                }
                tri.vi[j] = yedge_cache[e.z][e.y][e.x];
            } else { //edge_type == 2
                if(zedge_cache[e.z][e.y][e.x] == -1) {
                    zedge_cache[e.z][e.y][e.x] = verts->size(); //update cache
                    verts->push_back(lerp_verts[edge_idx]);
                    normals->push_back(GetVertexNormal(vc, edge_idx, densities, num_voxels));
                    uvs->push_back(float2(0, 0)); //filled later                }
                }
                tri.vi[j] = zedge_cache[e.z][e.y][e.x];
            }
            uvs->at(tri.vi[j]) = float2(verts->at(tri.vi[j]).x * M_1_UV_SCALE,
                                        verts->at(tri.vi[j]).z * M_1_UV_SCALE);
        }
        tris->push_back(tri);
    }
}

//these functions will never access the same vertex. synchronization is not needed
static inline
void ScheduleAcrossX(const int const_x, const float3& voxel_size, int num_verty,
    int num_vertz, float*** vert_densities, const float3& offset, io_service* thread_pool) {
    //
    for(int z = 0; z < num_vertz; z++) {
        for(int y = 0; y < num_verty; y++) {
            float3 corner(const_x * voxel_size.x, y * voxel_size.y, z * voxel_size.z);
            corner += offset;
            vert_densities[z][y][const_x] = DensityFunction(corner);
        }
    }
}

static inline
void ScheduleAcrossY(const int const_y, const float3& voxel_size, int num_vertx,
    int num_vertz, float*** vert_densities, const float3& offset, io_service* thread_pool) {
    //
    for(int z = 0; z < num_vertz; z++) {
        for(int x = 0; x < num_vertx; x++) {
            float3 corner(x * voxel_size.x, const_y * voxel_size.y, z * voxel_size.z);
            corner += offset;
            vert_densities[z][const_y][x] = DensityFunction(corner);
        }
    }
}

static inline
void ScheduleAcrossZ(const int const_z, const float3& voxel_size, int num_vertx,
    int num_verty, float*** vert_densities,  const float3& offset, io_service* thread_pool) {
    //
    for(int y = 0; y < num_verty; y++) {
        for(int x = 0; x < num_vertx; x++) {
            float3 corner(x * voxel_size.x, y * voxel_size.y, const_z * voxel_size.z);
            corner += offset;
            vert_densities[const_z][y][x] = DensityFunction(corner);
        }
    }
}

void GenerateTerrain(Mesh* mesh, const float3& voxel_size, const int3& num_voxels) {
    cout<<"About to setup thread pool"<<endl;
    io_service thread_pool;
    //work object informs thread_pool when work starts and finishes
    //so its run() will not exit while work is being done...keeps it alive
    io_service::work work(thread_pool);
    thread_group threads; //collection of threads that will chip away at tasks in queue
    //need to subtract one because a seperate thread will be calling this. otherwise it would have
    //to wait for a free spot to begin/other way around with these threads needing to wait
    for(int i = 0; i < max(1, (int)thread::hardware_concurrency() - 1); i++) { //add threads to group
        threads.create_thread(bind(&io_service::run, &thread_pool));
    }

    //offset for centering terrain around camera eye
    int num_vertx = num_voxels.x + 1;
    int num_verty = num_voxels.y + 1;
    int num_vertz = num_voxels.z + 1;
    float3 offset(-num_vertx, -num_verty, -num_vertz);
    offset *= voxel_size;
    offset *= 0.5f;
    offset += cam.eye;
    offset.y = 0; //dont offset in the y direction (so when player is fex +200y still see terrain)

    //allocate memory for each vertex density - put into dynamic 3D array
    float*** vert_densities;
    vert_densities = new float**[num_vertz];
    //allocate memory for storing vert densities
    for(int z = 0; z < num_vertz; z++) {
        vert_densities[z] = new float*[num_verty];
        for(int y = 0; y < num_verty; y++) {
            vert_densities[z][y] = new float[num_vertx];
        }
    }

    //allocate memory for edge/vert indices (see below comments)
    int*** xedge_cache = new int**[num_vertz]; //vertex indices for vertex on each x-axis
    int*** yedge_cache = new int**[num_vertz]; //vertex indices for vertex on each y-axis
    int*** zedge_cache = new int**[num_vertz]; //vertex indices for vertex on each z-axis
    //allocate mem
    for(int z = 0; z < num_vertz; z++) {
        xedge_cache[z] = new int*[num_verty];
        yedge_cache[z] = new int*[num_verty];
        zedge_cache[z] = new int*[num_verty];
        for(int y = 0; y < num_verty; y++) {
            xedge_cache[z][y] = new int[num_vertx];
            yedge_cache[z][y] = new int[num_vertx];
            zedge_cache[z][y] = new int[num_vertx];
        }
    }
    //set to no index (-1)
    for(int z = 0; z < num_vertz; z++) {
        for(int y = 0; y < num_verty; y++) {
            for(int x = 0; x < num_vertx; x++) {
                xedge_cache[z][y][x] = -1;
                yedge_cache[z][y][x] = -1;
                zedge_cache[z][y][x] = -1;
            }
        }
    }

    cout<<"Enqueueing tasks to thread pool"<<endl;
    //divide work to each thread
    int max_axis = Max(num_vertx, num_verty, num_vertz); //work divided along this axis
    if(max_axis == 0) { //x
        for(int x = 0; x < num_vertx; x++) {
            thread_pool.post(bind(ScheduleAcrossX, x, voxel_size, num_verty, num_vertz, vert_densities, offset, &thread_pool));
        }
    } else if(max_axis == 1) { //y
        for(int y = 0; y < num_verty; y++) {
            thread_pool.post(bind(ScheduleAcrossY, y, voxel_size, num_vertx, num_vertz, vert_densities, offset, &thread_pool));
        }
    } else { //z
        for(int z = 0; z < num_vertz; z++) {
            thread_pool.post(bind(ScheduleAcrossZ, z, voxel_size, num_vertx, num_verty, vert_densities, offset, &thread_pool));
        }
    }

    cout<<"About to wait for thread pool to finish"<<endl;
    //wait till all threads have been completed. then proceed
    thread_pool.stop();
    threads.join_all();

    cout<<"Polygonizing scalar field now"<<endl;
    //polygonize each voxel
    for(int z = 0; z < num_voxels.z; z++) {
        for(int y = 0; y < num_voxels.y; y++) {
            for(int x = 0; x < num_voxels.x; x++) {
            	//for each voxel corner
            	Voxel vox;
            	for(int i = 0; i < 8; i++) {
	                vox.pos[i] = voxel_size * (float3(x, y, z) + RELATIVE_CORNER_POS[i]) + offset;
	                //vox.density[i] = DensityFunction(vox.pos[i]);
	                int3 idx = int3(x, y, z) + RELATIVE_CORNER_POS[i];
	                vox.density[i] = vert_densities[idx.z][idx.y][idx.x];
            	}

            	//polygonize passed voxel with density values at each vertex
            	TriangulateVoxelData(vox, int3(x, y, z), (const float***)vert_densities, xedge_cache,
                    yedge_cache, zedge_cache, &mesh->triangles, &mesh->vertices, &mesh->normals, &mesh->uvs, num_voxels);
            }
        }
    }


    cout<<"Cleaning up vert density array"<<endl;
    //clean up 3D vert density array
    for(int z = 0; z < num_vertz; z++) {
        for(int y = 0; y < num_verty; y++) {
            delete[] vert_densities[z][y]; //delete x dyn mem
            delete[] xedge_cache[z][y]; //delete x dyn mem
            delete[] yedge_cache[z][y]; //delete x dyn mem
            delete[] zedge_cache[z][y]; //delete x dyn mem
        }
        delete[] vert_densities[z]; //delete y dyn mem
        delete[] xedge_cache[z]; //delete y dyn mem
        delete[] yedge_cache[z]; //delete y dyn mem
        delete[] zedge_cache[z]; //delete y dyn mem
    }
    delete[] vert_densities;
    delete[] xedge_cache;
    delete[] yedge_cache;
    delete[] zedge_cache;

    vert_densities = NULL;
    xedge_cache = NULL;
    yedge_cache = NULL;
    zedge_cache = NULL;

    cout<<"MC Terrain:"<<endl;
    cout<<"\tNum Tris: "<<mesh->triangles.size()<<endl;
    cout<<"\tNum Verts: "<<mesh->vertices.size()<<endl;
}
