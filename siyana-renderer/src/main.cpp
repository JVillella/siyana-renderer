#define __OPENCL_HOST__
    #include "Utilities.h"
    #include "BBox.h"
    #include "WindowClient.hpp"
#undef __OPENCL_HOST__

#include "OpenCLUtilities.h"
//#include "GLUTUtilities.h"
#include "BuildScene.h"

#include <iostream>
#include <string>

using namespace std;

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

void printCwd() {
    char str[1000];
    getcwd(str, 1000);
    cout<<"Current working directory: "<<endl;
    cout<<"\t"<<str<<endl;
}

int main(int argc, char** argv) {
    // Called after exit(int). Last function is the first to be called
    //    atexit(GLUTCleanUp);
    atexit(OpenCLCleanUp);
    atexit(CleanScene);
    
    printCwd();
    
    cout<<"Setting up scene"<<endl;
    SetupScene();
    cout<<"Scene setup complete"<<endl;
    
    cout<<"Setting up GUI"<<endl;
    int width, height;
    GLFWwindow* window = setupWindow(&width, &height);
    cout<<"GUI setup complete"<<endl;
    
    cout<<"Setting up OpenCL"<<endl;
    SetupOpenCL(width, height);
    cout<<"OpenCL setup complete"<<endl;

    cout<<"Starting main event loop"<<endl;
    startGui(window);
    
    return 0;
}
