#ifndef BUILDSCENE_H_INCLUDED
#define BUILDSCENE_H_INCLUDED

//Should put this in BuildScene.cpp but it
//needs to be here for the declarations below
#define __OPENCL_HOST__
    #include "Utilities.h"
    #include "Camera.h"
#undef __OPENCL_HOST__

//terrain constants - used in GlutUtils to determine when to update geometry
const float3 voxel_size = float3(2, 2, 2);
const int3 num_voxels = int3(200, 40, 200);

void SetupScene();
void UpdateScene();
void CleanScene();

#endif //BUILDSCENE_H_INCLUDED
