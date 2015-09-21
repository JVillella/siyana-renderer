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

#ifndef INTERSECTIONFUNCTIONS_H_INCLUDED
#define INTERSECTIONFUNCTIONS_H_INCLUDED

#define __OPENCL_DEVICE__
    #include "Utilities.h"
#undef __OPENCL_DEVICE__

inline
bool IntersectSky(const Ray ray, float* t, IsectData *isect) {
    float3 origin = (float3)(0.f, 0.f, 0.f);
    float radius = 1.f;

    float3 o = ray.o - origin;
    float A = dot(ray.d, ray.d);
    float B = 2.f * dot(ray.d, o);
    float C = dot(o, o) - (radius * radius);

    float disc = B*B - 4.f*A*C;
    if(disc <= 0.f) {
        return false;
    }
    //will never reach first condition
    /*float t0 = (-B - sqrt(disc))/(2.f*A);
    if(t0 > EPSILON) {
        *t = t0;
        isect->hit_point = GetHitPoint(ray, t0);
        isect->normal = isect->hit_point - origin;
        isect->hit_mesh_index = 0;
        return true;
    }*/
    float t1 = (-B + sqrt(disc))/(2.f*A);
    if(t1 > EPSILON) {
        *t = t1;
        isect->hit_point = GetHitPoint(ray, t1);
        isect->normal = isect->hit_point - origin;
        isect->hit_mesh_index = 0;
        return true;
    }
    return false;
}

//------------------------------------------Sphere Intersection
inline
bool Test_IntersectSphere(const Ray ray, float* t, IsectData *isect) {
    float3 origin = (float3)(0.f, 0.f, 0.f);
    float radius = 1.f;

    float3 o = ray.o - origin;
    float A = dot(ray.d, ray.d);
    float B = 2.f * dot(ray.d, o);
    float C = dot(o, o) - (radius * radius);

    float disc = B*B - 4.f*A*C;
    if(disc < 0.f) {
        return false;
    }
    float t0 = (-B - sqrt(disc))/(2.f*A);
    if(t0 > EPSILON) {
        *t = t0;
        isect->hit_point = GetHitPoint(ray, t0);
        isect->normal = isect->hit_point - origin;
        isect->hit_mesh_index = 0;
        return true;
    }
    float t1 = (-B + sqrt(disc))/(2.f*A);
    if(t1 > EPSILON) {
        *t = t1;
        isect->hit_point = GetHitPoint(ray, t1);
        isect->normal = isect->hit_point - origin;
        isect->hit_mesh_index = 0;
        return true;
    }
    return false;
}

//------------------------------------------Utils Used in Tri-Isect
inline
float3 SolidTriangleNormal(float3 edge_1, float3 edge_2) {
    return normalize(cross(edge_1, edge_2)); //Return unit normal
}

inline
float3 InterpolatedNormal(float u, float v, const Triangle tri, __global const float3* normals) {
    //normal = bary_3 * n0 + bary_1 * n1 + bary_2 * n2
    return normalize((1.f - u - v) * normals[tri.vi.s0] +
                                 u * normals[tri.vi.s1] +
                                 v * normals[tri.vi.s2]);
}

inline
float2 InterpolatedUV(float u, float v, const Triangle tri, __global const float2* uvs) {
    return (float2)(1.f - u - v) * uvs[tri.vi.s0] +
                               u * uvs[tri.vi.s1] +
                               v * uvs[tri.vi.s2];
}

//------------------------------------------Triangle Intersection
inline
bool IntersectTriangle(const Triangle tri, const Ray ray, float *t,
    IsectData *isect, __global const float3* vertices,
    __global const float3* normals, __global const float2* uvs) {
    //Moller and Trumbore's Method of Ray-Triangle Intersection
    //compute triangle edges
    const float3 edge_1 = (vertices[tri.vi.s1] - vertices[tri.vi.s0]);
    const float3 edge_2 = (vertices[tri.vi.s2] - vertices[tri.vi.s0]);

    //find first barycentric coordinate
    const float3 s1 = cross(ray.d, edge_2);
    const float divisor = dot(s1, edge_1);
    if(divisor == 0.f) { //divide by zero
        return false;
    }
    float inv_divisor = 1.f/divisor; //inverse of divisor

    const float3 d = ray.o - vertices[tri.vi.s0];
    const float u = dot(s1, d) * inv_divisor;
    //barycentric coordinates cannot go over 1 or under 0
    if(u < 0.f || u > 1.f) { //No Hit
        return false;
    }

    //find second barycentric coordinate
    const float3 s2 = cross(d, edge_1);
    const float v = dot(s2, ray.d) * inv_divisor;
    if (v < 0.f || (u + v) > 1.f) { //No Hit
        return false;
    }
    const float thit = dot(s2, edge_2) * inv_divisor;

    if(thit > EPSILON) { //Hit
        *t = thit;
        isect->hit_point = GetHitPoint(ray, thit); //hit point
        isect->normal = InterpolatedNormal(u, v, tri, normals);
        // OLD SOLID NORMALS // isect->normal = SolidTriangleNormal(edge_1, edge_2);
        isect->uv = InterpolatedUV(u, v, tri, uvs);
        return true;
    }
    return false; //No Hit
}

//------------------------------------------Triangle Mesh Intersection
inline
bool IntersectTriangleMesh(__global const TriangleMesh* mesh,
    __global const Triangle* triangles, const Ray ray, float* t,
    IsectData *isect, __global const float3* vertices,
    __global const float3* normals, __global const float2* uvs) {
    //
    bool hit = false;
    IsectData _isect;
    float _t = HUGE_VALUE;
    int end_index = mesh->start_index + mesh->num_tris;

    for(int i = mesh->start_index; i < end_index; i++) {
        if(IntersectTriangle(triangles[i], ray, &_t, &_isect, vertices, normals, uvs)) {
            hit = true;
            if(_t < *t) {
                *t = _t;
                *isect = _isect;
            }
        }
    }
    return hit;
}

//------------------------------------------Kd Tree Intersection
//Utilities
inline
int GetMeshIndex(__global const TriangleMesh* meshes, int num_meshes, int tri_index) {
    //
	int tri_count = -1; //offset
	for(int m = 0; m < num_meshes; m++) {
		tri_count += meshes[m].num_tris;
        if(tri_count >= tri_index) {
            return m;
		}
	}
	return 0;
}

inline
bool IntersectBBox(const BBox bbox, const Ray ray, float *tmin, float *tmax) {
    //Possible errors with infinities can arise if fast
    //switching is on. Could be a source of error, beware!
    float t0 = EPSILON; float t1 = HUGE_VALUE; //start-off values
    float ray_d[] = {ray.d.x, ray.d.y, ray.d.z};
    float ray_o[] = {ray.o.x, ray.o.y, ray.o.z};
    float bbox_max[] = {bbox.max.x, bbox.max.y, bbox.max.z};
    float bbox_min[] = {bbox.min.x, bbox.min.y, bbox.min.z};

    for(int i = 0; i < 3; i++) { //for x, y, z
        float inv_dir = 1.f / ray_d[i];
        float t_enter = (bbox_min[i] - ray_o[i]) * inv_dir;
        float t_exit  = (bbox_max[i] - ray_o[i]) * inv_dir;
        if(t_enter > t_exit) {swap(&t_enter, &t_exit);}
        //Update t0, t1, with smallest interval
        if(t_enter > t0) {t0 = t_enter;}
        if(t_exit < t1) {t1 = t_exit;}
        if(t0 > t1) //No Hit
            return false;
    }
    //Set values
    *tmin = t0;
    *tmax = t1;
    return true; //Hit
}

inline
bool __IntersectKdTree(__global const KdNode* kdnodes, int num_kdnodes,
    __global const int* kdsorted_tri_indices, __global const TriangleMesh* meshes,
    int num_meshes, __global const Triangle* triangles, __global const float3* vertices,
    __global const float3* normals, __global const float2* uvs, const BBox scene_bbox,
    const Ray ray, IsectData *isect) {
    //
    ToDoKdNode todo_nodes[64]; //64 should be large enough for any situation
    int todo_index = 0;

    __global const KdNode* node = &kdnodes[num_kdnodes - 1]; //parent is last element
    float t_near, t_far;
    if(!IntersectBBox(scene_bbox, ray, &t_near, &t_far)) {
        return false; //didn't hit scene bbox. won't hit anything inside then
    }

    float ray_o[] = {ray.o.x, ray.o.y, ray.o.z}; //allows is to index float3s on device
    float ray_d[] = {ray.d.x, ray.d.y, ray.d.z};
    float inv_dir[] = {1.f/ray.d.x, 1.f/ray.d.y, 1.f/ray.d.z};

    int close_mesh_index = -1;
    float t = HUGE_VALUE, t_temp = HUGE_VALUE;
    IsectData isect_temp;

    while(node != 0) {
        if(t < t_near) { //found closer intersection
            break;
        }
        if(node->is_leaf) {
            //perform intersection
            int end_index = node->data[TRI_START_INDEX] + node->data[NUM_TRIS];
            for(int i = node->data[TRI_START_INDEX]; i < end_index; i++) {
                int sorted_tri_index = kdsorted_tri_indices[i];
                int mesh_index = GetMeshIndex(meshes, num_meshes, sorted_tri_index);
                if(IntersectTriangle(triangles[sorted_tri_index], ray, &t, &isect_temp, vertices, normals, uvs)) {
                    if(t < t_temp) {
                        t_temp = t;
                        *isect = isect_temp;
                        close_mesh_index = mesh_index;
                    }
                }
            }
            //check stack
            if(todo_index > 0) {
                todo_index--;
                node  = &kdnodes[todo_nodes[todo_index].node_index];
                t_near= todo_nodes[todo_index].t_near;
                t_far = todo_nodes[todo_index].t_far;
            } else {
                break;
            }
        } else { //is inner node
            int split_axis = node->split_axis;
            float split_pos= node->split_pos;
            //distance from ray to split plane
            float t_split = (split_pos - ray_o[split_axis]) * inv_dir[split_axis];

            //origin starting point cases
            bool ori_behind_plane = ray_o[split_axis] < split_pos;
            bool ori_on_split = ray_o[split_axis] == split_pos && ray_d[split_axis] >= 0.f;

            int NEAR_CHILD, FAR_CHILD;
            if(ori_behind_plane || ori_on_split) {
                NEAR_CHILD = 0; FAR_CHILD = 1;
            } else {
                NEAR_CHILD = 1; FAR_CHILD = 0;
            }

            //deal with intersection cases
            //...1 cull far side, trace near
            //...2 cull near side,trace far
            //...3 traverse both sides (near to far)
            if(t_split > t_far || t_split <= 0.f) {
                node = &kdnodes[node->data[NEAR_CHILD]];
            } else if(t_split < t_near) {
                node = &kdnodes[node->data[FAR_CHILD]];
            } else {
                //add far node to "stack"
                todo_nodes[todo_index].node_index = node->data[FAR_CHILD];
                todo_nodes[todo_index].t_near = t_split;
                todo_nodes[todo_index].t_far = t_far;
                todo_index++;
                t_far = t_split;

                node = &kdnodes[node->data[NEAR_CHILD]];
            }
        }
    }
    if(close_mesh_index != -1) {
        isect->hit_mesh_index = close_mesh_index;
        return true;
    } else {
        return false;
    }
}

inline
bool IntersectKdTree(__global const KdNode* kdnodes, int num_kdnodes,
    __global const int* kdsorted_tri_indices, __global const TriangleMesh* meshes,
    int num_meshes, __global const Triangle* triangles, __global const float3* vertices,
    __global const float3* normals, __global const float2* uvs, const BBox scene_bbox,
    const Ray ray, float* t, IsectData *isect) {
    //
    __global const KdNode* root = &kdnodes[num_kdnodes - 1];
    __global const KdNode* node;

    float t_near, t_far, scene_far;
    if(!IntersectBBox(scene_bbox, ray, &t_near, &scene_far)) {
        return false; //didn't hit scene bbox. won't hit anything inside then
    }
    t_far = t_near;

    float ray_o[] = {ray.o.x, ray.o.y, ray.o.z}; //allows is to index float3s on device
    float ray_d[] = {ray.d.x, ray.d.y, ray.d.z};
    float inv_dir[] = {1.f/ray.d.x, 1.f/ray.d.y, 1.f/ray.d.z};

    int close_mesh_index = -1;
    *t = HUGE_VALUE;
    float t_temp = HUGE_VALUE;
    IsectData isect_temp;

//printf("t_near: %f\tt_far: %f\tscene_far: %f\n", t_near, t_far, scene_far);

    while(t_far <= scene_far) { //infinitely thing kdnodes
        //keep restarting at root
        t_near = t_far;
        t_far = scene_far;
        node = root; //parent is last element

        //--- Inner Node ---// (traverse to leaf)
        while(!node->is_leaf) {
            int split_axis = node->split_axis;
            float split_pos= node->split_pos;

            //distance from ray.o to split plane
            float t_split = (split_pos - ray_o[split_axis]) * inv_dir[split_axis];
            //origin starting point cases
            bool ori_behind_plane = ray_o[split_axis] < split_pos;
            bool ori_on_split = ray_o[split_axis] == split_pos && ray_d[split_axis] >= 0.f;

            int NEAR_CHILD, FAR_CHILD;
            if(ori_behind_plane || ori_on_split) {
                NEAR_CHILD = 0; FAR_CHILD = 1;
            } else {
                NEAR_CHILD = 1; FAR_CHILD = 0;
            }

            //deal with intersection cases
            //...1 cull far side, trace near
            //...2 cull near side,trace far
            //...3 traverse both sides (near to far)
            if(t_split >= t_far || t_split < 0.f) {
                node = &kdnodes[node->data[NEAR_CHILD]];
            } else if(t_split <= t_near) {
                node = &kdnodes[node->data[FAR_CHILD]];
            } else {
                node = &kdnodes[node->data[NEAR_CHILD]];
                t_far = t_split;
            }
        }

        //--- Leaf Node ---// (perform intersection)
        int end_index = node->data[TRI_START_INDEX] + node->data[NUM_TRIS];
        for(int i = node->data[TRI_START_INDEX]; i < end_index; i++) {
            int sorted_tri_index = kdsorted_tri_indices[i];
            int mesh_index = GetMeshIndex(meshes, num_meshes, sorted_tri_index);
            if(IntersectTriangle(triangles[sorted_tri_index], ray, &t_temp, &isect_temp, vertices, normals, uvs)) {
                if(t_temp < *t) {
                    *t = t_temp;
                    *isect = isect_temp;
                    close_mesh_index = mesh_index;
                    //return true;
                }
            }
            if(*t < t_near) {
                break;
            }
        }
        //for infinitely thin kdnodes. <= up top. needs to stop now
        if(t_far == scene_far)
        break;
    }
    if(close_mesh_index != -1) {
        isect->hit_mesh_index = close_mesh_index;
        return true;
    } else {
        return false;
    }
}

//------------------------------------------Scene Intersection
inline
bool NoAcceleration(__global const TriangleMesh* meshes,
    const int num_meshes, __global const Triangle* triangles,
    __global const float3* vertices, __global const float3* normals,
    __global const float2* uvs, const Ray ray, float* t, IsectData *isect) {
    //
    IsectData _isect;
    *t = HUGE_VALUE;
    float t_near = HUGE_VALUE;
    int close_mesh_index = -1;

    for(int i = 0; i < num_meshes; i++) {
        if(IntersectTriangleMesh(&meshes[i], triangles, ray, t, &_isect, vertices, normals, uvs)) {
            if(*t < t_near) {
                t_near = *t;
                *isect = _isect;
                close_mesh_index = i;
            }
        }
    }
    if(close_mesh_index != -1) {
        isect->hit_mesh_index = close_mesh_index;
        return true;
    } else {
        return false;
    }
}

inline
bool IntersectScene(__global const TriangleMesh* meshes, const int num_meshes,
    __global const Triangle* triangles, __global const float3* vertices,
    __global const float3* normals, __global const float2* uvs,
    __global const KdNode* kdnodes, const int num_kdnodes,
    __global const int* kdsorted_tri_indices,
    const BBox scene_bbox, const Ray ray, float* t, IsectData *isect) {
    //
    //return NoAcceleration(meshes, num_meshes, triangles, vertices, normals, uvs, ray, t, isect);
    return IntersectKdTree(kdnodes, num_kdnodes, kdsorted_tri_indices, meshes,
        num_meshes, triangles, vertices, normals, uvs, scene_bbox, ray, t, isect);
}

#endif // INTERSECTIONFUNCTIONS_H_INCLUDED
