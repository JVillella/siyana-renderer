#ifndef MARCHINGCUBES_H_INCLUDED
#define MARCHINGCUBES_H_INCLUDED

#define __OPENCL_HOST__
    #include "Utilities.h"
    #include "BuildMesh.h" //for intermediate Mesh class
#undef __OPENCL_HOST__

//------------------------------------------Kernel
void GenerateTerrain(Mesh* mesh, const float3& voxel_size, const int3& num_voxels);

#endif // MARCHINGCUBES_H_INCLUDED
