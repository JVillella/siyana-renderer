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

#ifndef INTN_H_INCLUDED
#define INTN_H_INCLUDED

#include <assert.h>
#include <math.h>
#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
using namespace std;

class int4;

class int2 {
public:
    //--------------------------------------Constructors
    int2(int _x = 0, int _y = 0) : x(_x), y(_y) {}
    //--------------------------------------Data Members
    int x, y;
    //--------------------------------------Member Functions
    int2 operator+(const int2& i) const {
        return int2(x + i.x, y + i.y);
    }
    int2 operator-(const int2& i) const {
        return int2(x - i.x, y - i.y);
    }
    //...........................................
    int2 operator-() const {
        return int2(-x, -y);
    }
    int2 operator*(int s) const {
        return int2(x * s, y * s);
    }
    //...........................................
    int2& operator+=(const int2& i) {
        if(this == &i) { //same address
            return *this;
        }
        x += i.x; y += i.y;
        return *this;
    }
    int2& operator-=(const int2& i) {
        if(this == &i) { //same address
            return *this;
        }
        x -= i.x; y -= i.y;
        return *this;
    }
    //...........................................
    int2& operator*=(int s) {
        x *= s; y *= s;
        return *this;
    }
    int& operator[](int i) {
        assert(i >= 0 && i <= 1); //assertion failure if == 0
        return (&x)[i];
    }
    int operator[](int i) const {
        assert(i >= 0 && i <= 1); //assertion failure if == 0
        return (&x)[i];
    }
    friend ostream& operator<< (ostream& os, const int2& f);
};

class int3 {
public:
    //--------------------------------------Data Members
    int x, y, z, w; //w var not used...for padding
    //--------------------------------------Constructors
    int3(int _x = 0, int _y = 0, int _z = 0) :
        x(_x), y(_y), z(_z) {}
    int3(const int4 &i4); //implemented after float4 declaration
    //--------------------------------------Member Functions
    int3 operator+(const int3& i) const {
        return int3(x + i.x, y + i.y, z + i.z);
    }
    int3 operator-(const int3& i) const {
        return int3(x - i.x, y - i.y, z - i.z);
    }
    int3 operator*(const int3& i) const {
        return int3(x * i.x, y * i.y, z * i.z);
    }
    //...........................................
    int3 operator+(int s) const {
        return int3(x + s, y + s, z + s);
    }
    int3 operator-(int s) const {
        return int3(x - s, y - s, z - s);
    }
    int3 operator-() const {
        return int3(-x, -y, -z);
    }
    int3 operator*(int s) const {
        return int3(x * s, y * s, z * s);
    }
    //...........................................
    int3& operator+=(const int3& i) {
        if(this == &i) { //same address
            return *this;
        }
        x += i.x; y += i.y; z += i.z;
        return *this;
    }
    int3& operator-=(const int3& i) {
        if(this == &i) { //same address
            return *this;
        }
        x -= i.x; y -= i.y; z -= i.z;
        return *this;
    }
    int3& operator*=(const int3& i) {
        if(this == &i) { //same address
            return *this;
        }
        x *= i.x; y *= i.y; z *= i.z;
        return *this;
    }
    //...........................................
    int3& operator+=(int s) {
        x += s; y += s; z += s;
        return *this;
    }
    int3& operator-=(int s) {
        x -= s; y -= s; z -= s;
        return *this;
    }
    int3& operator*=(int s) {
        x *= s; y *= s; z *= s;
        return *this;
    }
    bool operator==(const int3& i) const {
        return(x == i.x && y == i.y && z == i.z);
    }
    int& operator[](int i) {
        assert(i >= 0 && i <= 2); //assertion failure if == 0
        return (&x)[i];
    }
    int operator[](int i) const {
        assert(i >= 0 && i <= 2); //assertion failure if == 0
        return (&x)[i];
    }
    friend ostream& operator<< (ostream& os, const int3& i);
};

inline int3 operator* (int s, const int3& i) {
    return int3(i.x * s, i.y * s, i.z *s);
}

class int4 {
public:
    //--------------------------------------Constructors
    int4(int _x = 0, int _y = 0, int _z = 0, int _w = 0) :
        x(_x), y(_y), z(_z), w(_w) {}
    //--------------------------------------Data Members
    int x, y, z, w;
    //--------------------------------------Member Functions
    int4 operator+(const int4& i) const {
        return int4(x + i.x, y + i.y, z + i.z, w + i.w);
    }
    int4 operator-(const int4& i) const {
        return int4(x - i.x, y - i.y, z - i.z, w - i.w);
    }
    int4 operator*(const int4& i) const {
        return int4(x * i.x, y * i.y, z * i.z, w * i.w);
    }
    //...........................................
    int4 operator+(int s) const {
        return int4(x + s, y + s, z + s, w + s);
    }
    int4 operator-(int s) const {
        return int4(x - s, y - s, z - s, w - s);
    }
    int4 operator*(int s) const {
        return int4(x * s, y * s, z * s, w * s);
    }
    //...........................................
    int4& operator+=(const int4& i) {
        if(this == &i) { //same address
            return *this;
        }
        x += i.x; y += i.y; z += i.z; w += i.w;
        return *this;
    }
    int4& operator-=(const int4& i) {
        if(this == &i) { //same address
            return *this;
        }
        x -= i.x; y -= i.y; z -= i.z; w -= i.w;
        return *this;
    }
    int4& operator*=(const int4& i) {
        if(this == &i) { //same address
            return *this;
        }
        x *= i.x; y *= i.y; z *= i.z; w *= i.w;
        return *this;
    }
    //...........................................
    int4& operator+=(int s) {
        x += s; y += s; z += s; w += s;
        return *this;
    }
    int4& operator-=(int s) {
        x -= s; y -= s; z -= s; w -= s;
        return *this;
    }
    int4& operator*=(int s) {
        x *= s; y *= s; z *= s; w *= s;
        return *this;
    }
    int& operator[](int i) {
        assert(i >= 0 && i <= 3); //assertion failure if == 0
        return (&x)[i];
    }
    int operator[](int i) const {
        assert(i >= 0 && i <= 3); //assertion failure if == 0
        return (&x)[i];
    }
    friend ostream& operator<< (ostream& os, const int4& i);
};

//couts
inline
ostream& operator<< (ostream& os, const int2& i) {
    os << "(X:" <<i.x<<") ";
    os << "(Y:" <<i.y<<")";
    return os;
}

inline
ostream& operator<< (ostream& os, const int3& i) {
    os << "(X:" <<i.x<<") ";
    os << "(Y:" <<i.y<<") ";
    os << "(Z:" <<i.z<<")";
    return os;
}

inline
ostream& operator<< (ostream& os, const int4& i) {
    os << "(X:" <<i.x<<") ";
    os << "(Y:" <<i.y<<") ";
    os << "(Z:" <<i.z<<") ";
    os << "(W:" <<i.w<<")";
    return os;
}

#endif // INTN_H_INCLUDED
