//for clCreateImage2D
#define CL_USE_DEPRECATED_OPENCL_1_1_APIS

#define __OPENCL_HOST__
    #include "Utilities.h"
    #include "BBox.h"
#undef __OPENCL_HOST__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <sstream>

#include "OpenCLUtilities.h"

using namespace std;

//constants
#define MAX_SOURCE_SIZE 999999
//Indices specific to Julian's (my) PC
#define GPU 0
#define CPU 1
#define PLATFORM_INDEX GPU

//defined elsewhere
extern cl_int width;
extern cl_int height;
extern int spp;
extern float* pixels;

extern TriangleMesh* meshes;
extern cl_int num_meshes;

extern Triangle* triangles;
extern cl_int total_scene_tris;

extern float3* vertices;
extern float3* normals;
extern float2* uvs;
extern cl_int num_vertices;

extern KdNode* kdnodes;
extern cl_int num_kdnodes;
extern int* kdsorted_tri_indices;
extern cl_int num_kdsorted_tri_indices;

extern unsigned char* texture_data;
extern cl_int num_texture_bytes;

extern Texture environment_tex;
extern BBox scene_bbox;
extern Camera cam;

//OpenCL static variables
static cl_platform_id platform = NULL;
static cl_device_id device = NULL;
static cl_context context = NULL;
static cl_command_queue command_queue = NULL;
static cl_program program = NULL;
static cl_kernel kernel = NULL;

//image spec
static const cl_image_format image_format = {
    CL_RGBA, CL_FLOAT
};

//cl memory
static cl_mem mem_meshes = NULL;
static cl_mem mem_triangles = NULL;
static cl_mem mem_vertices = NULL;
static cl_mem mem_normals = NULL;
static cl_mem mem_uvs = NULL;
static cl_mem mem_kdnodes = NULL;
static cl_mem mem_kdsorted_tri_indices = NULL;
static cl_mem mem_texture_data = NULL;
static cl_mem mem_environment_tex = NULL;
static cl_mem mem_scene_bbox = NULL;
static cl_mem mem_cam = NULL;
static cl_mem mem_image = NULL;
static cl_mem mem_rand_states = NULL; //random number states
static unsigned int* rand_states = NULL; //local

//------------------------------------------Utilities
static size_t RoundUp(int groupSize, int globalSize) {
    int r = globalSize % groupSize;
    if(r == 0) { //no remainder
      return globalSize;
    } else {
      return globalSize + groupSize - r;
    }
}
static char* ReadSource(const char* file_path, size_t* source_size, cl_int* error) {
    FILE* file = fopen(file_path, "r"); //open file to read
    if (!file) {
        printf("OpenCL: Failed opening kernel source file\n");
        exit(*error); //fail
        return NULL;
    } else {
	    //write to char array and return its length
	    char* source_str = (char*)malloc(MAX_SOURCE_SIZE);
	    *source_size = fread(source_str, 1, MAX_SOURCE_SIZE, file);
	    fclose(file);
	    *error = 0; //success
	    return source_str;
	}
}
static cl_bool DeviceImageSupport(void) {
	cl_int error = 0;
    cl_bool image_support = CL_FALSE;
    error = clGetDeviceInfo(device, CL_DEVICE_IMAGE_SUPPORT,
    	sizeof(cl_bool), &image_support, NULL);
    if(error != CL_SUCCESS || image_support != CL_TRUE) {
        return CL_FALSE;
    } else {
        return CL_TRUE;
    }
}
static float GetVersionNumber(int* error) {
    char* version_num = (char*)malloc(MAX_SOURCE_SIZE);
    *error = clGetDeviceInfo(device, CL_DRIVER_VERSION, MAX_SOURCE_SIZE, version_num, NULL);
    float ver;
    stringstream(version_num)>>ver;
    if(version_num != NULL) {
        free(version_num);
        version_num = NULL;
    }
    return ver;
}

//Print OpenCL build log (for the .cl file)
static void PrintBuildLog(void) {
    cl_int error = 0;
    size_t log_size;

    //first call to determine log size
    error = clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
    if(error != CL_SUCCESS) {
        printf("OpenCL: Error calling clGetProgramBuildInfo(...)\n");
        exit(error);
    }
    char* log_str = (char*)malloc(log_size + 1);

    //second call to obtain log
    error = clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, log_str, NULL);
    if(error != CL_SUCCESS) {
        printf("OpenCL: Error w/ 2nd call to clGetProgramBuildInfo(...)\n");
        exit(error);
    }
    //FIXME do we need to null-terminate --> log_str[log_size] = '\0';
    printf("----- OpenCL: Log -----\n%s\n", log_str);
    free(log_str); //clean up resources
    log_str = NULL;
}

//------------------------------------------Procedure Wrappers
// TODO: Handle buffer sizes of zero. Currently just crashes
static void AllocateOpenCLMem(void) {
	printf("OpenCL: Allocating OpenCL memory\n");
	cl_int error = 0;
	mem_meshes = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(TriangleMesh) * num_meshes, NULL, &error);
    if(error != CL_SUCCESS) {
        printf("OpenCL: Error allocating mem_meshes memory\n");
        exit(error);
    }
	mem_triangles = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(Triangle) * total_scene_tris, NULL, &error);
    if(error != CL_SUCCESS) {
        printf("OpenCL: Error allocating mem_triangles memory\n");
        exit(error);
    }
	mem_vertices = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(float3) * num_vertices, NULL, &error);
    if(error != CL_SUCCESS) {
        printf("OpenCL: Error allocating mem_vertices memory\n");
        exit(error);
    }
	mem_normals = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(float3) * num_vertices, NULL, &error);
    if(error != CL_SUCCESS) {
        printf("OpenCL: Error allocating mem_normals memory\n");
        exit(error);
    }
	mem_uvs = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(float2) * num_vertices, NULL, &error);
    if(error != CL_SUCCESS) {
        printf("OpenCL: Error allocating mem_uvs memory\n");
        exit(error);
    }
	mem_kdnodes = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(KdNode) * num_kdnodes, NULL, &error);
    if(error != CL_SUCCESS) {
        printf("OpenCL: Error allocating mem_kdnodes memory\n");
        exit(error);
    }
	mem_kdsorted_tri_indices = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(int) * num_kdsorted_tri_indices, NULL, &error);
    if(error != CL_SUCCESS) {
        printf("OpenCL: Error allocating mem_kdsorted_tri_indices memory\n");
        exit(error);
    }
	mem_texture_data = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(unsigned char) * num_texture_bytes, NULL, &error);
    if(error != CL_SUCCESS) {
        printf("OpenCL: Error allocating mem_texture_data memory\n");
        exit(error);
    }
	mem_environment_tex = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(Texture), NULL, &error);
    if(error != CL_SUCCESS) {
        printf("OpenCL: Error allocating mem_environment_tex memory\n");
        exit(error);
    }
	mem_scene_bbox = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(BBox), NULL, &error);
    if(error != CL_SUCCESS) {
        printf("OpenCL: Error allocating mem_scene_bbox memory\n");
        exit(error);
    }
	mem_cam = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(Camera), NULL, &error);
    if(error != CL_SUCCESS) {
        printf("OpenCL: Error allocating mem_cam memory\n");
        exit(error);
    }
    //allocate memory for rand_states
    rand_states = (unsigned int*)malloc(sizeof(unsigned int) * width * height);
    int i; //fill array w/ rand vals
    for(i = 0; i < width * height; i++) {
        unsigned int r = rand();
        if(r < 2) {
            r = 2;
        }
        rand_states[i] = r;
    }
    mem_rand_states = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(unsigned int) * width * height, NULL, &error);
    if(error != CL_SUCCESS) {
        printf("OpenCL: Error allocating unsigned int* mem_rand_states (array) memory\n");
        exit(error);
    }
    mem_image = clCreateImage2D(context, CL_MEM_WRITE_ONLY, &image_format, width, height, 0, NULL, &error);
	if(error != CL_SUCCESS) {
		printf("OpenCL: Error allocating image2d_t image memory\n");
		exit(error);
	}
}

static void FreeOpenCLMem(void) {
	printf("OpenCL: Freeing OpenCL memory\n");
    clReleaseMemObject(mem_meshes);
    clReleaseMemObject(mem_triangles);
    clReleaseMemObject(mem_vertices);
    clReleaseMemObject(mem_normals);
    clReleaseMemObject(mem_uvs);
    clReleaseMemObject(mem_kdnodes);
    clReleaseMemObject(mem_kdsorted_tri_indices);
    clReleaseMemObject(mem_texture_data);
    clReleaseMemObject(mem_environment_tex);
    clReleaseMemObject(mem_scene_bbox);
    clReleaseMemObject(mem_cam);
    clReleaseMemObject(mem_image);
    clReleaseMemObject(mem_rand_states);

	if(rand_states != NULL) {
        free(rand_states);
        rand_states = NULL;
	}
}

void CopyLocalVarToOpenCLMem(void) {
	cl_int error = 0;
	printf("OpenCL: Copying local variables to OpenCL memory\n");
    error = clEnqueueWriteBuffer(command_queue, mem_meshes, CL_TRUE, 0, sizeof(TriangleMesh) * num_meshes, meshes, 0, NULL, NULL);
    if(error != CL_SUCCESS) {
        printf("OpenCL: Error copying local meshes to mem_meshes\n");
        exit(error);
    }
    error = clEnqueueWriteBuffer(command_queue, mem_triangles, CL_TRUE, 0, sizeof(Triangle) * total_scene_tris, triangles, 0, NULL, NULL);
    if(error != CL_SUCCESS) {
        printf("OpenCL: Error copying local triangles to mem_triangles\n");
        exit(error);
    }
    error = clEnqueueWriteBuffer(command_queue, mem_vertices, CL_TRUE, 0, sizeof(float3) * num_vertices, vertices, 0, NULL, NULL);
    if(error != CL_SUCCESS) {
        printf("OpenCL: Error copying local vertices to mem_vertices\n");
        exit(error);
    }
    error = clEnqueueWriteBuffer(command_queue, mem_normals, CL_TRUE, 0, sizeof(float3) * num_vertices, normals, 0, NULL, NULL);
    if(error != CL_SUCCESS) {
        printf("OpenCL: Error copying local normals to mem_normals\n");
        exit(error);
    }
    error = clEnqueueWriteBuffer(command_queue, mem_uvs, CL_TRUE, 0, sizeof(float2) * num_vertices, uvs, 0, NULL, NULL);
    if(error != CL_SUCCESS) {
        printf("OpenCL: Error copying local uvs to mem_uvs\n");
        exit(error);
    }
    error = clEnqueueWriteBuffer(command_queue, mem_kdnodes, CL_TRUE, 0, sizeof(KdNode) * num_kdnodes, kdnodes, 0, NULL, NULL);
    if(error != CL_SUCCESS) {
        printf("OpenCL: Error copying local kdnodes to mem_kdnodes\n");
        exit(error);
    }
    error = clEnqueueWriteBuffer(command_queue, mem_kdsorted_tri_indices, CL_TRUE, 0, sizeof(int) * num_kdsorted_tri_indices, kdsorted_tri_indices, 0, NULL, NULL);
    if(error != CL_SUCCESS) {
        printf("OpenCL: Error copying local kdsorted_tri_indices to mem_kdsorted_tri_indices\n");
        exit(error);
    }
    error = clEnqueueWriteBuffer(command_queue, mem_texture_data, CL_TRUE, 0, sizeof(unsigned char) * num_texture_bytes, texture_data, 0, NULL, NULL);
    if(error != CL_SUCCESS) {
        printf("OpenCL: Error copying local texture_data to mem_texture_data\n");
        exit(error);
    }
    error = clEnqueueWriteBuffer(command_queue, mem_environment_tex, CL_TRUE, 0, sizeof(Texture), &environment_tex, 0, NULL, NULL);
    if(error != CL_SUCCESS) {
        printf("OpenCL: Error copying local environment_tex to mem_environment_tex\n");
        exit(error);
    }
    error = clEnqueueWriteBuffer(command_queue, mem_scene_bbox, CL_TRUE, 0, sizeof(BBox), &scene_bbox, 0, NULL, NULL);
    if(error != CL_SUCCESS) {
        printf("OpenCL: Error copying local scene_bbox to mem_scene_bbox\n");
        exit(error);
    }
    error = clEnqueueWriteBuffer(command_queue, mem_cam, CL_TRUE, 0, sizeof(Camera), &cam, 0, NULL, NULL);
    if(error != CL_SUCCESS) {
        printf("OpenCL: Error copying local cam to mem_cam\n");
        exit(error);
    }
    error = clEnqueueWriteBuffer(command_queue, mem_rand_states, CL_TRUE, 0, sizeof(unsigned int) * width * height, rand_states, 0, NULL, NULL);
    if(error != CL_SUCCESS) {
        printf("OpenCL: Error copying local int* rand_states to cl_mem mem_rand_states\n");
        exit(error);
    }
}
void OpenCLCleanUp(void) {
	printf("OpenCL: Invoked OpenCL clean up\n");
	cl_int error = 0;

	error = clFlush(command_queue);
	error = clFinish(command_queue);
	error = clReleaseKernel(kernel);
	error = clReleaseProgram(program);
    FreeOpenCLMem();
	error = clReleaseCommandQueue(command_queue);
	error = clReleaseContext(context);

	if(error != CL_SUCCESS) {
		printf("OpenCL: Error cleaning up OpenCL resources\n");
		exit(error);
	}
}

static void SetKernelArguments(void) {
	printf("OpenCL: Setting kernel args\n");
    cl_int error = 0;
    error = clSetKernelArg(kernel, 0,  sizeof(cl_mem), (void*)&mem_meshes);
    error = clSetKernelArg(kernel, 1,  sizeof(cl_int), (void*)&num_meshes);
    error = clSetKernelArg(kernel, 2,  sizeof(cl_mem), (void*)&mem_triangles);
    error = clSetKernelArg(kernel, 3,  sizeof(cl_int), (void*)&total_scene_tris);
    error = clSetKernelArg(kernel, 4,  sizeof(cl_mem), (void*)&mem_vertices);
    error = clSetKernelArg(kernel, 5,  sizeof(cl_mem), (void*)&mem_normals);
    error = clSetKernelArg(kernel, 6,  sizeof(cl_mem), (void*)&mem_uvs);
    error = clSetKernelArg(kernel, 7,  sizeof(cl_mem), (void*)&mem_kdnodes);
    error = clSetKernelArg(kernel, 8,  sizeof(cl_int), (void*)&num_kdnodes);
    error = clSetKernelArg(kernel, 9,  sizeof(cl_mem), (void*)&mem_kdsorted_tri_indices);
    error = clSetKernelArg(kernel, 10, sizeof(cl_int), (void*)&num_kdsorted_tri_indices);
    error = clSetKernelArg(kernel, 11, sizeof(cl_mem), (void*)&mem_texture_data);
    error = clSetKernelArg(kernel, 12, sizeof(cl_int), (void*)&num_texture_bytes);
    error = clSetKernelArg(kernel, 13, sizeof(cl_mem), (void*)&mem_environment_tex);
    error = clSetKernelArg(kernel, 14, sizeof(cl_mem), (void*)&mem_scene_bbox);
    error = clSetKernelArg(kernel, 15, sizeof(cl_mem), (void*)&mem_cam);
    error = clSetKernelArg(kernel, 16, sizeof(cl_int), (void*)&width);
    error = clSetKernelArg(kernel, 17, sizeof(cl_int), (void*)&height);
    error = clSetKernelArg(kernel, 18, sizeof(cl_mem), (void*)&mem_image);
    error = clSetKernelArg(kernel, 19, sizeof(cl_mem), (void*)&mem_rand_states);
    if(error != CL_SUCCESS) {
        printf("OpenCL: error setting kernel args inside SetKernelArguments\n");
        exit(error);
    }
}
static void ExecuteKernel(void) {
//	printf("OpenCL: Executing kernel\n");
    cl_int error = 0;

//    size_t num_local_work_items[2] = {16, 16};
//    size_t num_global_work_items[2] = {RoundUp(num_local_work_items[0], width),
//                                       RoundUp(num_local_work_items[1], height)};

    size_t num_global_work_items[2] = {static_cast<size_t>(width), static_cast<size_t>(height)};
    error = clEnqueueNDRangeKernel(command_queue, kernel, 2, NULL, num_global_work_items, NULL, 0, NULL, NULL);
    if(error != CL_SUCCESS) {
        printf("OpenCL: Error enqueuing kernel to command queue\n");
        exit(error);
    }
}
void SetupOpenCL(void) {
	printf("OpenCL: Setting up OpenCL\n");
    cl_int error = 0; //stores error

    //setup platform
    cl_uint num_platforms;

    cl_platform_id* platforms = (cl_platform_id*)(malloc)(sizeof(cl_platform_id) * 2);

    error = clGetPlatformIDs(2, platforms, &num_platforms);
    if(error != CL_SUCCESS) {
        printf("OpenCL: Error getting platform ID\n");
        exit(error);
    }

    platform = platforms[PLATFORM_INDEX];
    free(platforms);

    printf("OpenCL: System has %u available OpenCL platforms\n", num_platforms);

    //setup device
    cl_uint num_devices;
    error = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 1, &device, &num_devices);
    if(error != CL_SUCCESS) {
        printf("OpenCL: Error getting device ID\n");
        exit(error);
    }
    printf("OpenCL: System has %u available OpenCL devices\n", num_devices);

    //setup context
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &error);
    if(error != CL_SUCCESS) {
        printf("OpenCL: Error creating context\n");
        exit(error);
    }

    //setup command queue
    command_queue = clCreateCommandQueue(context, device, 0, &error);
    if(error != CL_SUCCESS) {
        printf("OpenCL: Error creating command queue\n");
        exit(error);
    }

    float version_num = GetVersionNumber(&error);
    if(error != CL_SUCCESS) {
        printf("OpenCL: Error obtaining OpenCL version number for device\n");
        exit(error);
    }
    printf("OpenCL: Version number: %f\n", version_num);

    //check if device supports images
    if(DeviceImageSupport() != CL_TRUE) {
        printf("OpenCL: Device does not support images\n");
        exit(error);
    }

    //read kernel source into char array (source_str)
    printf("OpenCL: Read kernel source and headers\n");
    int num_modules = 4;
    size_t source_sizes[num_modules];
    char* source_strs[num_modules];
    source_strs[0] = ReadSource("src/RenderKernel.cl", &source_sizes[0], &error);
    if (error != CL_SUCCESS) {
        printf("OpenCL: Error reading (RenderKernel.cl) kernel source into char array\n");
        exit(error);
    }
    source_strs[1] = ReadSource("src/Utilities.h", &source_sizes[1], &error);
    if (error != CL_SUCCESS) {
        printf("OpenCL: Error reading (Utilities.h) kernel source into char array\n");
        exit(error);
    }
    source_strs[2] = ReadSource("src/Sampler.h", &source_sizes[2], &error);
    if (error != CL_SUCCESS) {
        printf("OpenCL: Error reading (Sampler.h)) kernel source into char array\n");
        exit(error);
    }
    source_strs[3] = ReadSource("src/IntersectionFunctions.h", &source_sizes[3], &error);
    if (error != CL_SUCCESS) {
        printf("OpenCL: Error reading (IntersectionFunctions.h) kernel source into char array\n");
        exit(error);
    }
    //create and build program (also shows build log)
	program = clCreateProgramWithSource(context, num_modules, (const char**)&source_strs, (const size_t*)&source_sizes, &error);
	if(error != CL_SUCCESS) {
		printf("OpenCL: Error creating program with source\n");
		exit(error);
	}
	//build program
	error = clBuildProgram(program, 1, &device, "-I src", NULL, NULL);
	PrintBuildLog();
	if(error != CL_SUCCESS) {
		printf("OpenCL: Error building program\n");
        exit(error);
	}

    //create kernel
    kernel = clCreateKernel(program, "Render", &error);
    if(error != CL_SUCCESS) {
        printf("OpenCL: Error creating kernel\n");
        exit(error);
    }

    //TODO - Uncommenting these lines causes crash
    if(source_strs != NULL) {
        //free(source_strs);
        //source_strs = NULL;
    }

    printf("OpenCL: Handling OpenCL Memory\n");
    AllocateOpenCLMem();
    CopyLocalVarToOpenCLMem();
    SetKernelArguments();
}
//TODO figure out more efficient way of averaging samples
void UpdateLocalPixels(void) {
//	printf("OpenCL: Reading OpenCL device memory into local pixel array\n");
	cl_int error = 0;
    const size_t origin[3] = {0, 0, 0};
    const size_t region[3] = {static_cast<size_t>(width), static_cast<size_t>(height), 1};
        float* old_pixels = (float*)malloc(width * height * 4 * sizeof(float));
        memcpy(old_pixels, pixels, width * height * 4 * sizeof(float));
    //local pixel array should already be allocated
    error = clEnqueueReadImage(command_queue, mem_image, CL_TRUE, origin,
        region, 0, 0, pixels, 0, NULL, NULL);
    if(error != CL_SUCCESS) {
        printf("OpenCL: Error reading output from mem_image into local variable\n");
        exit(error);
    }
    if(spp > 0) {
        int i, j;
        for(i = 0; i < width*height; i++) {
            for(j = 0; j < 4; j++) {
                pixels[i * 4 + j] = (old_pixels[i * 4 + j] * (spp - 1) + pixels[i * 4 + j]) / (float)spp;
            }
        }
    }
    free(old_pixels);
    old_pixels = NULL;
    spp++;
}
void OpenCLRender(void) {
    //printf("OpenCL: Called OpenCLRender(void)\n");
    ExecuteKernel(); //execute kernel to command queue
    clFlush(command_queue); //issue all queued opencl commands to device
    clFinish(command_queue); //wait till processing is done
    //cout<<"About to update local pixel array"<<endl;
    UpdateLocalPixels(); //update local image
}

void OpenCLFullReset(void) {
    cout<<"***Full OpenCl reset***"<<endl;
    //reset samples per pixel count
    spp = 0;

    //free memory, reallocate
    FreeOpenCLMem();
    AllocateOpenCLMem();
    CopyLocalVarToOpenCLMem();
    SetKernelArguments();
}
void OpenCLResetRender(void) {
    cl_int error = 0;
    spp = 0; //reset samples per pixel count

    //free memory, reallocate
    //--------------------------------------Image
    clReleaseMemObject(mem_image);
    mem_image = clCreateImage2D(context, CL_MEM_WRITE_ONLY, &image_format, width, height, 0, NULL, &error);
    error = clSetKernelArg(kernel, 16, sizeof(int), (void*)&width);
    error = clSetKernelArg(kernel, 17, sizeof(int), (void*)&height);
    error = clSetKernelArg(kernel, 18, sizeof(cl_mem), (void*)&mem_image);
    if(error != CL_SUCCESS) {
        printf("OpenCL: Error with image in OpenCLResetRender()\n");
        exit(error);
    }

    //--------------------------------------Rand States
    clReleaseMemObject(mem_rand_states);
	if(rand_states != NULL) {
        free(rand_states);
        rand_states = NULL;
	}
    rand_states = (unsigned int*)malloc(sizeof(unsigned int) * width * height);
    int i; //fill array w/ rand vals
    for(i = 0; i < width * height; i++) {
        unsigned int r = rand();
        if(r < 2) {
            r = 2;
        }
        rand_states[i] = r;
    }
    mem_rand_states = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(unsigned int) * width * height, NULL, &error);
    error = clEnqueueWriteBuffer(command_queue, mem_rand_states, CL_TRUE, 0, sizeof(unsigned int) * width * height, rand_states, 0, NULL, NULL);
    error = clSetKernelArg(kernel, 19, sizeof(cl_mem), (void*)&mem_rand_states);
    if(error != CL_SUCCESS) {
        printf("OpenCL: Error with rand_states in OpenCLResetRender()\n");
        exit(error);
    }

    //--------------------------------------Camera
    clReleaseMemObject(mem_cam);
	mem_cam = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(Camera), NULL, &error);
    error = clEnqueueWriteBuffer(command_queue, mem_cam, CL_TRUE, 0, sizeof(Camera), &cam, 0, NULL, NULL);
    error = clSetKernelArg(kernel, 15, sizeof(cl_mem), (void*)&mem_cam);
    if(error != CL_SUCCESS) {
        printf("OpenCL: Error with camera in OpenCLResetRender()\n");
        exit(error);
    }
}
void OpenCLResetGeometry(void) {
    cout<<"***OpenCl Geometry Reset***"<<endl;
    cl_int error = 0;

    //reset samples per pixel count
    spp = 0;

    //free memory
    clReleaseMemObject(mem_meshes);
    clReleaseMemObject(mem_triangles);
    clReleaseMemObject(mem_vertices);
    clReleaseMemObject(mem_normals);
    clReleaseMemObject(mem_uvs);
    clReleaseMemObject(mem_kdnodes);
    clReleaseMemObject(mem_kdsorted_tri_indices);
    clReleaseMemObject(mem_texture_data);
    clReleaseMemObject(mem_environment_tex);
    clReleaseMemObject(mem_scene_bbox);
    clReleaseMemObject(mem_cam);

    //allocate memory
    mem_meshes = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(TriangleMesh) * num_meshes, NULL, &error);
	mem_triangles = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(Triangle) * total_scene_tris, NULL, &error);
	mem_vertices = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(float3) * num_vertices, NULL, &error);
	mem_normals = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(float3) * num_vertices, NULL, &error);
	mem_uvs = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(float2) * num_vertices, NULL, &error);
	mem_kdnodes = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(KdNode) * num_kdnodes, NULL, &error);
	mem_kdsorted_tri_indices = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(int) * num_kdsorted_tri_indices, NULL, &error);
	mem_texture_data = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(unsigned char) * num_texture_bytes, NULL, &error);
	mem_environment_tex = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(Texture), NULL, &error);
	mem_scene_bbox = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(BBox), NULL, &error);
	mem_cam = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(Camera), NULL, &error);
    if(error != CL_SUCCESS) {
        printf("OpenCL: error allocating memory inside OpenCLResetGeometry\n");
        exit(error);
    }

	//copy local memory to opencl buffers
    error = clEnqueueWriteBuffer(command_queue, mem_meshes, CL_TRUE, 0, sizeof(TriangleMesh) * num_meshes, meshes, 0, NULL, NULL);
    error = clEnqueueWriteBuffer(command_queue, mem_triangles, CL_TRUE, 0, sizeof(Triangle) * total_scene_tris, triangles, 0, NULL, NULL);
    error = clEnqueueWriteBuffer(command_queue, mem_vertices, CL_TRUE, 0, sizeof(float3) * num_vertices, vertices, 0, NULL, NULL);
    error = clEnqueueWriteBuffer(command_queue, mem_normals, CL_TRUE, 0, sizeof(float3) * num_vertices, normals, 0, NULL, NULL);
    error = clEnqueueWriteBuffer(command_queue, mem_uvs, CL_TRUE, 0, sizeof(float2) * num_vertices, uvs, 0, NULL, NULL);
    error = clEnqueueWriteBuffer(command_queue, mem_kdnodes, CL_TRUE, 0, sizeof(KdNode) * num_kdnodes, kdnodes, 0, NULL, NULL);
    error = clEnqueueWriteBuffer(command_queue, mem_kdsorted_tri_indices, CL_TRUE, 0, sizeof(int) * num_kdsorted_tri_indices, kdsorted_tri_indices, 0, NULL, NULL);
    error = clEnqueueWriteBuffer(command_queue, mem_texture_data, CL_TRUE, 0, sizeof(unsigned char) * num_texture_bytes, texture_data, 0, NULL, NULL);
    error = clEnqueueWriteBuffer(command_queue, mem_environment_tex, CL_TRUE, 0, sizeof(Texture), &environment_tex, 0, NULL, NULL);
    error = clEnqueueWriteBuffer(command_queue, mem_scene_bbox, CL_TRUE, 0, sizeof(BBox), &scene_bbox, 0, NULL, NULL);
    error = clEnqueueWriteBuffer(command_queue, mem_cam, CL_TRUE, 0, sizeof(Camera), &cam, 0, NULL, NULL);
    if(error != CL_SUCCESS) {
        printf("OpenCL: error copying local memory to opencl buffers inside OpenCLResetGeometry\n");
        exit(error);
    }

    //set kernel arguments
    error = clSetKernelArg(kernel, 0,  sizeof(cl_mem), (void*)&mem_meshes);
    error = clSetKernelArg(kernel, 1,  sizeof(cl_int), (void*)&num_meshes);
    error = clSetKernelArg(kernel, 2,  sizeof(cl_mem), (void*)&mem_triangles);
    error = clSetKernelArg(kernel, 3,  sizeof(cl_int), (void*)&total_scene_tris);
    error = clSetKernelArg(kernel, 4,  sizeof(cl_mem), (void*)&mem_vertices);
    error = clSetKernelArg(kernel, 5,  sizeof(cl_mem), (void*)&mem_normals);
    error = clSetKernelArg(kernel, 6,  sizeof(cl_mem), (void*)&mem_uvs);
    error = clSetKernelArg(kernel, 7,  sizeof(cl_mem), (void*)&mem_kdnodes);
    error = clSetKernelArg(kernel, 8,  sizeof(cl_int), (void*)&num_kdnodes);
    error = clSetKernelArg(kernel, 9,  sizeof(cl_mem), (void*)&mem_kdsorted_tri_indices);
    error = clSetKernelArg(kernel, 10, sizeof(cl_int), (void*)&num_kdsorted_tri_indices);
    error = clSetKernelArg(kernel, 11, sizeof(cl_mem), (void*)&mem_texture_data);
    error = clSetKernelArg(kernel, 12, sizeof(cl_int), (void*)&num_texture_bytes);
    error = clSetKernelArg(kernel, 13, sizeof(cl_mem), (void*)&mem_environment_tex);
    error = clSetKernelArg(kernel, 14, sizeof(cl_mem), (void*)&mem_scene_bbox);
    error = clSetKernelArg(kernel, 15, sizeof(cl_mem), (void*)&mem_cam);
    if(error != CL_SUCCESS) {
        printf("OpenCL: error setting kernel args inside OpenCLResetGeometry\n");
        exit(error);
    }
}
