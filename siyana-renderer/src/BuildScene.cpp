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

#include "BuildScene.h"
#include "BuildMesh.h"
#include "MarchingCubes.h"
#include "Texture.h"
#include "KdTree.h"
#include <iostream>
using namespace std;

extern TriangleMesh* meshes;
extern int num_meshes;

extern Triangle* triangles;
extern int total_scene_tris;

extern float3* vertices;
extern float3* normals;
extern float2* uvs;
extern int num_vertices;

extern unsigned char* texture_data;
extern int num_texture_bytes;

extern Texture environment_tex;
extern Camera cam;

//------------------------------------------Filescope Variables
namespace {
    vector<Mesh> mesh_vec;
    vector<unsigned char> texture_vec;
    int terrain_mesh_index;

    int terrain_verts_start; //same for norms and uvs
    int num_terrain_verts;
};

//------------------------------------------Helper Functions
static void SetMeshTexture(Mesh* mesh, float4 col) {
    SetTexture(&texture_vec, &mesh->mat.tex, col);
}

static void SetMeshTexture(Mesh* mesh, const char* filename) {
    SetTexture(&texture_vec, &mesh->mat.tex, filename);
}

static void SetEnvironmentTexture(const char* filename) {
    SetTexture(&texture_vec, &environment_tex, filename);
}

static void SetEnvironmentTexture(float4 col) {
    SetTexture(&texture_vec, &environment_tex, col);
}

static void AddMesh(const Mesh& mesh) {
    mesh_vec.push_back(mesh);
}

//use this instead of AddMesh for terrain. set's a terrain global index
static void AddTerrainMesh(const Mesh& terrain_mesh) {
    mesh_vec.push_back(terrain_mesh);

    //set globals
    terrain_mesh_index = mesh_vec.size() - 1;
    num_terrain_verts = mesh_vec[terrain_mesh_index].vertices.size();
}

static void PrintTypeSizes() {
    printf("ON HOST: Size of BBox %i\n", sizeof(BBox));
    printf("ON HOST: Size of KdNode %i\n", sizeof(KdNode));
    printf("ON HOST: Size of ToDoKdNode %i\n", sizeof(ToDoKdNode));
    printf("ON HOST: Size of IsectData %i\n", sizeof(IsectData));
    printf("ON HOST: Size of Triangle %i\n", sizeof(Triangle));
    printf("ON HOST: Size of TriangleMesh %i\n", sizeof(TriangleMesh));
    printf("ON HOST: Size of Material %i\n", sizeof(Material));
    printf("ON HOST: Size of Texture %i\n", sizeof(Texture));
}

//copy all vector data within this module into an opencl
//friendly format. i.e. static c-like memory (arrays). Such
//(global) memory is defined in main.cpp.
static void MoveDataToGlobal() {
    //meshes and triangles
    num_meshes = mesh_vec.size();
    meshes = new TriangleMesh[num_meshes];
    //get total number of triangles in the scene and allocate mem
    total_scene_tris = 0;
    for(int i = 0; i < num_meshes; i++) {
        num_vertices += mesh_vec[i].vertices.size();
        total_scene_tris += mesh_vec[i].triangles.size();
    }
    triangles = new Triangle[total_scene_tris];
    //copy vector data into array
    int running_tri_total = 0;
    for(int i = 0; i < num_meshes; i++) {
        meshes[i].start_index = running_tri_total;
        meshes[i].num_tris = mesh_vec[i].triangles.size();
        for(int j = 0; j < (int)mesh_vec[i].triangles.size(); j++) { //fill triangle array
            triangles[running_tri_total] = mesh_vec[i].triangles[j];
            running_tri_total++;
        }
        meshes[i].material = mesh_vec[i].mat;
    }
    //textures
    num_texture_bytes = texture_vec.size();
    texture_data = new unsigned char[num_texture_bytes];
    for(int i = 0; i < num_texture_bytes; i++) {
        texture_data[i] = texture_vec[i]; //copy over
    }
    //vertices, normals and uvs (they are all the same length)
    vertices = new float3[num_vertices];
    normals = new float3[num_vertices];
    uvs = new float2[num_vertices];
    int k = 0;
    for(int i = 0; i < num_meshes; i++) {
        for(int j = 0; j < mesh_vec[i].vertices.size(); j++) {
            vertices[k] = mesh_vec[i].vertices[j];
            normals[k] = mesh_vec[i].normals[j];
            uvs[k] = mesh_vec[i].uvs[j];
            k++;
        }
    }


    //offset tri.vi by number of local (to mesh) vertices
    k = 0;
    int vi_offset = 0; //offset for tri.vi necessary cause arrays stored locally
    for(int i = 0; i < num_meshes; i++) {
        for(int j = 0; j < mesh_vec[i].triangles.size(); j++) {
            triangles[k].vi.x += vi_offset;
            triangles[k].vi.y += vi_offset;
            triangles[k].vi.z += vi_offset;
            k++;
        }
        vi_offset += mesh_vec[i].vertices.size();
    }

    //set global terrain vertex start in vertex buffer
    terrain_verts_start = 0;
    for(int i = 0; i < terrain_mesh_index; i++) {
        terrain_verts_start += mesh_vec[i].vertices.size();
    }

    //clean up
    mesh_vec.erase(mesh_vec.begin(), mesh_vec.end()); //should also erase contained vecs
}

void SetupScene() {
//PrintTypeSizes();
    //--------------------------------------Camera
    //NOTE: dont change this...affects camera mouse movements and sensitivity
    //NOTE: Camera must be the first thing setup. e.x. terrain construction depends on it
    float3 origin(0, 15, 0);
    float3 focal(8, 15, 0);

    CameraSetupOrthoBasis(origin, focal);

    //--------------------------------------Environment Light
    SetEnvironmentTexture("textures/environment/outside.jpg");
    //SetEnvironmentTexture("textures/environment/uffizi.jpg");
    //SetEnvironmentTexture(float4(1,1,1,1));

    //--------------------------------------Set Terrain
    Mesh terrain;
    GenerateTerrain(&terrain, voxel_size, num_voxels);
    terrain.mat.emission = 0.f;
    //SetMeshTexture(&terrain, "textures/cracked_earth.jpg");
    SetMeshTexture(&terrain, "textures/grass.jpg");
    AddTerrainMesh(terrain);

    //--------------------------------------Set Meshes
    Mesh door;
    SetMesh(&door, "meshes/tower/door.ply");
    door.mat.emission = 0.f;
    SetMeshTexture(&door, "textures/tower/door.png");
    AddMesh(door);

    Mesh door_arch;
    SetMesh(&door_arch, "meshes/tower/door_arch.ply");
    door_arch.mat.emission = 0.f;
    SetMeshTexture(&door_arch, "textures/tower/stone_wall.png");
    AddMesh(door_arch);

    Mesh roof;
    SetMesh(&roof, "meshes/tower/roof.ply");
    roof.mat.emission = 0.f;
    SetMeshTexture(&roof, "textures/tower/copper_roof.jpg");
    AddMesh(roof);

    Mesh roof_tip;
    SetMesh(&roof_tip, "meshes/tower/roof_tip.ply");
    roof_tip.mat.emission = 0.f;
    SetMeshTexture(&roof_tip, "textures/tower/roof_tip.png");
    AddMesh(roof_tip);

    Mesh stairs;
    SetMesh(&stairs, "meshes/tower/stairs.ply");
    stairs.mat.emission = 0.f;
    SetMeshTexture(&stairs, "textures/tower/stone_wall.png");
    AddMesh(stairs);

    Mesh window_casing;
    SetMesh(&window_casing, "meshes/tower/window_casing.ply");
    window_casing.mat.emission = 0.f;
    SetMeshTexture(&window_casing, "textures/tower/stone_wall.png");
    AddMesh(window_casing);

    Mesh window;
    SetMesh(&window, "meshes/tower/window.ply");
    window.mat.emission = 0.f;
    SetMeshTexture(&window, "textures/tower/window.jpg");
    AddMesh(window);

    Mesh tower;
    SetMesh(&tower, "meshes/tower/tower.ply");
    tower.mat.emission = 0.f;
    SetMeshTexture(&tower, "textures/tower/stone_wall.png");
    AddMesh(tower);

    //--------------------------------------Must call at this position
    MoveDataToGlobal();
    cout<<"data in scene file moved to global for opencl"<<endl;
    cout<<"\tnum tris: "<<total_scene_tris<<endl;
    cout<<"\tnum meshes: "<<num_meshes<<endl;

    //--------------------------------------Setup Kd-Tree
    cout<<"about to build the global kd tree"<<endl;
    BuildKdTree();
    cout<<"built global kd tree ("<<num_kdnodes<<" nodes)"<<endl;
}

//called to update terrain and such in the background while the user is moving
void UpdateScene() {
    cout<<"UpdateScene() invoked"<<endl;

    //start determining size of new triangle array
    int new_num_tris = total_scene_tris - meshes[terrain_mesh_index].num_tris;
    int new_num_verts= num_vertices - num_terrain_verts;

    //generate new terrain mesh
    Mesh terrain;
    GenerateTerrain(&terrain, voxel_size, num_voxels);

    //allocate memory for new arrays
    new_num_tris += terrain.triangles.size();
    new_num_verts+= terrain.vertices.size();
    Triangle* new_triangles = new Triangle[new_num_tris];
    float3* new_vertices = new float3[new_num_verts];
    float3* new_normals = new float3[new_num_verts];
    float2* new_uvs = new float2[new_num_verts];

    //***copy over all the mesh data (tri, vert, etc.) before the terrain***
    int current_tri_index = 0;
    int terrain_start_index = meshes[terrain_mesh_index].start_index;

    for(int i = 0; i < terrain_start_index; i++) {
        new_triangles[current_tri_index] = triangles[current_tri_index];
        current_tri_index++;
    }
    int current_vert_index = 0;
    for(int i = 0; i < terrain_verts_start; i++) {
        new_vertices[current_vert_index] = vertices[i];
        new_normals[current_vert_index] = normals[i];
        new_uvs[current_vert_index] = uvs[i];
        current_vert_index++;
    }

    //**add in terrain data to new arrays***
    int vi_offset = current_vert_index;
    for(int i = 0; i < terrain.triangles.size(); i++) {
        new_triangles[current_tri_index] = terrain.triangles[i];
        new_triangles[current_tri_index].vi.x += vi_offset;
        new_triangles[current_tri_index].vi.y += vi_offset;
        new_triangles[current_tri_index].vi.z += vi_offset;
        current_tri_index++;
    }

    for(int i = 0; i < terrain.vertices.size(); i++) {
        new_vertices[current_vert_index] = terrain.vertices[i];
        new_normals[current_vert_index] = terrain.normals[i];
        new_uvs[current_vert_index] = terrain.uvs[i];
        current_vert_index++;
    }

    vi_offset = terrain.vertices.size() - num_terrain_verts; //update offset

    //***update terrain tri mesh***
    meshes[terrain_mesh_index].start_index = current_tri_index - terrain.triangles.size();
    meshes[terrain_mesh_index].num_tris = terrain.triangles.size();

    //***add back all the mesh data after the terrain mesh***
    for(int i = terrain_mesh_index + 1; i < num_meshes; i++) {
        int start_index = meshes[i].start_index; //old params..updates after
        int end_index = start_index + meshes[i].num_tris;
        meshes[i].start_index = current_tri_index; //set new mesh starting index
        for(int j = start_index; j < end_index; j++) { //for each tri of each mesh
            new_triangles[current_tri_index] = triangles[j];

            new_triangles[current_tri_index].vi.x += vi_offset;
            new_triangles[current_tri_index].vi.y += vi_offset;
            new_triangles[current_tri_index].vi.z += vi_offset;
            current_tri_index++;
        }
    }
    for(int i = terrain_verts_start + num_terrain_verts; i < num_vertices; i++) {
        new_vertices[current_vert_index] = vertices[i];
        new_normals[current_vert_index] = normals[i];
        new_uvs[current_vert_index] = uvs[i];
        current_vert_index++;
    }

    //***deallocate old mesh data and set pointer to new mem***
    delete[] triangles;
    delete[] vertices;
    delete[] normals;
    delete[] uvs;

    triangles = new_triangles;
    vertices = new_vertices;
    normals = new_normals;
    uvs = new_uvs;

    new_triangles = NULL;
    new_vertices = NULL;
    new_normals = NULL;
    new_uvs = NULL;

    //terrain_verts_start will never change but num terrain verts will
    num_terrain_verts = terrain.vertices.size(); //set new size
    total_scene_tris = new_num_tris; //set new total tris
    num_vertices = new_num_verts;

    //rebuild kd tree
    ResetKdTree();
    BuildKdTree();
}

void CleanScene() {
    //TODO clean up rest of data
    if(meshes != NULL) {
        delete[] meshes;
        meshes = NULL;
    }
    if(triangles != NULL) {
        delete[] triangles;
        triangles = NULL;
    }
    if(vertices != NULL) {
        delete[] vertices;
        vertices = NULL;
    }
    if(normals != NULL) {
        delete[] normals;
        normals = NULL;
    }
    if(uvs != NULL) {
        delete[] uvs;
        uvs = NULL;
    }
}
