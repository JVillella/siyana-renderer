#ifndef FLOATN_H_INCLUDED
#define FLOATN_H_INCLUDED

#include <assert.h>
#include <math.h>
#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
using namespace std;

#include "intn.h"

class float4;

class float2 {
public:
    //--------------------------------------Constructors
    float2(float _x = 0.f, float _y = 0.f) :
        x(_x), y(_y) {}
    //--------------------------------------Data Members
    float x, y;
    //--------------------------------------Member Functions
    float2 operator+(const float2& f) const {
        return float2(x + f.x, y + f.y);
    }
    float2 operator-(const float2& f) const {
        return float2(x - f.x, y - f.y);
    }
    //...........................................
    float2 operator-() const {
        return float2(-x, -y);
    }
    float2 operator*(float s) const {
        return float2(x * s, y * s);
    }
    float2 operator/(float s) const {
        float inv_s = 1.f / s;
        return float2(x * inv_s, y * inv_s);
    }
    //...........................................
    float2& operator+=(const float2& f) {
        if(this == &f) { //same address
            return *this;
        }
        x += f.x; y += f.y;
        return *this;
    }
    float2& operator-=(const float2& f) {
        if(this == &f) { //same address
            return *this;
        }
        x -= f.x; y -= f.y;
        return *this;
    }
    //...........................................
    float2& operator*=(float s) {
        x *= s; y *= s;
        return *this;
    }
    float2& operator/=(float s) {
        float inv_s = 1.f / s;
        x *= inv_s; y *= inv_s;
        return *this;
    }
    float& operator[](int i) {
        assert(i >= 0 && i <= 1); //assertion failure if == 0
        return (&x)[i];
    }
    float operator[](int i) const {
        assert(i >= 0 && i <= 1); //assertion failure if == 0
        return (&x)[i];
    }
    friend ostream& operator<< (ostream& os, const float2& f);
};

class float3 {
public:
    //--------------------------------------Data Members
    float x, y, z, w; //w var not used...for padding
    //--------------------------------------Constructors
    float3(float _x = 0.f, float _y = 0.f, float _z = 0.f) :
        x(_x), y(_y), z(_z) {}
    float3(const int3& i) : x(i.x), y(i.y), z(i.z) {}
    float3(const float4 &f4); //implemented after float4 declaration
    //--------------------------------------Member Functions
    float3 operator+(const float3& f) const {
        return float3(x + f.x, y + f.y, z + f.z);
    }
    float3 operator+(const int3& i) const {
        return float3(x + i.x, y + i.y, z + i.z);
    }
    float3 operator-(const float3& f) const {
        return float3(x - f.x, y - f.y, z - f.z);
    }
    float3 operator*(const float3& f) const {
        return float3(x * f.x, y * f.y, z * f.z);
    }
    //...........................................
    float3 operator+(float s) const {
        return float3(x + s, y + s, z + s);
    }
    float3 operator-(float s) const {
        return float3(x - s, y - s, z - s);
    }
    float3 operator-() const {
        return float3(-x, -y, -z);
    }
    float3 operator*(float s) const {
        return float3(x * s, y * s, z * s);
    }
    float3 operator/(float s) const {
        float inv_s = 1.f / s;
        return float3(x * inv_s, y * inv_s, z * inv_s);
    }
    //...........................................
    float3& operator+=(const float3& f) {
        if(this == &f) { //same address
            return *this;
        }
        x += f.x; y += f.y; z += f.z;
        return *this;
    }
    float3& operator-=(const float3& f) {
        if(this == &f) { //same address
            return *this;
        }
        x -= f.x; y -= f.y; z -= f.z;
        return *this;
    }
    float3& operator*=(const float3& f) {
        if(this == &f) { //same address
            return *this;
        }
        x *= f.x; y *= f.y; z *= f.z;
        return *this;
    }
    //...........................................
    float3& operator+=(float s) {
        x += s; y += s; z += s;
        return *this;
    }
    float3& operator-=(float s) {
        x -= s; y -= s; z -= s;
        return *this;
    }
    float3& operator*=(float s) {
        x *= s; y *= s; z *= s;
        return *this;
    }
    float3& operator/=(float s) {
        float inv_s = 1.f / s;
        x *= inv_s; y *= inv_s; z *= inv_s;
        return *this;
    }
    bool operator==(const float3& f) const {
        return(fabs(x - f.x) < EPSILON &&
               fabs(y - f.y) < EPSILON &&
               fabs(z - f.z) < EPSILON);
    }
    float& operator[](int i) {
        assert(i >= 0 && i <= 2); //assertion failure if == 0
        return (&x)[i];
    }
    float operator[](int i) const {
        assert(i >= 0 && i <= 2); //assertion failure if == 0
        return (&x)[i];
    }
    friend ostream& operator<< (ostream& os, const float3& f);
};

inline float3 operator* (float s, float3 f) {
    return float3(f.x * s, f.y * s, f.z * s);
}

class float4 {
public:
    //--------------------------------------Constructors
    float4(float _x = 0.f, float _y = 0.f,
           float _z = 0.f, float _w = 0.f) :
        x(_x), y(_y), z(_z), w(_w) {}
    //seems incorrect...isn't - disregard the naming
    float4(float3 xyz, float _w = 0.f) : x(xyz.x), y(xyz.y),
        z(xyz.z), w(_w) {}
    //--------------------------------------Data Members
    float x, y, z, w;
    //--------------------------------------Member Functions
    float4 operator+(const float4& f) const {
        return float4(x + f.x, y + f.y, z + f.z, w + f.w);
    }
    float4 operator-(const float4& f) const {
        return float4(x - f.x, y - f.y, z - f.z, w - f.w);
    }
    float4 operator*(const float4& f) const {
        return float4(x * f.x, y * f.y, z * f.z, w * f.w);
    }
    //treating float4 as a quaternion - quat multiplication
    float4 qmul(const float4 &q) const {
        return float4(w * q.x + x * q.w + y * q.z - z * q.y,
                      w * q.y + y * q.w + z * q.x - x * q.z,
                      w * q.z + z * q.w + x * q.y - y * q.x,
                      w * q.w - x * q.x - y * q.y - z * q.z);
    }
    //...........................................
    float4 operator+(float s) const {
        return float4(x + s, y + s, z + s, w + s);
    }
    float4 operator-(float s) const {
        return float4(x - s, y - s, z - s, w - s);
    }
    float4 operator*(float s) const {
        return float4(x * s, y * s, z * s, w * s);
    }
    float4 operator/(float s) const {
        float inv_s = 1.f / s;
        return float4(x * inv_s, y * inv_s, z * inv_s, w * inv_s);
    }
    //...........................................
    float4& operator+=(const float4& f) {
        if(this == &f) { //same address
            return *this;
        }
        x += f.x; y += f.y; z += f.z; w += f.w;
        return *this;
    }
    float4& operator-=(const float4& f) {
        if(this == &f) { //same address
            return *this;
        }
        x -= f.x; y -= f.y; z -= f.z; w -= f.w;
        return *this;
    }
    float4& operator*=(const float4& f) {
        if(this == &f) { //same address
            return *this;
        }
        x *= f.x; y *= f.y; z *= f.z; w *= f.w;
        return *this;
    }
    //...........................................
    float4& operator+=(float s) {
        x += s; y += s; z += s; w += s;
        return *this;
    }
    float4& operator-=(float s) {
        x -= s; y -= s; z -= s; w -= s;
        return *this;
    }
    float4& operator*=(float s) {
        x *= s; y *= s; z *= s; w *= s;
        return *this;
    }
    float4& operator/=(float s) {
        float inv_s = 1.f / s;
        x *= inv_s; y *= inv_s; z *= inv_s; w *= inv_s;
        return *this;
    }
    float& operator[](int i) {
        assert(i >= 0 && i <= 3); //assertion failure if == 0
        return (&x)[i];
    }
    float operator[](int i) const {
        assert(i >= 0 && i <= 3); //assertion failure if == 0
        return (&x)[i];
    }
    friend ostream& operator<< (ostream& os, const float4& f);
};

inline
float3::float3(const float4 &f4): x(f4.x), y(f4.y), z(f4.z) {}

inline float4 conjugate(const float4 &f) {
    return float4(-f.x, -f.y, -f.z, f.w);
}
inline
float length(const float3 &a) {
    return sqrt(max(0.f, a.x*a.x + a.y*a.y + a.z*a.z));
}
//had to use dist instead of distance because STL takes that name
inline
float dist(const float3 &a, const float3 &b) {
    return length(a - b);
}


inline
float3 normalize(const float3 &v) {
    return v / length(v);
}

inline
float dot(const float3 &a, const float3 &b) {
    return (a.x*b.x + a.y*b.y + a.z*b.z);
}

inline
float3 cross(const float3 &a, const float3 &b) {
    return float3(a.y * b.z - a.z * b.y,
                  a.z * b.x - a.x * b.z,
                  a.x * b.y - a.y * b.x);
}

//couts
inline
ostream& operator<< (ostream& os, const float2& f) {
    os << "(X:" <<f.x<<") ";
    os << "(Y:" <<f.y<<")";
    return os;
}

inline
ostream& operator<< (ostream& os, const float3& f) {
    os << "(X:" <<f.x<<") ";
    os << "(Y:" <<f.y<<") ";
    os << "(Z:" <<f.z<<")";
    return os;
}

inline
ostream& operator<< (ostream& os, const float4& f) {
    os << "(X:" <<f.x<<") ";
    os << "(Y:" <<f.y<<") ";
    os << "(Z:" <<f.z<<") ";
    os << "(W:" <<f.w<<")";
    return os;
}

#endif // FLOATN_H_INCLUDED
