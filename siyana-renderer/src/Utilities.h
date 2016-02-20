#ifndef UTILITIES_H_INCLUDED
#define UTILITIES_H_INCLUDED

#define EPSILON 0.0005f
#define HUGE_VALUE 100000.f

//for kdnodes
#define LEFT_CHILD 0
#define RIGHT_CHILD 1
#define TRI_START_INDEX 0
#define NUM_TRIS 1

#ifdef __OPENCL_HOST__
    #include "floatn.h"
    #include "intn.h"
    #define M_PI_F 3.14159265359f
    #define M_PI_2_F 1.57079632679f
    #define M_PI_4_F 0.78539816339f
#endif //HOST

//------------------------------------------Ray, IsectData
typedef struct {
	float3 o, d;
} Ray;

typedef struct {
	float3 hit_point;
	float3 normal;
	float2 uv;
	int hit_mesh_index;
	char junk[4];
} IsectData;

//------------------------------------------Triangle, TriangleMesh, Material
typedef struct {
	int3 vi; //vertex index
} Triangle;

typedef struct {
    unsigned int start_index;
    unsigned int width;
    unsigned int height;
    unsigned int channel;
} Texture;

typedef struct {
    Texture tex;
	float emission; //safe between [0, 1]
} Material;

typedef struct {
    //used constant number of tris...can't use malloc inside kernel
    Material material;
    unsigned int start_index;
    unsigned int num_tris;
} TriangleMesh;

//------------------------------------------Camera, Scene
typedef struct {
	float3 eye, focal; //origin, focal position
	float3 up, u, v, w;
    float view_dist; //distance from eye to view plane
    float focal_plane_dist; //distance from eye to focal plane
    float f_stop; //f# not 1/f#
} Camera;

//------------------------------------------BBox
typedef struct {
    float3 min, max;
} BBox;

//------------------------------------------KdNode
typedef struct {
	int data[2]; //holds child indicies or num_tris and start index
    float split_pos; //position split plane
    int split_axis; //axis node was split on
    int is_leaf;
} KdNode;

typedef struct {
    int node_index;
    float t_far;
    float t_near;
} ToDoKdNode;

/////////////////////
//--- Functions ---//
/////////////////////
//------------------------------------------Ray, IsectData
inline
float3 GetHitPoint(const Ray ray, float t) {
	return ray.o + ray.d * t;
}

//------------------------------------------Random Number Generator
//linear congruential generator (LCG)
//num_i+1 = (a * num_i + c) % m
//copy of C's implementation found in <stdlib.h>
inline
unsigned int Rand(unsigned int* state) { //[0, 32767]
    return (*state = (*state * 214013 + 2531011) & 2147483647U) >> 16;
}
inline
unsigned int UnitRandInt(unsigned int* state) { //[0, 1]
	return Rand(state) % 2;
}
inline
unsigned int RandInt(int min, int max, unsigned int* state) { //[min, max]
	return (Rand(state) % (max - min + 1) + min);
}
inline
float UnitRandFloat(unsigned int* state) { //[0, 1]
	return (Rand(state) / 32767.f);
}
inline
float RandFloat(float min, float max, unsigned int* state) { //[min, max]
	return (UnitRandFloat(state) * (max - min) + min);
}

//------------------------------------------Misc Utils
inline
bool IsBlack(float4 col) {
    return (col.x < EPSILON && col.y < EPSILON && col.z < EPSILON);
}
inline
float3 OrientNormal(float3 normal, float3 dir) {
    if(dot(normal, dir) < 0.f) {
        return -normal;
    } else {
        return normal;
    }
}
inline
int Max(int x, int y, int z) {
    if(x > y && x > z) //x axis
        return 0;
    else if(y > z) //y axis
        return 1;
    else //z axis
        return 2;
}
//
inline
void swap(float* a, float *b) {
    float temp = *a;
    *a = *b;
    *b = temp;
}

#endif //UTILITIES_H_INCLUDED
