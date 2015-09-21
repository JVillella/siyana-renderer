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

#define __OPENCL_HOST__
    #include "Utilities.h"
    #include "BBox.h"
#undef __OPENCL_HOST__

#include "OpenCLUtilities.h"
#include "GLUTUtilities.h"
#include "BuildScene.h"

#include <iostream>
using namespace std;

#include <string>

//global vars
TriangleMesh* meshes = NULL;
int num_meshes = 0;

Triangle* triangles = NULL;
int total_scene_tris = 0;

float3* vertices = NULL;
float3* normals = NULL;
float2* uvs = NULL;
int num_vertices = 0;

KdNode* kdnodes = NULL;
int num_kdnodes = 0;
int* kdsorted_tri_indices = NULL;
int num_kdsorted_tri_indices = 0;

unsigned char* texture_data = NULL;
int num_texture_bytes = 0;

Texture environment_tex;
BBox scene_bbox;
Camera cam;

static void PressEnterToExit(void) {
    //cout<<endl<<"--- [Press enter to exit!] ---"<<endl;
    //cin.get();
}

int main(int argc, char** argv) {
    atexit(PressEnterToExit); //keeps cmd open

    chdir("./bin/");
    char str[1000];
    getcwd(str, 1000);
    cout<<"Current working directory: "<<endl;
    cout<<"\t"<<str<<endl;

    cout<<"Setting up scene..."<<endl;
    SetupScene();
    cout<<"Scene setup completed"<<endl;

    //called after exit(int) called last function is first to be called
    atexit(GLUTCleanUp);
    atexit(OpenCLCleanUp);
    atexit(CleanScene);

    //setup
    cout<<"Setting up GLUT"<<endl;
    SetupGLUT(argc, argv);
    cout<<"Setting up OpenCL"<<endl;
    SetupOpenCL();

    //enter main event loop
    glutMainLoop();

    return 0;
}
