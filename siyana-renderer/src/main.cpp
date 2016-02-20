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
