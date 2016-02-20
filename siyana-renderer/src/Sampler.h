#ifndef SAMPLER_H_INCLUDED
#define SAMPLER_H_INCLUDED

#define __OPENCL_DEVICE__
    #include "Utilities.h"
#undef __OPENCL_DEVICE__

//------------------------------------------float2 sampling
inline
float2 UniformSampleUnitSquare(unsigned int* state) { //[0, 1]
	return (float2)(UnitRandFloat(state), UnitRandFloat(state));
}
inline
float2 UniformSampleUnitDisk(unsigned int* state) {
    //polar coordinates
    float r, phi;
    float2 sp = (float2)(2.f * UnitRandFloat(state) - 1.f,
                         2.f * UnitRandFloat(state) - 1.f);

    //M_PI_4_F = PI/4, and M_PI_2_F = PI/2
    if(sp.x * sp.x > sp.y * sp.y) {
        r = sp.x;
        phi = M_PI_4_F * (sp.y / sp.x);
    } else {
        r = sp.y;
        phi =  M_PI_2_F - M_PI_4_F * (sp.x / sp.y);
        //Might have to use old one, it might
            //not work with Hammersly sampler though.
        //M_PI_4_F * (sp.x / sp.y) + M_PI_2_F;
    }
    return (float2)(r * cos(phi), r * sin(phi));
}

//------------------------------------------float3 sampling
inline
float3 UniformSampleHemi(float rand1, float rand2) { //exp is for "glossyness"
    float r = sqrt(max(0.f, 1.f - rand1 * rand1));
    float phi = 2.f * M_PI_F * rand2; //azimuth angle
    return (float3)(r * cos(phi), r * sin(phi), rand1);
}

inline
float3 UniformSampleSphere(float rand1, float rand2) {
    float z = 1.0f - 2.0f * rand1;
    float phi = 2.f * M_PI_F * rand2; //Azimuth
    float theta = sqrt(max(0.0f, 1.0f - z*z)); //Polar
    return (theta * cos(phi), theta * sin(phi), z);
}

inline
float3 CosineSampleHemi(float rand1, float rand2, float exp) { //exp is for "glossyness"
    float z = powr(1.f - rand1, 1.f / (exp + 1.f));
    float phi = 2.f * M_PI_F * rand2; //azimuth angle
    float theta = sqrt(max(0.f, 1.f - z*z)); //polar angle
    return (float3)(theta * cos(phi), theta * sin(phi), z);
}

inline
float3 ToWorldSpace(float3 p, float3 normal) { //normal must come in oriented
    float3 w = normal;
    float3 u = normalize(cross((float3)(0.00424f, 1.f, 0.00764f), w)); //jittered up
    float3 v = cross(u, w);
/*
    float3 wi =
    if(dot(normal, p) < 0.f) { //incase directions are inside mesh
        wi = u * -p.x - v * p.y + w * p.z;
    }
*/
    return (u * p.x + v * p.y + w * p.z); //linear projection
}

#endif // SAMPLER_H_INCLUDED
