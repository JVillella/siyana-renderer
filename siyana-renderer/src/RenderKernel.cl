#ifdef cl_amd_printf
    #pragma OPENCL EXTENSION cl_amd_printf : enable
#else
    //#printf cannot be enabled for this platform
#endif

//including a source file will put it in this compilation unit
//OpenCL doesn't seem to support different translation units
//in the this sense so this needs to be done
#define __OPENCL_DEVICE__
    #include "Utilities.h"
    #include "Sampler.h"
    #include "IntersectionFunctions.h"
#undef __OPENCL_DEVICE__

#define ENV_MULT 3.f

#define M_1_255_F 0.00392156862f

#ifndef M_1_PI_F
    #define M_1_PI_F 0.31830988618f
#endif

#ifndef M_2_PI_F
    #define M_2_PI_F 0.63661977236f
#endif

//1/(PI * 2)
#define M_1_PI_2_F 0.15915494309f

#define M_1_3_F 0.33333333333f

//--- Feature Defines ---//
#define DL_ONLY
#define MAX_BOUNCES 1
#define DEPTH_OF_FIELD_ON
#define VIGNETTE_SCALE -0.25f //negative gives white edges
#define AMBIENT_TERM 0.075f
//-----------------------//

//TODO Add fog to pahtracer (only in ray tracer at the moment)

float4 GetTexel(__global const unsigned char* texture_data, const Texture tex, const IsectData isect) {
//return (float4)(0.5);
    //the mod will cause the x and y values to loop back to the beginning of the texture
    int x = (int)(isect.uv.x * (tex.width - 1)) % tex.width;
    int y = (int)(isect.uv.y * (tex.height- 1)) % tex.height;

    unsigned char col_bytes[4];
    col_bytes[0] = 0; col_bytes[1] = 0; col_bytes[2] = 0; col_bytes[3] = 255;
    for(int c = 0; c < tex.channel; c++) { //per channel
        //3D indexing
        int index_3d = c + tex.channel * x + tex.channel * tex.width * y;
        col_bytes[c] = texture_data[tex.start_index + index_3d];
    }
    return (float4)(col_bytes[0] * M_1_255_F,
                    col_bytes[1] * M_1_255_F,
                    col_bytes[2] * M_1_255_F,
                    col_bytes[3] * M_1_255_F);
}

//GetTexel with simple bilinear filtering (NOT CORRECTLY IMPLEMENTED)
float4 __BiLinGetTexel(__global const unsigned char* texture_data, const Texture tex, const IsectData isect) {
    int tex_x = isect.uv.x * (tex.width - 1);
    int tex_y = isect.uv.y * (tex.height- 1);

    float4 texel = (float4)(0.f, 0.f, 0.f, 0.f);
    for(int y = tex_y - 1; y <= tex_y + 1; y++) { //[-1, 0, 1] (3x3)
        for(int x = tex_x - 1; x <= tex_x + 1; x++) {
            unsigned char col_bytes[4];
            col_bytes[0] = 0; col_bytes[1] = 0; col_bytes[2] = 0; col_bytes[3] = 255;
            for(int c = 0; c < tex.channel; c++) { //per channel
                //3D indexing
                int index_3d = c + tex.channel * x + tex.channel * tex.width * y;
                col_bytes[c] = texture_data[tex.start_index + index_3d];
            }
            texel += (col_bytes[0] * M_1_255_F, col_bytes[1] * M_1_255_F,
                      col_bytes[2] * M_1_255_F, col_bytes[3] * M_1_255_F);
        }
    }
    return (texel / 9); //3x3 filtering
}

//samples incoming direction and performs shading at the intersection
float4 SampleShade(__global const TriangleMesh* meshes,
    __global const unsigned char* texture_data, float3* wi,
    const float3 wo, const IsectData isect, unsigned int* state) {
    //
    float rand1 = UnitRandFloat(state);
    float rand2 = UnitRandFloat(state);

    float3 n = OrientNormal(isect.normal, wo);
    //OLD METHOD - JUST DIFFUSE
    float3 sp = UniformSampleHemi(rand1, rand2);
    *wi = ToWorldSpace(sp, n);

/*
    float3 sp = CosineSampleHemi(rand1, rand2, meshes[isect.hit_mesh_index].material.exponent);
    *wi = ToWorldSpace(sp, 2.0 * dot(n, wo) * n - wo);
*/
    float4 f = GetTexel(texture_data, meshes[isect.hit_mesh_index].material.tex, isect);
    return f/* * M_1_PI_F*/; //TODO find out purpose of "/ PI"
}

//performs shading at the intersection (wi is given)
float4 Shade(__global const TriangleMesh* meshes,
    __global const unsigned char* texture_data, const float3 wi,
    const float3 wo, const IsectData isect, unsigned int* state) {
    //
    float4 f = GetTexel(texture_data, meshes[isect.hit_mesh_index].material.tex, isect);
    return f/* * M_1_PI_F*/; //TODO find out purpose of "/ PI"
}

float3 SampleTriangle(const Triangle tri, __global const float3* vertices, unsigned int* state) {
    float sqrt_rand1 = sqrt(max(0.f, UnitRandFloat(state)));
    float u = 1.f - sqrt_rand1;
    float v = UnitRandFloat(state) * sqrt_rand1;
    float w = 1.f - u - v;
    return (u*vertices[tri.vi.s0] + v*vertices[tri.vi.s1] + w*vertices[tri.vi.s2]);
}

float3 SampleTriangleMesh(__global const TriangleMesh* mesh,
    __global const Triangle* triangles,
    __global const float3* vertices, unsigned int* state) {
    //
    int index = RandInt(mesh->start_index, mesh->start_index + mesh->num_tris - 1, state); //randomly select triangle to sample
    return SampleTriangle(triangles[index], vertices, state);
}

float GetTriangleArea(const Triangle tri, __global const float3* vertices) {
    return 0.5f * length(cross(vertices[tri.vi.s1] - vertices[tri.vi.s0],
                                    vertices[tri.vi.s2] - vertices[tri.vi.s0]));
}
float GetTriangleMeshArea(__global const TriangleMesh* mesh,
    __global const Triangle* triangles, __global const float3* vertices) {
    //
    float area = 0.f;
    int end_index = mesh->start_index + mesh->num_tris;
    for(unsigned int i = mesh->start_index; i < end_index; i++) {
        area += GetTriangleArea(triangles[i], vertices);
    }
    return area;
}

float2 LightProbeUVMap(float3 v) { //return uv coords
    v = normalize(-v);
    float alpha = acos(v.z);
    //need reall small float so we don't div by 0
    float radical = half_sqrt(max(0.f, v.x*v.x + v.y*v.y));
    float sin_beta = v.y/radical;
    float cos_beta = v.x/radical;
    return (float2)(0.5f*(1.f + alpha * M_1_PI_F * cos_beta),
                    0.5f*(1.f + alpha * M_1_PI_F * sin_beta));
}

float4 GetEnvironmentMapTexel(const float2 uv, const Texture env_tex,
    __global const unsigned char* texture_data) {
    //
    int x = uv.x * (env_tex.width - 1);
    int y = uv.y * (env_tex.height- 1);

    unsigned char col_bytes[4];
    col_bytes[0] = 0; col_bytes[1] = 0; col_bytes[2] = 0; col_bytes[3] = 255;
    for(int c = 0; c < env_tex.channel; c++) { //per channel
        //3D indexing
        int index_3d = c + env_tex.channel * x + env_tex.channel * env_tex.width * y;
        col_bytes[c] = texture_data[env_tex.start_index + index_3d];
    }
    return (float4)(col_bytes[0] * M_1_255_F,
                    col_bytes[1] * M_1_255_F,
                    col_bytes[2] * M_1_255_F,
                    col_bytes[3] * M_1_255_F);
}

float4 EstimateDirectLighting(__global const TriangleMesh* meshes, int num_meshes,
    __global const Triangle* triangles, __global const float3* vertices,
    __global const float3* normals, __global const float2* uvs, __global const KdNode* kdnodes,
    const int num_kdnodes, __global const int* kdsorted_tri_indices,
    __global const unsigned char* texture_data, const int num_texture_bytes, const Texture environment_tex,
    const BBox scene_bbox, const float3 wo, const IsectData isect, unsigned int* state) {
    //
    //int num_lights = 0;
    float4 Ld = (float4)(0.f, 0.f, 0.f, 0.f);

    for(int i = 0; i < num_meshes; i++) { //array may not be full
        if(meshes[i].material.emission == 0.f) { //mesh is not a light
            continue; //skip
        }
        //num_lights++; //is a light to get here
        float3 light_pos = SampleTriangleMesh(&meshes[i], triangles, vertices, state); //randomly sampled over triangle
        float3 wi = normalize(light_pos - isect.hit_point); //(to, from)
        if(dot(OrientNormal(isect.normal, wo), wi) < 0.f) {
            continue; //backside culling
        }

        //cast shadow ray
        IsectData shadow_isect;
        float shadow_t;
        Ray light_ray = (Ray){light_pos, -wi};
        if(IntersectScene(meshes, num_meshes, triangles, vertices, normals, uvs, kdnodes, num_kdnodes,
            kdsorted_tri_indices, scene_bbox, light_ray, &shadow_t, &shadow_isect)) { //object in light dir path
            float shadow_dist = distance(shadow_isect.hit_point, light_pos);
            if(shadow_dist > distance(isect.hit_point, light_pos) * (1.f - EPSILON)) { //not in shadow
                //no need to flip ray. already flipped
                float4 emission = meshes[i].material.emission * GetTexel(texture_data, meshes[i].material.tex, isect);
                float mesh_area = GetTriangleMeshArea(&meshes[i], triangles, vertices);
                float dist = distance(isect.hit_point, light_pos);
                float falloff = mesh_area / (dist * dist);
                Ld += emission * falloff * Shade(meshes, texture_data, wi, wo, isect, state) * fabs(dot(isect.normal, wi));
            }
        }
    }

    //factor in environment lighting
    float3 wi = ToWorldSpace(UniformSampleHemi(UnitRandFloat(state), UnitRandFloat(state)), OrientNormal(isect.normal, wo));
    IsectData shadow_isect;
    float shadow_t;
    Ray light_ray = (Ray){isect.hit_point, wi};

    if(!IntersectScene(meshes, num_meshes, triangles, vertices, normals, uvs, kdnodes,
            num_kdnodes, kdsorted_tri_indices, scene_bbox, light_ray, &shadow_t, &shadow_isect)) { //no object in light dir path
        //
        float2 env_uv = LightProbeUVMap(wi);
        float4 env_contib = GetEnvironmentMapTexel(env_uv, environment_tex, texture_data);
        Ld += Shade(meshes, texture_data, wi, wo, isect, state) * ENV_MULT * env_contib * fabs(dot(isect.normal, wi));
    }

    return (Ld/* * num_lights*/);
}

float4 PathTrace(__global const TriangleMesh* meshes, const int num_meshes, __global const Triangle* triangles,
    const int num_triangles,  __global const float3* vertices, __global const float3* normals,
    __global const float2* uvs, __global const KdNode* kdnodes, const int num_kdnodes,
    __global const int* kdsorted_tri_indices, const int num_kdsorted_tri_indices,
    __global const unsigned char* texture_data, const int num_texture_bytes, const Texture environment_tex,
    const BBox scene_bbox, const Ray ray, unsigned int* state) {
    //
    float4 L = (float4)(0.f, 0.f, 0.f, 0.f);
    float4 path_throughput = (float4)(1.f, 1.f, 1.f, 1.f);
    IsectData isect;
    float t;
    Ray wi_ray = ray;
    if(!IntersectScene(meshes, num_meshes, triangles, vertices, normals, uvs,
        kdnodes, num_kdnodes, kdsorted_tri_indices, scene_bbox, ray, &t, &isect)) {
        //
        return (/*ENV_MULT * */GetEnvironmentMapTexel(LightProbeUVMap(ray.d), environment_tex, texture_data));
    }
    int depth = 0;
    while(true) {
        float3 wo = -wi_ray.d;

        //calculate emission at vertex
        //TODO == specular bounce
        if(depth == 0) {
            L += path_throughput * meshes[isect.hit_mesh_index].material.emission
                * GetTexel(texture_data, meshes[isect.hit_mesh_index].material.tex, isect);
        }
        L += path_throughput * EstimateDirectLighting(meshes, num_meshes, triangles,
            vertices, normals, uvs, kdnodes, num_kdnodes, kdsorted_tri_indices, texture_data,
            num_texture_bytes, environment_tex, scene_bbox, wo, isect, state);
//return L;
        //sample incoming direction
        float3 wi;
        float4 f = SampleShade(meshes, texture_data, &wi, wo, isect, state);

        if(IsBlack(f)) { //black wont reflect light
            break;
        }
        path_throughput *= f;
        wi_ray = (Ray){isect.hit_point, wi};

        //break if at end of path
        if(depth == MAX_BOUNCES) {
            break;
        }

        //next intersection
        if(!IntersectScene(meshes, num_meshes, triangles, vertices, normals, uvs,
            kdnodes, num_kdnodes, kdsorted_tri_indices, scene_bbox, wi_ray, &t, &isect)) {
            //No need to add env lighting here...done when estimating DL
            break;
        }
        depth++;
    }
    return L;
}

float3 TriangleCenter(const Triangle tri, __global const float3* vertices) {
    return (vertices[tri.vi.s0] + vertices[tri.vi.s1] + vertices[tri.vi.s2] * M_1_3_F);
}

float3 TriangleMeshCenter(__global const TriangleMesh* mesh,
    __global const Triangle* triangles, __global const float3* vertices) {
    //
    int start_index = mesh->start_index;
    int end_index = start_index + mesh->num_tris;
    float3 ave = (float3)(0.f);
    for(int i = start_index; i < end_index; i++) {
        ave += TriangleCenter(triangles[i], vertices);
    }
    return ave / (float)(end_index - start_index);
}

float4 DirectLightOnly(__global const TriangleMesh* meshes, int num_meshes,
    __global const Triangle* triangles, __global const float3* vertices,
    __global const float3* normals, __global const float2* uvs, __global const KdNode* kdnodes,
    const int num_kdnodes, __global const int* kdsorted_tri_indices,
    __global const unsigned char* texture_data, const int num_texture_bytes, const Texture environment_tex,
    const BBox scene_bbox, const float3 wo, const IsectData isect, unsigned int* state) {
    //
    //int num_lights = 0;
    float4 Ld = (float4)(0.f, 0.f, 0.f, 0.f);

    for(int i = 0; i < num_meshes; i++) { //array may not be full
        if(meshes[i].material.emission == 0.f) { //mesh is not a light
            continue; //skip
        }
        float3 light_pos = TriangleMeshCenter(&meshes[i], triangles, vertices);
        float3 wi = normalize(light_pos - isect.hit_point); //(to, from)
        if(dot(OrientNormal(isect.normal, wo), wi) < 0.f) {
            continue; //backside culling
        }

        //cast shadow ray towards light (different this time due to light position ex. inside a sphere mesh
        IsectData shadow_isect;
        float shadow_t;
        Ray light_ray = (Ray){isect.hit_point * (1.f + EPSILON), wi};
        if(IntersectScene(meshes, num_meshes, triangles, vertices, normals, uvs, kdnodes,
            num_kdnodes, kdsorted_tri_indices, scene_bbox, light_ray, &shadow_t, &shadow_isect)) { //object in light dir path
            //
            int hit_index = shadow_isect.hit_mesh_index;
            if(meshes[hit_index].material.emission != 0.f) { //hit a light
                float4 emission = meshes[hit_index].material.emission * GetTexel(texture_data, meshes[hit_index].material.tex, isect);
                float mesh_area = GetTriangleMeshArea(&meshes[hit_index], triangles, vertices);
                float dist = fast_distance(isect.hit_point, light_pos);
                float falloff = mesh_area / (dist * dist);
                Ld += emission * falloff * Shade(meshes, texture_data, wi, wo, isect, state) * fabs(dot(isect.normal, wi));
            }
        }
    }

    //factor in ***directional light*** -> jittered down
    float3 wi = (float3)(0.01f, 1.f, 0.034f);
    IsectData shadow_isect;
    float shadow_t;
    Ray light_ray = (Ray){isect.hit_point, wi};

    if(!IntersectScene(meshes, num_meshes, triangles, vertices, normals, uvs, kdnodes,
        num_kdnodes, kdsorted_tri_indices, scene_bbox, light_ray, &shadow_t, &shadow_isect)) { //no object in light dir path
        //
        Ld += Shade(meshes, texture_data, wi, wo, isect, state) * fabs(dot(isect.normal, wi));
    }

    return (Ld/* * num_lights*/);
}

float4 ConstantFog(const float4 sp, float dist) {
    float f = exp(-dist * 0.005f); //Exponential
    f = 1.f - min(1.f, max(0.f, f)); //clamp [0, 1]
    const float4 fog_col = (float4)(0.71f, 0.78f, 0.94f, 1.f);
    return fog_col*f + (1.f - f)*sp;
}

float4 NonConstantFog(const float4 sp, float dist, const Ray ray) {
    const float c = 0.6f;
    const float b = 0.005f;
    float f = c * exp(-ray.o.y * b) * (1.f - exp(-dist * ray.d.y * b))/ray.d.y;
    //f = min(1.f, max(0.f, f)); //clamp [0, 1]
    //const float4 fog_col = (float4)(0.71f, 0.78f, 0.94f, 1.f);
    const float4 fog_col = (float4)(0.51f, 0.56f, 0.674f, 1.f);
    return fog_col*f + (1.f - f)*sp;
}

float4 RayTrace(__global const TriangleMesh* meshes, const int num_meshes, __global const Triangle* triangles,
    const int num_triangles, __global const float3* vertices, __global const float3* normals, __global const float2* uvs,
    __global const KdNode* kdnodes, const int num_kdnodes, __global const int* kdsorted_tri_indices,
    const int num_kdsorted_tri_indices, __global const unsigned char* texture_data, const int num_texture_bytes,
    const Texture environment_tex, const BBox scene_bbox, const Ray ray, unsigned int* state) {
    //
    float4 L = (float4)(0.f, 0.f, 0.f, 0.f);
    IsectData isect;
    float t;
    Ray wi_ray = ray;
    if(!IntersectScene(meshes, num_meshes, triangles, vertices, normals, uvs,
        kdnodes, num_kdnodes, kdsorted_tri_indices, scene_bbox, ray, &t, &isect)) {
        //
        return (/*ENV_MULT * */GetEnvironmentMapTexel(LightProbeUVMap(ray.d), environment_tex, texture_data));
    }

    float3 wo = -wi_ray.d;

    //calculate emission at vertex
    L += meshes[isect.hit_mesh_index].material.emission * GetTexel(texture_data,
                         meshes[isect.hit_mesh_index].material.tex, isect);

    L += DirectLightOnly(meshes, num_meshes, triangles, vertices, normals, uvs,
                         kdnodes, num_kdnodes, kdsorted_tri_indices, texture_data,
                         num_texture_bytes, environment_tex, scene_bbox, wo, isect, state);

        //add ambient lighting term
    L += AMBIENT_TERM * GetTexel(texture_data, meshes[isect.hit_mesh_index].material.tex, isect);

    return NonConstantFog(L, t, ray);
}

Ray GetRay(__global const Camera* cam, float2 pp, unsigned int *state) {
	#ifdef DEPTH_OF_FIELD_ON
	    Ray ray;
	    float focal_length = 1.f / (1.f/cam->view_dist + 1.f/cam->focal_plane_dist);
	    float lens_radius = (focal_length / cam->f_stop) * 0.5f;
	    float2 lens_point = UniformSampleUnitDisk(state) * lens_radius;
	    ray.o = cam->eye + (cam->u * lens_point.x) + (cam->v * lens_point.y); //origin
	    float2 focal_point = (float2)((pp * cam->focal_plane_dist) / cam->view_dist); //point on focal plane
	    float3 dir = (float3)((focal_point.x - lens_point.x) * cam->u +
                              (focal_point.y - lens_point.y) * cam->v +
                               cam->focal_plane_dist * cam->w);
	    ray.d = normalize(dir);
	    return ray;
	#else
	    Ray ray;
	    ray.o = cam->eye;
	    float3 dir = (float3)(pp.x * cam->u + pp.y * cam->v + cam->view_dist * cam->w);
	    ray.d = normalize(dir); //normalize direction
	    return ray;
	#endif
}

float4 Tonemapping(float4 col) {
    //gamma correction
    float exp = 1.f/2.2f;
    col.x = pow(col.x, exp);
    col.y = pow(col.y, exp);
    col.z = pow(col.z, exp);

    //clamp [0, 1]
    col = clamp(col, (float4)(0.f, 0.f, 0.f, 0.f), (float4)(1.f, 1.f, 1.f, 1.f));

    return col;

}

float Lerp(float a, float b, float t) {
    return a + t * (b - a);
}

//Technique taken from LuxRender
float4 Vignette(const int2 coord, const int width, const int height, const float4 col) {
    //calculate constants
    float nPx = coord.x / width;
    float nPy = coord.y / height;
    float xOffset = nPx - 0.5f;
    float yOffset = nPy - 0.5f;
    float tOffset = sqrt(xOffset * xOffset + yOffset * yOffset); //radius from center

    //apply vignette
    float invNtOffset = 1.f - (fabs(tOffset) * 1.42f); //normalize to range [0.f - 1.f]
    return col * Lerp(1.f - VIGNETTE_SCALE, 1.f, invNtOffset); //Lerp
}

void PrintTypeSizes() {
    printf("ON KERNEL: Size of BBox %i\n", sizeof(BBox));
    printf("ON KERNEL: Size of KdNode %i\n", sizeof(KdNode));
    printf("ON KERNEL: Size of ToDoKdNode %i\n", sizeof(ToDoKdNode));
    printf("ON KERNEL: Size of IsectData %i\n", sizeof(IsectData));
    printf("ON KERNEL: Size of Triangle %i\n", sizeof(Triangle));
    printf("ON KERNEL: Size of TriangleMesh %i\n", sizeof(TriangleMesh));
    printf("ON KERNEL: Size of Material %i\n", sizeof(Material));
    printf("ON KERNEL: Size of Texture %i\n", sizeof(Texture));
}

//=========================================================KERNEL
//Data parallel kernel
__kernel void Render(
    __global const TriangleMesh* meshes,
    const int num_meshes,
    __global const Triangle* triangles,
    const int num_triangles,
    __global const float3* vertices,
    __global const float3* normals,
    __global const float2* uvs,
    __global const KdNode* kdnodes,
    const int num_kdnodes,
    __global const int* kdsorted_tri_indices,
    const int num_kdsorted_tri_indices,
    __global const unsigned char* texture_data,
    const int num_texture_bytes,
    __global const Texture* _environment_tex,
    __global const BBox* _scene_bbox,
    __global const Camera* camera,
    const int width,
    const int height,
    __write_only image2d_t image,
    __global unsigned int* rand_states
    ) {
    int2 size = (int2)(width, height);
    int2 coord = (int2)(get_global_id(0), get_global_id(1));
    Texture environment_tex = *_environment_tex;
    BBox scene_bbox = *_scene_bbox;

    //use 2D location to index into 1D array
    unsigned int index = 2;
    if(coord.x < size.x && coord.y < size.y) {
        index = get_global_id(1) * width + get_global_id(0);
    }
    unsigned int seed = rand_states[index];

    if(coord.x < size.x && coord.y < size.y) {
        //spawn rays
        float2 pp = (float2)(coord.x - size.x*0.5f, coord.y - size.y*0.5f);
        pp += UniformSampleUnitSquare(&seed); //stochastic anti-aliasing
        Ray ray = GetRay(camera, pp, &seed);
        float4 col;
        #ifdef DL_ONLY
            col = RayTrace(meshes, num_meshes, triangles, num_triangles, vertices, normals, uvs,
                kdnodes, num_kdnodes, kdsorted_tri_indices, num_kdsorted_tri_indices,
                texture_data, num_texture_bytes, environment_tex, scene_bbox, ray, &seed);
        #else
            col = PathTrace(meshes, num_meshes, triangles, num_triangles, vertices, normals, uvs,
                kdnodes, num_kdnodes, kdsorted_tri_indices, num_kdsorted_tri_indices,
                texture_data, num_texture_bytes, environment_tex, scene_bbox, ray, &seed);
        #endif

        //*** apply post-processing ***
        col = Vignette(coord, width, height, col);
        col = Tonemapping(col);
        write_imagef(image, coord, col);
    }
    rand_states[index] = seed;
}
